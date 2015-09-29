/*
 * server.c
 *
 *  Created on: Sep 16, 2015
 *      Author: sujan
 */

#include "usp.h"

int main(int argc,char **argv){

	int timeserver_listenfd, echoserver_listenfd;
	int *echoserver_connfd, *timeserver_connfd;
	struct sockaddr_in timeserveraddr,echoserveraddr;
	int maxfdp1;
	pthread_t tid;
	fd_set rset;
	int optval = 1;

	timeserver_listenfd = Socket(AF_INET,SOCK_STREAM,0);
	echoserver_listenfd = Socket(AF_INET,SOCK_STREAM,0);

	setsockopt(timeserver_listenfd,SOL_SOCKET,SO_REUSEADDR,&optval,sizeof(int));

	setsockopt(echoserver_listenfd,SOL_SOCKET,SO_REUSEADDR,&optval,sizeof(int));

	bzero(&timeserveraddr,sizeof(timeserveraddr));
	timeserveraddr.sin_family = AF_INET;
	timeserveraddr.sin_addr.s_addr =  htons(INADDR_ANY);
	timeserveraddr.sin_port = htons(TIME_SERVER_PORT_NUMBER);

	bzero(&echoserveraddr,sizeof(echoserveraddr));
	echoserveraddr.sin_family = AF_INET;
	echoserveraddr.sin_addr.s_addr =  htons(INADDR_ANY);
	echoserveraddr.sin_port = htons(ECHO_SERVER_PORT_NUMBER);

	Bind(timeserver_listenfd,(SA *)&timeserveraddr, sizeof(timeserveraddr));

	Bind(echoserver_listenfd,(SA *)&echoserveraddr, sizeof(echoserveraddr));

	Listen(echoserver_listenfd,BACKLOG);

	Listen(timeserver_listenfd,BACKLOG);

	FD_ZERO(&rset);

	for(; ;){

		FD_SET(timeserver_listenfd, &rset);
		FD_SET(echoserver_listenfd,&rset);

		maxfdp1 = max(timeserver_listenfd,echoserver_listenfd) +1;

		Select(maxfdp1, &rset, NULL, NULL, NULL);

		if(FD_ISSET(timeserver_listenfd, &rset)){

			timeserver_connfd = malloc(sizeof(int));

			*timeserver_connfd = Accept(timeserver_listenfd, (SA *)NULL, NULL);

			Pthread_create(&tid,NULL,time_service, timeserver_connfd);
		}

		if(FD_ISSET(echoserver_listenfd, &rset)){

			echoserver_connfd = malloc(sizeof(int));

			*echoserver_connfd = Accept(echoserver_listenfd, (SA *)NULL, NULL);

			Pthread_create(&tid,NULL,echo_service, echoserver_connfd);
		}
	}
	return 0;
}

void* time_service(void *arg){

	Pthread_detach(pthread_self());

	char write_buff[MAXLINE];

	int fd =  *(int *)arg;

	free(arg);

	int flags = fcntl(fd, F_GETFL, 0);

	/**
	 * Changing the read to non-blocking
	 */
	fcntl(fd, F_SETFL, flags | O_NONBLOCK);

	for( ; ;){

		memset(write_buff,0,sizeof(write_buff));

		time_t ticks = time(NULL);

		snprintf(write_buff, sizeof(write_buff), "%.24s\r\n" , ctime(&ticks));

		int n=0;
		/**
		 *  This is a non blocking read used for receiving
		 *  reset from the client in case when the client
		 *  crashed and manually killed.
		 */
		if((n=read(fd,NULL,MAXLINE)) == 0){
			return NULL;
		}else if(n < 0){
			/**
			  *  resetting the error number
			  *  to zero to avoid reading previous
			  *  errors incase of no errors.
			   */
			errno = 0;
		}

		Writen(fd,write_buff,strlen(write_buff));

		usleep(5000000);
	}

	/**
	 * Closing the socket in the thread as the file descriptors are shared
	 * between threads. hence Main thread need not close on the socket.
	 * Reference count is not incremented with threads.
	 */
	close(fd);
	return NULL;
}

void* echo_service(void* arg){

	size_t n;
	char read_buff[MAXLINE];

	int fd = *(int *)arg;

again:
	while((n=read(fd,read_buff,sizeof(read_buff))) > 0){
		Writen(fd,read_buff,strlen(read_buff));
		memset(read_buff,0,sizeof(read_buff));
	}

	if(n < 0 && errno == EINTR)
		goto again;
	else if(n < 0)
		printf("Read Error : %s\n",strerror(errno));

	close(fd);
	return NULL;
}
