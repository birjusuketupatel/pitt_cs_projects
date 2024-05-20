#include "stdbool.h"
#include "net_include.h"
#include "gap_list.h"
#include "sendto_dbg.h"

#define NUM_BYTES_TO_PRINT_STATS 10000000
static const bool DEBUG_PRINTS = false;

static void Usage(int argc, char *argv[]);
static void Print_help();
static void initialization_sequence(int sock, struct addrinfo *servaddr);
static void sleep_sequence(int sock);
static void transmission_sequence(int sock, struct addrinfo *servaddr);
static void closing_sequence(int sock, struct addrinfo *servaddr);

static int loss_rate;
static char *env;
static char *Server_IP;
static char *Port_Str;
static char *src_file;
static char *dst_file;
static int state;

// Adjustable Parameters
static int WINDOW_SIZE;
static int WINDOW_TIMEOUT_S;
static int WINDOW_TIMEOUT_US;
static int NUM_TO_RESEND; // MUST BE AT LEAST 1

// Statistic Vars
int bytes_since_last_output, bytes_successfully_sent, total_data_sent;
struct timeval start_of_transfer, end_of_transfer;

int main(int argc, char *argv[])
{
    //parse commandline args
    Usage(argc, argv);
    printf("Sending %s to %s@%s:%s\n", src_file, dst_file, Server_IP, Port_Str);

    sendto_dbg_init(loss_rate);

    //setup socket
    struct addrinfo hints, *servinfo, *servaddr;
    int sock;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_protocol = IPPROTO_UDP;

    int ret = getaddrinfo(Server_IP, Port_Str, &hints, &servinfo);
    if (ret != 0) {
       printf("getaddrinfo error\n");
       exit(1);
    }

    for (servaddr = servinfo; servaddr != NULL; servaddr = servaddr->ai_next) {
      sock = socket(servaddr->ai_family, servaddr->ai_socktype, servaddr->ai_protocol);
      if (sock < 0) {
          printf("socket creation error\n");
          continue;
      }

      break; /* got a valid socket */
    }
    if (servaddr == NULL) {
        fprintf(stderr, "No valid address found...exiting\n");
        exit(1);
    }

    state = INITIALIZING;
    initialization_sequence(sock, servaddr);

    if(state == SLEEPING){
      sleep_sequence(sock);
    }

    transmission_sequence(sock, servaddr);

    closing_sequence(sock, servaddr);

    printf("transmission complete!\n");

    struct timeval diff_time;

    bytes_successfully_sent += bytes_since_last_output;
    timersub(&end_of_transfer, &start_of_transfer, &diff_time);
    printf("\n--------------- FINAL STATISTICS ---------------\n");
    printf("Size of file transferred: %f MB\n", bytes_successfully_sent/1000000.0);
    printf("Time required for transfer: %f sec\n", (diff_time.tv_sec + (diff_time.tv_usec / 1000000.0)));
    printf("Average transfer rate: %f MB/sec\n", (bytes_successfully_sent/1000000.0)/(diff_time.tv_sec + (diff_time.tv_usec / 1000000.0)));
    printf("Total data sent: %f MB\n", total_data_sent/1000000.0);


    return 0;
}

/*
 * begin INITIALIZATION sequence
 * send INIT packet to receiver in a loop
 * when SLEEP packet is received, transition to SLEEPING state
 */
