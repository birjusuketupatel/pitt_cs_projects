#include "stdbool.h"
#include "gap_list.h"
#include "rcv_buffer.h"
#include "sendto_dbg.h"

#define NUM_BYTES_TO_PRINT_STATS 10000000
#define FIN_TIMEOUT 3
static const bool DEBUG_PRINTS = false;

static void Usage(int argc, char *argv[]);
static void Print_help();
static void handle_init_pkt(struct upkt init_pkt, struct sockaddr from_addr, socklen_t from_len);
static void handle_fin_pkt(struct upkt init_pkt, struct sockaddr from_addr, socklen_t from_len);
static void close_conn();
static struct upkt create_pkt();
void prepare_message(rcv_msg *msg, int start_of_window);

static int loss_rate;
static char *Port_Str;
static char *env;
static int sock;

TAILQ_HEAD(tailhead, conn);
static struct tailhead conn_queue;
static struct conn *curr_conn;

// Adjustable Parameters
static int WINDOW_SIZE;
static int MESSAGE_TIMEOUT_S;
static int MESSAGE_TIMEOUT_US;

// File transfer variables
static int start_of_window, written_since_last_message;
static int greatest_recvd_seq;
static gap_list *list;
static rcv_buffer *buf;
static struct timeval last_sent_time;
static FILE *fp;

// Statistic Variables
static int bytes_since_last_output, bytes_successfully_received;
struct timeval start_of_transfer, end_of_transfer, time_of_last_output;

