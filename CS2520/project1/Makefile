CC=gcc

CFLAGS = -g -c -Wall -pedantic

all: sendto_dbg gap_list rcv_buffer ncp rcv t_ncp t_rcv

sendto_dbg:
	$(CC) -Wall -c sendto_dbg.c

gap_list:
	$(CC) -Wall -c gap_list.c

rcv_buffer:
	$(CC) -Wall -c rcv_buffer.c

ncp: ncp.o
	$(CC) -o ncp ncp.o sendto_dbg.o rcv_buffer.o gap_list.o

rcv: rcv.o
	$(CC) -o rcv rcv.o sendto_dbg.o rcv_buffer.o gap_list.o

t_ncp: t_ncp.o
	$(CC) -o t_ncp t_ncp.o

t_rcv: t_rcv.o
	$(CC) -o t_rcv t_rcv.o

clean:
	rm *.o
	rm ncp
	rm rcv
	rm t_ncp
	rm t_rcv

%.o:    %.c
	$(CC) $(CFLAGS) $*.c
