/*
 * daytimecli.c
 *
 *  Created on: Sep 15, 2015
 *      Author: sujan
 */

#include "usp.h"

// TO DO : Have to understand how to use sys_err function to print the
// error msg.

int main(int argc, char **argv){

	int sockfd, n;
	struct sockaddr_in serveraddr;
	char recvbuff[MAXLINE];

	if((sockfd = socket(AF_INET,SOCK_STREAM,0)) < 0){
		printf("%s\n",strerror(errno));
		exit(-1);
	}

	bzero(&serveraddr,sizeof(serveraddr));

	serveraddr.sin_family = AF_INET;
	serveraddr.sin_port = htons(TIME_SERVE_PORT_NUMBER);

	if(inet_pton(AF_INET,argv[1],&serveraddr.sin_addr) <= 0){
		printf("%s\n",strerror(errno));
		exit(-1);
	}

	if(connect(sockfd,(struct sockaddr *)&serveraddr, sizeof(serveraddr)) < 0){
		printf("%s\n",strerror(errno));
		exit(-1);
	}

	while((n=read(sockfd,recvbuff,sizeof(recvbuff))) > 0){

		recvbuff[n] = 0;

		printf("%s\n",recvbuff);
	}

	if(n < 0)
		printf("Read Error");

	return 0;
}

