#include "include/BlockAllocator.h"

using namespace std;

BlockAllocator::BlockAllocator()
{
  len = 0;
  head = NULL;
  tail = NULL;
}

void BlockAllocator::delete_node(Node *to_delete)
{
  if(to_delete == NULL)
  {
    return;
  }

  if(to_delete->next == NULL && to_delete->prev == NULL)
  {
    head = NULL;
    tail = NULL;
  }
  else if(to_delete->next == NULL)
  {
    tail = to_delete->prev;
    to_delete->prev->next = NULL;
  }
  else if(to_delete->prev == NULL)
  {
    head = to_delete->next;
    to_delete->next->prev = NULL;
  }
  else
  {
    to_delete->next->prev = to_delete->prev;
    to_delete->prev->next = to_delete->next;
  }

  len--;
  free(to_delete);
}

Node* BlockAllocator::insert_after(Node *before, country_of_origin key)
{
  if(before == NULL)
  {
    return NULL;
  }

  Node *next = (Node *) malloc(sizeof(Node));
  next->key = key;

  Node *after = before->next;

  if(after == NULL)
  {
    return insert_tail(key);
  }

  before->next = next;
  next->prev = before;
  after->prev = next;
  next->next = after;
  len++;
  return next;
}

Node* BlockAllocator::insert_head(country_of_origin key)
{
  Node *next = (Node *) malloc(sizeof(Node));
  next->key = key;

  if(head == NULL)
  {
    head = next;
    tail = next;
    next->next = NULL;
    next->prev = NULL;
  }
  else
  {
    head->prev = next;
    next->next = head;
    next->prev = NULL;
    head = next;
  }

  len++;
  return next;
}

Node* BlockAllocator::insert_tail(country_of_origin key)
{
  Node *next = (Node *) malloc(sizeof(Node));
  next->key = key;

  if(tail == NULL)
  {
    head = next;
    tail = next;
    next->next = NULL;
    next->prev = NULL;
  }
  else
  {
    tail->next = next;
    next->prev = tail;
    next->next = NULL;
    tail = next;
  }

  len++;
  return next;
}

/*
 * allocates space for insertion of new record with given countryOfOrigin
 * returns the offset at which this record may be allocated
 */
int BlockAllocator::block_insert(string block_id, FileManager *mem)
{
  if(block_id.length() <= 0 || block_id.length() > 8)
  {
    throw std::invalid_argument("block id must be between 1 and 8 characters long");
  }

  // case 1, no block has been allocated
  // case 2, block has been allocated, block has space
  // case 3, block has been allocated, block has no more space
  alloc_info next_block;
  country_of_origin header;
  Node *node;
  int total_offset;
  if(block_map.find(block_id) == block_map.end())
  {
    // case 1, search for smallest available free block
    total_offset = get_min_free_block(MIN_BLOCK_SIZE, &next_block);

    if(next_block.loc == NULL)
    {
      // no suitable free block found
      // allocate new block to end of list
      header = make_header(block_id, MIN_BLOCK_SIZE, 1);
      node = insert_tail(header);
      next_block = alloc_info{total_offset, node};
      block_map[block_id] = next_block;

      return total_offset;
    }
    else
    {
      // suitable free block found
      // split free block if large enough
      if(next_block.loc->key.block_size >= 2 * MIN_BLOCK_SIZE)
      {
        int split_size = next_block.loc->key.block_size - MIN_BLOCK_SIZE;
        next_block.loc->key.block_size = MIN_BLOCK_SIZE;
        header = make_header("", split_size, -1);
        insert_after(next_block.loc, header);
      }

      // allocate free block
      header = make_header(block_id, MIN_BLOCK_SIZE, 1);
      next_block.loc->key = header;
      block_map[block_id] = next_block;

      return next_block.offset;
    }
  }
  else
  {
    // block found
    alloc_info curr_block = block_map.at(block_id);

    if(curr_block.loc->key.block_size <= curr_block.loc->key.record_count)
    {
      // case 3, check if next block in the list is free
      if(curr_block.loc->next != NULL && is_free(curr_block.loc->next->key))
      {
        if(curr_block.loc->next->key.block_size >= 2 * MIN_BLOCK_SIZE)
        {
          // take MIN_BLOCK_SIZE space from next block and transfer to current block
          curr_block.loc->next->key.block_size -= MIN_BLOCK_SIZE;
          curr_block.loc->key.block_size += MIN_BLOCK_SIZE;
          curr_block.loc->key.record_count++;
          return curr_block.offset + curr_block.loc->key.record_count - 1;
        }
        else
        {
          // coalesce next block with current block
          curr_block.loc->key.block_size += curr_block.loc->next->key.block_size;
          delete_node(curr_block.loc->next);
          curr_block.loc->key.record_count++;
          return curr_block.offset + curr_block.loc->key.record_count - 1;
        }
      }
      else if(curr_block.loc->next == NULL)
      {
        // block is at end of file, increase by MIN_BLOCK_SIZE
        curr_block.loc->key.block_size+= MIN_BLOCK_SIZE;
        curr_block.loc->key.record_count++;
        return curr_block.offset + curr_block.loc->key.record_count - 1;
      }

      // next block is not free and block or not at end of file
      // reallocate to smallest free block that is at least twice as large as the current block
      int old_size = curr_block.loc->key.block_size;
      int old_count = curr_block.loc->key.record_count;
      int old_offset = curr_block.offset;

      total_offset = get_min_free_block(2 * old_size, &next_block);

      if(next_block.loc == NULL)
      {
        // no suitable free block found
        // free current block and allocate new block to end of list
        free_block(curr_block.loc);
        header = make_header(block_id, 2 * old_size, old_count + 1);
        node = insert_tail(header);
        next_block = alloc_info{total_offset, node};
        block_map[block_id] = next_block;

        // copy records from old to new block
        mem->copy_records(COFFEEID, old_offset, total_offset, old_count);
        mem->copy_records(COFFEENAME, old_offset, total_offset, old_count);
        mem->copy_records(INTENSITY, old_offset, total_offset, old_count);

        return total_offset + old_count;
      }
      else
      {
        // suitable free block found
        // split free block if large enough
        if(next_block.loc->key.block_size - 2 * old_size >= MIN_BLOCK_SIZE)
        {
          int split_size = next_block.loc->key.block_size - 2 * old_size;
          next_block.loc->key.block_size = 2 * old_size;
          header = make_header("", split_size, -1);
          insert_after(next_block.loc, header);
        }

        // allocate to free block
        header = make_header(block_id, 2 * old_size, old_count + 1);
        next_block.loc->key = header;
        block_map[block_id] = next_block;

        // copy records from old to new block
        mem->copy_records(COFFEEID, old_offset, next_block.offset, old_count);
        mem->copy_records(COFFEENAME, old_offset, next_block.offset, old_count);
        mem->copy_records(INTENSITY, old_offset, next_block.offset, old_count);

        // free current block
        free_block(curr_block.loc);

        return next_block.offset + old_count;
      }
    }
    else
    {
      // case 2, append to block
      curr_block.loc->key.record_count++;
      return curr_block.offset + curr_block.loc->key.record_count - 1;
    }
  }
}

