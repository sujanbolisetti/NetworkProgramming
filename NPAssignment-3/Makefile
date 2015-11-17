CC = gcc

LIBS = -lresolv -lnsl -lpthread  -lm
	
FLAGS = -g -O2

CFLAGS = ${FLAGS} -Isrc/

DEPS =  src/usp.h src/constants.h

all: client server odr

client: client.o wrapsock.o common_utils.o
	${CC} ${FLAGS} client.o wrapsock.o common_utils.o -o client ${LIBS}
	
client.o: src/client.c ${DEPS}
	${CC} ${FLAGS} -c src/client.c
	
server: server.o wrapsock.o common_utils.o
	${CC} ${FLAGS} server.o wrapsock.o common_utils.o -o server ${LIBS}

odr: odr.o sock_ntop_host.o get_hw_addrs.o common_utils.o wrapsock.o odr_frame_utils.o odr_utils.o routing_utils.o
	${CC} ${FLAGS} odr.o sock_ntop_host.o get_hw_addrs.o common_utils.o wrapsock.o odr_frame_utils.o odr_utils.o routing_utils.o -o odr ${LIBS}

server.o: src/server.c ${DEPS}
	${CC} ${CFLAGS} -c src/server.c
	
wrapsock.o: utils/wrapsock.c
	${CC} ${CFLAGS} -c utils/wrapsock.c
	
common_utils.o: utils/common_utils.c
	${CC} ${CFLAGS} -c utils/common_utils.c	

get_hw_addrs.o: src/get_hw_addrs.c
	${CC} ${CFLAGS} -c src/get_hw_addrs.c
	
odr_utils.o: utils/odr_utils.o
	${CC} ${CFLAGS} -c utils/odr_utils.c

odr.o: src/odr.c ${DEPS}
	${CC} ${CFLAGS} -c src/odr.c

odr_frame_utils.o: utils/odr_frame_utils.c
	${CC} ${CFLAGS} -c utils/odr_frame_utils.c
	
sock_ntop_host.o: utils/sock_ntop_host.c
	${CC} ${CFLAGS} -c utils/sock_ntop_host.c

routing_utils.o: utils/routing_utils.c
	${CC} ${CFLAGS} -c utils/routing_utils.c


clean:
	rm client server odr *.o
