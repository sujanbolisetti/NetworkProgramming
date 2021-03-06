/*
 * daytimecli.c
 *			Time Client Program.
 *
 *  Created on: Sep 15, 2015
 *      Author: sujan
 */

#include "usp.h"

int main(int argc, char **argv){


	int sockfd, n;
	struct sockaddr_in serveraddr;
	char recvbuff[MAXLINE];
	char errbuff[MAXLINE];
	/**
	 *  pipe fd to communicate with the parent.
	 */
	int ppfd = atoi(argv[2]);

	if((sockfd = socket(AF_INET,SOCK_STREAM,0)) < 0){
		strcpy(errbuff,create_error_message(SOCKET_CREATION_ERROR,errno));
		if(write(ppfd,errbuff,sizeof(errbuff)) < 0){
			printf("Write Error :%s\n",strerror(errno));
		}
		exit(-1);
	}

	bzero(&serveraddr,sizeof(serveraddr));

	serveraddr.sin_family = AF_INET;
	serveraddr.sin_port = htons(TIME_SERVER_PORT_NUMBER);

	if(inet_pton(AF_INET,argv[1],&serveraddr.sin_addr) <= 0){
		strcpy(errbuff,create_error_message(PTONERROR,errno));
		if(write(ppfd,errbuff,sizeof(errbuff)) < 0){
			printf("Write Error :%s\n",strerror(errno));
		}
		exit(-1);
	}

	if(connect(sockfd,(SA *)&serveraddr, sizeof(serveraddr)) < 0){
		strcpy(errbuff,create_error_message(CONNECT_ERROR,errno));
		if(write(ppfd,errbuff,sizeof(errbuff)) < 0){
			printf("Write Error :%s\n",strerror(errno));
		}
		exit(-1);
	}

	printf("Time Service\n");
	while((n=read(sockfd,recvbuff,sizeof(recvbuff))) > 0){
		recvbuff[n] = 0;
		printf("%s\n",recvbuff);
		memset(recvbuff,0,MAXLINE);
	}

	if(n < 0){
		strcpy(errbuff,"Read Error");
		if(write(ppfd,errbuff,sizeof(errbuff)) < 0){
			printf("Write Error :%s\n",strerror(errno));
		}
	}

	/**
	 *  On receiving a reset we conclude that the server was terminated.
	 */
	if(n==0){
		strcpy(errbuff,"Server Terminated");
		if(write(ppfd,errbuff,sizeof(errbuff)) < 0){
			printf("Write Error :%s\n",strerror(errno));
		}
	}

	close(sockfd);
	close(ppfd);
	return 0;
}

