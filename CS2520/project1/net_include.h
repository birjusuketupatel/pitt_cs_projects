#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/time.h>
#include <stdbool.h>
#include <sys/queue.h>
#include <errno.h>
#include <signal.h>

#define MAX_MESS_LEN 1400
#define MAX_FILENAME 255

/*
 * variables to describe the state of the connection
 * INITIALIZING: The sender is initializing the connection
 * SLEEPING: The sender is waiting in the queue
 * ACTIVE: The sender is transmitting its file
 * CLOSING: The sender has sent a FIN
 * DEAD: The sender has completed transmission or else has timed out
 */

#define INITIALIZING 1
#define SLEEPING 2
#define ACTIVE 3
#define CLOSING 4

/*
 * On receipt of INIT packet from client, server create a new SLEEPING connection,
 * place connection in the queue, sends a SLEEP packet to the client
 *
 * When server selects client from the queue,
 * sends an INIT packet to signal the beginning of transmission,
 * moves connection from SLEEPING to ACTIVE state
 *
 * When server receives FIN from client, server moves connection to CLOSING
 * Server waits for 3 seconds before assuming the transmission is over
 */
struct upkt {
    bool is_rcv_msg;
    bool init;
    bool fin;
    bool sleep;

    int64_t  ts_sec;
    int32_t  ts_usec;
    int seq;
    uint32_t payload_len;
    char     payload[MAX_MESS_LEN];
};

struct conn {
  uint32_t conn_state;
  uint32_t curr_seq;
  char ip[NI_MAXHOST];
  char port[NI_MAXSERV];
  char filename[MAX_FILENAME];
  TAILQ_ENTRY(conn) tailq;
};
