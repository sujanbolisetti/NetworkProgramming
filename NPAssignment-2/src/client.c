/*
 * client.c
 *
 *  Created on: Oct 7, 2015
 *      Author: sujan
 */
#include "usp.h"
#include  "unpifi.h"

int seq_num = 0;
int get_seq_num();

int main(int argc,char **argv){

	FILE *fp;
	char IPServer[128];
	int portNumber,sockfd;
	char fileName[120], buff[MAXLINE];
	struct sockaddr_in *clientAddr;
	struct ifi_info *if_head,*if_temp;
	char clientIP[128];
	struct binded_sock_info *head = NULL, *temp=NULL;
	long max = 0,tempMax=0;
	struct sockaddr_in serverAddr,IPClient;
	int length = sizeof(IPClient),optval=1;
	bool isLocal=false;
	unsigned long maxMatch=0;
	bool isLoopBack=false;

	if(argc < 2){
		printf("Kindly enter the input file name\n");
		exit(0);
	}

	fp=fopen(argv[1],"r");
	Fscanf(fp,"%s",IPServer);
	Fscanf(fp,"%d",&portNumber);
	Fscanf(fp,"%s",fileName);

	printf("server address %s\n",IPServer);

	inet_pton(AF_INET,IPServer,&serverAddr.sin_addr);
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(portNumber);
	IPClient.sin_family = AF_INET;
	IPClient.sin_port = 0;

	/**
	 *  Check whether the server is local/external.
	 */
	if(strcmp(IPServer,LOOPBACK_ADDRESS) == 0){
		inet_pton(AF_INET,IPServer,&IPClient.sin_addr);
		isLoopBack=true;
	}

	for(if_head = if_temp = Get_ifi_info(AF_INET,1);
						if_temp!=NULL;if_temp = if_temp->ifi_next){

		if(if_temp->ifi_flags & IFF_UP){

			clientAddr = (struct sockaddr_in *) if_temp->ifi_addr;

			struct binded_sock_info *bsock_info = (struct binded_sock_info *)malloc(sizeof(struct binded_sock_info));

			if(head == NULL){
				head = bsock_info;
				temp = head;
			}else{
				temp->next = bsock_info;
				temp =  bsock_info;
			}

			inet_ntop(AF_INET,&clientAddr->sin_addr,bsock_info->ip_address,sizeof(bsock_info->ip_address));

			struct sockaddr_in *netAddr = (struct sockaddr_in *)if_temp->ifi_ntmaddr;

			if(!isLoopBack){
				maxMatch = getClientIPAddress(clientAddr,netAddr,&serverAddr,&IPClient,maxMatch);
			}

			inet_ntop(AF_INET,&netAddr->sin_addr,bsock_info->network_mask,sizeof(bsock_info->network_mask));
		}

	}
	temp->next = NULL;
	temp = head;

	/**
	 *  We are printing the interface information to STDOUT.
	 */
	while(temp!=NULL){
		printf("IP Adress %s\n",temp->ip_address);
		printf("Network Mask %s\n",temp->network_mask);
		temp=temp->next;
	}


	/**
	 * Checking the server address is local
	 * else assigning a random address
	 */
	if(maxMatch == 0 && !isLoopBack){
		temp = head;
		while(temp!=NULL){
			if(strcmp(temp->ip_address,LOOPBACK_ADDRESS) != 0){
				inet_pton(AF_INET,temp->ip_address,&IPClient.sin_addr);
			}
			temp=temp->next;
		}
	}else{
		printf("server is in local\n");
		setsockopt(sockfd,SOL_SOCKET,MSG_DONTROUTE,&optval,sizeof(int));
	}

	sockfd = Socket(AF_INET,SOCK_DGRAM,0);

	Bind(sockfd,(SA *)&IPClient,sizeof(IPClient));

	Getsockname(sockfd,(SA *)&IPClient,&length);

	if(connect(sockfd,(SA *)&serverAddr,sizeof(serverAddr)) < 0){
		printf("Connection Error :%s",strerror(errno));
	}

	printf(" client port number %d\n",ntohs(IPClient.sin_port));

	struct sockaddr_in peerAddress;
	int peerAddrLength = sizeof(peerAddress);

	Getpeername(sockfd,(SA *)&peerAddress,&peerAddrLength);

	printf("server port number %d\n",peerAddress.sin_port);

	struct dg_payload pload;

	memset(&pload,0,sizeof(pload));

	strcpy(pload.buff,fileName);
	pload.type = PAYLOAD;
	pload.seq_number = get_seq_num();

	printf("Before Sending\n");
	sendto(sockfd,(void *)&pload,sizeof(pload),0,NULL,0);

	printf("After Sending\n");

	int conn_port;
	memset(&pload,0,sizeof(pload));
	recvfrom(sockfd,&pload,sizeof(pload),0,NULL,NULL);

	printf("Packet type:%d : new server port number %d\n", ntohs(pload.type), pload.portNumber);

	struct sockaddr_in newServerAddr;
	newServerAddr.sin_port = htons(pload.portNumber);
	newServerAddr.sin_addr.s_addr = serverAddr.sin_addr.s_addr;
	newServerAddr.sin_family = AF_INET;

	close(sockfd);

	sockfd = Socket(AF_INET,SOCK_DGRAM,0);
	Bind(sockfd,(SA *)&IPClient,sizeof(IPClient));

	Getsockname(sockfd,(SA *)&IPClient,&length);
	if(connect(sockfd,(SA *)&newServerAddr,sizeof(newServerAddr)) < 0){
		printf("Connection Error :%s",strerror(errno));
	}

	printf("sending ack that sockets set\n");
	memset(&pload,0,sizeof(pload));
	strcpy(pload.buff, "DONE");
	pload.type = ACK;
	pload.seq_number = get_seq_num();

	sendto(sockfd,(void *)&pload,sizeof(pload),0,NULL,0);

	printf("reading on port Number :%d\n",ntohs(IPClient.sin_port));
	int k = 0;
	while(k < 10) {
		memset(&pload,0,sizeof(pload));
		recvfrom(sockfd,&pload,sizeof(pload),0,NULL,NULL);
		printf("received number is %d\n",ntohs(pload.portNumber));
		k++;
	}
}

int get_seq_num()
{
	return seq_num++;
}
