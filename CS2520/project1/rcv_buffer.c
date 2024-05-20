#include "rcv_buffer.h"

rcv_buffer *new_buffer() 
{
    rcv_buffer *b = malloc(sizeof(rcv_buffer));

    if (b == NULL)
    {
        printf("Buffer error: not enough memory for buffer");
        exit(1);
    }

    b->head = NULL;
    b->size = 0;

    return b;
}

void free_buffer(rcv_buffer *buffer)
{
    buffer_node *curr;
    buffer_node *next_node;
    if (buffer == NULL)
        return;

    curr = buffer->head;
    while(curr != NULL){
        next_node = curr->next;
        free(curr);
        curr = next_node;
    }
    
    free(buffer);
}

bool buffer_insert(rcv_buffer *buffer, struct upkt *packet)
{
    buffer_node *new_node;
    buffer_node *curr, *next;

    if (buffer == NULL)
        return false;    
    new_node = malloc(sizeof(buffer_node));

    if (new_node == NULL)
    {
        printf("Buffer error: not enough memory for new node");
        exit(1);
    }

    buffer->size += 1;

    new_node->seq = packet->seq;
    new_node->next = NULL;
    memset(new_node->payload, 0, sizeof(new_node->payload));
    new_node->payload_len = packet->payload_len;
    strncpy(new_node->payload, packet->payload, packet->payload_len);
    curr = buffer->head;
    // If the list is empty, simply add to the front
    // Otherwise, add it in order according to sequence number
    if (curr == NULL)
    { 
        buffer->head = new_node;
    } else {
        if(curr->seq == new_node->seq)
        {
            free(new_node);
            return false;
        }
        if(curr->seq > new_node->seq)
        {
            new_node->next = curr;
            buffer->head = new_node;
        }
        else
        {
            next = curr->next;
            while (next != NULL)
            {
                if(next->seq == new_node->seq)
                {
                    free(new_node);
                    return false;
                }
                if(next->seq > new_node->seq)
                {
                    new_node->next = next;
                    curr->next = new_node;
                    return true;
                }
                curr = next;
                next = curr->next;
            }
            // If we haven't added the node to the list by now, simply add it to the end
            curr->next = new_node;
        }
    }

    return true;
}

bool free_head(rcv_buffer *buffer)
{
    buffer_node *curr;
    curr = buffer->head;
    if(curr == NULL)
        return false;

    buffer->head = curr->next;
    buffer->size -= 1;

    free(curr);

    return true;
}