void initialization_sequence(int sock, struct addrinfo *servaddr){
  //sends an INIT packet once every 250 ms until a SLEEP packet is received
  fd_set mask, read_mask;
  struct timeval timeout;
  int num;
  struct timeval now;

  FD_ZERO(&read_mask);
  FD_SET(sock, &read_mask);

  /*
   * create an INIT packet
   * payload = destination file name
   * init flag = true
   * sequence number = 0
   */
  struct upkt init_pkt;
  memset(&init_pkt, 0, sizeof(init_pkt));
  init_pkt.init = true;
  init_pkt.seq = 0;

  strcpy(init_pkt.payload, dst_file);
  init_pkt.payload_len = strlen(dst_file);
  init_pkt.payload[init_pkt.payload_len] = '\0';

  while(true){
    mask = read_mask;
    timeout.tv_sec = 0;
    timeout.tv_usec = 250000;

    //wait for a response or a timeout
    num = select(FD_SETSIZE, &mask, NULL, NULL, &timeout);

    if(num > 0){
      if(FD_ISSET(sock, &mask)){
        /*
         * received a packet from the server
         * transition to either SLEEPING or ACTIVE state
         */
        struct upkt rcv_pkt;
        recv(sock, (char *) &rcv_pkt, sizeof(rcv_pkt), 0);

        if(rcv_pkt.sleep == true){
          if(DEBUG_PRINTS){
            printf("SLEEP rcvd\n");
          }
          /*
           * received a SLEEP packet, so the server is busy with another client
           * transition to SLEEPING state
           */
          printf("server is busy, entering sleep state\n");
          state = SLEEPING;
          return;
        }
        else if(rcv_pkt.init == true){
          if(DEBUG_PRINTS){
            printf("INIT rcvd\n");
          }
          /*
           * received an INIT packet, begin transmitting immediately
           * transition to ACTIVE state
           */
          state = ACTIVE;
          return;
        }
        else{
          if(DEBUG_PRINTS){
            printf("something else rcvd\n");
          }
          continue;
        }
      }
    }
    else{
      //resend INIT packet
      gettimeofday(&now, NULL);
      init_pkt.ts_sec  = now.tv_sec;
      init_pkt.ts_usec = now.tv_usec;

      sendto_dbg(sock, (char *) &init_pkt,
                 sizeof(init_pkt) - MAX_MESS_LEN + init_pkt.payload_len, 0,
                 servaddr->ai_addr,
                 servaddr->ai_addrlen);
    }
  }
}

/*
 * server is busy, client is in the queue
 * block until INIT packet is received
 */
static void sleep_sequence(int sock){
  struct upkt pkt;
  struct timeval timeout;
  fd_set mask, read_mask;
  int num;

  FD_ZERO(&read_mask);
  FD_SET(sock, &read_mask);

  while(true){
    mask = read_mask;

    //after 10 minutes, assume the server is down
    timeout.tv_sec = 600;
    timeout.tv_usec = 0;

    num = select(FD_SETSIZE, &mask, NULL, NULL, &timeout);

    if(num > 0){
      if(FD_ISSET(sock, &mask)){
        recv(sock, (char *) &pkt, sizeof(pkt), 0);

        if(pkt.init == true){
          //received INIT packet, transistion to ACTIVE state
          state = ACTIVE;
          return;
        }
      }
    }
    else{
      //timeout, connection is dead
      printf("no response after 10 minutes, exiting...\n");
      exit(0);
    }
  }
}

/*
 * data transfer protocol done by Alex
 */