/*
 * sets a given Node as free and coalesces it with its neighboring free nodes
 */
void BlockAllocator::free_block(Node *to_free)
{
  if(to_free == NULL)
  {
    return;
  }

  to_free->key = make_header("", to_free->key.block_size, -1);
  if(to_free->next != NULL && is_free(to_free->next->key))
  {
    to_free->key.block_size += to_free->next->key.block_size;
    delete_node(to_free->next);
  }
  if(to_free->prev != NULL && is_free(to_free->prev->key))
  {
    to_free->prev->key.block_size += to_free->key.block_size;
    delete_node(to_free);
  }
}

int BlockAllocator::get_block_from_offset(int offset, country_of_origin *val)
{
  Node *next = head;
  country_of_origin block;
  int running_offset = 0;

  while(next != NULL)
  {
    block = next->key;

    if(running_offset <= offset && offset < running_offset + block.block_size)
    {
      memcpy(val, &block, sizeof(block));
      return 0;
    }

    running_offset += block.block_size;
    next = next->next;
  }

  return -1;
}

country_of_origin BlockAllocator::make_header(string block_id, int total_lines, int used_lines)
{
  country_of_origin header;
  strncpy(header.val, block_id.c_str(), 8);
  header.block_size = total_lines;
  header.record_count = used_lines;
  return header;
}

/*
 * finds the smallest free block that is larger than the given size
 * returns a pointer to that block and its cummulative offset
 */
int BlockAllocator::get_min_free_block(int size, alloc_info *min_block)
{
  Node *next = head;
  int offset = 0;
  min_block->offset = -1;
  min_block->loc = NULL;

  while(next != NULL)
  {
    if(is_free(next->key) && next->key.block_size >= size)
    {
      if(min_block->loc == NULL)
      {
        min_block->offset = offset;
        min_block->loc = next;
      }
      else if(min_block->loc->key.block_size > next->key.block_size)
      {
        min_block->offset = offset;
        min_block->loc = next;
      }
    }

    offset += next->key.block_size;
    next = next->next;
  }

  return offset;
}

bool BlockAllocator::is_free(country_of_origin block)
{
  return strcmp("", block.val) == 0 && block.record_count == -1;
}

void BlockAllocator::print_allocations()
{
  Node *next = head;
  country_of_origin block;
  char block_name[9];

  while(next != NULL)
  {
    block = next->key;
    memset(block_name, 0, sizeof(block_name));
    strncpy(block_name, block.val, 8);
    cout << block_name << ", " << block.block_size << ", " << block.record_count << endl;

    next = next->next;
  }
}
