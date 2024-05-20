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

#define MAX_MESS_LEN 1400

struct upkt {
    bool init;
    bool live;
    bool nack;

    int64_t  ts_sec;
    int32_t  ts_usec;
    uint32_t seq;
    uint32_t payload_size;
    char     payload[MAX_MESS_LEN];
};

struct pkt_list {
  struct upkt pkt;
  int64_t recvd_ts_sec;
  int32_t recvd_ts_usec;
  TAILQ_ENTRY(pkt_list) tailq;
};

struct lost_seq {
  int seq;
  TAILQ_ENTRY(lost_seq) tailq;
};
