CC = gcc

LIBS = -lresolv -lnsl -lpthread
	
FLAGS = -g -O2

CFLAGS = ${FLAGS} -Isrc/

DEPS =  src_usp.h src_constants.h

all: src_tour

src_tour: src_tour.o tour_utils.o mcast_join_utils.o utils_wrapsock.o
	${CC} ${FLAGS} src_tour.o tour_utils.o mcast_join_utils.o utils_wrapsock.o -o src_tour ${LIBS}

src_tour.o: src_tour.c ${DEPS}
	${CC} ${CFLAGS} -c src_tour.c
	
tour_utils.o: tour_utils.c ${DEPS}
	${CC} ${CFLAGS} -c tour_utils.c	
	
mcast_join_utils.o: mcast_join_utils.c
	${CC} ${CFLAGS} -c mcast_join_utils.c	

utils_wrapsock.o: utils_wrapsock.c
	${CC} ${CFLAGS} -c utils_wrapsock.c	
	

clean:
	rm src_tour *.o