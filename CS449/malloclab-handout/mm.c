/*
 ***********************************************************************************
 *                                   mm.c                                          *
 *  Starter package for a 64-bit struct-based explicit free list memory allocator  *
 *                                                                                 *
 *  ********************************************************************************
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stddef.h>
#include <inttypes.h>

#include "memlib.h"
#include "mm.h"

 /*
 *
 * Each block has header and footer of the form:
 *
 *      63                  4  3  2  1  0
 *      -----------------------------------
 *     | s  s  s  s  ... s  s  0  0  0  a/f
 *      -----------------------------------
 *
 * where s are the meaningful size bits and a/f is set
 * iff the block is allocated. The list has the following form:
 *
 *
 *    begin                                   end
 *    heap                                    heap
 *  +-----------------------------------------------+
 *  | ftr(0:a)   | zero or more usr blks | hdr(0:a) |
 *  +-----------------------------------------------+
 *  |  prologue  |                       | epilogue |
 *  |  block     |                       | block    |
 *
 *
 * The allocated prologue and epilogue blocks are overhead that
 * eliminate edge conditions during coalescing.
 *
 */

/*  Empty block
 *  ------------------------------------------------*
 *  |HEADER:    block size   |     |     | alloc bit|
 *  |-----------------------------------------------|
 *  | pointer to prev free block in this size list  |
 *  |-----------------------------------------------|
 *  | pointer to next free block in this size list  |
 *  |-----------------------------------------------|
 *  |FOOTER:    block size   |     |     | alloc bit|
 *  ------------------------------------------------
 */

/*   Allocated block
 *   ------------------------------------------------*
 *   |HEADER:    block size   |     |     | alloc bit|
 *   |-----------------------------------------------|
 *   |               Data                            |
 *   |-----------------------------------------------|
 *   |               Data                            |
 *   |-----------------------------------------------|
 *   |FOOTER:    block size   |     |     | alloc bit|
 *   -------------------------------------------------
 */

/* Basic constants */

typedef uint64_t word_t;

// Word and header size (bytes)
static const size_t wsize = sizeof(word_t);

// Double word size (bytes)
static const size_t dsize = 2 * sizeof(word_t);

/*
  Minimum useable block size (bytes):
  two words for header & footer, two words for payload
*/
static const size_t min_block_size = 4 * sizeof(word_t);

/* Initial heap size (bytes), requires (chunksize % 16 == 0)
*/
static const size_t chunksize = (1 << 12);

// Mask to extract allocated bit from header
static const word_t alloc_mask = 0x1;

/*
 * Assume: All block sizes are a multiple of 16
 * and so can use lower 4 bits for flags
 */
static const word_t size_mask = ~(word_t) 0xF;

/*
  All blocks have both headers and footers

  Both the header and the footer consist of a single word containing the
  size and the allocation flag, where size is the total size of the block,
  including header, (possibly payload), unused space, and footer
*/

typedef struct block block_t;

/* Representation of the header and payload of one block in the heap */
struct block
{
    /* Header contains:
    *  a. size
    *  b. allocation flag
    */
    word_t header;

    union
    {
        struct
        {
            block_t *prev;
            block_t *next;
        } links;
        /*
        * We don't know what the size of the payload will be, so we will
        * declare it as a zero-length array.  This allows us to obtain a
        * pointer to the start of the payload.
        */
        unsigned char data[0];

    /*
     * Payload contains:
     * a. only data if allocated
     * b. pointers to next/previous free blocks if unallocated
     */
    } payload;

    /*
     * We can't declare the footer as part of the struct, since its starting
     * position is unknown
     */
};

/* Global variables */

// Pointer to first block
static block_t *heap_start = NULL;

// Pointer to the first block in the free list
static block_t *free_list_head = NULL;

/* Function prototypes for internal helper routines */

static size_t max(size_t x, size_t y);
static block_t *find_fit(size_t asize);
static block_t *coalesce_block(block_t *block);
static void split_block(block_t *block, size_t asize);

static size_t round_up(size_t size, size_t n);
static word_t pack(size_t size, bool alloc);

static size_t extract_size(word_t header);
static size_t get_size(block_t *block);

static bool extract_alloc(word_t header);
static bool get_alloc(block_t *block);
static bool is_free(block_t *block);
static bool is_alloc(block_t *block);

static void write_header(block_t *block, size_t size, bool alloc);
static void write_footer(block_t *block, size_t size, bool alloc);

static block_t *payload_to_header(void *bp);
static void *header_to_payload(block_t *block);
static word_t *header_to_footer(block_t *block);

static block_t *find_next(block_t *block);
static word_t *find_prev_footer(block_t *block);
static block_t *find_prev(block_t *block);

static bool check_heap();
static void examine_heap();

