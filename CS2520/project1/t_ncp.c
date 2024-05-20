#include "net_include.h"
#include <stdbool.h>

#define BUF_SIZE 1024
#define MEGABYTES_SENT 10

static int loss_rate;
static char *env;
static char *Server_IP;
static char *Port_Str;
static char *src_file;
static char *dst_file;

static void Usage(int argc, char *argv[]);
static void Print_help();

int main(int argc, char *argv[]){
  //for statistics calculations
  struct timeval start_of_transfer, end_of_transfer, diff_time;
  uint64_t bytes_sent = 0;

  //parse commandline args
  Usage(argc, argv);
  printf("Sending %s to %s@%s:%s\n", src_file, dst_file, Server_IP, Port_Str);

  struct addrinfo hints, *servinfo, *servaddr;
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_protocol = IPPROTO_TCP;

  int ret = getaddrinfo(Server_IP, Port_Str, &hints, &servinfo);

  if(ret != 0){
    printf("t_ncp: getaddrinfo error\n");
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
    printf("t_ncp: socket creation error\n");
    exit(1);
  }

  ret = connect(sock, servaddr->ai_addr, servaddr->ai_addrlen);

  if(ret < 0){
    printf("t_ncp: connection to server error\n");
    exit(1);
  }

  //start timer
  gettimeofday(&start_of_transfer, NULL);

  char mess_buf[BUF_SIZE];
  memset(mess_buf, 0, sizeof(mess_buf));
  strcpy(mess_buf, dst_file);
  strcat(mess_buf, "\n");

  ret = send(sock, &mess_buf, strlen(mess_buf), 0);

  FILE *f = fopen(src_file, "r");

  while((ret = fread(mess_buf, 1, sizeof(mess_buf) - 1, f)) > 0){
    bytes_sent += ret;
    ret = send(sock, mess_buf, ret, 0);
  }

  //end timer, calculate summary statistics
  double megabytes_sent = (double) bytes_sent / 1000000.0;
  gettimeofday(&end_of_transfer, NULL);
  timersub(&end_of_transfer, &start_of_transfer, &diff_time);
  printf("\n--------------- FINAL STATISTICS ---------------\n");
  printf("Time required for transfer: %f sec\n", (diff_time.tv_sec + (diff_time.tv_usec / 1000000.0)));
  printf("Megabytes in total sent successfully: %f MB\n", megabytes_sent);
  printf("Average transfer rate: %f MB/sec\n", megabytes_sent/(diff_time.tv_sec + (diff_time.tv_usec / 1000000.0)));

  close(sock);
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

    if(strcmp("WAN", env) != 0 && strcmp("LAN", env) != 0){
      printf("valid environment choices are \"WAN\" or \"LAN\"\n");
      Print_help();
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
