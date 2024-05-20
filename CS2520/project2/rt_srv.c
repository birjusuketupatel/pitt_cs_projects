/*
 * rt_srv.c, by Birju Patel and Alex Kwiatkowski
 * October 22, 2022
 * Dr. Amy Babay, Pitt CS2520
 *
 * OPTIONS:
 *  PRINT_DEBUG -
 *    0 - does not print debug messages
 *    1 - prints info every time a packet is sent or received
 */

#include "net_include.h"
#include "sendto_dbg.h"

#define PRINT_DEBUG 0

#define WINDOW_SIZE 1800
#define FREE 0
#define WAITING 1
#define BUSY 2

static void Usage(int argc, char *argv[]);
static void Print_help();
static int Cmp_time(struct timeval t1, struct timeval t2);
static int setup_client_sock(char *port_str);

static int handle_init(int sock, struct upkt pkt, struct sockaddr_storage from_addr, socklen_t from_len);
static int handle_live(int sock, struct upkt pkt, struct sockaddr_storage from_addr, socklen_t from_len);
static int handle_nack(int sock, struct upkt pkt, struct sockaddr_storage from_addr, socklen_t from_len);

static const struct timeval Report_Interval = {5, 0};

static char *App_Port_Str;
static char *Client_Port_Str;
static int loss_rate;

static char *Receiver_IP;
static char *Receiver_Port;

static int state;
static int seq;

static int clean_bytes_sent;
static int clean_pkts_sent;
static int pkt_retransmissions;
static struct timeval stream_start_time;

static struct upkt pkt_buf[WINDOW_SIZE];

int main(int argc, char *argv[])
{
    int num;

    //server is initially free to accept a new connection
    state = FREE;
    seq = 0;
    Receiver_IP = NULL;
    Receiver_Port = NULL;

    //parse commandline args
    Usage(argc, argv);
    printf("Awaiting a connection on port %s with %d%% loss.\n", Client_Port_Str, loss_rate);
    sendto_dbg_init(loss_rate);

    //setup client socket and app socket
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    int client_sock = setup_client_sock(Client_Port_Str);
    int app_sock = setup_client_sock(App_Port_Str);

    fd_set mask, read_mask;
    int bytes;
    FD_ZERO(&read_mask);
    FD_SET(client_sock, &read_mask);
    FD_SET(app_sock, &read_mask);

    /*
     * statistics report timeout event:
     * prints summary statistics of the run every 5 seconds.
     * start the timer after a client has connected and we sent data
     * we received from the application.
     */
    struct timeval timeout;
    struct timeval *timeout_ptr = NULL;
    struct timeval now, stat_report_time;
    bool first_data_pkt_rcvd = false;

    while(true){
      mask = read_mask;

      if(first_data_pkt_rcvd && state == BUSY){
        //report statistics only if we are currently streaming data
        gettimeofday(&now, NULL);
        timersub(&stat_report_time, &now, &timeout);
        timeout_ptr = &timeout;
      }
      else{
        //otherwise, we are waiting for a connection attempt or app data
        timeout_ptr = NULL;
      }

      num = select(FD_SETSIZE, &mask, NULL, NULL, timeout_ptr);

      //event: packet recevied at a socket
      if(num > 0){

        //event: packet received from the application
        if(FD_ISSET(app_sock, &mask)){
          int index = seq % WINDOW_SIZE;
          memset(&pkt_buf[index], 0, MAX_MESS_LEN);
          bytes = recvfrom(app_sock, &pkt_buf[index].payload, MAX_MESS_LEN, 0, NULL, NULL);
          //ignore data unless we are connected to a client ready to receive it
          if(state == BUSY){
            gettimeofday(&now, NULL);

            //stream is beginning, reset statistics and start timer
            if(first_data_pkt_rcvd == false){
              seq = 0;
              clean_bytes_sent = 0;
              clean_pkts_sent = 0;
              pkt_retransmissions = 0;
              gettimeofday(&stream_start_time, NULL);
              first_data_pkt_rcvd = true;
              timeradd(&now, &Report_Interval, &stat_report_time);
            }

            //get client's IP and Port
            struct addrinfo *client_info;
            getaddrinfo(Receiver_IP, Receiver_Port, &hints, &client_info);

            //forward data to client
            pkt_buf[index].ts_sec = now.tv_sec;
            pkt_buf[index].ts_usec = now.tv_usec;
            pkt_buf[index].seq = seq;
            pkt_buf[index].payload_size = bytes;

            if(PRINT_DEBUG) { printf("sending data packet #%d\n", seq); }

            bytes = sendto_dbg(client_sock, (char *) &pkt_buf[index], sizeof(struct upkt) - (MAX_MESS_LEN - bytes),
                               0, client_info->ai_addr, client_info->ai_addrlen);
            clean_bytes_sent += bytes;
            clean_pkts_sent += 1;

            //Increment seq
            seq += 1;
          }
        }

        //event: packet received from client
        if(FD_ISSET(client_sock, &mask)){
          struct upkt client_pkt;
          struct sockaddr_storage from_addr;
          socklen_t from_len = sizeof(from_addr);

          bytes = recvfrom(client_sock, &client_pkt, sizeof(client_pkt),
                           0, (struct sockaddr *) &from_addr, &from_len);

          if(client_pkt.init == true){
            handle_init(client_sock, client_pkt, from_addr, from_len);
          }
          else if(client_pkt.live == true){
            handle_live(client_sock, client_pkt, from_addr, from_len);
          }
          else if(client_pkt.nack == true){
            if(PRINT_DEBUG) { printf("retransmission request for packet #%d\n", client_pkt.seq); }
            handle_nack(client_sock, client_pkt, from_addr, from_len);
          }
        }
      }

      gettimeofday(&now, NULL);

      //event: statistics timeout triggered
      if(first_data_pkt_rcvd && Cmp_time(now, stat_report_time) >= 0){
        timeradd(&now, &Report_Interval, &stat_report_time); //reset stats timer

        long int stream_time = now.tv_sec - stream_start_time.tv_sec;
        stream_time *= 1000000;
        stream_time += now.tv_usec - stream_start_time.tv_usec;

        //throughput in Mbps
        double bit_rate = clean_bytes_sent * 8;
        bit_rate = bit_rate / stream_time;

        //throughput in packets per second
        stream_time /= 1000000;
        double pkt_rate = clean_pkts_sent / stream_time;

        printf("\n---STREAM STATISTICS---\n");
        printf("total data sent: %f MB\n", clean_bytes_sent / 1000000.0);
        printf("total data sent: %d packets\n", clean_pkts_sent);
        printf("streaming rate: %f Mbps\n", bit_rate);
        printf("streaming rate: %f packets per second\n", pkt_rate);
        printf("next sequence number: %d\n", seq);
        printf("total retransmissions: %d\n", pkt_retransmissions);
        printf("-----------------------\n\n");
      }
    }

    return 0;
}