static block_t *extend_heap(size_t size);
static void insert_block(block_t *free_block);
static void remove_block(block_t *free_block);

/*
 * mm_init - Initialize the memory manager
 */
int mm_init(void)
{
    free_list_head = NULL;
    /* Create the initial empty heap */
    word_t *start = (word_t *)(mem_sbrk(2*wsize));
    if ((ssize_t)start == -1) {
        printf("ERROR: mem_sbrk failed in mm_init, returning %p\n", start);
        return -1;
    }

    /* Prologue footer */
    start[0] = pack(0, true);
    /* Epilogue header */
    start[1] = pack(0, true);

    /* Heap starts with first "block header", currently the epilogue header */
    heap_start = (block_t *) &(start[1]);

    /* Extend the empty heap with a free block of chunksize bytes */
    block_t *free_block = extend_heap(chunksize);
    if (free_block == NULL) {
        printf("ERROR: extend_heap failed in mm_init, returning");
        return -1;
    }

    return 0;
}

/*
 * mm_malloc - Allocate a block with at least size bytes of payload
 */
void *mm_malloc(size_t size)
{
  size_t asize;      // Allocated block size

  if (size == 0) // Ignore spurious request
      return NULL;

  // Too small block
  if (size <= dsize) {
      asize = min_block_size;
  } else {
      // Round up and adjust to meet alignment requirements
      asize = round_up(size + dsize, dsize);
  }

  //find a block that has at least asize of space in the free list
  block_t *alloc_block = find_fit(asize);

  //if no block of asize or more is found, extends heap and tries again
  if(alloc_block == NULL){

    if(extend_heap(chunksize) == NULL){
      return NULL;
    }
    return mm_malloc(size);
  }

  //if found, removes alloc_block from the free list
  remove_block(alloc_block);

  //resets heading to mark block as allocated
  size_t block_size = get_size(alloc_block);
  write_header(alloc_block, block_size, true);
  write_footer(alloc_block, block_size, true);

  //splits block into allocated and free portion if possible
  split_block(alloc_block, asize);

  block_t *bp = header_to_payload(alloc_block);

  return bp;
}


/*
 * mm_free - Free a block
 */
void mm_free(void *bp)
{
    if (bp == NULL){
        return;
    }

    block_t *to_free = payload_to_header(bp);

    //checks if the block to be freed is the prologue or the epilogue
    //verifies that the block to be freed is allocated
    if(get_alloc(to_free) == false){
      printf("ERROR: Attempted to free unallocated block");
      return;
    }
    if(get_size(to_free) == 0){
      printf("ERROR: Attempted to free dummy block");
      return;
    }

    //rewrites the block's heading
    //inserts block into free list
    write_header(to_free, get_size(to_free), false);
    write_footer(to_free, get_size(to_free), false);
    insert_block(to_free);

    //merges to_free with free blocks adjacent to it
    coalesce_block(to_free);
}

/*
 * insert_block - Insert block at the head of the free list (e.g., LIFO policy)
 */
static void insert_block(block_t *free_block)
{
  //sets free_block as first free block in free_list
  //if free_list is empty or free list has elements
  if(free_list_head == NULL){
    free_block->payload.links.next = NULL;
    free_block->payload.links.prev = NULL;
  }
  else{
    block_t *old_root = free_list_head;
    free_block->payload.links.next = old_root;
    free_block->payload.links.prev = NULL;
    old_root->payload.links.prev = free_block;
  }

  free_list_head = free_block;
}

/*
 * remove_block - Remove a free block from the free list
 */
static void remove_block(block_t *free_block)
{
  if(free_block == NULL){
    return;
  }

  block_t *previous = free_block->payload.links.prev;
  block_t *next = free_block->payload.links.next;

  if(previous == NULL && next == NULL){
    free_list_head = NULL;
  }
  else if(previous == NULL){
    free_list_head = next;
    next->payload.links.prev = NULL;
  }
  else if(next == NULL){
    previous->payload.links.next = NULL;
  }
  else{
    previous->payload.links.next = next;
    next->payload.links.prev = previous;
  }
}

/*
 * Finds a free block that of size at least asize
 */
static block_t *find_fit(size_t asize)
{
  if(free_list_head == NULL){
    return NULL;
  }

  bool first = true;
  block_t *best_fit = NULL, *curr_block = free_list_head;
  size_t best_size = 0;

  while(curr_block != NULL){
    size_t curr_size = get_size(curr_block);

    //if the curr_size is greater than the required size
    //and the block is not allocated, curr_block can be allocated
    if(curr_size >= asize && !get_alloc(curr_block)){

      //if first is true, curr_block is first block in list that fits request
      if(first == true){
        best_fit = curr_block;
        best_size = curr_size;
        first = false;
      }

      //if a block fits better than the one already found,
      //that block replaces the old one
      if(curr_size < best_size && first == false){
        best_fit = curr_block;
        best_size = curr_size;
      }
    }

    //moves curr_block to the next block in the free list
    curr_block = curr_block->payload.links.next;
  }

  return best_fit;
}

