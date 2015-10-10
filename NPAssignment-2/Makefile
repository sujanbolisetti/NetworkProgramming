# This is a sample Makefile which compiles source files named:
# - tcpechotimeserv.c
# - tcpechotimecliv.c
# - time_cli.c
# - echo_cli.c
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
	
FLAGS = -g -O2

all: prifinfo get_ifi_info sock_ntop_host wrapsock wrapunix client server 

prifinfo: prifinfo.o
	${CC} ${FLAGS} prifinfo.o
prifinfo.o: prifinfo.c
	${CC} ${FLAGS} -c prifinfo.c

get_ifi_info: get_ifi_info.o
	${CC} ${FLAGS} get_ifi_info.o
get_ifi_info.o: get_ifi_info.c
	${CC} ${FLAGS} -c ../utils/get_ifi_info.c
	
sock_ntop_host: sock_ntop_host.o
	${CC} ${FLAGS} sock_ntop_host.o
sock_ntop_host.o: sock_ntop_host.c
	${CC} ${FLAGS} -c ../utils/sock_ntop_host.c
	
wrapsock: wrapsock.o
	${CC} ${FLAGS} wrapsock.o
wrapsock.o: wrapsock.c
	${CC} ${FLAGS} -c ../utils/wrapsock.c
	
wrapunix: wrapunix.o
	${CC} ${FLAGS} wrapunix.o
wrapunix.o: wrapunix.c
	${CC} ${FLAGS} -c ../utils/wrapunix.c
	
client: client.o
	${CC} ${FLAGS} client.o -o client
client.o: client.c
	${CC} ${FLAGS} -c ../utils/wrapsock.o ../utils/wrapunix.o ../utils/get_ifi_info.o client.c

server: server.o 
	${CC} ${FLAGS} -o server server.o
server.o: server.c
	${CC} ${FLAGS} -c ../utils/wrapsock.o ../utils/wrapunix.o ../utils/get_ifi_info.o server.c

clean:
	rm server client