int main(int argc, char *argv[])
{
  // New Vars
  gap new_gap;
  bool add_gap;
  rcv_msg msg;
  struct timeval now, diff_time;

  /* Parse commandline args */
  Usage(argc, argv);
  printf("Awaiting file transfer on port %s with %d%% loss\n", Port_Str, loss_rate);

  sendto_dbg_init(loss_rate);

  struct addrinfo hints, *servinfo, *servaddr;

  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET; //uses IPv4
  hints.ai_socktype = SOCK_DGRAM;
  hints.ai_flags = AI_PASSIVE;

  //get list of available IP addresses
  int ret = getaddrinfo(NULL, Port_Str, &hints, &servinfo);
  if(ret != 0){
      printf("rcv: getaddrinfo error\n");
      exit(1);
  }

  //select an address and bind a socket to it
  for (servaddr = servinfo; servaddr != NULL; servaddr = servaddr->ai_next){
      sock = socket(servaddr->ai_family, servaddr->ai_socktype, servaddr->ai_protocol);

      if(sock < 0){
          printf("rcv: socket creation error\n");
          continue;
      }

      if(bind(sock, servaddr->ai_addr, servaddr->ai_addrlen) < 0) {
          close(sock);
          printf("rcv: bind error\n");
          continue;
      }

      //socket created
      break;
  }

  //no socket created
  if (servaddr == NULL) {
      printf("rcv: No valid address found...exiting\n");
      exit(1);
  }

  // File Transfer Vars
  start_of_window = -1;
  written_since_last_message = 0;
  greatest_recvd_seq = -1;
  fp = NULL;
  gettimeofday(&last_sent_time, NULL);
  list = new_list();
  buf = new_buffer();

  //store pending connections in a queue
  //initially empty
  TAILQ_INIT(&conn_queue);

  //points to the struct tracking the current connection
  //when serving nobody, set as NULL
  curr_conn = NULL;

  struct timeval timeout;
  struct upkt pkt;
  struct sockaddr from_addr;
  socklen_t from_len;
  int num;

  fd_set mask, read_mask;
  FD_ZERO(&read_mask);
  FD_SET(sock, &read_mask);

  /* main control loop */
  while(true){
      mask = read_mask;
      timeout.tv_sec = MESSAGE_TIMEOUT_S;
      timeout.tv_usec = MESSAGE_TIMEOUT_US;

      //wait for a packet or a timeout
      num = select(FD_SETSIZE, &mask, NULL, NULL, &timeout);

      if(num > 0){
        if(FD_ISSET(sock, &mask)){
          //received a packet
          from_len = sizeof(from_addr);
          int bytes_recvd = recvfrom(sock, (char *) &pkt, sizeof(pkt), 0,
              &from_addr, &from_len);

          if(pkt.init == true){
            if(DEBUG_PRINTS){
              printf("INIT rcvd\n");
            }

            handle_init_pkt(pkt, from_addr, from_len);
          }
          else if(pkt.fin == true)
          {
            if(DEBUG_PRINTS){
              printf("FIN rcvd\n");
            }

            handle_fin_pkt(pkt, from_addr, from_len);
          }
          else
          {
            if(DEBUG_PRINTS)
              printf("\n---packet with %d bytes received, seq = %d---\n\n", bytes_recvd, pkt.seq);
            if(curr_conn != NULL && curr_conn->conn_state == SLEEPING)
            {
              gettimeofday(&start_of_transfer, NULL);
              bytes_since_last_output = 0;
              bytes_successfully_received = 0;
              curr_conn->conn_state = ACTIVE;
            }
            if(curr_conn != NULL && fp == NULL)
            {
              fp = fopen(curr_conn->filename, "w+");
              printf("Created file: %s\n", curr_conn->filename);
            }
            if(DEBUG_PRINTS)
              printf("Received Packet: %d with payload size:%d\nWindow at: %d\n", pkt.seq, pkt.payload_len, start_of_window);
            if(pkt.seq <= start_of_window)
            {
              if(DEBUG_PRINTS)
                printf("Discard old packet\n");
            }
            else if (start_of_window + 1 == pkt.seq) // Just received packet directly follows start of window -> can be written to file
            {
              start_of_window = pkt.seq;
              fputs(pkt.payload, fp);

              bytes_since_last_output += pkt.payload_len;
              written_since_last_message += 1;

              if(DEBUG_PRINTS)
                printf("Packet %d wrote to file!\n", start_of_window);

              // Also need to see if this packet has filled any gaps
              if(split_gap(list, pkt.seq))
              {
                  if(DEBUG_PRINTS)
                    printf("Gap filled. There are now %d gaps\n", list->size);
              }
              else
              {
                if(DEBUG_PRINTS)
                  printf("No gap needed filled\n");
              }

              // Need to write any packets in the buffer that directly follow just received packet
              while(buf->head != NULL && buf->head->seq == start_of_window + 1)
              {
                start_of_window = buf->head->seq;
                fputs(buf->head->payload, fp);
                bytes_since_last_output += buf->head->payload_len;
                written_since_last_message += 1;

                free_head(buf);

                if(DEBUG_PRINTS)
                  printf("Packet %d wrote to file from buffer! Buffer now contains %d packets\n", start_of_window, buf->size);
              }

              if(bytes_since_last_output >= NUM_BYTES_TO_PRINT_STATS)
              {
                gettimeofday(&now, NULL);
                timersub(&now, &time_of_last_output, &diff_time);
                bytes_successfully_received += bytes_since_last_output;
                printf("\n--------------- %f Megabytes received in order since last output ---------------\n", bytes_since_last_output/1000000.0);
                printf("Megabytes in total received successfully: %f MB\n", bytes_successfully_received/1000000.0);
                printf("Average transfer rate: %f MB/sec\n", (bytes_since_last_output/1000000.0)/(diff_time.tv_sec + (diff_time.tv_usec / 1000000.0)));

                // Reset certain metrics
                gettimeofday(&time_of_last_output, NULL);
                bytes_since_last_output = 0;
              }
            }
            else
            {
              if(DEBUG_PRINTS)
                printf("Doesn't directly follow, save packet %d to buffer\n", pkt.seq);
              buffer_insert(buf, &pkt);
              if(DEBUG_PRINTS)
                printf("Buffer now has %d packets stored\n", buf->size);
              // Check to see if this packet fills an existing gap, and if so update gap list accordingly
              add_gap = !split_gap(list, pkt.seq);

              // If this did not split an existing gap and it has a sequence number greater than 1 plus highest received sequence number
              // We need to add a new gap to the gap list
              if (add_gap && pkt.seq > greatest_recvd_seq + 1)
              {
                new_gap.start = greatest_recvd_seq + 1;
                new_gap.end = pkt.seq;

                insert(list, &new_gap);
                if(DEBUG_PRINTS)
                  printf("New gap detected from %d to %d\n", new_gap.start, new_gap.end);

                prepare_message(&msg, start_of_window);
                gettimeofday(&last_sent_time, NULL);
                sendto_dbg(sock, (char *) &msg, (sizeof(rcv_msg)-(MAX_GAPS-msg.num_gaps)*sizeof(gap)), 0, (struct sockaddr *)&from_addr,
                        from_len);
                written_since_last_message = 0;
              }
            }

            if((int) pkt.seq > greatest_recvd_seq)
              greatest_recvd_seq = pkt.seq;

            gettimeofday(&now, NULL);
            timersub(&now, &last_sent_time, &diff_time);
            if (written_since_last_message >= WINDOW_SIZE/2 || diff_time.tv_sec + (diff_time.tv_usec / 1000000.0) >= MESSAGE_TIMEOUT_S + (MESSAGE_TIMEOUT_US / 1000000.0) )
            {
              if(DEBUG_PRINTS)
                printf("TIMEOUT A: Resending message\n");
              prepare_message(&msg, start_of_window);
              gettimeofday(&last_sent_time, NULL);
              sendto_dbg(sock, (char *) &msg, (sizeof(rcv_msg)-(MAX_GAPS-msg.num_gaps)*sizeof(gap)), 0, (struct sockaddr *)&from_addr,
                      from_len);
              written_since_last_message = 0;
            }
          }
        }
      }
      else
      {
          //if current connection is in SLEEPING state, attempt to wake it up
          if(curr_conn != NULL && curr_conn->conn_state == SLEEPING)
          {
            struct addrinfo *client_info;
            num = getaddrinfo(curr_conn->ip, curr_conn->port, &hints, &client_info);

            if(num != 0)
            {
                printf("getaddrinfo error\n");
                exit(1);
            }

            struct upkt wakeup_pkt = create_pkt();
            wakeup_pkt.init = true;

            sendto_dbg(sock, (char *) &wakeup_pkt,
                    sizeof(wakeup_pkt) - MAX_MESS_LEN, 0,
                    client_info->ai_addr,
                    client_info->ai_addrlen);
          }
          // If we current connection is in the active state, and we have not sent a message in a while, resend gaps and aru
          else if(curr_conn != NULL && curr_conn->conn_state == ACTIVE)
          {
            struct addrinfo *client_info;
            num = getaddrinfo(curr_conn->ip, curr_conn->port, &hints, &client_info);
            if(DEBUG_PRINTS)
              printf("TIMEOUT B: Resending message\n");
            prepare_message(&msg, start_of_window);
            gettimeofday(&last_sent_time, NULL);
            sendto_dbg(sock, (char *) &msg, (sizeof(rcv_msg)-(MAX_GAPS-msg.num_gaps)*sizeof(gap)), 0,
                    client_info->ai_addr,
                    client_info->ai_addrlen);
            written_since_last_message = 0;
          }
      }
  }

  return 0;
}