/*
 * a new client sends an INIT packet to request a connection
 * if we are already servicing a client, we ignore this
 * otherwise, we initiate a connection with this client
 *
 * high level logic -
 * if state == FREE:
 *    save the sender IP and sender Port
 *    transition state to WAITING
 *    accept connection, send an INIT
 * else if sender IP and sender Port do not match saved IP and saved Port:
 *    reject connection, send a NACK
 * else:
 *    initial accept message lost, resend an INIT
 */
static int handle_init(int sock, struct upkt pkt, struct sockaddr_storage from_addr, socklen_t from_len){
  char hbuf[NI_MAXHOST], sbuf[NI_MAXSERV];
  struct timeval now;
  gettimeofday(&now, NULL);

  struct upkt response;
  memset(&response, 0, sizeof(response));
  response.seq = pkt.seq;
  response.ts_sec = now.tv_sec;
  response.ts_usec = now.tv_usec;

  int ret = getnameinfo((struct sockaddr *) &from_addr, from_len, hbuf,
                        sizeof(hbuf), sbuf, sizeof(sbuf), NI_NUMERICHOST |
                        NI_NUMERICSERV);

  if (ret != 0) {
    printf("getnameinfo error: %s\n", gai_strerror(ret));
    return -1;
  }

  printf("Received initialization request from %s:%s\n", hbuf, sbuf);

  if(state == FREE){
    Receiver_IP = (char *) malloc(strlen(hbuf));
    Receiver_Port = (char *) malloc(strlen(sbuf));

    strcpy(Receiver_IP, hbuf);
    strcpy(Receiver_Port, sbuf);

    state = WAITING;
    response.init = true;
  }
  else if(strcmp(Receiver_IP, hbuf) != 0 || strcmp(Receiver_Port, sbuf) != 0){
    printf("Rejecting initialization requested from %s:%s\n", hbuf, sbuf);
    response.nack = true;
  }
  else{
    response.init = true;
  }

  sendto_dbg(sock, (char *) &response, sizeof(response) - MAX_MESS_LEN, 0, (struct sockaddr *) &from_addr, from_len);
  return 0;
}

/*
 * the client sends LIVE packets to signal it is still there and wants data
 * it also uses it to calculate RTT and clock difference between it and the server
 *
 * high level logic -
 * if sender IP and sender Port do not match saved IP and saved Port:
 *    invalid sender, disregard message
 * if state == WAITING:
 *    transition to BUSY state
 *    signal received to begin sending data
 * send a LIVE packet to the client with the current time
 */
