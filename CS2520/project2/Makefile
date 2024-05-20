CC=gcc

CFLAGS = -c -Wall -pedantic -g

all: udp_stream udp_stream_rcv sendto_dbg rt_rcv rt_srv rt_rcv_ec rt_srv_ec

udp_stream: udp_stream.o
	    $(CC) -o udp_stream udp_stream.o

udp_stream_rcv: udp_stream_rcv.o
	    $(CC) -o udp_stream_rcv udp_stream_rcv.o

sendto_dbg:
	$(CC) -Wall -c sendto_dbg.c

rt_rcv: rt_rcv.o
	$(CC) -o rt_rcv rt_rcv.o sendto_dbg.o

rt_srv: rt_srv.o
	$(CC) -o rt_srv rt_srv.o sendto_dbg.o

rt_rcv_ec: rt_rcv_ec.o
	$(CC) -o rt_rcv_ec rt_rcv_ec.o sendto_dbg.o

rt_srv_ec: rt_srv_ec.o
	$(CC) -o rt_srv_ec rt_srv_ec.o sendto_dbg.o

clean:
	rm *.o

veryclean:
	rm rt_rcv
	rm rt_srv
	rm udp_stream
	rm udp_stream_rcv
	rm rt_rcv_ec
	rm rt_srv_ec

%.o:    %.c
	$(CC) $(CFLAGS) $*.c