static void transmission_sequence(int sock, struct addrinfo *servaddr){
    struct sockaddr_storage from_addr;
    socklen_t               from_len;

    int                     num;
    fd_set                  mask, read_mask;
    struct timeval          now, time_of_last_output, diff_time, time_of_last_win_move;

    // General Transfer Vars
    struct timeval          timeout;
    int curr_in_flight, start_of_window, index, gap_num, seq_to_resend;
    struct rcv_msg          recvd_msg;
    struct upkt             window[WINDOW_SIZE];
    bool check_gaps;
    FILE *fp;

    fp = fopen(src_file, "r");
    if(fp == NULL)
    {
      printf("Unable to open file: %s\n", src_file);
      exit(1);
    }

    FD_ZERO(&read_mask);
    FD_SET(sock, &read_mask);

    // General Transfer Vars
    gettimeofday(&time_of_last_win_move, NULL);
    start_of_window = 0;
    curr_in_flight = 0;
    timeout.tv_sec = 0;
    timeout.tv_usec = 10;
    check_gaps = false;

    //For statistic calculations
    gettimeofday(&start_of_transfer, NULL);
    gettimeofday(&time_of_last_output, NULL);
    bytes_since_last_output = 0;
    bytes_successfully_sent = 0;
    total_data_sent = 0;

    for(;;)
    {
        mask = read_mask;
        if(check_gaps)
        {
            if(DEBUG_PRINTS)
              printf("%d gaps to fill \n", recvd_msg.num_gaps);
            for (gap_num = 0; gap_num < recvd_msg.num_gaps; gap_num += 1)
            {
                if(DEBUG_PRINTS)
                  printf("Resending packets in gap %d to %d\n", recvd_msg.gaps[gap_num].start, recvd_msg.gaps[gap_num].end);
                for(seq_to_resend = recvd_msg.gaps[gap_num].start; seq_to_resend < recvd_msg.gaps[gap_num].end; seq_to_resend += 1)
                {
                    index = seq_to_resend % WINDOW_SIZE;
                    if(DEBUG_PRINTS)
                      printf("Sending packet %d\n", window[index].seq); //This needs to be adjusted when an actual array is being used for window

                    /* Fill in header info */
                    gettimeofday(&now, NULL);
                    window[index].ts_sec  = now.tv_sec;

                    total_data_sent += (window[index].payload_len + 1); // Plus 1 to include null terminator in the payload

                    sendto_dbg(sock, (char *) &window[index],
                                sizeof(window[index]) - MAX_MESS_LEN + window[index].payload_len - 1, 0,
                                servaddr->ai_addr,
                                servaddr->ai_addrlen);
                }
            }
            check_gaps = false;
            if(DEBUG_PRINTS)
              printf("Done resending packets\n");
        }

        if (curr_in_flight < WINDOW_SIZE && !feof(fp))
        {
            index = (start_of_window + curr_in_flight) % WINDOW_SIZE;
            memset(&window[index], 0, sizeof(window[index]));
            fread(&window[index].payload, 1, MAX_MESS_LEN - 1, fp);
            window[index].payload[MAX_MESS_LEN - 1] = '\0';

            /* Fill in header info */
            gettimeofday(&now, NULL);
            window[index].ts_sec  = now.tv_sec;
            window[index].seq = start_of_window + curr_in_flight;
            window[index].payload_len = strlen(window[index].payload);

            if(DEBUG_PRINTS)
              printf("Sending packet %d Payload len:%d\n", window[index].seq, window[index].payload_len);

            total_data_sent += (window[index].payload_len + 1); // Plus 1 to include null terminator in the payload

            sendto_dbg(sock, (char *) &window[index],
                        sizeof(window[index])-(MAX_MESS_LEN - window[index].payload_len + 1), 0,
                        servaddr->ai_addr,
                        servaddr->ai_addrlen);
            curr_in_flight += 1;
        }

        // See if any messages have been received
        num = select(FD_SETSIZE, &mask, NULL, NULL, &timeout);
        if (num > 0)
        {
            from_len = sizeof(from_addr);
            recvfrom(sock, &recvd_msg, sizeof(recvd_msg), 0,
                     (struct sockaddr *)&from_addr,
                     &from_len);

            // Print_IP((struct sockaddr *)&from_addr);
            if(DEBUG_PRINTS)
            {
              printf("Window is at %d\n", start_of_window);
              printf("Up to %d acknowledged with %d gaps\n", recvd_msg.aru, recvd_msg.num_gaps);
            }
            if(recvd_msg.aru > 0)
            {
                curr_in_flight -= (recvd_msg.aru - start_of_window);
                if(DEBUG_PRINTS)
                  printf("Window moved forward by %d, now at %d\n%d packets still in flight\n", (recvd_msg.aru - start_of_window), recvd_msg.aru, curr_in_flight);

                // Don't forget to accumulate statistics for outputting
                for(index = start_of_window; index < recvd_msg.aru; index += 1)
                  bytes_since_last_output += window[index % WINDOW_SIZE].payload_len;

                // If our window has moved, update timeout check
                if (start_of_window != recvd_msg.aru)
                  gettimeofday(&time_of_last_win_move, NULL);

                start_of_window = recvd_msg.aru;
            }
            if(recvd_msg.num_gaps > 0)
                check_gaps = true;
        }

        // If we have no more packets to send break out and start finalization procedure
        if(feof(fp) && curr_in_flight <= 0)
            break;

        // If window has not moved in a while, resend the first packet as well as a couple additional packets
        gettimeofday(&now, NULL);
        timersub(&now, &time_of_last_win_move, &diff_time);
        if (diff_time.tv_sec + (diff_time.tv_usec / 1000000.0) >= WINDOW_TIMEOUT_S + (WINDOW_TIMEOUT_US / 1000000.0) )
        {
          if(DEBUG_PRINTS)
            printf("WINDOW TIMEOUT: Window has not moved recently\n");

          // We should only resend as many packets as are in flight as well as limit to the NUM_TO_RESEND
          for(seq_to_resend = 0; seq_to_resend < NUM_TO_RESEND && seq_to_resend < curr_in_flight; seq_to_resend += 1)
          {
            index = (start_of_window + seq_to_resend) % WINDOW_SIZE;

            if(DEBUG_PRINTS)
              printf("Resending packet: %d\n", index);

            total_data_sent += (window[index].payload_len + 1); // Plus 1 to include null terminator in the payload

            // Update header info
            window[index].ts_sec  = now.tv_sec;

            sendto_dbg(sock, (char *) &window[index],
                        sizeof(window[index])-(MAX_MESS_LEN - window[index].payload_len + 1), 0,
                        servaddr->ai_addr,
                        servaddr->ai_addrlen);
          }

          gettimeofday(&time_of_last_win_move, NULL);
        }

        if (bytes_since_last_output >= NUM_BYTES_TO_PRINT_STATS)
        {
          gettimeofday(&now, NULL);
          timersub(&now, &time_of_last_output, &diff_time);
          bytes_successfully_sent += bytes_since_last_output;
          printf("\n--------------- %f Megabytes sent in order since last output ---------------\n", bytes_since_last_output/1000000.0);
          printf("Megabytes in total sent successfully: %f MB\n", bytes_successfully_sent/1000000.0);
          printf("Average transfer rate: %f MB/sec\n", (bytes_since_last_output/1000000.0)/(diff_time.tv_sec + (diff_time.tv_usec / 1000000.0)));

          // Reset certain metrics
          gettimeofday(&time_of_last_output, NULL);
          bytes_since_last_output = 0;
        }
    }

    gettimeofday(&end_of_transfer, NULL);
    fclose(fp);
}

