/*
 * rt_srv_ec.c, by Birju Patel and Alex Kwiatkowski
 * October 22, 2022
 * Dr. Amy Babay, Pitt CS2520
 *
 * OPTIONS:
 *  PRINT_DEBUG -
 *    0 - does not print debug messages
 *    1 - prints info every time a packet is sent or received
 *  SET_CLOCK_MODE -
 *    0 - randomizes whether the clock is slow or fast
 *    1 - clock is artificially slow
 *    2 - clock is artificially fast
 *    3 - clock is fine (no drift)
 *  INCLUDE_DELAY -
 *    0 - the round trip time is not changed
 *    1 - the round trip time is affected by random delay
 */

#include "net_include.h"
#include "sendto_dbg.h"

#define PRINT_DEBUG 0
#define WINDOW_SIZE 1800
#define FREE 0
#define WAITING 1
#define BUSY 2

// Added for EC
#define PRINT_DRIFT_AND_DELAY 0
#define SET_CLOCK_MODE 0
#define INCLUDE_DELAY 1
#define MAX_DRIFT_US 1000
#define MAX_RTT_DELAY_US 1000000
#define MAX_DELAY_TIMEOUT_US 1000000

static void Usage(int argc, char *argv[]);
static void Print_help();
static int Cmp_time(struct timeval t1, struct timeval t2);
static int setup_client_sock(char *port_str);

static int handle_init(int sock, struct upkt pkt, struct sockaddr_storage from_addr, socklen_t from_len);
static int handle_live(int sock, struct upkt pkt, struct sockaddr_storage from_addr, socklen_t from_len);
static int handle_nack(int sock, struct upkt pkt, struct sockaddr_storage from_addr, socklen_t from_len);

// Added for EC
static void get_new_drift();
static void get_new_delay();
static void gettimeofday_drift(struct timeval *curr_time);
static void gettimeofday_drift_and_delay(struct timeval *curr_time);

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