/*
 * if no client is being serviced, create a new connection and immediately begin transmission
 * if server is busy, place new connection in the queue
 */
static void handle_init_pkt(struct upkt init_pkt, struct sockaddr from_addr, socklen_t from_len){
  //get ip and port of the client
  char ip_buf[NI_MAXHOST], port_buf[NI_MAXSERV];

  int ret = getnameinfo(&from_addr, from_len, ip_buf,
                    sizeof(ip_buf), port_buf, sizeof(port_buf),
                    NI_NUMERICHOST | NI_NUMERICSERV);

  if(ret != 0){
    printf("rcv: getnameinfo error\n");
    return;
  }

  struct conn *next_conn;
  struct upkt init_response, sleep_pkt;

  if(curr_conn == NULL){
    /*
     * the server is not currently busy
     * create a new connection and put it immediately in the ACTIVE state
     */
    next_conn = (struct conn *) malloc(sizeof(struct conn));

    next_conn->conn_state = ACTIVE;
    next_conn->curr_seq = 0;
    strcpy(next_conn->ip, ip_buf);
    strcpy(next_conn->port, port_buf);
    strcpy(next_conn->filename, init_pkt.payload);

    printf("beginning transfer of %s from %s:%s\n", next_conn->filename, next_conn->ip, next_conn->port);

    curr_conn = next_conn;

    // Resetting File Transfer related globals (seq, gap_list, and buffer)
    start_of_window = -1;
    written_since_last_message = 0;
    fp = NULL;
    gettimeofday(&last_sent_time, NULL);
    greatest_recvd_seq = -1;

    //Reset statistic related globals
    gettimeofday(&start_of_transfer, NULL);
    bytes_since_last_output = 0;
    bytes_successfully_received = 0;

    if(list != NULL)
    {
        free_list(list);
    }
    list = new_list();
    if(buf != NULL)
    {
        free_buffer(buf);
    }
    buf = new_buffer();

    //create a file
    //FILE *f = fopen(next_conn->filename, "w");
    //fclose(f);

    //send INIT packet, tells client to start transmitting
    init_response = create_pkt();
    init_response.init = true;

     sendto_dbg(sock, (char *) &init_response,
                sizeof(init_pkt) - MAX_MESS_LEN, 0,
                &from_addr,
                from_len);
  }
  else{
    /*
     * the server is busy
     * check if the connection is already in the queue
     * if not, put a new connection in the SLEEPING state in the queue
     */
    if(strcmp(curr_conn->ip, ip_buf) == 0 && strcmp(curr_conn->port, port_buf) == 0){
      //duplicate INIT sent on an ACTIVE connection
      //resend INIT
      init_response = create_pkt();
      init_response.init = true;
      sendto_dbg(sock, (char *) &init_response,
                 sizeof(init_pkt) - MAX_MESS_LEN, 0,
                 &from_addr,
                 from_len);
      return;
    }

    struct conn *c;
    TAILQ_FOREACH(c, &conn_queue, tailq){
      if(strcmp(c->ip, ip_buf) == 0 && strcmp(c->port, port_buf) == 0){
        //duplicate INIT for SLEEPING connection
        //resend SLEEP
        sleep_pkt = create_pkt();
        sleep_pkt.sleep = true;
        sendto_dbg(sock, (char *) &sleep_pkt,
                   sizeof(sleep_pkt) - MAX_MESS_LEN, 0,
                   &from_addr,
                   from_len);
        return;
      }
    }

    //new connection request
    //create new conn struct, place in queue
    next_conn = (struct conn *) malloc(sizeof(struct conn));
    next_conn->conn_state = SLEEPING;
    next_conn->curr_seq = 0;
    strcpy(next_conn->ip, ip_buf);
    strcpy(next_conn->port, port_buf);
    strcpy(next_conn->filename, init_pkt.payload);

    TAILQ_INSERT_TAIL(&conn_queue, next_conn, tailq);

    //send SLEEP packet
    sleep_pkt = create_pkt();
    sleep_pkt.sleep = true;
    sendto_dbg(sock, (char *) &sleep_pkt,
               sizeof(sleep_pkt) - MAX_MESS_LEN, 0,
               &from_addr,
               from_len);
  }
}

