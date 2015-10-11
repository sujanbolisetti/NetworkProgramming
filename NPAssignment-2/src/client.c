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
	struct in_addr subnet_addr;
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

	printf("Connecting to Server with IP-Address %s\n",IPServer);

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

	if(!isLoopBack){
		head = getInterfaces(portNumber,&maxMatch,&serverAddr,&IPClient);
	}else{
		head = getInterfaces(portNumber,&maxMatch,NULL,NULL);
	}

	/**
	 *  We are printing the interface information to STDOUT.
	 */
	printf("Client Interfaces and their details\n");
	printInterfaceDetails(head);

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

	inet_ntop(AF_INET,&IPClient.sin_addr,clientIP,sizeof(clientIP));

	printf("Client is running on IP-Adress :%s with Port Number :%d\n",clientIP,ntohs(IPClient.sin_port));

	if(connect(sockfd,(SA *)&serverAddr,sizeof(serverAddr)) < 0){
		printf("Connection Error :%s",strerror(errno));
	}

	struct sockaddr_in peerAddress;

	int peerAddrLength = sizeof(peerAddress);

	Getpeername(sockfd,(SA *)&peerAddress,&peerAddrLength);

	inet_ntop(AF_INET,&peerAddress.sin_addr,IPServer,sizeof(IPServer));

	printf("Connecting to server running on IPAddress :%s with portNumber :%d\n",IPServer,ntohs(peerAddress.sin_port));

	struct dg_payload pload;

	memset(&pload,0,sizeof(pload));

	strcpy(pload.buff,fileName);
	pload.type = PAYLOAD;
	pload.seq_number = get_seq_num();
	sendto(sockfd,(void *)&pload,sizeof(pload),0,NULL,0);

	printf("Client has sent the file Name\n");

	memset(&pload,0,sizeof(pload));
	recvfrom(sockfd,&pload,sizeof(pload),0,NULL,NULL);

	printf("New Emphemeral PortNumber of the Server %d\n",pload.portNumber);

	close(sockfd);

	struct sockaddr_in newServerAddr;
	newServerAddr.sin_port = htons(pload.portNumber);
	newServerAddr.sin_addr.s_addr = serverAddr.sin_addr.s_addr;
	newServerAddr.sin_family = AF_INET;

	sockfd = Socket(AF_INET,SOCK_DGRAM,0);

	Bind(sockfd,(SA *)&IPClient,sizeof(IPClient));

	if(connect(sockfd,(SA *)&newServerAddr,sizeof(newServerAddr)) < 0){
		printf("Connection Error :%s",strerror(errno));
	}

	printf("sending ack that sockets set\n");
	memset(&pload,0,sizeof(pload));
	strcpy(pload.buff, "DONE");
	pload.type = ACK;
	pload.seq_number = get_seq_num();
	sendto(sockfd,(void *)&pload,sizeof(pload),0,NULL,0);

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
