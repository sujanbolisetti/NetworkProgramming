/*
 * server.c
 *
 *  Created on: Oct 4, 2015
 *      Author: sujan
 */

#include "usp.h"
#include  "unpifi.h"

int main(int argc, char **argv){

	FILE *fp;
	int portNumber,slidingWindowSize;
	int sockfd,maxfd=0,pid;
	struct ifi_info *if_head, *if_temp;
	struct sockaddr_in *servaddr;
	struct sockaddr_in IPClient;
	struct sockaddr *sa;
	int length=sizeof(IPClient);
	struct binded_sock_info *head=NULL, *temp=NULL;
	fd_set rset;

	fp=fopen("server.in","r");
	fscanf(fp,"%d",&portNumber);
	fscanf(fp,"%d",&slidingWindowSize);

	printf("%d %d\n",portNumber,slidingWindowSize);

	for(if_head = if_temp = Get_ifi_info(AF_INET,1);
				if_temp!=NULL;if_temp = if_temp->ifi_next){

			sockfd = Socket(AF_INET,SOCK_DGRAM,0);

			if(sockfd > maxfd){
				maxfd = sockfd;
			}
			servaddr = (struct sockaddr_in *) if_temp->ifi_addr;
			servaddr->sin_family = AF_INET;
			servaddr->sin_port = htons(portNumber);

			struct binded_sock_info *bsock_info = (struct binded_sock_info *)malloc(sizeof(struct binded_sock_info));

			if(head == NULL){

				head = bsock_info;
				temp = head;
			}else{

				temp->next = bsock_info;
				temp =  bsock_info;
			}

			bsock_info->sockfd = sockfd;

			inet_ntop(AF_INET,&servaddr->sin_addr,bsock_info->ip_address,sizeof(bsock_info->ip_address));

			struct sockaddr_in *netAddr = (struct sockaddr_in *)if_temp->ifi_ntmaddr;

			inet_ntop(AF_INET,&netAddr->sin_addr,bsock_info->network_mask,sizeof(bsock_info->network_mask));

			Bind(sockfd, (SA *)servaddr,sizeof(*servaddr));

			printf("presentation address %s\n",bsock_info->ip_address);

			printf("presentation address %s\n",bsock_info->ip_address);

	}

	temp->next = NULL;

	temp = head;

	while(temp!=NULL){

		printf("IP Adress %s\n",temp->ip_address);
		printf("Network Mask %s\n",temp->network_mask);
		temp=temp->next;
	}

	FD_ZERO(&rset);

	maxfd = maxfd+1;
	for(; ;){

		temp = head;

		while(temp!=NULL){

			FD_SET(temp->sockfd,&rset);
			temp = temp->next;
		}

		Select(maxfd,&rset,NULL,NULL,NULL);

		temp = head;
		fflush(stdout);

		while(temp!=NULL){

			if(FD_ISSET(temp->sockfd,&rset)){

				struct dg_payload pload;
				memset(&pload,0,sizeof(pload));
				recvfrom(temp->sockfd,&pload,sizeof(pload),0,(SA *)&IPClient,&length);
				printf("requested file name :%s\n",pload.buff);

				if((pid = fork()) == 0){

					printf("forked a child and handled client connection\n");
					doFileTransfer(temp,IPClient);

				}else{
					// have to use the PID for tracking
				}
			}
			temp = temp->next;
		}
	}
}

void doFileTransfer(struct binded_sock_info *sock_info,struct sockaddr_in IPClient){

	char buff[MAXLINE];
	bool isLocal=false;
	int conn_sockfd = Socket(AF_INET,SOCK_DGRAM,0);

	struct sockaddr_in serverAddr;
	int length = sizeof(serverAddr);

	inet_pton(AF_INET,sock_info->ip_address,&serverAddr);

	printf("client port number %d\n",ntohs(IPClient.sin_port));
	serverAddr.sin_port = 0;
	serverAddr.sin_family = AF_INET;

	Bind(conn_sockfd,(SA *)&serverAddr,sizeof(serverAddr));

	Getsockname(conn_sockfd,(SA *)&serverAddr,&length);

	printf("conn_fd port number :%d\n",ntohs(serverAddr.sin_port));

	int optval=1;

	struct in_addr networkMaskAddr;

	inet_pton(AF_INET,sock_info->network_mask,&networkMaskAddr);

	if(networkMaskAddr.s_addr & IPClient.sin_addr.s_addr){
		isLocal = true;
	}

	if(isLocal){
		setsockopt(conn_sockfd,SOL_SOCKET,MSG_DONTROUTE,&optval,sizeof(int));
	}

	struct dg_payload pload;

	memset(&pload,0,sizeof(pload));
	 //= (struct dg_payload)malloc(sizeof(struct dg_payload));
	pload.portNumber = htons(serverAddr.sin_port);

	sendto(sock_info->sockfd,(void *)&pload,sizeof(pload),0,(SA *)&IPClient,sizeof(IPClient));



	if(connect(conn_sockfd,(SA *)&IPClient,sizeof(IPClient)) < 0){
		printf("Connection Error :%s",strerror(errno));
	}

	int k = 0;
	while(k < 10){
		memset(&pload,0,sizeof(pload));
		pload.portNumber = htons(k);
		printf("writing in the socket\n");
		sendto(conn_sockfd,(void *)&pload,sizeof(pload),0,NULL,0);
		k++;
	}

}
