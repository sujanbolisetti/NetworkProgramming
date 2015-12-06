CC = gcc

LIBS = -lresolv -lnsl -lpthread
	
FLAGS = -g -O2

CFLAGS = ${FLAGS} -Isrc/

DEPS =  src_usp.h src_constants.h

all: src_tour src_arp

src_tour: src_tour.o tour_utils.o mcast_join_utils.o utils_wrapsock.o arp_utils.o icmp_utils.o
	${CC} ${FLAGS} src_tour.o tour_utils.o mcast_join_utils.o utils_wrapsock.o arp_utils.o icmp_utils.o -o src_tour ${LIBS}
	
src_arp: src_arp.o arp_utils.o get_hw_addrs.o utils_wrapsock.o tour_utils.o mcast_join_utils.o
	${CC} ${FLAGS} src_arp.o arp_utils.o get_hw_addrs.o utils_wrapsock.o tour_utils.o mcast_join_utils.o -o src_arp ${LIBS}

src_arp.o: src_arp.c ${DEPS}
	${CC} ${CFLAGS} -c src_arp.c

arp_utils.o: arp_utils.c 
	${CC} ${CFLAGS} -c arp_utils.c 
	
get_hw_addrs.o: get_hw_addrs.c
	${CC} ${CFLAGS} -c get_hw_addrs.c

src_tour.o: src_tour.c ${DEPS}
	${CC} ${CFLAGS} -c src_tour.c
	
tour_utils.o: tour_utils.c ${DEPS}
	${CC} ${CFLAGS} -c tour_utils.c	
	
mcast_join_utils.o: mcast_join_utils.c
	${CC} ${CFLAGS} -c mcast_join_utils.c	

utils_wrapsock.o: utils_wrapsock.c
	${CC} ${CFLAGS} -c utils_wrapsock.c	
	
icmp_utils.o: icmp_utils.c
	${CC} ${CFLAGS} -c icmp_utils.c	

clean:
	rm src_tour src_arp *.o