/*
 * send a FIN to the current client
 * wait for 5 seconds, if the client sends nothing else, the transmission is over
 * remove current connection and service the next one in the queue
 */
static void handle_fin_pkt(struct upkt init_pkt, struct sockaddr from_addr, socklen_t from_len){
  //get time the transmission ended
  gettimeofday(&end_of_transfer, NULL);

  //Start by closing the file
  if (fp != NULL)
  {
    fclose(fp);
    fp = NULL;
  }
  //get ip and port of the sender
  char ip_buf[NI_MAXHOST], port_buf[NI_MAXSERV];

  int ret = getnameinfo(&from_addr, from_len, ip_buf,
                    sizeof(ip_buf), port_buf, sizeof(port_buf),
                    NI_NUMERICHOST | NI_NUMERICSERV);

  if(ret != 0){
    printf("rcv: getnameinfo error\n");
    return;
  }

  if(curr_conn == NULL){
    return;
  }

  if(strcmp(curr_conn->ip, ip_buf) == 0 && strcmp(curr_conn->port, port_buf) == 0){
    //the client wants to close the current connection
    //move to CLOSING state
    curr_conn->conn_state = CLOSING;

    //send a FIN to client to confirm termination
    struct upkt fin_pkt = create_pkt();
    fin_pkt.fin = true;
    sendto_dbg(sock, (char *) &fin_pkt,
               sizeof(fin_pkt) - MAX_MESS_LEN, 0,
               &from_addr,
               from_len);

    /*
     * set alarm for 5 seconds
     * if no additional FIN packets come from this connection, close the connection
     */
     signal(SIGALRM, close_conn);
     alarm(FIN_TIMEOUT);
  }
}

/*
 * deletes current connection
 * poll next connection from the queue
 */
