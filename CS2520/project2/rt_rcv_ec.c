/*
 * rt_rcv_ec.c, by Birju Patel and Alex Kwiatkowski
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
 */

#include "net_include.h"
#include "sendto_dbg.h"

#define PRINT_DEBUG 0

// Added for EC
#define PRINT_DRIFT_AND_DELAY 0
#define SET_CLOCK_MODE 0
#define MAX_DRIFT_US 1000
#define NUM_LIVE_TO_KEEP 10
#define AVG_WEIGHT 0.6 // Give new RTT/BaseDelta this weight and old 1-AVG_WEIGHT

static void Usage(int argc, char *argv[]);
static void Print_help();
static void Print_IP(const struct sockaddr *sa);
static int Cmp_time(struct timeval t1, struct timeval t2);
static double timeval_to_ms(struct timeval t);
static struct timeval ms_to_timeval(int ms);
static uint32_t timeval_to_usec(struct timeval t);
static struct timeval usec_to_timeval(uint32_t usec);
static struct timeval * Min_Time(struct timeval *time_events[], int len);

static int initialization_sequence(int sock, struct addrinfo *servaddr);
static void transmission_protocol(int sock, struct addrinfo *servaddr);

//Added for EC
static void get_new_drift();
static void gettimeofday_drift(struct timeval *curr_time);

TAILQ_HEAD(tailhead, pkt_list);
TAILQ_HEAD(lost_ls, lost_seq);

static char *Server_IP;
static char *Port_Str;
static char *App_Port_Str;
static int loss_rate;
static int latency_window_ms;
static struct timeval latency_window;

static struct timeval rtt, base_delta;

static const struct timeval Init_Interval = {1, 500000};
static const struct timeval Live_Interval = {2, 0};
static const struct timeval Report_Interval = {5, 0};
static const double RTT_MULTIPLIER = 1.25;
static const int NUM_EVENTS = 4;

static int clean_bytes_rcvd;
static int clean_pkts_rcvd;
static struct timeval stream_start_time;
static int num_pkts_delivered;
static struct timeval max_delay;
static struct timeval min_delay;
static struct timeval delay_sum;

static int highest_seq_recvd;
static int next_seq_to_deliver;

// Added for EC
static const struct timeval Drift_Interval = {1, 0};
static struct timeval curr_drift;
static int drift_mode; //if 0 subtract drift, if 1 add, if 2 no drift
// Need to keep track of live packets in case delay is incredibly high
// Might send a live packet before server can respond and thus never get a value
struct upkt live_pkt_buf[NUM_LIVE_TO_KEEP];

int main(int argc, char *argv[])
{
    int                     sock;
    struct addrinfo         hints, *servinfo, *servaddr;

    int ret;
    /* Parse commandline args */
    Usage(argc, argv);
    printf("Attempting to open connection to %s at port %s with %d%% loss.\n", Server_IP, Port_Str, loss_rate);
    sendto_dbg_init(loss_rate);

    /* Set up hints to use with getaddrinfo */
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET; /* we'll use AF_INET for IPv4, but can use AF_INET6 for IPv6 or AF_UNSPEC if either is ok */
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_protocol = IPPROTO_UDP;

    /* Use getaddrinfo to get IP info for string IP address (or hostname) in
     * correct format */
    ret = getaddrinfo(Server_IP, Port_Str, &hints, &servinfo);
    if (ret != 0) {
       printf("getaddrinfo error: %s\n", gai_strerror(ret));
       exit(1);
    }

    /* Loop over list of available addresses and take the first one that works
     * */
    for (servaddr = servinfo; servaddr != NULL; servaddr = servaddr->ai_next) {
        /* print IP, just for demonstration */
        printf("Found server address:\n");
        Print_IP(servaddr->ai_addr);
        printf("\n");

        /* setup socket based on addr info. manual setup would look like:
         *   socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP) */
        sock = socket(servaddr->ai_family, servaddr->ai_socktype, servaddr->ai_protocol);
        if (sock < 0) {
            continue;
        }

        break; /* got a valid socket */
    }

    if (servaddr == NULL) {
        printf("No valid address found...exiting\n");
        exit(1);
    }

    // If the initialization sequence failed, simply exit
    if (!initialization_sequence(sock, servaddr))
    {
      printf("Server is busy with another receiver. Exiting...\n");
    }
    else
    {
      printf("Connected and ready to receive data.\n");
      transmission_protocol(sock, servaddr);
    }

    /* Cleanup */
    freeaddrinfo(servinfo);
    close(sock);

    return 0;

}