static int handle_live(int sock, struct upkt pkt, struct sockaddr_storage from_addr, socklen_t from_len){
  char hbuf[NI_MAXHOST], sbuf[NI_MAXSERV];
  struct timeval now;
  gettimeofday(&now, NULL);

  //on receipt of a LIVE packet, we send a LIVE packet back with our current timestamp
  struct upkt response;
  memset(&response, 0, sizeof(response));
  response.seq = pkt.seq;
  response.live = true;
  response.ts_sec = now.tv_sec;
  response.ts_usec = now.tv_usec;

  int ret = getnameinfo((struct sockaddr *) &from_addr, from_len, hbuf,
                        sizeof(hbuf), sbuf, sizeof(sbuf), NI_NUMERICHOST |
                        NI_NUMERICSERV);

  if (ret != 0) {
    printf("getnameinfo error: %s\n", gai_strerror(ret));
    return -1;
  }

  if(strcmp(Receiver_IP, hbuf) != 0 || strcmp(Receiver_Port, sbuf) != 0){
    //disregard message, it was sent by an invalid sender
    return -1;
  }

  if(state == WAITING){
    //client is ready to receive data stream
    printf("Ready to begin streaming data to %s:%s\n", hbuf, sbuf);
    state = BUSY;
  }

  sendto_dbg(sock, (char *) &response, sizeof(response) - MAX_MESS_LEN, 0, (struct sockaddr *) &from_addr, from_len);
  return 0;
}

/*
 * the client uses a NACK packet to request retransmission of a packet
 * the sequence number in the NACK packet is the packet we must retransmit
 * we buffer old packets in pkt_buf
 *
 * high level logic -
 * if state == FREE or WAITING:
 *    disregard packet
 * if sender IP and sender Port do not match saved IP and saved Port:
 *    disregard packet
 * look in buffer
 * if packet is in the buffer, resend it
 */
static int handle_nack(int sock, struct upkt pkt, struct sockaddr_storage from_addr, socklen_t from_len){
  char hbuf[NI_MAXHOST], sbuf[NI_MAXSERV];
  struct timeval now;
  gettimeofday(&now, NULL);

  if(state == FREE || state == WAITING){
    return 0;
  }

  int ret = getnameinfo((struct sockaddr *) &from_addr, from_len, hbuf,
                        sizeof(hbuf), sbuf, sizeof(sbuf), NI_NUMERICHOST |
                        NI_NUMERICSERV);

  if (ret != 0) {
    printf("getnameinfo error: %s\n", gai_strerror(ret));
    return -1;
  }

  if(strcmp(Receiver_IP, hbuf) != 0 || strcmp(Receiver_Port, sbuf) != 0){
    //disregard message, it was sent by an invalid sender
    return -1;
  }

  if(pkt.seq > seq){
    //requested packet has not been sent yet, ignore request
    return -1;
  }

  struct upkt to_resend = pkt_buf[pkt.seq % WINDOW_SIZE];

  if(to_resend.seq != pkt.seq){
    //requested packet has been overwritten, ignore request
    return -1;
  }

  //resend packet
  sendto_dbg(sock, (char *) &to_resend, sizeof(to_resend), 0, (struct sockaddr *) &from_addr, from_len);
  pkt_retransmissions += 1;
  return 0;
}

/* Creates a UDP client socket at a given port */
static int setup_client_sock(char *port_str){
  struct addrinfo hints, *servinfo, *servaddr;
  int sock;

  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_DGRAM;
  hints.ai_flags = AI_PASSIVE;

  int ret = getaddrinfo(NULL, port_str, &hints, &servinfo);
  if (ret != 0) {
     printf("rt_srv: getaddrinfo error %s\n", gai_strerror(ret));
     exit(1);
  }

  for (servaddr = servinfo; servaddr != NULL; servaddr = servaddr->ai_next) {
    sock = socket(servaddr->ai_family, servaddr->ai_socktype, servaddr->ai_protocol);
    if (sock < 0) {
        printf("rt_rcv: socket creation error\n");
        continue;
    }

    if(bind(sock, servaddr->ai_addr, servaddr->ai_addrlen) < 0) {
        close(sock);
        printf("rt_rcv: bind error\n");
        continue;
    }

    break; /* got a valid socket */
  }

  if (servaddr == NULL) {
      printf("rt_rcv: invalid port, exiting\n");
      exit(1);
  }

  return sock;
}

/* Read commandline arguments */
static void Usage(int argc, char *argv[]) {
    if (argc != 4) {
        Print_help();
    }

    loss_rate = atoi(argv[1]);
    App_Port_Str = argv[2];
    Client_Port_Str = argv[3];

    if(loss_rate < 0 || loss_rate > 100){
        printf("rt_srv: loss rate must be between 0 and 100\n");
        Print_help();
    }
}

static void Print_help() {
    printf("Usage: rt_srv <loss_rate_percent> <app_port> <client_port>\n");
    exit(0);
}

/* Returns 1 if t1 > t2, -1 if t1 < t2, 0 if equal */
static int Cmp_time(struct timeval t1, struct timeval t2) {
    if      (t1.tv_sec  > t2.tv_sec) return 1;
    else if (t1.tv_sec  < t2.tv_sec) return -1;
    else if (t1.tv_usec > t2.tv_usec) return 1;
    else if (t1.tv_usec < t2.tv_usec) return -1;
    else return 0;
}
