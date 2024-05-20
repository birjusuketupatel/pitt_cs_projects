/*
 * CS 1652 Project 3
 * (c) Amy Babay, 2022
 * (c) Birju Patel, Carter Moriarity
 *
 * Computer Science Department
 * University of Pittsburgh
 */


#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/errno.h>

#include <spu_alarm.h>
#include <spu_events.h>

#include "packets.h"
#include "client_list.h"
#include "node_list.h"
#include "edge_list.h"

#define PRINT_DEBUG 0

#define MAX_CONF_LINE 1024
#define INFINITY 4294967295
#define MAX_OUTSTANDING_HEARTBEATS 10

enum mode {
    MODE_NONE,
    MODE_LINK_STATE,
    MODE_DISTANCE_VECTOR,
};

struct link_state {
  uint32_t * distances;
  bool *     in_set;
  uint32_t * parent_of;
  uint32_t   num_nodes;
};

struct dv {
  uint32_t ** vectors;
  uint32_t num_adj_nodes;
  uint32_t num_nodes;
};

void reset_link_state(struct link_state * ls);
void calc_link_state(struct link_state * ls);
void set_forwarding_table_ls(struct link_state * ls);
uint32_t find_parent(struct link_state * ls, uint32_t target_id, uint32_t curr_id, uint32_t count);
void print_debug_link_state();

uint32_t get_edge_cost(struct edge * e);

void send_heartbeats();
void send_heartbeat_over_link(struct edge * e);

void broadcast_lsa(struct edge * target, bool alive);
void forward_lsa(uint32_t pkt_src_id, struct edge * target, bool alive);
void send_lsa_over_link(struct edge * target, bool alive, struct edge * e);

void calc_distance_vector(struct dv * table);
void broadcast_dv(struct dv * table);
void send_dv_over_link(struct dv * table, uint32_t target_id);
void print_debug_distance_vector();

static uint32_t           My_IP      = 0;
static uint32_t           My_ID      = 0;
static uint16_t           My_Port    = 0;
static enum mode          Route_Mode = MODE_NONE;
static struct client_list Client_List;
static struct node_list   Node_List;
static struct edge_list   Edge_List;
static struct link_state  Link_State;
static struct dv          Dist_Vec;

static const sp_time Data_Timeout = {1, 0};