/*
 * client has sent all data and all packets have been ACK'd
 * send FIN packets every 250 ms until server responds with FIN packet
 * if no response in 15 seconds, assume transmission is over and exit
 */
static void closing_sequence(int sock, struct addrinfo *servaddr){
  //sends a FIN once every 250 ms until a FIN is received
  fd_set mask, read_mask;
  struct timeval timeout;
  int num;
  struct timeval now;

  FD_ZERO(&read_mask);
  FD_SET(sock, &read_mask);

  //create FIN packet
  struct upkt fin_pkt;
  memset(&fin_pkt, 0, sizeof(fin_pkt));
  fin_pkt.fin = true;

  if(DEBUG_PRINTS){
    printf("FIN\n");
  }

  sendto_dbg(sock, (char *) &fin_pkt,
              sizeof(fin_pkt) - MAX_MESS_LEN, 0,
              servaddr->ai_addr,
              servaddr->ai_addrlen);

  //max of 600 (25 ms * 600 = 15 seconds) FIN packets may be sent
  int fins_sent = 0;

  while(true){
    mask = read_mask;
    timeout.tv_sec = 0;
    timeout.tv_usec = 25000;

    //15 seconds have passed, assume connection is over
    if(fins_sent > 600){
      state = CLOSING;
      break;
    }

    //wait for a response or a timeout
    num = select(FD_SETSIZE, &mask, NULL, NULL, &timeout);

    if(num > 0){
      if(FD_ISSET(sock, &mask)){
        struct upkt rcv_pkt;
        recv(sock, (char *) &rcv_pkt, sizeof(rcv_pkt), 0);

        if(rcv_pkt.is_rcv_msg == true){
          continue;
        }

        if(rcv_pkt.fin == true){
          printf("closing connection...");
          state = CLOSING;
          return;
        }
        else{
          continue;
        }
      }
    }
    else{
      //resend FIN packet
      gettimeofday(&now, NULL);
      fin_pkt.ts_sec  = now.tv_sec;
      fin_pkt.ts_usec = now.tv_usec;

      if(DEBUG_PRINTS){
        printf("FIN\n");
      }

      sendto_dbg(sock, (char *) &fin_pkt,
                 sizeof(fin_pkt) - MAX_MESS_LEN, 0,
                 servaddr->ai_addr,
                 servaddr->ai_addrlen);

      fins_sent++;
    }
  }
}

