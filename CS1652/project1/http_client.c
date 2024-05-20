#include "net_include.h"
#include <stdbool.h>

#define BUF_SIZE 1024

bool http_error(int);

int main(const int argc, const char** argv){
  if(argc != 4){
    fprintf(stderr, "usage: http_client <server_name> <server_port> <server_path>\n");
    exit(-1);
  }

  //get command line args
  const char *server_name = argv[1];
  int port = atoi(argv[2]);
  const char *server_path = argv[3];
  char *req_str = NULL;

  if(port < 0 || port > 65535){
    fprintf(stderr, "valid port numbers: 0-65535\n");
    exit(-1);
  }

  //create HTTP request
  char *get_req = "GET /%s HTTP/1.0\r\n\r\n";
  int ret = asprintf(&req_str, get_req, server_path);
  if (ret == -1) {
      fprintf(stderr, "Failed to allocate request string\n");
      exit(-1);
  }

  //open TCP socket
  int s;
  if ((s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
      perror("http_client: failed to create socket");
      exit(-1);
  }

  //get IP of server_name via DNS
  struct hostent *hp;
  if ((hp = gethostbyname(server_name)) == NULL) {
      herror("http_client: gethostbyname error");
      exit(-1);
  }

  //set up address struct
  struct sockaddr_in saddr;
  memset(&saddr, 0, sizeof(saddr));
  saddr.sin_family = AF_INET;
  memcpy(&saddr.sin_addr.s_addr, hp->h_addr, hp->h_length);
  saddr.sin_port = htons(port);

  //connect to server
  if (connect(s, (struct sockaddr *) &saddr, sizeof(saddr)) < 0) {
      perror("http_client: could not connect to server");
      exit(-1);
  }

  //send HTTP request to server via TCP connection
  int res = send(s, req_str, strlen(req_str), 0);
  if (res <= 0) {
      perror("http_client: send error");
      close(s);
      exit(-1);
  }

  //read http response header character by character
  char *resp_head = (char *) malloc(BUF_SIZE);
  char next[2];
  int mem = BUF_SIZE, len = 1;

  next[0] = 'A';
  next[1] = '\0';
  resp_head[0] = '\0';

  //search for end of first line
  //store first line
  while(next[0] != '\n'){
    if(recv(s, next, 1, 0) <= 0){
      perror("http_client: recv error");
      close(s);
      exit(-1);
    }

    //resize string if needed
    if(len >= mem){
      resp_head = (char *) realloc(resp_head, mem * 2);
      mem *= 2;
    }

    resp_head = strcat(resp_head, next);
    len++;
  }

  //find http response code in first line
  //first line is in the form "HTTP/X.X CODE DESCRIPTION"
  int code = 0;
  char *line = (char *) malloc(len + 1);
  line = strcpy(line, resp_head);

  char *word = strtok(line, " ");
  while(word != NULL){
    code = atoi(word);

    if(code > 0){
      break;
    }

    word = strtok(NULL, " ");
  }

  //read rest of response header until "\r\n\r\n" terminator found
  while(strstr(resp_head, "\r\n\r\n") == NULL){
    if(recv(s, next, 1, 0) <= 0){
      perror("http_client: recv error");
      close(s);
      exit(-1);
    }

    //resize string if needed
    if(len >= mem){
      resp_head = (char *) realloc(resp_head, mem * 2);
      mem *= 2;
    }

    resp_head = strcat(resp_head, next);
    len++;
  }

  //print header if http error occurred
  if(http_error(code)){
    fprintf(stderr, "%s", resp_head);
  }

  //print rest of http response
  while(true){
    res = recv(s, next, 1, 0);

    if(res == 0){
      break;
    }
    else if(res < 0){
      perror("http_client: recv error");
      close(s);
      exit(-1);
    }

    if(http_error(code)){
      fprintf(stderr, "%s", next);
    }
    else{
      printf("%s", next);
    }
  }

  //close TCP connection
  close(s);

  if(http_error(code)){
    exit(-1);
  }

  exit(0);
}//main

bool http_error(int response_code){
  return response_code < 200 || response_code >= 300;
}//http_error
