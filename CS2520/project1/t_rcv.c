#include "net_include.h"
#include <ctype.h>
#include <stdbool.h>

#define BUF_SIZE 1024

static int loss_rate;
static char *Port_Str;
static char *env;

static void Usage(int argc, char *argv[]);
static void Print_help();
static void handle_connection(int conn, int sock);

int main(int argc, char *argv[]){
  /* Parse commandline args */
  Usage(argc, argv);
  printf("Awaiting file transfer on port %s\n", Port_Str);

  struct addrinfo hints, *servinfo, *servaddr;

  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;

  int ret = getaddrinfo(NULL, Port_Str, &hints, &servinfo);

  if(ret != 0){
    printf("t_rcv: getaddrinfo error\n");
    exit(1);
  }

  int sock = -1;
  for(servaddr = servinfo; servaddr != NULL; servaddr = servaddr->ai_next){
    sock = socket(servaddr->ai_family, servaddr->ai_socktype, servaddr->ai_protocol);
    if(sock < 0){
      continue;
    }

    //created valid socket
    break;
  }

  if(servaddr == NULL){
    printf("t_rcv: socket creation error\n");
    exit(1);
  }

  ret = bind(sock, servaddr->ai_addr, servaddr->ai_addrlen);

  if(ret < 0){
    printf("t_rcv: bind error\n");
    exit(1);
  }

  if (listen(sock, 32) < 0) {
      printf("t_rcv: listen error\n");
      close(sock);
      exit(-1);
  }

  int conn;
  while(true){
    conn = accept(sock, NULL, NULL);

    if(conn < 0){
      printf("t_rcv: accept error\n");
      close(sock);
      exit(1);
    }

    handle_connection(conn, sock);

    close(conn);
  }
}

static void handle_connection(int conn, int sock){
  char *dst_file = (char *) malloc(BUF_SIZE);
  char mess_buf[BUF_SIZE];
  int bytes, file_len = 0;

  printf("new connection received, transferring data...\n");

  //for statistics calculations
  struct timeval start_of_transfer, end_of_transfer, diff_time;
  uint64_t bytes_sent = 0;

  //start timer
  gettimeofday(&start_of_transfer, NULL);

  //search for the end of the first line, which specifies file name
  char next[2];
  next[0] = 'A';
  next[1] = '\0';
  dst_file[0] = '\0';

  while(next[0] != '\n'){
    if(recv(conn, next, 1, 0) <= 0){
      printf("t_rcv: recv error");
      close(conn);
      close(sock);
      exit(1);
    }

    if(next[0] == '\n'){
      break;
    }

    dst_file = strcat(dst_file, next);
    file_len++;
  }

  FILE *f = fopen(dst_file, "w");

  while(true){
    bytes = recv(conn, mess_buf, BUF_SIZE, 0);

    bytes_sent += bytes;

    if(bytes == 0){
      break;
    }
    else if(bytes < 0){
      printf("t_rcv: recv error");
      close(conn);
      close(sock);
      exit(1);
    }

    fwrite(mess_buf, 1, bytes, f);
  }

  fclose(f);

  //end timer, calculate summary statistics
  double megabytes_sent = (double) bytes_sent / 1000000.0;
  gettimeofday(&end_of_transfer, NULL);
  timersub(&end_of_transfer, &start_of_transfer, &diff_time);
  printf("\n--------------- FINAL STATISTICS ---------------\n");
  printf("Time required for transfer: %f sec\n", (diff_time.tv_sec + (diff_time.tv_usec / 1000000.0)));
  printf("Megabytes in total sent successfully: %f MB\n", megabytes_sent);
  printf("Average transfer rate: %f MB/sec\n", megabytes_sent / (diff_time.tv_sec + (diff_time.tv_usec / 1000000.0)));
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

  if(strcmp("WAN", env) != 0 && strcmp("LAN", env) != 0){
    printf("rcv: valid environment choices are \"WAN\" or \"LAN\"\n");
    Print_help();
  }
}

static void Print_help() {
  printf("Usage: rcv <loss_rate_percent> <port> <env>\n");
  exit(0);
}
