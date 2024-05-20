#include <stdbool.h>
#define MAX_GAPS 100

typedef struct gap {
    int start; //inclusive
    int end;  //not inclusive
} gap;

typedef struct rcv_msg {
    bool is_rcv_msg;
    int aru; // All received up to
    int num_gaps; // Number of gaps included in message
    gap gaps[MAX_GAPS];
} rcv_msg;

typedef struct node
{
    gap *gap_data;
    struct node *next;
    struct node *prev;
} node;

typedef struct gap_list
{
    node *head;
    node *tail;
    int size;
} gap_list;

gap_list *new_list();

void free_list(gap_list *list);

bool insert(gap_list *list, gap *new_gap);

void print_list(gap_list *list);

void print_list_reverse(gap_list *list);

// If a split_point fills any gaps in the list:
// split a gap in two, or if a gap becomes completely filled, drop it from the list
bool split_gap(gap_list *list, int split_point);
