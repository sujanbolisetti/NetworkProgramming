CC = gcc

LIBS = -lresolv -lnsl -lpthread
	
FLAGS = -g -O2

CFLAGS = ${FLAGS} -Isrc/

DEPS =  src/usp.h src/constants.h

all: client server

client: client.o wrapsock.o wrapunix.o get_ifi_info.o common_utils.o rtt_utils.o random.o client_utils.o
	${CC} ${FLAGS} client.o wrapsock.o wrapunix.o get_ifi_info.o common_utils.o rtt_utils.o random.o client_utils.o -o client ${LIBS}
	
server: server.o wrapsock.o wrapunix.o get_ifi_info.o common_utils.o rtt_utils.o server_utils.o
	${CC} ${FLAGS} server.o wrapsock.o wrapunix.o get_ifi_info.o common_utils.o rtt_utils.o server_utils.o -o server ${LIBS}
	
server.o: src/server.c ${DEPS}
	${CC} ${CFLAGS} -c src/server.c
	
client.o: src/client.c ${DEPS}
	${CC} ${CFLAGS} -c src/client.c
	
wrapsock.o: utils/wrapsock.c
	${CC} ${CFLAGS} -c utils/wrapsock.c

wrapunix.o: utils/wrapunix.c
	${CC} ${CFLAGS} -c utils/wrapunix.c
	
get_ifi_info.o: utils/get_ifi_info.c
	${CC} ${CFLAGS} -c utils/get_ifi_info.c
	
common_utils.o: utils/common_utils.c
	${CC} ${CFLAGS} -c utils/common_utils.c	

rtt_utils.o: utils/rtt_utils.c
	${CC} ${CFLAGS} -c utils/rtt_utils.c
	
random.o: utils/random.c
	${CC} ${CFLAGS} -c utils/random.c

server_utils.o: utils/server_utils.c ${DEPS} 
	${CC} ${CFLAGS} -c utils/server_utils.c
	
client_utils.o: utils/client_utils.c ${DEPS} 
	${CC} ${CFLAGS} -c utils/client_utils.c
	
clean:
	rm client server *.o
