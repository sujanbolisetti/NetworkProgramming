/*
 * client.c
 *
 *  Created on: Oct 7, 2015
 *      Author: sujan
 */
#include "usp.h"
#include  "unpifi.h"

#define	RTT_DEBUG

static struct rtt_info   rttinfo;
static int	rttinit = 0;
static sigjmp_buf jmpbuf;

int main(int argc,char **argv){

	FILE *fp;
	char IPServer[128];
	int portNumber,sockfd;
	char fileName[120];
	uint32_t windowSize;
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
	char buff[480];
	struct dg_payload pload;
	struct sigaction new_action, old_action;
	//struct msghdr msgsend;
	//struct iovec iovsend[2];
	uint32_t seqNum=0;
	char data_buff[40][480];


	fp=fopen("client.in","r");
	Fscanf(fp,"%s",IPServer);
	Fscanf(fp,"%d",&portNumber);
	Fscanf(fp,"%s",fileName);
	Fscanf(fp,"%d",&windowSize);

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
		printf("Server is not local to the client\n");
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

	if (rttinit == 0) {
		rtt_init(&rttinfo);		/* first time we're called */
		rttinit = 1;
		rtt_d_flag = 1;
	}

	memset(&pload,0,sizeof(pload));

	new_action.sa_handler = sig_alrm;
	sigemptyset(&new_action.sa_mask);
	new_action.sa_flags = 0;
	sigaction(SIGALRM,NULL,&old_action);

	if(old_action.sa_handler != SIG_IGN){
		printf("New Action Set\n");
		sigaction(SIGALRM,&new_action,&old_action);
	}

	strcpy(pload.buff,fileName);
	pload.type = PAYLOAD;
	pload.seq_number = seqNum++;
	pload.windowSize = windowSize;

	sendagain:
		pload.ts = rtt_ts(&rttinfo);
		sendto(sockfd,(void *)&pload,sizeof(pload),0,NULL,0);

		printf("wait time :%d\n",rtt_start(&rttinfo));

		alarm(rtt_start(&rttinfo)/1000);

		if (sigsetjmp(jmpbuf, 1) != 0) {
			printf("received sigalrm retransmitting\n");
			if (rtt_timeout(&rttinfo) < 0) {
				printf("Reached Maximum retransmission attempts : No response from the server giving up\n");
				rttinit = 0;	/* reinit in case we're called again */
				errno = ETIMEDOUT;
				return(-1);
			}
			goto sendagain;
		}


	printf("Client has sent the file Name.. Awaiting for the child port number from the server\n");

	Recvfrom(sockfd,&pload,sizeof(pload),0,NULL,NULL);

	alarm(0);

	int serverChildPortNumber = atoi(pload.buff);

	printf("New Emphemeral PortNumber of the Server Child %d\n",serverChildPortNumber);
	close(sockfd);

	struct sockaddr_in newServerAddr;
	newServerAddr.sin_port = htons(serverChildPortNumber);
	newServerAddr.sin_addr.s_addr = serverAddr.sin_addr.s_addr;
	newServerAddr.sin_family = AF_INET;

	sockfd = Socket(AF_INET,SOCK_DGRAM,0);

	Bind(sockfd,(SA *)&IPClient,sizeof(IPClient));

	if(connect(sockfd,(SA *)&newServerAddr,sizeof(newServerAddr)) < 0){
		printf("Connection Error :%s",strerror(errno));
	}

	uint32_t ts =  pload.ts;

	memset(&pload,0,sizeof(pload));
	pload.type = ACK;
	pload.ts = ts;

	pload.seq_number = seqNum++;

	sendto(sockfd,(void *)&pload,sizeof(pload),0,NULL,0);

	printf("Has sent the ack for the port number\n");

	int i=0,j=0,size=40;
	bool print= false;

	for(;;){

		memset(&pload,0,sizeof(pload));
		recvfrom(sockfd,&pload,sizeof(pload),0,NULL,NULL);

		if(strcmp(pload.buff,"DONE") == 0){
			printf("Done with the file transfer\n");
			print=true;
			goto stdout;
		}

		strcpy(data_buff[i%40],pload.buff);
		int ts = pload.ts;
		int seq = pload.seq_number;
		memset(&pload,0,sizeof(pload));
		pload.ts=ts;
		pload.ack = seq+1;
		pload.windowSize = windowSize - i;
		i++;
		sendto(sockfd,(void *)&pload,sizeof(pload),0,NULL,0);

		stdout:
			if(pload.windowSize == 0 || print){

				for(j=0;j<i;j++){

					printf("%s\n",data_buff[j]);
				}
				if(print)
					break;
			}
	}
}

void
sig_alrm(int signo){
	printf("received the sigalrm\n");
	siglongjmp(jmpbuf,1);
}

