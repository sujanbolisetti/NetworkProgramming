CC = gcc

LIBS = -lresolv -lnsl -lpthread
	
FLAGS = -g -O2

CFLAGS = ${FLAGS} -Isrc/

DEPS =  src/usp.h src/constants.h

all: client server

client: client.o wrapsock.o wrapunix.o get_ifi_info.o common_utils.o
	${CC} ${FLAGS} client.o wrapsock.o wrapunix.o get_ifi_info.o common_utils.o -o client ${LIBS}
	
server: server.o wrapsock.o wrapunix.o get_ifi_info.o common_utils.o
	${CC} ${FLAGS} server.o wrapsock.o wrapunix.o get_ifi_info.o common_utils.o -o server ${LIBS}
	
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


	
clean:
	rm client server *.o