/*
 * Coalesces current block with previous and next blocks if either or both are unallocated; otherwise the block is not modified.
 * Returns pointer to the coalesced block. After coalescing, the immediate contiguous previous and next blocks must be allocated.
 */
static block_t *coalesce_block(block_t *block)
{
  //4 possible
  //1: block before is free, block after is allocated
  //2: block before is allocated, block after is free
  //3: block before is free, block after is free
  //4: block before is allocated, block after is allocated
	block_t *block_before = find_prev(block);
  block_t *block_after  = find_next(block);
  block_t *coalesced_block;
  size_t new_size;

  if(is_free(block_before) && get_alloc(block_after)){
    remove_block(block);

    new_size = get_size(block_before) + get_size(block);
    write_header(block_before, new_size, false);
    write_footer(block_before, new_size, false);
    coalesced_block = block_before;
  }
  else if(get_alloc(block_before) && is_free(block_after)){
    remove_block(block_after);

    new_size = get_size(block) + get_size(block_after);
    write_header(block, new_size, false);
    write_footer(block, new_size, false);
    coalesced_block = block;
  }
  else if(is_free(block_before) && is_free(block_after)){
    new_size = get_size(block_before) + get_size(block) + get_size(block_after);
    remove_block(block);
    remove_block(block_after);
    write_header(block_before, new_size, false);
    write_footer(block_before, new_size, false);
    coalesced_block = block_before;
  }
  else{
    coalesced_block = block;
  }

  return coalesced_block;
}

static bool is_free(block_t *block){
  return (!get_alloc(block));
}

/*
 * See if new block can be split one to satisfy allocation
 * and one to keep free
 */
static void split_block(block_t *block, size_t asize)
{
  size_t block_size = get_size(block);
  size_t split_size = block_size - asize;

  //split_size is the amount of extra space in block
  //if there is more than min_block_size of extra space, block is split
  if(split_size >= min_block_size){
    //bytes from block to block+asize become an allocated block
    write_header(block, asize, true);
    write_footer(block, asize, true);

    //bytes from block+asize to block+block_size become a free block
    block_t *next_block = find_next(block);
    write_header(next_block, split_size, false);
    write_footer(next_block, split_size, false);

    //new free block is added to free list
    insert_block(next_block);
  }
}


/*
 * Extends the heap with the requested number of bytes, and recreates end header.
 * Returns a pointer to the result of coalescing the newly-created block with previous free block,
 * if applicable, or NULL in failure.
 */
static block_t *extend_heap(size_t size)
{
    void *bp; // bp is a pointer to the new memory block requested

    // Allocate an even number of words to maintain alignment
    //extension must be at least min_block_size long
    size = round_up(size, dsize);
    if(size < min_block_size){
      size = min_block_size;
    }
    if ((bp = mem_sbrk(size)) == (void *)-1) {
        return NULL;
    }

    //last_block is last block in the heap prior to extension
    //new_block is block created by extension
    block_t *new_block = payload_to_header(bp);
    block_t *last_block = find_prev(new_block);

    //if last block is free, increase size of last block by size
    //if block is allocated, create a new free block
    //and adds free block to beginning of free list
    if(get_alloc(last_block) == false){
      size_t last_size = get_size(last_block);
      write_header(last_block, last_size + size, false);
      write_footer(last_block, last_size + size, false);

      //adds new epilogue at end of heap
      block_t *epilogue = find_next(last_block);
      write_header(epilogue, 0, true);

      return last_block;
    }
    else{
      //adds header and footer to new free block
      write_header(new_block, size, false);
      write_footer(new_block, size, false);

      //adds new free block to free list
      insert_block(new_block);

      //adds new epilogue at end of heap
      block_t *epilogue = find_next(new_block);
      write_header(epilogue, 0, true);

      return new_block;
    }
}

/******** The remaining content below are helper and debug routines ********/

/*
 * Return whether the pointer is in the heap.
 * May be useful for debugging.
 */
static int in_heap(const void* p)
{
    return p <= mem_heap_hi() && p >= mem_heap_lo();
}

/*
 * examine_heap -- Print the heap by iterating through it as an implicit free list.
 */
static void examine_heap() {
  block_t *block;

  /* print to stderr so output isn't buffered and not output if we crash */
  fprintf(stderr, "free_list_head: %p\n", (void *)free_list_head);

  for (block = heap_start; /* first block on heap */
      get_size(block) > 0 && block < (block_t*)mem_heap_hi();
      block = find_next(block)) {

    /* print out common block attributes */
    fprintf(stderr, "%p: %ld %d\t", (void *)block, get_size(block), get_alloc(block));

    /* and allocated/free specific data */
    if (get_alloc(block)) {
      fprintf(stderr, "ALLOCATED\n");
    } else {
      fprintf(stderr, "FREE\tnext: %p, prev: %p\n",
      (void *)block->payload.links.next,
      (void *)block->payload.links.prev);
    }
  }
  fprintf(stderr, "END OF HEAP\n\n");
}