// Added for EC
static const struct timeval Drift_Interval = {1, 0};
static struct timeval curr_drift;
static struct timeval curr_rtt_delay, Delay_Interval;
static int drift_mode; //if 0 subtract drift, if 1 add, if 2 no drift

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

    // Set up for artificial drift and delay
    struct timeval t;
    gettimeofday( &t, NULL );
    srand( t.tv_sec );
    // printf("\n+++++++++\n drift seed is %d\n++++++++\n", (int)t.tv_sec);

    // If clock mode flag is false, randomly choose between fast and slow; otherwise set accordingly
    if(!SET_CLOCK_MODE)
      drift_mode = rand()%2 == 0;
    else if (SET_CLOCK_MODE == 1)
      drift_mode = 0;
    else if (SET_CLOCK_MODE == 2)
      drift_mode = 1;
    else
      drift_mode = 2;
    printf("This clock on this machine is: %s\n", (drift_mode == 0 ? "Artificially Slow" : (drift_mode == 1 ? "Artificially Fast" : "Completely Fine (No drift)")));

    curr_drift = (struct timeval) {0,0};
    Delay_Interval = (struct timeval) {0, rand() % MAX_DELAY_TIMEOUT_US};
    curr_rtt_delay = (struct timeval) {0,0};

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
    struct timeval stat_timeout;
    struct timeval *timeout_ptr = NULL;
    struct timeval now, stat_report_time, new_delay, new_drift, delay_timeout, drift_timeout;
    bool first_data_pkt_rcvd = false;

    while(true){
      mask = read_mask;

      if(first_data_pkt_rcvd && state == BUSY){
        //report statistics only if we are currently streaming data
        gettimeofday(&now, NULL);
        timersub(&stat_report_time, &now, &stat_timeout);
        timeout_ptr = &stat_timeout;

        timersub(&new_drift, &now, &drift_timeout);
        timersub(&new_delay, &now, &delay_timeout);
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
            gettimeofday_drift_and_delay(&now);

            //stream is beginning, reset statistics and start timer
            if(first_data_pkt_rcvd == false){
              seq = 0;
              clean_bytes_sent = 0;
              clean_pkts_sent = 0;
              pkt_retransmissions = 0;
              gettimeofday(&stream_start_time, NULL);
              first_data_pkt_rcvd = true;
              timeradd(&now, &Report_Interval, &stat_report_time);
              timeradd(&now, &Drift_Interval, &new_drift);
              timeradd(&now, &Delay_Interval, &new_delay);
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

      gettimeofday(&now, NULL);
      //event: New RTT Delay
      // This event should not trigger if INCLUDE DELAY is set to 0
      if(first_data_pkt_rcvd && INCLUDE_DELAY && Cmp_time(now, new_delay) >= 0){
        if(PRINT_DRIFT_AND_DELAY)
          printf("Delay Timeout After ~%f Seconds\n", Delay_Interval.tv_usec/1000000.0);
        Delay_Interval = (struct timeval) {0, rand() % MAX_DELAY_TIMEOUT_US}; // Get new Delay Timeout
        timeradd(&now, &Delay_Interval, &new_delay); //reset delay timer
        get_new_delay();
      }

      gettimeofday(&now, NULL);
      //event: New Clock Drift
      //This event should not trigger if drift_mode is 2 (i.e. no drift)
      if(first_data_pkt_rcvd && drift_mode != 2 && Cmp_time(now, new_drift) >= 0){
        timeradd(&now, &Drift_Interval, &new_drift); //reset drift timer
        get_new_drift();
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
  gettimeofday_drift_and_delay(&now);
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
  gettimeofday_drift_and_delay(&now);

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
        printf("rt_srv_ec: loss rate must be between 0 and 100\n");
        Print_help();
    }
}

static void Print_help() {
    printf("Usage: rt_srv_ec <loss_rate_percent> <app_port> <client_port>\n");
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

// Calculates an artificial drift to add/subtract to the clock
static void get_new_drift()
{
  curr_drift = (struct timeval) {0, curr_drift.tv_usec + rand() % MAX_DRIFT_US};
  if(PRINT_DRIFT_AND_DELAY)
    printf("New drift: %f seconds\n", curr_drift.tv_usec/1000000.0);
}

// Calculates an artificial delay to add to RTT
static void get_new_delay()
{
  curr_rtt_delay = (struct timeval) {0, rand() % MAX_RTT_DELAY_US};
  if(PRINT_DRIFT_AND_DELAY)
    printf("New rtt delay: %f seconds\n", curr_rtt_delay.tv_usec/1000000.0);
}

// Gets the current time using an artificial drift
// This drift is either added, subtracted, or discarded depending on the SET_CLOCK_MODE flag and drift_mode
static void gettimeofday_drift(struct timeval *curr_time)
{
  struct timeval t;
  gettimeofday(&t, NULL);
  if(drift_mode == 2)
    return;
  else if(drift_mode == 1)
    timeradd(&t, &curr_drift, curr_time);
  else
    timersub(&t, &curr_drift, curr_time);
  if(PRINT_DEBUG)
    printf("Actual time: %f \nDrift time: %f \n", (t.tv_sec+t.tv_usec/1000000.0), (curr_time->tv_sec+curr_time->tv_usec/1000000.0));
}

// Gets the current time using an artificial drift
// Also sleeps according to the delay in order to affect RTT
// Only use when we are getting the time to send to the receiver
// If INCLUDE_DELAY flag is set to 0, no sleeping occurs
static void gettimeofday_drift_and_delay(struct timeval *curr_time)
{
  gettimeofday_drift(curr_time);
  if(!INCLUDE_DELAY)
    return;
  sleep(curr_rtt_delay.tv_usec/1000000.0);
  if(PRINT_DEBUG)
    printf("Slept for %f seconds\n", curr_rtt_delay.tv_usec/1000000.0);
}
