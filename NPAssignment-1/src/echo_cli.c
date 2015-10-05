/*
 * echoclient.c
 *			Echo Client Program.
 *
 *  Created on: Sep 18, 2015
 *      Author: sujan
 */

#include "usp.h"

int main(int argc, char **argv){

	int sockfd,maxfdp1,n=0;
	struct sockaddr_in serveraddr;
	char recvbuff[MAXLINE];
	char sendbuff[MAXLINE];
	char errbuff[MAXLINE];
	fd_set rset;
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
	serveraddr.sin_port = htons(ECHO_SERVER_PORT_NUMBER);

	if(inet_pton(AF_INET,argv[1],&serveraddr.sin_addr) < 0){
		strcpy(errbuff,create_error_message(PTONERROR,errno));
		/**
		 * Writing the error message to the client parent program.
		 */
		if(write(ppfd,errbuff,sizeof(errbuff)) < 0){
			printf("Write Error :%s\n",strerror(errno));
		}
		exit(-1);
	}

	if((connect(sockfd,(SA *)&serveraddr,sizeof(serveraddr))) < 0 ){
		strcpy(errbuff,create_error_message(CONNECT_ERROR,errno));
		if(write(ppfd,errbuff,sizeof(errbuff)) < 0){
			printf("Write Error :%s\n",strerror(errno));
		}
		exit(-1);
	}

	FD_ZERO(&rset);

	printf("Echo Service\n");

	for(; ;){

		memset(sendbuff,0,MAXLINE);
		memset(recvbuff,0,MAXLINE);

		FD_SET(fileno(stdin),&rset);
		FD_SET(sockfd,&rset);

		maxfdp1 = max(fileno(stdin),sockfd) +1;

		/**
		 * Using select as we are monitoring stdin and
		 * socket connected to the server.
		 */
		Select(maxfdp1, &rset, NULL, NULL, NULL);

		if(FD_ISSET(fileno(stdin),&rset)){

			if(fgets(sendbuff,MAXLINE,stdin) != NULL){
				Writen(sockfd,sendbuff,MAXLINE);
			}else{
				if(!feof(stdin) && ferror(stdin)){
					strcpy(errbuff,"stdin error");
					if(write(ppfd,errbuff,sizeof(errbuff)) < 0){
						printf("Write Error :%s\n",strerror(errno));
					}
				}
				break;
			}
		}

		if(FD_ISSET(sockfd,&rset)){

			if((n=read(sockfd,recvbuff,MAXLINE)) > 0){
				printf("%s\n",recvbuff);
			}else if(n < 0){
				strcpy(errbuff,"Read Error");
				if(write(ppfd,errbuff,sizeof(errbuff)) < 0){
					printf("Write Error :%s\n",strerror(errno));
				}
			}else{
				strcpy(errbuff,"Server Terminated");
				if(write(ppfd,errbuff,sizeof(errbuff)) < 0){
					printf("Write Error :%s\n",strerror(errno));
				}
				break;
			}
		}

	}

	close(sockfd);
	close(ppfd);
	return 0;
}