/* Forward the packet to the next-hop node based on forwarding table */
void forward_data(struct data_pkt *pkt)
{
    Alarm(DEBUG, "overlay_node: forwarding data to overlay node %u, client port "
                 "%u\n", pkt->hdr.dst_id, pkt->hdr.dst_port);

    //we use the node list as the forwarding table
    struct node * dst_node = get_node_from_id(&Node_List, pkt->hdr.dst_id);

    //error, node not in map
    if(dst_node == NULL){
      Alarm(PRINT, "overlay_node: node with id = %u does not exist\n", pkt->hdr.dst_id);
      return;
    }

    if(dst_node->next_hop == NULL){
      Alarm(PRINT, "overlay_node: node with id = %u unreachable, packet dropped\n", pkt->hdr.dst_id);
      return;
    }

    struct node * forward_node = dst_node->next_hop;

    //stamp packet with path info
    if(pkt->hdr.path_len < 8){
      pkt->hdr.path[pkt->hdr.path_len] = My_ID;
      pkt->hdr.path_len++;
    }

    //send packet to forward_node via UDP socket
    int sock = -1;
    int ret = -1;

    if ((sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
        Alarm(EXIT, "overlay_node: UDP socket creation error\n");
        return;
    }

    int bytes = sizeof(struct data_pkt) - MAX_PAYLOAD_SIZE + pkt->hdr.data_len;

    ret = sendto(sock, pkt, bytes, 0, (struct sockaddr *) &forward_node->addr, sizeof(forward_node->addr));

    if (ret < 0) {
        Alarm(PRINT, "overlay_node: UDP socket send error\n");
    }

    close(sock);
}

/* Deliver packet to one of my local clients */
void deliver_locally(struct data_pkt *pkt)
{
    int path_len = 0;
    int bytes = 0;
    int ret = -1;
    struct client_conn *c = get_client_from_port(&Client_List, pkt->hdr.dst_port);

    /* Check whether we have a local client with this port to deliver to. If
     * not, nothing to do */
    if (c == NULL) {
        Alarm(PRINT, "overlay_node: received data for client that does not "
                     "exist! overlay node %d : client port %u\n",
                     pkt->hdr.dst_id, pkt->hdr.dst_port);
        return;
    }

    Alarm(DEBUG, "overlay_node: Delivering data locally to client with local "
                 "port %d\n", c->data_local_port);

    /* stamp packet so we can see the path taken */
    path_len = pkt->hdr.path_len;
    if (path_len < MAX_PATH) {
        pkt->hdr.path[path_len] = My_ID;
        pkt->hdr.path_len++;
    }

    /* Send data to client */
    bytes = sizeof(struct data_pkt) - MAX_PAYLOAD_SIZE + pkt->hdr.data_len;
    ret = sendto(c->data_sock, pkt, bytes, 0,
                 (struct sockaddr *)&c->data_remote_addr,
                 sizeof(c->data_remote_addr));
    if (ret < 0) {
        Alarm(PRINT, "Error sending to client with sock %d %d:%d\n",
              c->data_sock, c->data_local_port, c->data_remote_port);
        goto err;
    }

    return;

err:
    remove_client_with_sock(&Client_List, c->control_sock);
}

/* Handle incoming data message from another overlay node. Check whether we
 * need to deliver locally to a connected client, or forward to the next hop
 * overlay node */
void handle_overlay_data(int sock, int code, void *data)
{
    int bytes;
    struct data_pkt pkt;
    struct sockaddr_in recv_addr;
    socklen_t fromlen;

    Alarm(DEBUG, "overlay_node: received overlay data msg!\n");

    fromlen = sizeof(recv_addr);
    bytes = recvfrom(sock, &pkt, sizeof(pkt), 0, (struct sockaddr *)&recv_addr,
                     &fromlen);
    if (bytes < 0) {
        Alarm(EXIT, "overlay node: Error receiving overlay data: %s\n",
              strerror(errno));
    }

    /* If there is data to forward, find next hop and forward it */
    if (pkt.hdr.data_len > 0) {
        char tmp_payload[MAX_PAYLOAD_SIZE+1];
        memcpy(tmp_payload, pkt.payload, pkt.hdr.data_len);
        tmp_payload[pkt.hdr.data_len] = '\0';
        Alarm(DEBUG, "Got forwarded data packet of %d bytes: %s\n",
              pkt.hdr.data_len, tmp_payload);

        if (pkt.hdr.dst_id == My_ID) {
            deliver_locally(&pkt);
        } else {
            forward_data(&pkt);
        }
    }
}

/* Respond to heartbeat message by sending heartbeat echo */
void handle_heartbeat(struct heartbeat_pkt *pkt)
{
    if (pkt->hdr.type != CTRL_HEARTBEAT) {
        Alarm(PRINT, "Error: non-heartbeat msg in handle_heartbeat\n");
        return;
    }

    Alarm(DEBUG, "Got heartbeat from %d\n", pkt->hdr.src_id);

    //send heartbeat echo to the sender immediately
    struct heartbeat_echo_pkt * echo_pkt = (struct heartbeat_echo_pkt *) malloc(sizeof(struct heartbeat_echo_pkt));

    echo_pkt->hdr.type = CTRL_HEARTBEAT_ECHO;
    echo_pkt->hdr.src_id = My_ID;
    echo_pkt->hdr.dst_id = pkt->hdr.src_id;

    struct node * dst_node = get_node_from_id(&Node_List, pkt->hdr.src_id);

    //send heartbeat packet on adjacent link via UDP
    int sock = -1;
    int ret = -1;

    if ((sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
        Alarm(EXIT, "overlay_node: UDP socket creation error\n");
        return;
    }

    int bytes = sizeof(struct heartbeat_echo_pkt);

    ret = sendto(sock, echo_pkt, bytes, 0, (struct sockaddr *) &dst_node->ctrl_addr, sizeof(dst_node->ctrl_addr));

    if (ret < 0) {
        Alarm(PRINT, "overlay_node: UDP socket send error\n");
    }

    close(sock);
}

/* Handle heartbeat echo. This indicates that the link is alive, so update our
 * link weights and send update if we previously thought this link was down.
 * Push forward timer for considering the link dead */
void handle_heartbeat_echo(struct heartbeat_echo_pkt *pkt)
{
    if (pkt->hdr.type != CTRL_HEARTBEAT_ECHO) {
        Alarm(PRINT, "Error: non-heartbeat_echo msg in "
                     "handle_heartbeat_echo\n");
        return;
    }

    Alarm(DEBUG, "Got heartbeat_echo from %d\n", pkt->hdr.src_id);

    //gets the edge this heartbeat_echo was sent over
    struct edge * e = get_edge_from_list(&Edge_List, My_ID, pkt->hdr.src_id);

    //if link was previously dead, wake up link and recalculate
    if(e->outstanding_heartbeats > MAX_OUTSTANDING_HEARTBEATS){
      e->outstanding_heartbeats = 0;

      if(Route_Mode == MODE_LINK_STATE){
        reset_link_state(&Link_State);
        calc_link_state(&Link_State);
        set_forwarding_table_ls(&Link_State);

        //this link is now alive
        broadcast_lsa(e, true);

        print_debug_link_state();
      }
      else if(Route_Mode == MODE_DISTANCE_VECTOR){
        calc_distance_vector(&Dist_Vec);
      }
    }

    e->outstanding_heartbeats = 0;
}

/*
 * broadcasts to all adjacent nodes the status of a given edge
 */
void broadcast_lsa(struct edge * target, bool alive)
{
  struct edge * e;

  for(uint32_t i = 0; i < Edge_List.num_edges; i++){
    e = Edge_List.edges[i];

    //edge is connected to this node
    if(e->src_id == My_ID){
      send_lsa_over_link(target, alive, e);
    }
  }
}

/*
 * sends lsa about target along a given edge
 */
 void send_lsa_over_link(struct edge * target, bool alive, struct edge * e)
 {
   struct node * adj_node = e->dst_node;

   struct lsa_pkt * lsa = (struct lsa_pkt *) malloc(sizeof(struct lsa_pkt));

   lsa->hdr.type = CTRL_LSA;
   lsa->hdr.dst_id = adj_node->id;
   lsa->hdr.src_id = My_ID;
   lsa->target_src_id = target->src_id;
   lsa->target_dst_id = target->dst_id;
   lsa->alive = alive;

   //send lsa packet on adjacent link via UDP
   int sock = -1;
   int ret = -1;

   if ((sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
       Alarm(EXIT, "overlay_node: UDP socket creation error\n");
       return;
   }

   int bytes = sizeof(struct lsa_pkt);

   ret = sendto(sock, lsa, bytes, 0, (struct sockaddr *) &adj_node->ctrl_addr, sizeof(adj_node->ctrl_addr));

   if (ret < 0) {
       Alarm(PRINT, "overlay_node: UDP socket send error\n");
   }

   close(sock);
 }

/* Process received link state advertisement */
void handle_lsa(struct lsa_pkt *pkt)
{
    if (pkt->hdr.type != CTRL_LSA) {
        Alarm(PRINT, "Error: non-lsa msg in handle_lsa\n");
        return;
    }

    if (Route_Mode != MODE_LINK_STATE) {
        Alarm(PRINT, "Error: LSA msg but not in link state routing mode\n");
    }

    Alarm(DEBUG, "Got lsa from %d\n", pkt->hdr.src_id);

    struct edge * target_edge = get_edge_from_list(&Edge_List, pkt->target_src_id, pkt->target_dst_id);
    struct edge * inverse_edge = get_edge_from_list(&Edge_List, pkt->target_dst_id, pkt->target_src_id);

    if(target_edge == NULL || inverse_edge == NULL){
      Alarm(PRINT, "Error: LSA msg for link that does not exist\n");
    }

    bool target_alive = target_edge->outstanding_heartbeats <= MAX_OUTSTANDING_HEARTBEATS || inverse_edge->outstanding_heartbeats <= MAX_OUTSTANDING_HEARTBEATS;

    if(pkt->alive && !target_alive){
      //wake up a sleeping edge
      target_edge->outstanding_heartbeats = 0;
      target_edge->outstanding_heartbeats = 0;

      inverse_edge->outstanding_heartbeats = 0;
      inverse_edge->outstanding_heartbeats = 0;
    }
    else if(!pkt->alive && target_alive){
      //put an alive edge to sleep
      target_edge->outstanding_heartbeats = MAX_OUTSTANDING_HEARTBEATS + 1;
      target_edge->outstanding_heartbeats = MAX_OUTSTANDING_HEARTBEATS + 1;

      inverse_edge->outstanding_heartbeats = MAX_OUTSTANDING_HEARTBEATS + 1;
      inverse_edge->outstanding_heartbeats = MAX_OUTSTANDING_HEARTBEATS + 1;
    }

    if((pkt->alive && !target_alive) || (!pkt->alive && target_alive)){
      //recalculate link states
      reset_link_state(&Link_State);
      calc_link_state(&Link_State);
      set_forwarding_table_ls(&Link_State);

      //only forwards lsa if lsa caused change in node's forwarding table
      forward_lsa(pkt->hdr.src_id, target_edge, pkt->alive);

      print_debug_link_state();
    }
}

/*
 * sends lsa to all adjacent nodes except the one that sent this packet
 */
void forward_lsa(uint32_t pkt_src_id, struct edge * target, bool alive)
{
  struct edge * e;

  for(uint32_t i = 0; i < Edge_List.num_edges; i++){
    e = Edge_List.edges[i];

    //forward packet along all adjacent edges
    if(e->src_id == My_ID && e->dst_id != pkt_src_id){
      send_lsa_over_link(target, alive, e);
    }
  }
}

/* Process received distance vector update */
void handle_dv(struct dv_pkt *pkt)
{
    if (pkt->hdr.type != CTRL_DV) {
        Alarm(PRINT, "Error: non-dv msg in handle_dv\n");
        return;
    }

    if (Route_Mode != MODE_DISTANCE_VECTOR) {
        Alarm(PRINT, "Error: Distance Vector Update msg but not in distance "
                     "vector routing mode\n");
    }

    Alarm(DEBUG, "Got dv from %d\n", pkt->hdr.src_id);

    //prints all data from DV
    Alarm(DEBUG, "Received distance vector, length = %u\n", pkt->num_nodes);
    Alarm(DEBUG, "Vector: ");
    for(uint32_t i = 0; i < pkt->num_nodes; i++){
      Alarm(DEBUG, "%u ", pkt->vec[i]);
    }
    Alarm(DEBUG, "\n");

    //find row to copy data into
    uint32_t target_index = -1;
    for(uint32_t i = 0; i <= Dist_Vec.num_adj_nodes; i++){
      if(Dist_Vec.vectors[i][0] == pkt->hdr.src_id){
        target_index = i;
        break;
      }
    }

    if(target_index == -1){
      Alarm(PRINT, "Error: Distance Vector update sent to wrong node\n");
      return;
    }

    //copy received distance vector
    for(uint32_t i = 1; i <= Dist_Vec.num_nodes; i++){
      Dist_Vec.vectors[target_index][i] = pkt->vec[i - 1];
    }

    //recalculate distance vector
    calc_distance_vector(&Dist_Vec);

    print_debug_distance_vector();
}

/* Process received overlay control message. Identify message type and call the
 * relevant "handle" function */
void handle_overlay_ctrl(int sock, int code, void *data)
{
    char buf[MAX_CTRL_SIZE];
    struct sockaddr_in recv_addr;
    socklen_t fromlen;
    struct ctrl_hdr * hdr = NULL;
    int bytes = 0;

    Alarm(DEBUG, "overlay_node: received overlay control msg!\n");

    fromlen = sizeof(recv_addr);
    bytes = recvfrom(sock, buf, sizeof(buf), 0, (struct sockaddr *)&recv_addr,
                     &fromlen);
    if (bytes < 0) {
        Alarm(EXIT, "overlay node: Error receiving ctrl message: %s\n",
              strerror(errno));
    }
    hdr = (struct ctrl_hdr *)buf;

    /* sanity check */
    if (hdr->dst_id != My_ID) {
        Alarm(PRINT, "overlay_node: Error: got ctrl msg with invalid dst_id: "
              "%d\n", hdr->dst_id);
    }

    if (hdr->type == CTRL_HEARTBEAT) {
        /* handle heartbeat */
        handle_heartbeat((struct heartbeat_pkt *)buf);
    } else if (hdr->type == CTRL_HEARTBEAT_ECHO) {
        /* handle heartbeat echo */
        handle_heartbeat_echo((struct heartbeat_echo_pkt *)buf);
    } else if (hdr->type == CTRL_LSA) {
        /* handle link state update */
        handle_lsa((struct lsa_pkt *)buf);
    } else if (hdr->type == CTRL_DV) {
        /* handle distance vector update */
        handle_dv((struct dv_pkt *)buf);
    }
}

void handle_client_data(int sock, int unused, void *data)
{
    int ret, bytes;
    struct data_pkt pkt;
    struct sockaddr_in recv_addr;
    socklen_t fromlen;
    struct client_conn *c;

    Alarm(DEBUG, "Handle client data\n");

    c = (struct client_conn *) data;
    if (sock != c->data_sock) {
        Alarm(EXIT, "Bad state! sock %d != data sock\n", sock, c->data_sock);
    }

    fromlen = sizeof(recv_addr);
    bytes = recvfrom(sock, &pkt, sizeof(pkt), 0, (struct sockaddr *)&recv_addr,
                     &fromlen);
    if (bytes < 0) {
        Alarm(PRINT, "overlay node: Error receiving from client: %s\n",
              strerror(errno));
        goto err;
    }

    /* Special case: initial data packet from this client. Use it to set the
     * source port, then ack it */
    if (c->data_remote_port == 0) {
        c->data_remote_addr = recv_addr;
        c->data_remote_port = ntohs(recv_addr.sin_port);
        Alarm(DEBUG, "Got initial data msg from client with sock %d local port "
                     "%u remote port %u\n", sock, c->data_local_port,
                     c->data_remote_port);

        /* echo pkt back to acknowledge */
        ret = sendto(c->data_sock, &pkt, bytes, 0,
                     (struct sockaddr *)&c->data_remote_addr,
                     sizeof(c->data_remote_addr));
        if (ret < 0) {
            Alarm(PRINT, "Error sending to client with sock %d %d:%d\n", sock,
                  c->data_local_port, c->data_remote_port);
            goto err;
        }
    }

    /* If there is data to forward, find next hop and forward it */
    if (pkt.hdr.data_len > 0) {
        char tmp_payload[MAX_PAYLOAD_SIZE+1];
        memcpy(tmp_payload, pkt.payload, pkt.hdr.data_len);
        tmp_payload[pkt.hdr.data_len] = '\0';
        Alarm(DEBUG, "Got data packet of %d bytes: %s\n", pkt.hdr.data_len, tmp_payload);

        /* Set up header with my info */
        pkt.hdr.src_id = My_ID;
        pkt.hdr.src_port = c->data_local_port;

        /* Deliver / Forward */
        if (pkt.hdr.dst_id == My_ID) {
            deliver_locally(&pkt);
        } else {
            forward_data(&pkt);
        }
    }

    return;

err:
    remove_client_with_sock(&Client_List, c->control_sock);

}

void handle_client_ctrl_msg(int sock, int unused, void *data)
{
    int bytes_read = 0;
    int bytes_sent = 0;
    int bytes_expected = sizeof(struct conn_req_pkt);
    struct conn_req_pkt rcv_req;
    struct conn_ack_pkt ack;
    int ret = -1;
    int ret_code = 0;
    char * err_str = "client closed connection";
    struct sockaddr_in saddr;
    struct client_conn *c;

    Alarm(DEBUG, "Client ctrl message, sock %d\n", sock);

    /* Get client info */
    c = (struct client_conn *) data;
    if (sock != c->control_sock) {
        Alarm(EXIT, "Bad state! sock %d != data sock\n", sock, c->control_sock);
    }

    if (c == NULL) {
        Alarm(PRINT, "Failed to find client with sock %d\n", sock);
        ret_code = -1;
        goto end;
    }

    /* Read message from client */
    while (bytes_read < bytes_expected &&
           (ret = recv(sock, ((char *)&rcv_req)+bytes_read,
                       sizeof(rcv_req)-bytes_read, 0)) > 0) {
        bytes_read += ret;
    }
    if (ret <= 0) {
        if (ret < 0) err_str = strerror(errno);
        Alarm(PRINT, "Recv returned %d; Removing client with control sock %d: "
                     "%s\n", ret, sock, err_str);
        ret_code = -1;
        goto end;
    }

    if (c->data_local_port != 0) {
        Alarm(PRINT, "Received req from already connected client with sock "
                     "%d\n", sock);
        ret_code = -1;
        goto end;
    }

    /* Set up UDP socket requested for this client */
    if ((c->data_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
        Alarm(PRINT, "overlay_node: client UDP socket error: %s\n", strerror(errno));
        ret_code = -1;
        goto send_resp;
    }

    /* set server address */
    memset(&saddr, 0, sizeof(saddr));
    saddr.sin_family = AF_INET;
    saddr.sin_addr.s_addr = INADDR_ANY;
    saddr.sin_port = htons(rcv_req.port);

    /* bind UDP socket */
    if (bind(c->data_sock, (struct sockaddr *)&saddr, sizeof(saddr)) < 0) {
        Alarm(PRINT, "overlay_node: client UDP bind error: %s\n", strerror(errno));
        ret_code = -1;
        goto send_resp;
    }

    /* Register socket with event handling system */
    ret = E_attach_fd(c->data_sock, READ_FD, handle_client_data, 0, c, MEDIUM_PRIORITY);
    if (ret < 0) {
        Alarm(PRINT, "Failed to register client UDP sock in event handling system\n");
        ret_code = -1;
        goto send_resp;
    }

send_resp:
    /* Send response */
    if (ret_code == 0) { /* all worked correctly */
        c->data_local_port = rcv_req.port;
        ack.id = My_ID;
    } else {
        ack.id = 0;
    }
    bytes_expected = sizeof(ack);
    Alarm(DEBUG, "Sending response to client with control sock %d, UDP port "
                 "%d\n", sock, c->data_local_port);
    while (bytes_sent < bytes_expected) {
        ret = send(sock, ((char *)&ack)+bytes_sent, sizeof(ack)-bytes_sent, 0);
        if (ret < 0) {
            Alarm(PRINT, "Send error for client with sock %d (removing...): "
                         "%s\n", sock, strerror(ret));
            ret_code = -1;
            goto end;
        }
        bytes_sent += ret;
    }

end:
    if (ret_code != 0 && c != NULL) remove_client_with_sock(&Client_List, sock);
}

void handle_client_conn(int sock, int unused, void *data)
{
    int conn_sock;
    struct client_conn new_conn;
    struct client_conn *ret_conn;
    int ret;

    Alarm(DEBUG, "Handle client connection\n");

    /* Accept the connection */
    conn_sock = accept(sock, NULL, NULL);
    if (conn_sock < 0) {
        Alarm(PRINT, "accept error: %s\n", strerror(errno));
        goto err;
    }

    /* Set up the connection struct for this new client */
    new_conn.control_sock     = conn_sock;
    new_conn.data_sock        = -1;
    new_conn.data_local_port  = 0;
    new_conn.data_remote_port = 0;
    ret_conn = add_client_to_list(&Client_List, new_conn);
    if (ret_conn == NULL) {
        goto err;
    }

    /* Register the control socket for this client */
    ret = E_attach_fd(new_conn.control_sock, READ_FD, handle_client_ctrl_msg,
                      0, ret_conn, MEDIUM_PRIORITY);
    if (ret < 0) {
        goto err;
    }

    return;

err:
    if (conn_sock >= 0) close(conn_sock);
}

void init_overlay_data_sock(int port)
{
    int sock = -1;
    int ret = -1;
    struct sockaddr_in saddr;

    if ((sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
        Alarm(EXIT, "overlay_node: data socket error: %s\n", strerror(errno));
    }

    /* set server address */
    memset(&saddr, 0, sizeof(saddr));
    saddr.sin_family = AF_INET;
    saddr.sin_addr.s_addr = INADDR_ANY;
    saddr.sin_port = htons(port);

    /* bind listening socket */
    if (bind(sock, (struct sockaddr *)&saddr, sizeof(saddr)) < 0) {
        Alarm(EXIT, "overlay_node: data bind error: %s\n", strerror(errno));
    }

    /* Register socket with event handling system */
    ret = E_attach_fd(sock, READ_FD, handle_overlay_data, 0, NULL, MEDIUM_PRIORITY);
    if (ret < 0) {
        Alarm(EXIT, "Failed to register overlay data sock in event handling system\n");
    }

}

void init_overlay_ctrl_sock(int port)
{
    int sock = -1;
    int ret = -1;
    struct sockaddr_in saddr;

    if ((sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
        Alarm(EXIT, "overlay_node: ctrl socket error: %s\n", strerror(errno));
    }

    /* set server address */
    memset(&saddr, 0, sizeof(saddr));
    saddr.sin_family = AF_INET;
    saddr.sin_addr.s_addr = INADDR_ANY;
    saddr.sin_port = htons(port);

    /* bind listening socket */
    if (bind(sock, (struct sockaddr *)&saddr, sizeof(saddr)) < 0) {
        Alarm(EXIT, "overlay_node: ctrl bind error: %s\n", strerror(errno));
    }

    /* Register socket with event handling system */
    ret = E_attach_fd(sock, READ_FD, handle_overlay_ctrl, 0, NULL, MEDIUM_PRIORITY);
    if (ret < 0) {
        Alarm(EXIT, "Failed to register overlay ctrl sock in event handling system\n");
    }
}

void init_client_sock(int client_port)
{
    int client_sock = -1;
    int ret = -1;
    struct sockaddr_in saddr;

    if ((client_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
        Alarm(EXIT, "overlay_node: client socket error: %s\n", strerror(errno));
    }

    /* set server address */
    memset(&saddr, 0, sizeof(saddr));
    saddr.sin_family = AF_INET;
    saddr.sin_addr.s_addr = INADDR_ANY;
    saddr.sin_port = htons(client_port);

    /* bind listening socket */
    if (bind(client_sock, (struct sockaddr *)&saddr, sizeof(saddr)) < 0) {
        Alarm(EXIT, "overlay_node: client bind error: %s\n", strerror(errno));
    }

    /* start listening */
    if (listen(client_sock, 32) < 0) {
        Alarm(EXIT, "overlay_node: client bind error: %s\n", strerror(errno));
        exit(-1);
    }

    /* Register socket with event handling system */
    ret = E_attach_fd(client_sock, READ_FD, handle_client_conn, 0, NULL, MEDIUM_PRIORITY);
    if (ret < 0) {
        Alarm(EXIT, "Failed to register client sock in event handling system\n");
    }

}

void init_link_state()
{
    Alarm(DEBUG, "init link state\n");

    //initializes a link_state struct
    Link_State.num_nodes = Node_List.num_nodes;
    Link_State.distances = (uint32_t *) malloc(Link_State.num_nodes * sizeof(uint32_t));
    Link_State.parent_of = (uint32_t *) malloc(Link_State.num_nodes * sizeof(uint32_t));
    Link_State.in_set = (bool *) malloc(Link_State.num_nodes * sizeof(bool));

    reset_link_state(&Link_State);

    calc_link_state(&Link_State);

    set_forwarding_table_ls(&Link_State);

    print_debug_link_state();
}
/*
 * Prints node ids, minimum distances, and current state of forwarding table.
 */
void print_debug_link_state()
{
  Alarm(DEBUG, "nodes: ");
  for(uint32_t i = 1; i <= Link_State.num_nodes; i++){
    Alarm(DEBUG, "%u ", i);
  }
  Alarm(DEBUG, "\n");

  Alarm(DEBUG, "minimum distances from node %u:\n", My_ID);
  for(uint32_t i = 0; i < Link_State.num_nodes; i++){
    Alarm(DEBUG, "%u ", Link_State.distances[i]);
  }
  Alarm(DEBUG, "\n");

  struct node * n;
  Alarm(DEBUG, "fowarding table of node %u:\n", My_ID);
  Alarm(DEBUG, "(destination node id, forward node id)\n");
  for(uint32_t i = 0; i < Node_List.num_nodes; i++){
    n = Node_List.nodes[i]->next_hop;

    if(n == NULL){
      Alarm(DEBUG, "(%u, NULL)\n", i + 1);
    }
    else{
      Alarm(DEBUG, "(%u, %u)\n", i + 1, n->id);
    }
  }
  Alarm(DEBUG, "\n");
}


/*
 * Before running Dijkstra's algorithm, the link_state struct must be reset.
 * By default, distance to this node = 0, distance to other nodes = infinity.
 * The value of parent_of denotes the index of the previous node in the shortest path to this node.
 * No node has had its edges enumerated.
 */
void reset_link_state(struct link_state * ls)
{
  //We assume node IDs are given in the form 1, 2, ... , N.
  //The distance to each node is stored at the index of ID - 1.
  uint32_t node_id;
  for(uint32_t i = 0; i < ls->num_nodes; i++){
    node_id = i + 1;

    if(node_id == My_ID){
      ls->distances[i] = 0;
      ls->parent_of[i] = i;
    }
    else{
      ls->distances[i] = INFINITY;
      ls->parent_of[i] = -1;
    }

    ls->in_set[i] = false;
  }
}

/*
 * Runs Dijkstra's algorithm to calculate distance to each node.
 */
void calc_link_state(struct link_state * ls)
{
  uint32_t curr_index = My_ID - 1, adj_index;
  uint32_t curr_node, adj_node;
  uint32_t edge_cost;
  struct edge * e;

  //parent of this node is itself
  ls->parent_of[curr_index] = My_ID;

  //iterate through all nodes
  for(uint32_t i = 0; i < ls->num_nodes; i++){
    curr_node = curr_index + 1;

    //iterate through all edges of current node
    for(uint32_t j = 0; j < Edge_List.num_edges; j++){
      e = Edge_List.edges[j];

      //no edge between src and dst
      if(e->src_id != curr_node){
        continue;
      }

      //edge found, check if new edge can shorten distance
      adj_node = e->dst_id;
      adj_index = adj_node - 1;

      edge_cost = get_edge_cost(e);

      //disregard sleeping edges
      if(edge_cost == INFINITY){
        continue;
      }

      if(ls->distances[curr_index] + edge_cost < ls->distances[adj_index]){
        ls->distances[adj_index] = ls->distances[curr_index] + edge_cost;
        ls->parent_of[adj_index] = curr_index + 1;
      }
    }

    //src_node has had all its edges enumerated
    ls->in_set[curr_index] = true;

    //find node not in set with smallest path
    uint32_t small_node = -1;
    uint32_t small_dist = INFINITY;
    for(uint32_t k = 0; k < ls->num_nodes; k++){
      if(ls->distances[k] < small_dist && !ls->in_set[k]){
        small_dist = ls->distances[k];
        small_node = k + 1;
      }
    }

    if(small_node == -1){
      //all reachable nodes have been checked
      break;
    }
    else{
      //now enumerate through edges of this node
      curr_index = small_node - 1;
    }
  }
}

/*
 * The cost of an edge is e->cost if its number of outstanding heartbeats is <= 10.
 * If it has > 10 outstanding heartbeats, the edge is considered dead and has a cost of infinity.
 */
uint32_t get_edge_cost(struct edge * e)
{
  if(e->outstanding_heartbeats > MAX_OUTSTANDING_HEARTBEATS){
    return INFINITY;
  }

  return e->cost;
}

/*
 * Assuming minimum path tree has been calculated, sets all forwarding nodes.
 * The forwarding node is the next node on the min cost path to a given node.
 */
void set_forwarding_table_ls(struct link_state * ls)
{
  uint32_t node_id, forward_id;
  struct node * curr_node;
  struct node * forward_node;

  for(uint32_t i = 0; i < ls->num_nodes; i++){
    node_id = i + 1;

    forward_id = find_parent(ls, My_ID, node_id, 0);

    //uses node_list as forwarding table
    //pointer to forwarding node stored in next_hop of current node
    curr_node = get_node_from_id(&Node_List, node_id);

    if(forward_id < 0){
      //no way to forward to this node
      curr_node->next_hop = NULL;
    }
    else{
      forward_node = get_node_from_id(&Node_List, forward_id);
      curr_node->next_hop = forward_node;
    }
  }
}

/*
 * Given a node id and assuming minimum path tree has been calculated, finds the forwarding node.
 */
uint32_t find_parent(struct link_state * ls, uint32_t target_id, uint32_t curr_id, uint32_t count)
{
  //infinite loop detection
  if(count > ls->num_nodes){
    return -1;
  }

  count++;

  uint32_t index = curr_id - 1;

  if(index < 0 || index >= ls->num_nodes){
    return -1;
  }

  uint32_t next_id = ls->parent_of[index];

  if(next_id == target_id){
    return curr_id;
  }

  return find_parent(ls, target_id, next_id, count);
}

void init_distance_vector()
{
    Alarm(DEBUG, "init distance vector\n");

    //empty forwarding table
    for(uint32_t i = 0; i < Node_List.num_nodes; i++){
      if(Node_List.nodes[i]->id == My_ID){
        Node_List.nodes[i]->next_hop = Node_List.nodes[i];
      }
      else{
        Node_List.nodes[i]->next_hop = NULL;
      }
    }

    /*
     * counts number of nodes adjacent to this node.
     * we must store the distance vector of each adjacent node
     * and the distance vector of this node.
     */
    Dist_Vec.num_adj_nodes = 0;
    struct edge * e;
    for(uint32_t i = 0; i < Edge_List.num_edges; i++){
      e = Edge_List.edges[i];

      if(e->src_id == My_ID){
        Dist_Vec.num_adj_nodes++;
      }
    }

    Dist_Vec.num_nodes = Node_List.num_nodes;

    //distance vector routing table is 2d array
    Dist_Vec.vectors = (uint32_t **) malloc((Dist_Vec.num_adj_nodes + 1) * sizeof(uint32_t *));

    for(uint32_t i = 0; i < Dist_Vec.num_adj_nodes + 1; i++){
      Dist_Vec.vectors[i] = (uint32_t *) malloc((Dist_Vec.num_nodes + 1) * sizeof(uint32_t));
    }

    //fills node ids in first column
    uint32_t index = 1;
    Dist_Vec.vectors[0][0] = My_ID;
    for(uint32_t i = 0; i < Edge_List.num_edges; i++){
      e = Edge_List.edges[i];

      if(e->src_id == My_ID){
        Dist_Vec.vectors[index][0] = e->dst_id;
        index++;
      }
    }

    /*
     * example of initialized table...
     * dist to node   1   2   3   4
     * vector 1 = (1  0  INF INF INF)
     * vector 2 = (2 INF  0  INF INF)
     * vector 3 = (3 INF INF  0  INF)
     *
     * column 0 = node id that sent vector
     * row i = distance vector of node i
     * row 0 = distance vector of this node, calculated from table and edge weights
     */
    uint32_t node_id;
    for(uint32_t i = 0; i <= Dist_Vec.num_adj_nodes; i++){
      node_id = Dist_Vec.vectors[i][0];

      for(uint32_t j = 1; j <= Dist_Vec.num_nodes; j++){
        if(j == node_id){
          Dist_Vec.vectors[i][j] = 0;
        }
        else{
          Dist_Vec.vectors[i][j] = INFINITY;
        }
      }
    }

    calc_distance_vector(&Dist_Vec);

    print_debug_distance_vector();
}

/*
 * Given the distance vectors of all neighbors and the
 * distance to each neighbor, calculates the distance vector of this node.
 */
void calc_distance_vector(struct dv * table)
{
  struct edge * e;
  struct node * adj_node;
  struct node * target_node;
  uint32_t adj_id, edge_cost;

  //saves previous state of distance vector
  uint32_t * prev_vec = (uint32_t *) malloc(table->num_nodes);

  //resets distance vector of this node to default
  for(uint32_t i = 1; i <= table->num_nodes; i++){
    prev_vec[i - 1] = table->vectors[0][i];

    if(i == My_ID){
      table->vectors[0][i] = 0;
    }
    else{
      table->vectors[0][i] = INFINITY;
    }
  }

  //empty forwarding table
  for(uint32_t i = 0; i < Node_List.num_nodes; i++){
    if(Node_List.nodes[i]->id == My_ID){
      Node_List.nodes[i]->next_hop = Node_List.nodes[i];
    }
    else{
      Node_List.nodes[i]->next_hop = NULL;
    }
  }

  for(uint32_t i = 1; i <= table->num_adj_nodes; i++){
    adj_id = table->vectors[i][0];
    adj_node = get_node_from_id(&Node_List, adj_id);

    e = get_edge_from_list(&Edge_List, My_ID, adj_id);
    edge_cost = get_edge_cost(e);

    //disregard sleeping edges
    if(edge_cost == INFINITY){
      continue;
    }

    for(uint32_t j = 1; j <= table->num_nodes; j++){
      //neighbor i has no path to j
      if(table->vectors[i][j] == INFINITY){
        continue;
      }

      //forwarding to this neighbor improves our path to node j, update path and cost
      if(table->vectors[i][j] + edge_cost < table->vectors[0][j]){
        //update path length
        table->vectors[0][j] = table->vectors[i][j] + edge_cost;

        //update forwarding table
        target_node = get_node_from_id(&Node_List, j);
        target_node->next_hop = adj_node;
      }
    }
  }

  //compares new and previous state of vector
  //if change occurred, broadcast new vector
  bool change_vec = false;

  for(uint32_t i = 1; i <= table->num_nodes; i++){
    if(table->vectors[0][i] != prev_vec[i - 1]){
      change_vec = true;
      break;
    }
  }

  if(change_vec){
    broadcast_dv(table);
  }
}

/*
 * Send distance vector to all neighbors
 */
void broadcast_dv(struct dv * table)
{
  struct edge * e;

  for(uint32_t i = 0; i < Edge_List.num_edges; i++){
    e = Edge_List.edges[i];

    if(e->src_id == My_ID){
      send_dv_over_link(table, e->dst_id);
    }
  }
}

/*
 * Sends distance vector to one neighbor
 */
void send_dv_over_link(struct dv * table, uint32_t target_id)
{
  struct node * target_node = get_node_from_id(&Node_List, target_id);

  if(target_node == NULL){
    Alarm(PRINT, "overlay_node: error, node with id %u does not exist", target_id);
    return;
  }

  struct dv_pkt * d_vec = (struct dv_pkt *) malloc(sizeof(struct dv_pkt));

  d_vec->hdr.type = CTRL_DV;
  d_vec->hdr.src_id = My_ID;
  d_vec->hdr.dst_id = target_id;

  d_vec->num_nodes = table->num_nodes;

  /*
   * count to infinity problem occurs when we advertise a path to a node that uses itself
   * if we route to a particular node to reach a destination, do not send it a path
   */
  struct node * n;
  for(uint32_t i = 1; i <= table->num_nodes; i++){
    n = get_node_from_id(&Node_List, i);

    if(n->next_hop != NULL && n->next_hop->id == target_id){
      d_vec->vec[i - 1] = INFINITY;
    }
    else{
      d_vec->vec[i - 1] = table->vectors[0][i];
    }
  }

  //send packet to neighbor via UDP socket
  int sock = -1;
  int ret = -1;

  if ((sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
      Alarm(EXIT, "overlay_node: UDP socket creation error\n");
      return;
  }

  int bytes = sizeof(struct dv_pkt) - MAX_VEC_SIZE + d_vec->num_nodes * 4;

  ret = sendto(sock, d_vec, bytes, 0, (struct sockaddr *) &target_node->ctrl_addr, sizeof(target_node->ctrl_addr));

  if (ret < 0) {
      Alarm(PRINT, "overlay_node: UDP socket send error\n");
  }

  close(sock);
}

/*
 * Prints state of distance vector
 */

void print_debug_distance_vector()
{
  Alarm(DEBUG, "state of dv, node id = %u:\n", My_ID);
  Alarm(DEBUG, "number of adjacent nodes: %u\n", Dist_Vec.num_adj_nodes);
  Alarm(DEBUG, "number of total nodes: %u\n", Dist_Vec.num_nodes);

  for(uint32_t i = 0; i < Dist_Vec.num_adj_nodes + 1; i++){
    Alarm(DEBUG, "vector %u: ", Dist_Vec.vectors[i][0]);

    for(uint32_t j = 1; j <= Dist_Vec.num_nodes; j++){
      Alarm(DEBUG, "%u ", Dist_Vec.vectors[i][j]);
    }
    Alarm(DEBUG, "\n");
  }

  struct node * n;
  Alarm(DEBUG, "fowarding table of node %u:\n", My_ID);
  Alarm(DEBUG, "(destination node id, forward node id)\n");
  for(uint32_t i = 0; i < Node_List.num_nodes; i++){
    n = Node_List.nodes[i]->next_hop;

    if(n == NULL){
      Alarm(DEBUG, "(%u, NULL)\n", i + 1);
    }
    else{
      Alarm(DEBUG, "(%u, %u)\n", i + 1, n->id);
    }
  }
  Alarm(DEBUG, "\n");
}

/*
 * Sends heartbeat over every adjacent edge once per second.
 */
void send_heartbeats()
{
  struct edge * e;

  for(uint32_t i = 0; i < Edge_List.num_edges; i++){
    e = Edge_List.edges[i];

    //edge is connected to this node
    if(e->src_id == My_ID){
      send_heartbeat_over_link(e);

      //link now has one more outstanding heartbeat it must respond to
      //after 10 outstanding heartbeats, link is considered dead
      if(e->outstanding_heartbeats <= MAX_OUTSTANDING_HEARTBEATS){
        e->outstanding_heartbeats++;
      }

      //an adjacent edge has now died
      //recalculate the forwarding table, send a link state update
      if(e->outstanding_heartbeats == MAX_OUTSTANDING_HEARTBEATS + 1){

        if(Route_Mode == MODE_LINK_STATE){
          reset_link_state(&Link_State);
          calc_link_state(&Link_State);
          set_forwarding_table_ls(&Link_State);

          //this link is now dead
          broadcast_lsa(e, false);

          print_debug_link_state();
        }
        else if(Route_Mode == MODE_DISTANCE_VECTOR){
          calc_distance_vector(&Dist_Vec);
          print_debug_distance_vector();
        }

        e->outstanding_heartbeats++;
      }
    }
  }

  /* Enqueue resending heartbeat in one second*/
  E_queue(send_heartbeats, 0, NULL, Data_Timeout);
}

/*
 * Sends heartbeat on a given edge.
 */
void send_heartbeat_over_link(struct edge * e)
{
  struct node * adj_node = e->dst_node;

  struct heartbeat_pkt * heart_pkt = (struct heartbeat_pkt *) malloc(sizeof(struct heartbeat_pkt));

  heart_pkt->hdr.type = CTRL_HEARTBEAT;
  heart_pkt->hdr.dst_id = adj_node->id;
  heart_pkt->hdr.src_id = My_ID;

  //send heartbeat packet on adjacent link via UDP
  int sock = -1;
  int ret = -1;

  if ((sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
      Alarm(EXIT, "overlay_node: UDP socket creation error\n");
  }

  int bytes = sizeof(struct heartbeat_pkt);

  ret = sendto(sock, heart_pkt, bytes, 0, (struct sockaddr *) &adj_node->ctrl_addr, sizeof(adj_node->ctrl_addr));

  if (ret < 0) {
      Alarm(PRINT, "overlay_node: UDP socket send error\n");
  }
}

uint32_t ip_from_str(char *ip)
{
    struct in_addr addr;

    inet_pton(AF_INET, ip, &addr);
    return ntohl(addr.s_addr);
}

void process_conf(char *fname, int my_id)
{
    char     buf[MAX_CONF_LINE];
    char     ip_str[MAX_CONF_LINE];
    FILE *   f        = NULL;
    uint32_t id       = 0;
    uint16_t port     = 0;
    uint32_t src      = 0;
    uint32_t dst      = 0;
    uint32_t cost     = 0;
    int node_sec_done = 0;
    int ret           = -1;
    struct node n;
    struct edge e;
    struct node *retn = NULL;
    struct edge *rete = NULL;

    Alarm(DEBUG, "Processing configuration file %s\n", fname);

    /* Open configuration file */
    f = fopen(fname, "r");
    if (f == NULL) {
        Alarm(EXIT, "overlay_node: error: failed to open conf file %s : %s\n",
              fname, strerror(errno));
    }

    /* Read list of nodes from conf file */
    while (fgets(buf, MAX_CONF_LINE, f)) {
        Alarm(DEBUG, "Read line: %s", buf);

        if (!node_sec_done) {
            // sscanf
            ret = sscanf(buf, "%u %s %hu", &id, ip_str, &port);
            Alarm(DEBUG, "    Node ID: %u, Node IP %s, Port: %u\n", id, ip_str, port);
            if (ret != 3) {
                Alarm(DEBUG, "done reading nodes\n");
                node_sec_done = 1;
                continue;
            }

            if (id == my_id) {
                Alarm(DEBUG, "Found my ID (%u). Setting IP and port\n", id);
                My_Port = port;
                My_IP = ip_from_str(ip_str);
            }

            n.id = id;

            memset(&n.addr, 0, sizeof(n.addr));
            n.addr.sin_family = AF_INET;
            n.addr.sin_addr.s_addr = htonl(ip_from_str(ip_str));
            n.addr.sin_port = htons(port);

            memset(&n.ctrl_addr, 0, sizeof(n.ctrl_addr));
            n.ctrl_addr.sin_family = AF_INET;
            n.ctrl_addr.sin_addr.s_addr = htonl(ip_from_str(ip_str));
            n.ctrl_addr.sin_port = htons(port + 1);

            n.next_hop = NULL;
            retn = add_node_to_list(&Node_List, n);
            if (retn == NULL) {
                Alarm(EXIT, "Failed to add node to list\n");
            }

        } else { /* Edge section */
            ret = sscanf(buf, "%u %u %u", &src, &dst, &cost);
            Alarm(DEBUG, "    Src ID: %u, Dst ID %u, Cost: %u\n", src, dst, cost);
            if (ret != 3) {
                Alarm(DEBUG, "done reading nodes\n");
                node_sec_done = 1;
                continue;
            }

            e.src_id = src;
            e.dst_id = dst;
            e.cost = cost;
            e.outstanding_heartbeats = 0;
            e.src_node = get_node_from_id(&Node_List, e.src_id);
            e.dst_node = get_node_from_id(&Node_List, e.dst_id);
            if (e.src_node == NULL || e.dst_node == NULL) {
                Alarm(EXIT, "Failed to find node for edge (%u, %u)\n", src, dst);
            }
            rete = add_edge_to_list(&Edge_List, e);
            if (rete == NULL) {
                Alarm(EXIT, "Failed to add edge to list\n");
            }
        }
    }
}

int
main(int argc, char ** argv)
{

    char * conf_fname    = NULL;

    if (PRINT_DEBUG) {
        Alarm_set_types(DEBUG);
    }

    /* parse args */
    if (argc != 4) {
        Alarm(EXIT, "usage: overlay_node <id> <config_file> <mode: LS/DV>\n");
    }

    My_ID      = atoi(argv[1]);
    conf_fname = argv[2];

    if (!strncmp("LS", argv[3], 3)) {
        Route_Mode = MODE_LINK_STATE;
    } else if (!strncmp("DV", argv[3], 3)) {
        Route_Mode = MODE_DISTANCE_VECTOR;
    } else {
        Alarm(EXIT, "Invalid mode %s: should be LS or DV\n", argv[5]);
    }

    Alarm(DEBUG, "My ID             : %d\n", My_ID);
    Alarm(DEBUG, "Configuration file: %s\n", conf_fname);
    Alarm(DEBUG, "Mode              : %d\n\n", Route_Mode);

    process_conf(conf_fname, My_ID);
    Alarm(DEBUG, "My IP             : "IPF"\n", IP(My_IP));
    Alarm(DEBUG, "My Port           : %u\n", My_Port);

    { /* print node and edge lists from conf */
        int i;
        struct node *n;
        struct edge *e;
        for (i = 0; i < Node_List.num_nodes; i++) {
            n = Node_List.nodes[i];
            Alarm(DEBUG, "Node %u : "IPF":%u\n", n->id,
                  IP(ntohl(n->addr.sin_addr.s_addr)),
                  ntohs(n->addr.sin_port));
        }

        for (i = 0; i < Edge_List.num_edges; i++) {
            e = Edge_List.edges[i];
            Alarm(DEBUG, "Edge (%u, %u) : "IPF":%u -> "IPF":%u\n",
                  e->src_id, e->dst_id,
                  IP(ntohl(e->src_node->addr.sin_addr.s_addr)),
                  ntohs(e->src_node->addr.sin_port),
                  IP(ntohl(e->dst_node->addr.sin_addr.s_addr)),
                  ntohs(e->dst_node->addr.sin_port));
        }
    }

    /* Initialize event system */
    E_init();

    /* Set up TCP socket for client connection requests */
    init_client_sock(My_Port);

    /* Set up UDP sockets for sending and receiving messages from other
     * overlay nodes */
    init_overlay_data_sock(My_Port);
    init_overlay_ctrl_sock(My_Port+1);

    if (Route_Mode == MODE_LINK_STATE) {
        init_link_state();
    } else {
        init_distance_vector();
    }

    send_heartbeats();

    /* Enter event handling loop */
    Alarm(DEBUG, "Entering event loop!\n");
    E_handle_events();

    return 0;
}