// Returns 1 if the server is not busy and able to handle our request
// Else returns 0
static int initialization_sequence(int sock, struct addrinfo *servaddr)
{
    struct sockaddr_storage from_addr;
    socklen_t               from_len;

    fd_set                  mask, read_mask;
    struct upkt             send_pkt, recvd_pkt;
    struct timeval          now, recv_time, msg_time, timeout;
    int                     num, seq = 0;
    memset(&send_pkt, 0, sizeof(send_pkt));

    FD_ZERO(&read_mask);
    FD_SET(sock, &read_mask);

    printf("Starting initialization sequence..\n");

    for(;;)
    {
        /* (Re)set mask and timeout */
        mask = read_mask;
        timeout = Init_Interval;

        num = select(FD_SETSIZE, &mask, NULL, NULL, &timeout);

        if (num > 0)
        {
            if (FD_ISSET(sock, &mask)) {
                from_len = sizeof(from_addr);
                recvfrom(sock, &recvd_pkt, sizeof(recvd_pkt), 0,
                          (struct sockaddr *)&from_addr,
                          &from_len);

                // If the packet has the init flag set and matches our last sent sequence number:
                // Use it to calculate rtt and base_delta
                // Else if the packet is a nack, exit.
                if (recvd_pkt.init && recvd_pkt.seq == seq - 1)
                {
                    /* Need to know when message was received for base_delta */
                    gettimeofday(&recv_time, NULL);


                    if(PRINT_DEBUG) { printf("Received init ACK, sequence #%d\n", recvd_pkt.seq); }

                    msg_time.tv_sec = recvd_pkt.ts_sec;
                    msg_time.tv_usec = recvd_pkt.ts_usec;

                    // Calculate RTT and BASE_DELTA
                    timersub(&recv_time, &now, &rtt);
                    timersub(&recv_time, &msg_time, &base_delta);

                    if(PRINT_DEBUG) { printf("RTT: %f ms\nBASE_DELTA: %f ms\n", timeval_to_ms(rtt), timeval_to_ms(base_delta)); }
                    return 1;
                }
                else if (recvd_pkt.nack)
                {
                    return 0;
                }
            }
        }
        else
        {
            /* Fill in header info */
            // now can also be used for our rtt calculations
            gettimeofday(&now, NULL);
            send_pkt.ts_sec  = now.tv_sec;
            send_pkt.ts_usec = now.tv_usec;
            send_pkt.seq     = seq++;
            send_pkt.init    = true;

            if(PRINT_DEBUG) { printf("Sending init packet #%d\n", seq - 1); }

            sendto_dbg(sock, (char *) &send_pkt,
                    sizeof(send_pkt) - MAX_MESS_LEN, 0,
                    servaddr->ai_addr,
                    servaddr->ai_addrlen);
        }
    }
    return 1;
}

