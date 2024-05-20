#include <stdio.h>
#include <stdlib.h>
#include "gap_list.h"


gap_list *new_list()
{
    gap_list *g = malloc(sizeof(gap_list));

    if (g == NULL)
    {
        printf("Gap_List: No memory available\n");
        exit(1);
    }

    g->head = NULL;
    g->tail = NULL;
    g->size = 0;

    return g;
}

void free_list(gap_list *list)
{
    node *curr;
    node *next_node;
    if (list == NULL)
        return;

    curr = list->head;
    while(curr != NULL){
        free(curr->gap_data);
        next_node = curr->next;
        free(curr);
        curr = next_node;
    }

    free(list);
}

bool insert(gap_list *list, gap *new_gap)
{
    node *new_node;

    if (list == NULL)
        return false;

    new_node = malloc(sizeof(node));

    if (new_node == NULL)
    {
        printf("Gap_List: No memory available for new node\n");
        exit(1);
    }

    new_node->gap_data = malloc(sizeof(gap));

    if (new_node->gap_data == NULL)
    {
        free(new_node);
        printf("Gap_List: No memory avaible for gap data\n");
        exit(1);
    }

    new_node->next = NULL;
    new_node->prev = NULL;
    new_node->gap_data->start = new_gap->start;
    new_node->gap_data->end = new_gap->end;

    // If the list is empty, simply add to the front
    // Otherwise, since it is impossible for a new gap to appear between two preexisting gaps, add to the end
    if (list->head == NULL)
    {
        list->head = new_node;
        list->tail = new_node;
    } else {
        list->tail->next = new_node;
        new_node->prev = list->tail;
        list->tail = new_node;
    }

    list->size += 1;

    return true;
}

// Given a split_point adjust the endpoints of the affected node if any
// If the node's endpoints equal one another, this gap can be dropped
bool split_gap(gap_list *list, int split_point)
{
    node *curr, *new_node;
    for(curr = list->head; curr != NULL; curr = curr->next)
    {
        // If split point is less than start, return false as it won't be found in any future nodes
        if (split_point < curr->gap_data->start)
            return false;

        // If it is greater than this node, continue to the next one
        if(split_point >= curr->gap_data->end)
            continue;

        // Check endpoints first to see if we can just move them forward or back 1
        if (split_point == curr->gap_data->start) {
            curr->gap_data->start += 1;
        } else if (split_point == curr->gap_data->end-1) {
            curr->gap_data->end -= 1;
        } else {
            // Otherwise we need to create a new gap node
            new_node = malloc(sizeof(node));
            if(new_node == NULL)
                return false;

            new_node->gap_data = malloc(sizeof(gap));
            if(new_node->gap_data == NULL)
                return false;

            new_node->gap_data->start = split_point+1;
            new_node->gap_data->end = curr->gap_data->end;
            new_node->prev = curr;

            if(curr->next == NULL)
                list->tail = new_node;
            else
                curr->next->prev = new_node;
            new_node->next = curr->next;

            curr->gap_data->end = split_point;
            curr->next = new_node;

            list->size += 1;

            return true;
        }

        // If the endpoints equal one another, we no longer have a gap and we can drop this from the list
        if(curr->gap_data->start >= curr->gap_data->end)
        {
            list->size -= 1;

            if(curr->next == NULL && curr->prev == NULL)
            {
                list->size = 0;
                list->tail = NULL;
                list->head = NULL;
            }
            else if(curr->next == NULL)
            {
                list->tail = curr->prev;
                curr->prev->next = NULL;
            }
            else if(curr->prev == NULL)
            {
                list->head = curr->next;
                curr->next->prev = NULL;
            }
            else
            {
                curr->prev->next = curr->next;
                curr->next->prev = curr->prev;
            }

            free(curr->gap_data);
            free(curr);
        }

        return true;
    }

    return false;
}

void print_list(gap_list *list){
  node *curr;
  printf("         current gap list: ");
  for(curr = list->head; curr != NULL; curr = curr->next){
    printf("(%d,%d)->", curr->gap_data->start, curr->gap_data->end);
  }
  printf("\n");
}

void print_list_reverse(gap_list *list){
  node *curr;
  printf("current gap list reversed: ");
  for(curr = list->tail; curr != NULL; curr = curr->prev){
    printf("(%d,%d)->", curr->gap_data->start, curr->gap_data->end);
  }
  printf("\n");
}