/* check_heap: checks the heap for correctness; returns true if
 *               the heap is correct, and false otherwise.
 */
static bool check_heap()
{

    // Implement a heap consistency checker as needed.

    /* Below is an example, but you will need to write the heap checker yourself. */

    if (!heap_start) {
        printf("NULL heap list pointer!\n");
        return false;
    }

    block_t *curr = heap_start;
    block_t *next;
    block_t *hi = mem_heap_hi();

    while ((next = find_next(curr)) + 1 < hi) {
        word_t hdr = curr->header;
        word_t ftr = *find_prev_footer(next);

        if (hdr != ftr) {
            printf(
                    "Header (0x%016lX) != footer (0x%016lX)\n",
                    hdr, ftr
                  );
            return false;
        }

        curr = next;
    }

    return true;
}


/*
 *****************************************************************************
 * The functions below are short wrapper functions to perform                *
 * bit manipulation, pointer arithmetic, and other helper operations.        *
 *****************************************************************************
 */

/*
 * max: returns x if x > y, and y otherwise.
 */
static size_t max(size_t x, size_t y)
{
    return (x > y) ? x : y;
}


/*
 * round_up: Rounds size up to next multiple of n
 */
static size_t round_up(size_t size, size_t n)
{
    return n * ((size + (n-1)) / n);
}


/*
 * pack: returns a header reflecting a specified size and its alloc status.
 *       If the block is allocated, the lowest bit is set to 1, and 0 otherwise.
 */
static word_t pack(size_t size, bool alloc)
{
    return alloc ? (size | alloc_mask) : size;
}


/*
 * extract_size: returns the size of a given header value based on the header
 *               specification above.
 */
static size_t extract_size(word_t word)
{
    return (word & size_mask);
}


/*
 * get_size: returns the size of a given block by clearing the lowest 4 bits
 *           (as the heap is 16-byte aligned).
 */
static size_t get_size(block_t *block)
{
    return extract_size(block->header);
}


/*
 * extract_alloc: returns the allocation status of a given header value based
 *                on the header specification above.
 */
static bool extract_alloc(word_t word)
{
    return (bool) (word & alloc_mask);
}


/*
 * get_alloc: returns true when the block is allocated based on the
 *            block header's lowest bit, and false otherwise.
 */
static bool get_alloc(block_t *block)
{
    return extract_alloc(block->header);
}


/*
 * write_header: given a block and its size and allocation status,
 *               writes an appropriate value to the block header.
 */
static void write_header(block_t *block, size_t size, bool alloc)
{
    block->header = pack(size, alloc);
}


/*
 * write_footer: given a block and its size and allocation status,
 *               writes an appropriate value to the block footer by first
 *               computing the position of the footer.
 */
static void write_footer(block_t *block, size_t size, bool alloc)
{
    word_t *footerp = header_to_footer(block);
    *footerp = pack(size, alloc);
}


/*
 * find_next: returns the next consecutive block on the heap by adding the
 *            size of the block.
 */
static block_t *find_next(block_t *block)
{
    return (block_t *) ((unsigned char *) block + get_size(block));
}


/*
 * find_prev_footer: returns the footer of the previous block.
 */
static word_t *find_prev_footer(block_t *block)
{
    // Compute previous footer position as one word before the header
    return &(block->header) - 1;
}


/*
 * find_prev: returns the previous block position by checking the previous
 *            block's footer and calculating the start of the previous block
 *            based on its size.
 */
static block_t *find_prev(block_t *block)
{
    word_t *footerp = find_prev_footer(block);
    size_t size = extract_size(*footerp);

    if(size == 0){
      return find_prev_footer(block);
    }
    else{
      return (block_t *) ((unsigned char *) block - size);
    }
}


/*
 * payload_to_header: given a payload pointer, returns a pointer to the
 *                    corresponding block.
 */
static block_t *payload_to_header(void *bp)
{
    return (block_t *) ((unsigned char *) bp - offsetof(block_t, payload));
}


/*
 * header_to_payload: given a block pointer, returns a pointer to the
 *                    corresponding payload data.
 */
static void *header_to_payload(block_t *block)
{
    return (void *) (block->payload.data);
}


/*
 * header_to_footer: given a block pointer, returns a pointer to the
 *                   corresponding footer.
 */
static word_t *header_to_footer(block_t *block)
{
    return (word_t *) (block->payload.data + get_size(block) - dsize);
}
