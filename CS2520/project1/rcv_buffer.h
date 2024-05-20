#include "net_include.h"

#define MAX_MESS_LEN 1400

typedef struct buffer_node 
{
    int seq;
    struct buffer_node *next;
    int payload_len;
    char     payload[MAX_MESS_LEN];
} buffer_node;

typedef struct rcv_buffer 
{
    buffer_node *head;
    int size;
} rcv_buffer;

rcv_buffer *new_buffer();

void free_buffer(rcv_buffer *buffer);

bool buffer_insert(rcv_buffer *buffer, struct upkt *packet);

bool free_head(rcv_buffer *buffer);