static void close_conn(){
  // print summary statistics for transmission
  struct timeval diff_time;
  bytes_successfully_received += bytes_since_last_output;
  bytes_since_last_output = 0;
  timersub(&end_of_transfer, &start_of_transfer, &diff_time);
  printf("\n--------------- FINAL STATISTICS ---------------\n");
  printf("Size of file received: %f MB\n", bytes_successfully_received/1000000.0);
  printf("Time required for transfer: %f sec\n", (diff_time.tv_sec + (diff_time.tv_usec / 1000000.0)));
  printf("Average transfer rate: %f MB/sec\n", (bytes_successfully_received/1000000.0)/(diff_time.tv_sec + (diff_time.tv_usec / 1000000.0)));

  printf("finished handling %s:%s\n", curr_conn->ip, curr_conn->port);

  free(curr_conn);
  free_list(list);
  free_buffer(buf);

  if(!TAILQ_EMPTY(&conn_queue)){
    struct conn *next_conn = TAILQ_FIRST(&conn_queue);
    TAILQ_REMOVE(&conn_queue, next_conn, tailq);

    curr_conn = next_conn;

    // Resetting File Transfer related globals (seq, gap_list, and buffer)
    start_of_window = -1;
    written_since_last_message = 0;
    fp = NULL;
    gettimeofday(&last_sent_time, NULL);
    greatest_recvd_seq = -1;

    list = new_list();
    buf = new_buffer();

    //Reset statistic related globals
    gettimeofday(&start_of_transfer, NULL);
    bytes_since_last_output = 0;
    bytes_successfully_received = 0;

    printf("now servicing %s:%s\n", curr_conn->ip, curr_conn->port);
  }
  else{
    curr_conn = NULL;
    list = NULL;
    buf = NULL;
  }
}

/*
 * creates and returns an empty packet
 */
static struct upkt create_pkt(){
  struct upkt pkt;
  memset(&pkt, 0, sizeof(pkt));

  struct timeval now;
  gettimeofday(&now, NULL);
  pkt.is_rcv_msg = false;
  pkt.ts_sec  = now.tv_sec;
  pkt.ts_usec = now.tv_usec;

  return pkt;
}

/* Read commandline arguments */
static void Usage(int argc, char *argv[]) {
  if (argc != 4) {
    Print_help();
  }

  loss_rate = atoi(argv[1]);
  Port_Str = argv[2];
  env = argv[3];

  if(loss_rate < 0 || loss_rate > 100){
    printf("rcv: loss rate must be between 0 and 100\n");
    Print_help();
  }

  if(strcmp("WAN", env) != 0 && strcmp("LAN", env) != 0 && strcmp("WAN-EC", env) != 0){
    printf("rcv: valid environment choices are \"WAN\", \"LAN\", or \"WAN-EC\"\n");
    Print_help();
  }
  else if (strcmp("WAN", env) == 0)
  {
    WINDOW_SIZE = 45;
    MESSAGE_TIMEOUT_S = 0;
    MESSAGE_TIMEOUT_US = 85000;
  }
  else if (strcmp("LAN", env) == 0)
  {
    WINDOW_SIZE = 45;
    MESSAGE_TIMEOUT_S = 0;
    MESSAGE_TIMEOUT_US = 125000;
  }
  else if (strcmp("WAN-EC", env) == 0)
  {
    WINDOW_SIZE = 55;
    MESSAGE_TIMEOUT_S = 0;
    MESSAGE_TIMEOUT_US = 50000;
  }
}

static void Print_help() {
  printf("Usage: rcv <loss_rate_percent> <port> <env>\n");
  exit(0);
}

void prepare_message(rcv_msg *msg, int start_of_window)
{
    int i;
    node *gap_node;

    msg->is_rcv_msg = true;
    msg->aru = start_of_window + 1;
    msg->num_gaps = list->size;

    i = 0;
    memset(msg->gaps, 0, sizeof(msg->gaps));
    for(gap_node = list->head; gap_node != NULL && i < MAX_GAPS; gap_node = gap_node->next)
    {
      if(DEBUG_PRINTS)
        printf("Current Gap: start: %d end: %d\n", gap_node->gap_data->start, gap_node->gap_data->end);
      // Eventually loop over gaps to only add ones that need sent
      msg->gaps[i].start = gap_node->gap_data->start;
      msg->gaps[i].end = gap_node->gap_data->end;
      i += 1;
    }

    if(DEBUG_PRINTS)
      printf("Sending message aru %d number of gaps %d.\n\n", msg->aru, msg->num_gaps);
}
