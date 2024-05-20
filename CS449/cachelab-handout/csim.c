/*
 * csim.c - A cache simulator that can replay traces from Valgrind
 *     and output statistics such as number of hits, misses, and
 *     evictions.  The replacement policy is LRU.
 *
 * Implementation and assumptions:
 *
 *  1. Each load/store can cause at most one cache miss. (I examined the trace,
 *  the largest request I saw was for 8 bytes).
 *
 *  2. Instruction loads (I) are ignored, since we are interested in evaluating
 *  data cache performance.
 *
 *  3. data modify (M) is treated as a load followed by a store to the same
 *  address. Hence, an M operation can result in two cache hits, or a miss and a
 *  hit plus an possible eviction.
 *
 * The function printSummary() is given to print output.
 * Please use this function to print the number of hits, misses and evictions.
 * IMPORTANT: This is crucial for the driver to evaluate your work.
 *
 */

#include <getopt.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <assert.h>
#include <math.h>
#include <limits.h>
#include <string.h>
#include <errno.h>

//#define DEBUG_ON
#define ADDRESS_LENGTH 64

/* Type: Memory address */
typedef unsigned long long int mem_addr_t;

/*
 * Data structures to represent the cache we are simulating
 *
 * TODO: Define your own!
 *
 * E.g., Types: cache, cache line, cache set
 * (you can use a counter to implement LRU replacement policy)
 */

typedef struct {
  unsigned int valid;
  unsigned int tag;
  //stores number of times this line is accessed
  //lowest last_use in a set is evicted
  unsigned int last_use;
} my_line;

typedef struct {
  my_line *lines;
} my_set;

typedef struct {
  my_set *sets;
} my_cache;

//my_cache data structure is a 2d array
my_cache cache;

/* Globals set by command line args */
int verbosity = 0; /* print trace if set */
int s = 0; /* set index bits */
int b = 0; /* block offset bits */
int E = 0; /* associativity */
char* trace_file = NULL;

/* Derived from command line args */
int S; /* number of sets */
int B; /* block size (bytes) */

/* Counters used to record cache statistics */
int miss_count = 0;
int hit_count = 0;
int eviction_count = 0;
unsigned long long int lru_counter = 1;

void print_cache()
{
  for(int i = 0; i < S; i++){
    printf("S%d:\n", i);
    my_set curr_set = cache.sets[i];
    for(int j = 0; j < E; j++){
        my_line curr_line = curr_set.lines[j];
        printf("  L%d: ", j);
        printf("valid = %d, tag = %d, last use = %d\n", curr_line.valid, curr_line.tag, curr_line.last_use);
    }
  }
}

/*
 * initCache - Allocate memory (with malloc) for cache data structures (i.e., for each of the sets and lines per set),
 * writing 0's for valid and tag and LRU
 *
 * TODO: Implement
 *
 */
void initCache()
{
  //allocates space for S long array of sets inside cache
  cache.sets = malloc(S * sizeof(my_set));

  for(int i = 0; i < S; i++){
    my_set curr_set; //creates a set to be added to the cache
    curr_set.lines = malloc(E * sizeof(my_line)); //allocates space for E long array of lines inside current set

    for(int j = 0; j < E; j++){
      //creates a line and adds it to the current set
      //initializes all line values to 0
      my_line curr_line;
      curr_line.valid = 0;
      curr_line.tag = 0;
      curr_line.last_use = 0;

      curr_set.lines[j] = curr_line;
    }

    cache.sets[i] = curr_set;
  }
}

/*
 * freeCache - free allocated memory
 *
 * This function deallocates (with free) the cache data structures of each
 * set and line.
 *
 * TODO: Implement
 */
void freeCache()
{
  for(int i = 0; i < S; i++){
    free(cache.sets[i].lines);
  }
  free(cache.sets);
}


/*
 * accessData - Access data at memory address addr
 *   If it is already in cache, increase hit_count
 *   If it is not in cache, bring it in cache, increase miss count.
 *   Also increase eviction_count if a line is evicted.
 *
 * TODO: Implement
 */
void accessData(mem_addr_t addr)
{
  //address is 64 bit number
  //0-b contains block offset
  //b-s contains set index
  //s-64 contains tag
  mem_addr_t mask = 0xffffffffffffffff;
  mem_addr_t set_val = (addr >> b) & ~(mask << s);
  mem_addr_t tag_val = (addr >> (s + b)) & ~(mask << (64 - s - b));

  int set_full = 1; //if after for loop cache is full, will remain 1, else 0
  int hit = 0; //if after the for loop hit is 1, there was a cache hit
  int hit_index = 0; //stores which line was hit
  my_set curr_set = cache.sets[set_val]; //searches set specified in addr

  //searches through all lines in current set
  //if a line has a valid bit=1 and has the same tag as addr, cache hit
  //if a line has a valid bit=0, the set is not full
  for(int i = 0; i < E; i++){
    if(curr_set.lines[i].valid == 0){
      set_full = 0;
    }
    else if(curr_set.lines[i].valid == 1 && curr_set.lines[i].tag == tag_val){
      hit = 1;
      hit_index = i;
    }
  }

  if(hit == 1){
    hit_count++;
    //resets last access time of the line to current time
    curr_set.lines[hit_index].last_use = lru_counter;
    if(verbosity == 1){ printf(" hit"); }
  }
  else if(hit == 0){
    miss_count++;
    if(verbosity == 1){ printf(" miss"); }
  }

  //if a miss and the set is not full, store address in an empty line
  if(hit == 0 && set_full == 0){
    for(int i = 0; i < E; i++){
      if(curr_set.lines[i].valid == 0){
        curr_set.lines[i].tag = tag_val;
        curr_set.lines[i].valid = 1;
        curr_set.lines[i].last_use = lru_counter;
        break;
      }
    }
  }

  //if a miss and set is full, evict and replace least recently used line
  if(hit == 0 && set_full == 1){
    //finds the line in set with lowest timestamp
    int low_val = lru_counter, low_index = 0;
    for(int i = 0; i < E; i++){
      if(curr_set.lines[i].last_use < low_val){
        low_index = i;
        low_val = curr_set.lines[i].last_use;
      }
    }

    //stores address in this line
    curr_set.lines[low_index].tag = tag_val;
    curr_set.lines[low_index].last_use = lru_counter;

    eviction_count++;
    if(verbosity == 1){ printf(" eviction"); }
  }

  lru_counter++;
}

