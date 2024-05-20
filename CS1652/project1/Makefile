CC=gcc

CFLAGS = -g -c -Wall -pedantic

all: http_client http_server1 http_server2

http_client: http_client.o
	    $(CC) -o http_client http_client.o

http_server1: http_server1.o
			$(CC) -o http_server1 http_server1.o

http_server2: http_server2.o
			$(CC) -o http_server2 http_server2.o

clean:
	rm *.o
	rm http_client
	rm http_server1
	rm http_server2

%.o:    %.c
	$(CC) $(CFLAGS) $*.c