// Implement the "almost reliable" streaming protocol
static void transmission_protocol(int sock, struct addrinfo *servaddr)
{
    struct sockaddr_storage from_addr;
    socklen_t               from_len;
    fd_set                  mask, read_mask;

    int                     num, ret, bytes;
    struct upkt             recvd_pkt, nack_pkt;
    struct pkt_list         *next_item, *insert_target;
    struct lost_seq         *next_gap;
    struct timeval          now, drift_now, pkt_delay;
    struct timeval          live_timeout, live_send_time, *live_ptr = NULL;
    struct timeval          sent_time, deliver_timeout, deliver_time, *deliver_ptr = NULL;
    struct timeval          stat_time, stat_timeout, *stat_ptr = NULL;
    struct timeval          nack_time, nack_timeout, *nack_ptr = NULL;
    struct timeval          *time_events[NUM_EVENTS], *min_timeout;
    struct timeval          last_live_ack, *last_live_ack_ptr = NULL;
    struct timeval          drift_timeout;
    int                     live_seq_num = 0, buf_size = 0, outstanding_pkts = 0;
    struct tailhead         latency_buffer;
    struct lost_ls          missing_pkts;
    bool                    target_found, is_dup, first_pkt_rcvd = false;
    struct addrinfo         hints, *app_info;

    //get address of application
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_protocol = IPPROTO_UDP;

    ret = getaddrinfo("127.0.0.1", App_Port_Str, &hints, &app_info);
    if(ret != 0){
      printf("rt_srv: getaddrinfo error %s\n", gai_strerror(ret));
      exit(1);
    }


    //for statistics calculations
    highest_seq_recvd = 0;
    next_seq_to_deliver = 0;
    clean_pkts_rcvd = 0;
    clean_bytes_rcvd = 0;
    num_pkts_delivered = 0;
    delay_sum = (struct timeval) {0, 0};

    // Set up Drift Timeouts
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

    /* Set up latency buffer and lost packet list */
    TAILQ_INIT(&latency_buffer);
    TAILQ_INIT(&missing_pkts);

    /* Set up mask for file descriptors we want to read from */
    FD_ZERO(&read_mask);
    FD_SET(sock, &read_mask);

    /* Set up live timeout */
    gettimeofday(&now, NULL);
    timeradd(&now, &Live_Interval, &live_send_time);

    // Set up Drift Timeout
    timeradd(&now, &Drift_Interval, &drift_timeout);

    for(;;)
    {
        mask = read_mask;

        /* Calculate time until next LIVE packet is sent */
        gettimeofday(&now, NULL);
        gettimeofday_drift(&drift_now);
        timersub(&live_send_time, &drift_now, &live_timeout);
        live_ptr = &live_timeout;

        /* Calculate time until next buffered packet is delivered */
        if(buf_size > 0){
          timersub(&deliver_time, &drift_now, &deliver_timeout);
          deliver_ptr = &deliver_timeout;
        }
        else{
          deliver_ptr = NULL;
        }

        /* Calculate time until next statistics report */
        if(highest_seq_recvd > 0){
          timersub(&stat_time, &now, &stat_timeout);
          stat_ptr = &stat_timeout;
        }
        else{
          stat_ptr = NULL;
        }

        /* Calculate time until next retranmission of NACKs */
        if(outstanding_pkts > 0){
          timersub(&nack_time, &drift_now, &nack_timeout);
          nack_ptr = &nack_timeout;
        }
        else{
          nack_ptr = NULL;
        }

        time_events[0] = live_ptr;
        time_events[1] = deliver_ptr;
        time_events[2] = stat_ptr;
        time_events[3] = nack_ptr;
        min_timeout = Min_Time(time_events, NUM_EVENTS);

        num = select(FD_SETSIZE, &mask, NULL, NULL, min_timeout);

        //event: packet received from server
        if (num > 0) {
            if (FD_ISSET(sock, &mask)) {
                from_len = sizeof(from_addr);
                bytes = recvfrom(sock, &recvd_pkt, sizeof(recvd_pkt), 0,
                                 (struct sockaddr *) &from_addr,
                                 &from_len);

                if(recvd_pkt.live){
                  //received live packet
                  gettimeofday_drift(&drift_now);
                  if(PRINT_DEBUG) { printf("live packet #%d received\n", recvd_pkt.seq); }
                  if(recvd_pkt.seq == live_pkt_buf[recvd_pkt.seq % NUM_LIVE_TO_KEEP].seq)
                  {
                    struct timeval temp_rtt, temp_base_delta;
                    struct timeval recv_time_ts = (struct timeval) {live_pkt_buf[recvd_pkt.seq % NUM_LIVE_TO_KEEP].ts_sec, live_pkt_buf[recvd_pkt.seq % NUM_LIVE_TO_KEEP].ts_usec};
                    last_live_ack = (struct timeval) {recvd_pkt.ts_sec, recvd_pkt.ts_usec};
                    last_live_ack_ptr = &last_live_ack;
                    timersub(&drift_now, &recv_time_ts, &temp_rtt);
                    if(PRINT_DEBUG)
                      printf("Current RTT: %f, New RTT: %f\n", (rtt.tv_sec + rtt.tv_usec/1000000.0), (temp_rtt.tv_sec + temp_rtt.tv_usec/1000000.0));

                    // Update RTT
                    rtt.tv_sec = (1-AVG_WEIGHT)*rtt.tv_sec + (AVG_WEIGHT)*temp_rtt.tv_sec;
                    rtt.tv_usec = (1-AVG_WEIGHT)*rtt.tv_usec + (AVG_WEIGHT)*temp_rtt.tv_usec;

                    timersub(&drift_now, last_live_ack_ptr, &temp_base_delta);
                    if(PRINT_DEBUG)
                      printf("Current Base Delta: %f, New Base Delta: %f\n", (base_delta.tv_sec + base_delta.tv_usec/1000000.0), (temp_base_delta.tv_sec + temp_base_delta.tv_usec/1000000.0));

                    // Update Base Delta
                    base_delta.tv_sec = (1-AVG_WEIGHT)*base_delta.tv_sec + (AVG_WEIGHT)*temp_base_delta.tv_sec;
                    base_delta.tv_usec = (1-AVG_WEIGHT)*base_delta.tv_usec + (AVG_WEIGHT)*temp_base_delta.tv_usec;

                    if(PRINT_DEBUG)
                    {
                      printf("Averaged RTT: %f", (rtt.tv_sec + rtt.tv_usec/1000000.0));
                      printf("Averaged Base Delta: %f", (base_delta.tv_sec + base_delta.tv_usec/1000000.0));
                    }
                  }
                }
                else if(recvd_pkt.init || recvd_pkt.nack){
                  //received spurious packet
                  if(PRINT_DEBUG) { printf("spurious packet received\n"); }
                }
                else{
                  //received data packet
                  if(!first_pkt_rcvd){
                    printf("First data packet is received. The stream has started.\n");
                    first_pkt_rcvd = true;
                  }

                  sent_time = (struct timeval) {recvd_pkt.ts_sec, recvd_pkt.ts_usec};

                  if(highest_seq_recvd == 0){
                    //this is the first data packet received
                    gettimeofday_drift(&stream_start_time);
                    timersub(&stream_start_time, &sent_time, &min_delay);
                    timersub(&stream_start_time, &sent_time, &max_delay);
                    timeradd(&stream_start_time, &Report_Interval, &stat_time);
                  }

                  if(buf_size == 0){
                    //first data packet is received, begin delivery timer
                    timeradd(&sent_time, &latency_window, &deliver_time);
                    timeradd(&deliver_time, &base_delta, &deliver_time);

                    //latency buffer is empty, so current packet will be the next to be delivered
                    next_seq_to_deliver = recvd_pkt.seq;
                  }

                  insert_target = NULL;
                  target_found = false;
                  is_dup = false;

                  if(recvd_pkt.seq == highest_seq_recvd + 1){
                    //expected packet received, no gap
                    //only calculate statistics for clean packets (exclude retransmissions)
                    clean_bytes_rcvd += bytes;
                    clean_pkts_rcvd += 1;

                    //update min and max delay statistics
                    gettimeofday_drift(&drift_now);
                    timersub(&drift_now, &sent_time, &pkt_delay);
                    if(Cmp_time(min_delay, pkt_delay) > 0){
                      min_delay = pkt_delay;
                    }

                    if(Cmp_time(max_delay, pkt_delay) < 0){
                      max_delay = pkt_delay;
                    }

                    //for average delay calculations
                    timeradd(&delay_sum, &pkt_delay, &delay_sum);
                  }
                  else if(recvd_pkt.seq > highest_seq_recvd + 1){
                    //unexpected packet received, gap detected
                    int gap_size = recvd_pkt.seq - highest_seq_recvd;

                    for(int i = 0; i < gap_size - 1; i++){
                      //if the lost packet comes before the very next packet to
                      //be delivered to the application, do not request it
                      if(highest_seq_recvd + 1 + i <= next_seq_to_deliver){
                        continue;
                      }

                      //if lost packet list was previously empty, restart NACK timer
                      if(outstanding_pkts == 0){
                        //resend NACKs for every missing packet every RTT_MULTIPLIER RTTs
                        uint32_t rtt_as_usec = timeval_to_usec(rtt);
                        struct timeval nack_interval = usec_to_timeval((int) (rtt_as_usec * RTT_MULTIPLIER));
                        gettimeofday_drift(&drift_now);
                        timeradd(&drift_now, &nack_interval, &nack_time);
                      }

                      //add to list of missing packets
                      next_gap = (struct lost_seq *) malloc(sizeof(struct lost_seq));
                      next_gap->seq = highest_seq_recvd + 1 + i;
                      TAILQ_INSERT_TAIL(&missing_pkts, next_gap, tailq);
                      outstanding_pkts += 1;

                      //send a nack
                      gettimeofday_drift(&drift_now);
                      memset(&nack_pkt, 0, sizeof(nack_pkt));
                      nack_pkt.nack = true;
                      nack_pkt.seq = highest_seq_recvd + 1 + i;
                      nack_pkt.ts_sec = drift_now.tv_sec;
                      nack_pkt.ts_usec = drift_now.tv_usec;

                      if(PRINT_DEBUG) { printf("sending nack for packet #%d\n", nack_pkt.seq); }

                      sendto_dbg(sock, (char *) &nack_pkt,
                              sizeof(nack_pkt) - MAX_MESS_LEN, 0,
                              servaddr->ai_addr,
                              servaddr->ai_addrlen);
                    }
                  }
                  else if(recvd_pkt.seq > next_seq_to_deliver && recvd_pkt.seq < highest_seq_recvd){
                    //old packet received, fulfills a retransmission request
                    //remove from list of missing packets
                    TAILQ_FOREACH(next_gap, &missing_pkts, tailq){
                      if(next_gap->seq == recvd_pkt.seq){
                        TAILQ_REMOVE(&missing_pkts, next_gap, tailq);
                        free(next_gap);
                        outstanding_pkts -= 1;
                        break;
                      }
                    }

                    //add new packet in latency window
                    struct pkt_list *ptr = NULL;
                    TAILQ_FOREACH(ptr, &latency_buffer, tailq){
                      if((ptr->pkt).seq < recvd_pkt.seq){
                        insert_target = ptr;
                        continue;
                      }
                      else if((ptr->pkt).seq == recvd_pkt.seq){
                        //we have received a duplicate packet
                        //packet is already in latency window, disregard it
                        is_dup = true;
                        break;
                      }
                      else{
                        target_found = true;
                        break;
                      }
                    }
                  }

                  if(recvd_pkt.seq >= highest_seq_recvd){
                    highest_seq_recvd = recvd_pkt.seq;
                  }

                  //place packet in queue
                  next_item = (struct pkt_list *) malloc(sizeof(struct pkt_list));
                  memcpy(&(next_item->pkt), &recvd_pkt, sizeof(recvd_pkt));
                  gettimeofday_drift(&drift_now);
                  next_item->recvd_ts_sec = drift_now.tv_sec;
                  next_item->recvd_ts_usec = drift_now.tv_usec;

                  if(target_found){
                    TAILQ_INSERT_AFTER(&latency_buffer, insert_target, next_item, tailq);
                    buf_size += 1;
                  }
                  else if(!is_dup && recvd_pkt.seq >= next_seq_to_deliver){
                    TAILQ_INSERT_TAIL(&latency_buffer, next_item, tailq);
                    buf_size += 1;
                  }

                  if(PRINT_DEBUG) { printf("received packet #%d\n", recvd_pkt.seq); }
                }
            }
        }

        //event: time to send a live packet
        gettimeofday_drift(&drift_now);
        if(live_ptr != NULL && Cmp_time(drift_now, live_send_time) >= 0) {
            int index = live_seq_num % NUM_LIVE_TO_KEEP;
            //send next LIVE packet
            memset(&live_pkt_buf[index], 0, sizeof(live_pkt_buf[index]));

            gettimeofday_drift(&drift_now);
            live_pkt_buf[index].ts_sec  = drift_now.tv_sec;
            live_pkt_buf[index].ts_usec = drift_now.tv_usec;
            live_pkt_buf[index].live = true;
            live_pkt_buf[index].seq = live_seq_num;

            if(PRINT_DEBUG) { printf("Sending live packet #%d\n", live_seq_num); }
            live_seq_num += 1;

            sendto_dbg(sock, (char *) &live_pkt_buf[index],
                    sizeof(live_pkt_buf[index]) - MAX_MESS_LEN, 0,
                    servaddr->ai_addr,
                    servaddr->ai_addrlen);

            //set timer to send next live packet
            gettimeofday_drift(&drift_now);
            timeradd(&drift_now, &Live_Interval, &live_send_time);
        }

        //event: time to deliver a packet to the application
        if(deliver_ptr != NULL && Cmp_time(drift_now, deliver_time) >= 0){
          //remove packet at head of buffer
          next_item = TAILQ_FIRST(&latency_buffer);

          //send payload to application
          sendto(sock, (char *) &next_item->pkt.payload, (next_item->pkt).payload_size,
                 0, app_info->ai_addr, app_info->ai_addrlen);

          if(PRINT_DEBUG) { printf("delivered packet #%d\n", (next_item->pkt).seq); }

          TAILQ_REMOVE(&latency_buffer, next_item, tailq);
          free(next_item);
          num_pkts_delivered += 1;
          buf_size -= 1;

          if(buf_size > 0){
            //recalculate time to deliver next packet
            next_item = TAILQ_FIRST(&latency_buffer);
            sent_time = (struct timeval) {(next_item->pkt).ts_sec, (next_item->pkt).ts_usec};
            timeradd(&sent_time, &latency_window, &deliver_time);
            timeradd(&deliver_time, &base_delta, &deliver_time);
            next_seq_to_deliver = (next_item->pkt).seq;
          }

          if(outstanding_pkts > 0){
            //remove any packet lower than the next packet to be delivered from the lost packet list
            //we are giving up on all those packets
            int to_remove = 0;
            struct lost_seq *ptr;
            TAILQ_FOREACH(ptr, &missing_pkts, tailq){
              if(ptr->seq <= next_seq_to_deliver){
                to_remove += 1;
              }
              else{
                break;
              }
            }

            for(int i = 0; i < to_remove; i++){
              ptr = TAILQ_FIRST(&missing_pkts);
              TAILQ_REMOVE(&missing_pkts, ptr, tailq);
              free(ptr);
              outstanding_pkts -= 1;
            }
          }
        }

        //event: time to print statistics report
        gettimeofday(&now, NULL);
        if(stat_ptr != NULL && Cmp_time(now, stat_time) > 0){
          //reset timer for next report
          gettimeofday(&now, NULL);
          timeradd(&now, &Report_Interval, &stat_time);

          long int stream_time = now.tv_sec - stream_start_time.tv_sec;
          stream_time *= 1000000;
          stream_time += now.tv_usec - stream_start_time.tv_usec;

          //throughput in Mbps
          double bit_rate = clean_bytes_rcvd * 8;
          bit_rate = bit_rate / stream_time;

          //throughput in packets per second
          stream_time /= 1000000;
          double pkt_rate = clean_pkts_rcvd / stream_time;

          //loss rate
          int lost_pkts = highest_seq_recvd - num_pkts_delivered - buf_size + 1;
          double loss_rate = (double) lost_pkts / (double) highest_seq_recvd;

          printf("\n---STREAM STATISTICS---\n");
          printf("total clean data received: %f MB\n", clean_bytes_rcvd / 1000000.0);
          printf("total clean packets received: %d packets\n", clean_pkts_rcvd);
          printf("streaming rate: %f Mbps\n", bit_rate);
          printf("streaming rate: %f packets per second\n\n", pkt_rate);
          printf("highest sequence number received: %d\n", highest_seq_recvd);
          printf("lost packets: %d packets\n", lost_pkts);
          printf("loss rate: %f%%\n\n", loss_rate * 100.0);
          printf("minimum oneway delay: %f ms\n", timeval_to_ms(min_delay));
          printf("maximum oneway delay: %f ms\n", timeval_to_ms(max_delay));
          printf("average oneway delay: %f ms\n", timeval_to_ms(delay_sum) / clean_pkts_rcvd);
          printf("-----------------------\n\n");
        }

        //event: time to resend NACKs for missing packets
        if(nack_ptr != NULL && Cmp_time(drift_now, nack_time) > 0){
          //reset timer for next resend event
          uint32_t rtt_as_usec = timeval_to_usec(rtt);
          struct timeval nack_interval = usec_to_timeval((int) (rtt_as_usec * RTT_MULTIPLIER));
          gettimeofday_drift(&drift_now);
          timeradd(&drift_now, &nack_interval, &nack_time);

          //resend NACKs for all missing packets
          struct lost_seq *ptr;
          TAILQ_FOREACH(ptr, &missing_pkts, tailq){
            memset(&nack_pkt, 0, sizeof(nack_pkt));
            nack_pkt.nack = true;
            nack_pkt.seq = ptr->seq;
            nack_pkt.ts_sec = drift_now.tv_sec;
            nack_pkt.ts_usec = drift_now.tv_usec;

            if(PRINT_DEBUG) { printf("resending nack for packet #%d\n", nack_pkt.seq); }

            sendto_dbg(sock, (char *) &nack_pkt,
                    sizeof(nack_pkt) - MAX_MESS_LEN, 0,
                    servaddr->ai_addr,
                    servaddr->ai_addrlen);
          }
        }

        //event: New Clock Drift
        //This event should not trigger if drift_mode is 2 (i.e. no drift)
        if(first_pkt_rcvd && drift_mode != 2 && Cmp_time(now, drift_timeout) >= 0){
          timeradd(&now, &Drift_Interval, &drift_timeout); //reset drift timer
          get_new_drift();
        }
    }
}