/* Read commandline arguments */
static void Usage(int argc, char *argv[]) {
    if (argc != 5) {
        Print_help();
    }

    loss_rate = atoi(argv[1]);
    env = argv[2];
    src_file = argv[3];

    if(loss_rate < 0 || loss_rate > 100){
      printf("loss rate must be between 0 and 100\n");
      Print_help();
    }

    if(strcmp("WAN", env) != 0 && strcmp("LAN", env) != 0 && strcmp("WAN-EC", env) != 0){
      printf("rcv: valid environment choices are \"WAN\", \"LAN\", or \"WAN-EC\"\n");
      Print_help();
    }
    else if (strcmp("WAN", env) == 0)
    {
      WINDOW_SIZE = 45;
      WINDOW_TIMEOUT_S = 1;
      WINDOW_TIMEOUT_US = 0;
      NUM_TO_RESEND = 2;
    }
    else if (strcmp("LAN", env) == 0)
    {
      WINDOW_SIZE = 45;
      WINDOW_TIMEOUT_S = 1;
      WINDOW_TIMEOUT_US = 0;
      NUM_TO_RESEND = 2;
    }
    else if (strcmp("WAN-EC", env) == 0)
    {
      WINDOW_SIZE = 55;
      WINDOW_TIMEOUT_S = 0;
      WINDOW_TIMEOUT_US = 50000;
      NUM_TO_RESEND = 2;
    }

    char *file_delim = strchr(argv[4], '@');
    if(file_delim == NULL){
      printf("invalid destination address format\n");
      Print_help();
    }
    *file_delim = '\0';

    char *server_addr = file_delim + 1;

    dst_file = argv[4];

    if(strlen(dst_file) > MAX_FILENAME){
      printf("destination file name cannot be longer than %d\n", MAX_FILENAME);
    }

    char *port_delim = strrchr(server_addr, ':');
    if (port_delim == NULL) {
        printf("invalid IP/port format\n");
        Print_help();
    }
    *port_delim = '\0';

    Port_Str = port_delim + 1;
    Server_IP = server_addr;

    /* allow IPv6 format like: [::1]:5555 by striping [] from Server_IP */
    if (Server_IP[0] == '[') {
        Server_IP++;
        Server_IP[strlen(Server_IP) - 1] = '\0';
    }
}

static void Print_help() {
    printf("Usage: ncp <loss_rate_percent> <env> <source_file_name> <dest_file_name>@<ip_address>:<port>\n");
    exit(0);
}