/*
 * replayTrace - replays the given trace file against the cache
 *
 * This function:
 * - opens file trace_fn for reading (using fopen)
 * - reads lines (e.g., using fgets) from the file handle (may name `trace_fp` variable)
 * - skips lines not starting with ` S`, ` L` or ` M`
 * - parses the memory address (unsigned long, in hex) and len (unsigned int, in decimal)
 *   from each input line
 * - calls `access_data(address)` for each access to a cache line
 *
 * TODO: Implement
 *
 */
void replayTrace(char* trace_fn)
{
  //opens the file the user inputted
  FILE *input_file;
  input_file = fopen(trace_fn, "r");

  //does nothing if user enters invalid filename
  if(input_file == NULL){
    printf("Error: could not find trace file");
    return;
  }

  //each line contains an instruction, its target address, and the size
  unsigned int address;
  char instruction;
  unsigned int size;
  char file_line[32]; //contains line of file to be parsed

  //reads lines from input_file until it reaches an empty line
  while(fgets(file_line, 32, input_file) != NULL){

    //removes newline character at end of line
    int count = 0;
    while(file_line[count] != '\n'){
      count++;
    }
    file_line[count] = '\0';

    //parses string based on standard format " inst addr,size"
    //automatically converts hex address to decimal
    sscanf(file_line, " %c %x,%d", &instruction, &address, &size);

    if(verbosity == 1){ printf("%c %x,%d", instruction, address, size); }

    //if M, access cache twice
    //if L, S, access cache once
    //if I, do nothing
    switch(instruction){
      case 'M':
        accessData(address);
        accessData(address);
        break;
      case 'L':
        accessData(address);
        break;
      case 'S':
        accessData(address);
        break;
      case 'I':
        break;
    }

    if(verbosity == 1){ printf("\n"); }

  }

  fclose(input_file); //closes file
}

/*
 * printUsage - Print usage info
 */
void printUsage(char* argv[])
{
    printf("Usage: %s [-hv] -s <num> -E <num> -b <num> -t <file>\n", argv[0]);
    printf("Options:\n");
    printf("  -h         Print this help message.\n");
    printf("  -v         Optional verbose flag.\n");
    printf("  -s <num>   Number of set index bits.\n");
    printf("  -E <num>   Number of lines per set.\n");
    printf("  -b <num>   Number of block offset bits.\n");
    printf("  -t <file>  Trace file.\n");
    printf("\nExamples:\n");
    printf("  linux>  %s -s 4 -E 1 -b 4 -t traces/yi.trace\n", argv[0]);
    printf("  linux>  %s -v -s 8 -E 2 -b 4 -t traces/yi.trace\n", argv[0]);
    exit(0);
}

/*
 *
 * !! DO NOT MODIFY !!
 *
 * printSummary - Summarize the cache simulation statistics. Student cache simulators
 *                must call this function in order to be properly autograded.
 */
void printSummary(int hits, int misses, int evictions)
{
    printf("hits:%d misses:%d evictions:%d\n", hits, misses, evictions);
    FILE* output_fp = fopen(".csim_results", "w");
    assert(output_fp);
    fprintf(output_fp, "%d %d %d\n", hits, misses, evictions);
    fclose(output_fp);
}

/*
 * main - Main routine
 */
int main(int argc, char* argv[])
{
    char c;

    while( (c=getopt(argc,argv,"s:E:b:t:vh")) != -1){
        switch(c){
        case 's':
            s = atoi(optarg);
            break;
        case 'E':
            E = atoi(optarg);
            break;
        case 'b':
            b = atoi(optarg);
            break;
        case 't':
            trace_file = optarg;
            break;
        case 'v':
            verbosity = 1;
            break;
        case 'h':
            printUsage(argv);
            exit(0);
        default:
            printUsage(argv);
            exit(1);
        }
    }

    /* Make sure that all required command line args were specified */
    if (s == 0 || E == 0 || b == 0 || trace_file == NULL) {
        printf("%s: Missing required command line argument\n", argv[0]);
        printUsage(argv);
        exit(1);
    }

    /* Compute S, E and B from command line args */
    S = (unsigned int) pow(2, s);
    B = (unsigned int) pow(2, b);

    /* Initialize cache */
    initCache();

#ifdef DEBUG_ON
    printf("DEBUG: S:%u E:%u B:%u trace:%s\n", S, E, B, trace_file);
#endif

    replayTrace(trace_file);

    /* Free allocated memory */
    freeCache();

    /* Output the hit and miss statistics for the autograder */
    printSummary(hit_count, miss_count, eviction_count);

    return 0;
}