/* Print an IP address from sockaddr struct, using API functions */
void Print_IP(const struct sockaddr *sa)
{
    char ipstr[INET6_ADDRSTRLEN];
    void *addr;
    char *ipver;
    uint16_t port;

    if (sa->sa_family == AF_INET) {
        struct sockaddr_in *ipv4 = (struct sockaddr_in *)sa;
        addr = &(ipv4->sin_addr);
        port = ntohs(ipv4->sin_port);
        ipver = "IPv4";
    } else if (sa->sa_family == AF_INET6) {
        struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)sa;
        addr = &(ipv6->sin6_addr);
        port = ntohs(ipv6->sin6_port);
        ipver = "IPv6";
    } else {
        printf("Unknown address family\n");
        return;
    }

    inet_ntop(sa->sa_family, addr, ipstr, sizeof(ipstr));
    printf("%s: %s:%d\n", ipver, ipstr, port);
}

/* Read commandline arguments */
static void Usage(int argc, char *argv[]) {
    char *port_delim;

    if (argc != 5) {
        Print_help();
    }

    loss_rate = atoi(argv[1]);

    if(loss_rate < 0 || loss_rate > 100){
      printf("rt_rcv_ec: loss rate must be between 0 and 100\n");
      Print_help();
    }

    /* Find ':' separating IP and port, and zero it */
    port_delim = strrchr(argv[2], ':');
    if (port_delim == NULL) {
        printf("invalid IP/port format\n");
        Print_help();
    }
    *port_delim = '\0';

    Port_Str = port_delim + 1;

    Server_IP = argv[2];
    /* allow IPv6 format like: [::1]:5555 by striping [] from Server_IP */
    if (Server_IP[0] == '[') {
        Server_IP++;
        Server_IP[strlen(Server_IP) - 1] = '\0';
    }

    App_Port_Str = argv[3];

    latency_window_ms = atoi(argv[4]);
    if(latency_window_ms > 1000){
      printf("latency window must be smaller than 1 second\n");
      Print_help();
    }

    latency_window = ms_to_timeval(latency_window_ms);
    printf("latency window set as %f ms\n", timeval_to_ms(latency_window));
}

