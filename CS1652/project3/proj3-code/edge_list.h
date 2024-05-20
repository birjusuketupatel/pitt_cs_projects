/*
 * CS 1652 Project 3
 * (c) Amy Babay, 2022
 * (c) Birju Patel, Carter Moriarity
 *
 * Computer Science Department
 * University of Pittsburgh
 */

#ifndef __EDGE_LIST_H__
#define __EDGE_LIST_H__

#include "node_list.h"

#define MAX_EDGES 500

struct edge {
    uint32_t      src_id;
    uint32_t      dst_id;
    uint32_t      cost;
    uint32_t      outstanding_heartbeats;
    struct node * src_node;
    struct node * dst_node;
};

struct edge_list {
    struct edge *edges[MAX_EDGES];
    int num_edges;
};

/* Add edge to edge list.
 *     returns pointer to edge if added successfully, NULL if list is full.
 *   Note that this functions creates a new edge struct and copies the edge
 *   param passed in into it */
struct edge * add_edge_to_list(struct edge_list *list, struct edge e);

/* Get edge from edge list.
 * returns pointer to edge if one exists from src_id to dst_id
 * else returns NULL
 */
struct edge * get_edge_from_list(struct edge_list *list, uint32_t src_id, uint32_t dst_id);

#endif
