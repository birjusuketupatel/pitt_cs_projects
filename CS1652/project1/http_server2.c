#include "net_include.h"
#include <ctype.h>
#include <stdbool.h>

#define BUF_SIZE 1024

void handle_connection(int, int);

int main(const int argc, const char** argv){
  //get command line args
  if(argc != 2){
    fprintf(stderr, "usage: http_server2 <port>\n");
    exit(-1);
  }

  int port = atoi(argv[1]);

  if (port < 1500 || port > 65535) {
    fprintf(stderr, "INVALID PORT NUMBER: %d, must be between 1500 and 65535\n", port);
    exit(-1);
  }

  //open TCP socket
  int s;
  if ((s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
      perror("http_server2: failed to create socket");
      exit(-1);
  }

  //set up address struct
  struct sockaddr_in saddr;
  memset(&saddr, 0, sizeof(saddr));
  saddr.sin_family = AF_INET;
  saddr.sin_addr.s_addr = INADDR_ANY;
  saddr.sin_port = htons(port);

  //bind to receive messages on port
  if (bind(s, (struct sockaddr *)&saddr, sizeof(saddr)) < 0) {
      perror("http_server2: bind error");
      close(s);
      exit(-1);
  }

  //listen for connection attempts on socket
  //max of 32 connections can wait to be serviced
  if (listen(s, 32) < 0) {
      perror("http_server2: listen error");
      close(s);
      exit(-1);
  }

  //implements socket multiplexing using select
  fd_set read_set;
  int num;

  //stores list of open connections in an array
  //server can handle max of BUF_SIZE parallel connections
  int open_conns[BUF_SIZE];
  int num_conns = 0;

  while(true){
    //fill read set
    FD_ZERO(&read_set);
    for(int i = 0; i < num_conns; i++){
      FD_SET(open_conns[i], &read_set);
    }
    FD_SET(s, &read_set);

    //block until next socket can be read
    num = select(FD_SETSIZE, &read_set, NULL, NULL, NULL);

    if(num > 0){
      if(FD_ISSET(s, &read_set) != 0){
        //do not accept a new connection if server is at capacity
        if(num_conns >= BUF_SIZE){
          continue;
        }

        //new connection requested
        //accepts a new connection if there is space in the list
        int new_conn;
        if((new_conn = accept(s, NULL, NULL)) < 0){
          perror("http_server2: accept error");
          close(s);
          exit(-1);
        }

        open_conns[num_conns] = new_conn;
        num_conns++;
      }

      //handle all open connections that may be read
      for(int i = 0; i < num_conns; i++){
        if(FD_ISSET(open_conns[i], &read_set) != 0){
          handle_connection(open_conns[i], s);

          close(open_conns[i]);

          if(num_conns > 0){
            num_conns--;
            open_conns[i] = open_conns[num_conns];
            i--;
          }
        }
      }
    }
    else{
      perror("http_server2: select error");
      close(s);
      exit(-1);
    }
  }

  close(s);
  exit(0);
}//main

/*
 * returns requested resource to a given tcp connection
 */
 void handle_connection(int conn, int s){
   //read http request from socket character by character
   char *req = (char *) malloc(BUF_SIZE);
   char next[2];
   int mem = BUF_SIZE, len = 1;

   next[0] = 'A';
   next[1] = '\0';
   req[0] = '\0';

   //search for end of first line
   //store first line
   while(next[0] != '\n'){
     if(recv(conn, next, 1, 0) <= 0){
       perror("http_server2: recv error");
       close(conn);
       close(s);
       exit(-1);
     }

     //resize string if needed
     if(len >= mem){
       req = (char *) realloc(req, mem * 2);
       mem *= 2;
     }

     req = strcat(req, next);
     len++;
   }

   //first line is in the form "GET ADDRESS HTTP/X.X"
   //gets address of file requested
   char *line = (char *) malloc(len + 1);
   line = strcpy(line, req);

   char *word = strtok(line, " ");
   char *server_path;
   while(word != NULL){
     if(word[0] == '/'){
       server_path = word + 1;
       break;
     }
     word = strtok(NULL, " ");
   }

   //read rest of request until "\r\n\r\n" terminator found
   while(strstr(req, "\r\n\r\n") == NULL){
     if(recv(conn, next, 1, 0) < 0){
       perror("http_server2: recv error");
       close(conn);
       close(s);
       exit(-1);
     }

     //resize string if needed
     if(len >= mem){
       req = (char *) realloc(req, mem * 2);
       mem *= 2;
     }

     req = strcat(req, next);
     len++;
   }

   FILE *f;
   f = fopen(server_path, "rb");

   //file not found, return a 404 error
   int res;
   if(f == NULL){
     char *notok_str = "HTTP/1.0 404 FILE NOT FOUND\r\n" \
                       "Content-type: text/html\r\n\r\n" \
                       "<html><body bgColor=black text=white>\n" \
                       "<h2>404 FILE NOT FOUND</h2>\n" \
                       "</body></html>\n";

     res = send(conn, notok_str, strlen(notok_str), 0);

     if(res <= 0){
       perror("http_server2: recv error");
       close(conn);
       close(s);
       exit(-1);
     }

     //handling complete
     return;
   }

   //file found, send header with response code 200
   char *response;
   char *ok_str = "HTTP/1.0 200 OK\r\n" \
                  "Content-type: text/plain\r\n" \
                  "Content-length: %d \r\n\r\n";

   //get size of the file
   fseek(f, 0, SEEK_END);
   int fsize = ftell(f);
   fseek(f, 0, SEEK_SET);

   asprintf(&response, ok_str, fsize);

   res = send(conn, response, strlen(response), 0);

   if(res <= 0){
     perror("http_server2: send error");
     close(conn);
     close(s);
     exit(-1);
   }

   //send all data from the file in BUF_SIZE chunks
   char buf[BUF_SIZE];
   while((res = fread(buf, 1, sizeof(buf) - 1, f)) > 0){
     buf[res] = '\0';

     res = send(conn, buf, strlen(buf), 0);

     if(res <= 0){
       perror("http_server2: send error");
       close(conn);
       close(s);
       exit(-1);
     }
   }
 }//handle_request
