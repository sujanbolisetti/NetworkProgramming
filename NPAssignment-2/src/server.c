/*
 * server.c
 *
 *  Created on: Oct 4, 2015
 *      Author: sujan
 */

#include "usp.h"
#include  "unpifi.h"

int seq_num = 0;
int get_seq_num();

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
	struct connected_client_address *head_client_address=NULL, *temp_client_address=NULL;
	struct dg_payload pload;
	fd_set rset;

	int seq_num = 0;
	char ipAddressSocket[256];


	if(argc < 2){
		printf("Kindly enter the input file name\n");
		exit(0);
	}

	fp=fopen(argv[1],"r");

	Fscanf(fp,"%d",&portNumber);
	Fscanf(fp,"%d",&slidingWindowSize);

	printf("%d %d\n",portNumber,slidingWindowSize);

	for(if_head = if_temp = Get_ifi_info(AF_INET,1);
				if_temp!=NULL;if_temp = if_temp->ifi_next){

		if(if_temp->ifi_flags & IFF_UP){


			sockfd = Socket(AF_INET,SOCK_DGRAM,0);

			if(sockfd > maxfd){
				printf("sock fd %d\n",sockfd);
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

		}
	}

	temp->next = NULL;

	temp = head;

	while(temp!=NULL){

		printf("IP Address %s\n",temp->ip_address);
		printf("Network Mask %s\n",temp->network_mask);
		temp=temp->next;
	}

	FD_ZERO(&rset);


	maxfd = maxfd+1;
	printf("max fd %d\n",maxfd);

	for(; ;){

		temp = head;

		while(temp!=NULL){

			FD_SET(temp->sockfd,&rset);
			temp = temp->next;
		}

		Select(maxfd,&rset,NULL,NULL,NULL);
		printf("Came out of select\n");

		temp = head;

		while(temp!=NULL){

			if(FD_ISSET(temp->sockfd,&rset)){

				memset(&pload,0,sizeof(pload));
				recvfrom(temp->sockfd,&pload,sizeof(pload),0,(SA *)&IPClient,&length);

				inet_ntop(AF_INET,&IPClient.sin_addr,ipAddressSocket,128);

				char portNumber[24];
				sprintf(portNumber,"%d",ntohs(IPClient.sin_port));

				strcat(ipAddressSocket,":");
				strcat(ipAddressSocket,portNumber);

				printf("socket address :%s\n",ipAddressSocket);

				bool addressExists =false;

				if(head_client_address!=NULL){

					struct connected_client_address *temp2 = head_client_address;

					while(temp2!=NULL){
						if(strcmp(temp2->client_sockaddress,ipAddressSocket) == 0){
							addressExists = true;
						}
						temp2 = temp2->next;
					}

				}

				if(addressExists){
					printf("Continuing..\n");
					continue;
				}

				struct connected_client_address *client_sock_address = (struct connected_client_address*)malloc(sizeof(struct connected_client_address));

				strcpy(client_sock_address->client_sockaddress,ipAddressSocket);

				if(head_client_address == NULL){
					head_client_address = client_sock_address;
					temp_client_address = head_client_address;
				}else{
					temp_client_address->next=client_sock_address;
					temp_client_address = client_sock_address;
					temp_client_address->next = NULL;
				}

				if((pid = fork()) == 0){

					printf("forked a child and handled client connection\n");
					doFileTransfer(temp,IPClient);

				}else if(pid > 0){
					// have to use the PID for tracking
					sprintf(temp_client_address->child_pid,"%d",pid);
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

	inet_pton(AF_INET,sock_info->ip_address,&serverAddr.sin_addr);

	printf("client port number %d\n",ntohs(IPClient.sin_port));
	serverAddr.sin_port = 0;
	serverAddr.sin_family = AF_INET;

	Bind(conn_sockfd,(SA *)&serverAddr,sizeof(serverAddr));

	Getsockname(conn_sockfd,(SA *)&serverAddr,&length);

	printf("conn_fd port number :%d\n", ntohs(serverAddr.sin_port));

	int optval=1;

	struct sockaddr_in networkMaskAddr;

	inet_pton(AF_INET,sock_info->network_mask,&networkMaskAddr.sin_addr);

	if(getClientIPAddress(&IPClient,&networkMaskAddr,&serverAddr,NULL,0) > 0){
		printf("Client is Local\n");
		setsockopt(conn_sockfd,SOL_SOCKET,MSG_DONTROUTE,&optval,sizeof(int));
	}

	struct dg_payload pload;

	memset(&pload,0,sizeof(pload));
	 //= (struct dg_payload)malloc(sizeof(struct dg_payload));
	pload.portNumber = htons(serverAddr.sin_port);
	pload.seq_number = get_seq_num();
	pload.type = PAYLOAD;

	sendto(sock_info->sockfd,(void *)&pload,sizeof(pload),0,(SA *)&IPClient,sizeof(IPClient));

	if(connect(conn_sockfd,(SA *)&IPClient,sizeof(IPClient)) < 0){
		printf("Connection Error :%s",strerror(errno));
	}

	memset(&pload,0,sizeof(pload));
	recvfrom(conn_sockfd, &pload, sizeof(pload), 0, NULL, NULL);

	if(pload.type ==  ACK)
	{
		printf("Sending data...\n");
		int k = 0;
		while(k < 10){
			memset(&pload,0,sizeof(pload));
			pload.portNumber = htons(k);
			pload.seq_number = get_seq_num();
			pload.type = PAYLOAD;
			printf("writing in the socket : %d\n", htons(pload.portNumber));
			sendto(conn_sockfd, &pload, sizeof(pload), 0, NULL, 0);
			k++;
		}
	}
}

int get_seq_num()
{
	seq_num++;
	return seq_num;
}

