# This is a sample Makefile which compiles source files named:
# src/daytimecli.c
# src/echoclient.c
# src/server.c
# src/client.c
#
# and creating executables: "server", "client", "time_cli"
# and "echo_cli", respectively.
#
# It uses various standard libraries, and the copy of Stevens'
# library "libunp.a" in ~cse533/Stevens/unpv13e_solaris2.10 .
#
# It also picks up the thread-safe version of "readline.c"
# from Stevens' directory "threads" and uses it when building
# the executable "server".
#
# It is set up, for illustrative purposes, to enable you to use
# the Stevens code in the ~cse533/Stevens/unpv13e_solaris2.10/lib
# subdirectory (where, for example, the file "unp.h" is located)
# without your needing to maintain your own, local copies of that
# code, and without your needing to include such code in the
# submissions of your assignments.
#
# Modify it as needed, and include it with your submission.

CC = gcc

LIBS = -lresolv -lnsl -lpthread
	
FLAGS = -g -O2

CFLAGS = ${FLAGS} -Isrc/

DEPS =  src/usp.h src/constants.h

all: client server time_cli echo_cli

time_cli: time_cli.o errorutils.o
	${CC} ${FLAGS} time_cli.o errorutils.o -o time_cli ${LIBS}
time_cli.o: src/time_cli.c ${DEPS}
	${CC} ${CFLAGS} -c src/time_cli.c

echo_cli: echo_cli.o errorutils.o wrapsock.o written.o
	${CC} ${FLAGS} echo_cli.o errorutils.o wrapsock.o written.o -o echo_cli ${LIBS}
echo_cli.o: src/echo_cli.c ${DEPS}
	${CC} ${CFLAGS} -c src/echo_cli.c

client: client.o errorutils.o
	${CC} ${FLAGS} client.o errorutils.o -o client ${LIBS}
client.o: src/client.c ${DEPS}
	${CC} ${CFLAGS} -c src/client.c

server: server.o wrapsock.o wrappthread.o errorutils.o written.o
	${CC} ${FLAGS} server.o wrapsock.o wrappthread.o errorutils.o written.o -o server ${LIBS}
server.o: src/server.c ${DEPS}
	${CC} ${CFLAGS} -c src/server.c


written.o: utils/written.c
	${CC} ${CFLAGS} -c utils/written.c

wrapsock.o: utils/wrapsock.c
	${CC} ${CFLAGS} -c utils/wrapsock.c

wrappthread.o: utils/wrappthread.c
	${CC} ${CFLAGS} -c utils/wrappthread.c 


errorutils.o: utils/errorutils.c
	  ${CC} ${CFLAGS} -c utils/errorutils.c

# pick up the thread-safe version of readline.c from directory "threads"

#readline.o: /home/courses/cse533/Stevens/unpv13e_solaris2.10/threads/readline.c
#	${CC} ${CFLAGS} -c /home/courses/cse533/Stevens/unpv13e_solaris2.10/threads/readline.c


clean:
	rm echo_cli server client time_cli *.o

