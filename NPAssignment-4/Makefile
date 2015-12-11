CC = gcc

LIBS = -lresolv -lnsl -lpthread
	
FLAGS = -g -O2

CFLAGS = ${FLAGS} -Isrc/

DEPS =  src_usp.h src_constants.h

all: tour_sbolisetti_sthota arp_sbolisetti_sthota

tour_sbolisetti_sthota: src_tour.o icmp_utils.o tour_utils.o mcast_join_utils.o utils_wrapsock.o arp_utils.o get_hw_addrs.o
	${CC} ${FLAGS} src_tour.o icmp_utils.o tour_utils.o mcast_join_utils.o utils_wrapsock.o arp_utils.o get_hw_addrs.o -o tour_sbolisetti_sthota ${LIBS}
	
arp_sbolisetti_sthota: src_arp.o arp_utils.o get_hw_addrs.o utils_wrapsock.o tour_utils.o mcast_join_utils.o icmp_utils.o
	${CC} ${FLAGS} src_arp.o arp_utils.o get_hw_addrs.o utils_wrapsock.o tour_utils.o mcast_join_utils.o icmp_utils.o -o arp_sbolisetti_sthota ${LIBS}

src_arp.o: src_arp.c ${DEPS}
	${CC} ${CFLAGS} -c src_arp.c

arp_utils.o: arp_utils.c 
	${CC} ${CFLAGS} -c arp_utils.c 
	
get_hw_addrs.o: get_hw_addrs.c
	${CC} ${CFLAGS} -c get_hw_addrs.c

src_tour.o: src_tour.c ${DEPS}
	${CC} ${CFLAGS} -c src_tour.c
	
icmp_utils.o: icmp_utils.c ${DEPS}
	${CC} ${CFLAGS} -c icmp_utils.c	
	
tour_utils.o: tour_utils.c ${DEPS}
	${CC} ${CFLAGS} -c tour_utils.c
	
mcast_join_utils.o: mcast_join_utils.c
	${CC} ${CFLAGS} -c mcast_join_utils.c	

utils_wrapsock.o: utils_wrapsock.c
	${CC} ${CFLAGS} -c utils_wrapsock.c	

clean:
	rm tour_sbolisetti_sthota arp_sbolisetti_sthota *.o
