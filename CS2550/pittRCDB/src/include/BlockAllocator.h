#include "Page.h"
#include "DiskManager.h"
using namespace std;

#ifndef BLOCK_ALLOCATOR_HEADER
#define BLOCK_ALLOCATOR_HEADER
struct Node {
  country_of_origin key;
  Node *next;
  Node *prev;
};

struct alloc_info {
  int offset;
  Node *loc;
};

class BlockAllocator {
public:
  int len;
  Node *head, *tail;
  map<string,alloc_info> block_map;

  BlockAllocator();

  void delete_node(Node *to_delete);

  Node* insert_after(Node *before, country_of_origin key);

  Node* insert_head(country_of_origin key);

  Node* insert_tail(country_of_origin key);

  int block_insert(string block_id, FileManager *mem);

  void free_block(Node *to_free);

  country_of_origin make_header(string block_id, int total_lines, int used_lines);

  int get_min_free_block(int size, alloc_info *min_block);

  int get_block_from_offset(int offset, country_of_origin *val);

  bool is_free(country_of_origin block);

  void print_allocations();
};

#endif // BLOOMFILTER_HEADER