static void Print_help() {
    printf("Usage: rt_rcv_ec <loss_rate_percent> <server_ip>:<port> <app_port> <latency_window (ms)>\n");
    exit(0);
}

static struct timeval * Min_Time(struct timeval *time_events[], int len){
  struct timeval *min_time_ptr = NULL;
  struct timeval min_time;

  for(int i = 0; i < len; i++){
    if(time_events[i] != NULL){
      if(min_time_ptr == NULL){
        min_time_ptr = time_events[i];
        min_time = *time_events[i];
      }
      else{
        if(Cmp_time(min_time, *time_events[i]) > 0){
          min_time = *time_events[i];
          min_time_ptr = time_events[i];
        }
      }
    }
  }

  return min_time_ptr;
}

static uint32_t timeval_to_usec(struct timeval t){
  return t.tv_sec * 1000000 + t.tv_usec;
}

static struct timeval usec_to_timeval(uint32_t usec){
  struct timeval time;
  time.tv_sec = (int) (usec / 1000000);
  time.tv_usec = usec % 1000000;
  return time;
}

static double timeval_to_ms(struct timeval t){
  double ms;
  ms = t.tv_sec * 1000.0 + t.tv_usec / 1000.0;
  return ms;
}

static struct timeval ms_to_timeval(int ms){
  struct timeval time;
  time.tv_sec = (int) (ms / 1000);
  time.tv_usec = (ms % 1000) * 1000;
  return time;
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

// Gets the current time using an artificial drift
static void gettimeofday_drift(struct timeval *curr_time)
{
  struct timeval t;
  gettimeofday(&t, NULL);
  if(drift_mode == 2){
    gettimeofday(curr_time, NULL);
    return;
  }
  else if(drift_mode == 1){
    timeradd(&t, &curr_drift, curr_time);
  }
  else{
    timersub(&t, &curr_drift, curr_time);
  }
  if(PRINT_DEBUG)
    printf("Actual time: %f \nDrift time: %f \n", (t.tv_sec+t.tv_usec/1000000.0), (curr_time->tv_sec+curr_time->tv_usec/1000000.0));
}
