/*
 * server.c
 *
 *  Created on: Oct 4, 2015
 *      Author: sujan
 */

#include "usp.h"
#include  "unpifi.h"

#define	RTT_DEBUG

struct connected_client_address *head_client_address=NULL;

static struct rtt_info   rttinfo;
static int	rttinit = 0;
static sigjmp_buf jmpbuf;
static int jmpParameter=0;

int main(int argc, char **argv){

	FILE *fp;
	int portNumber,slidingWindowSize;
	int sockfd,maxfd=0,pid;
	struct sockaddr_in *servaddr;
	struct sockaddr_in IPClient;
	struct sockaddr *sa;
	struct binded_sock_info *head=NULL, *temp=NULL;
	struct connected_client_address *temp_client_address=NULL;
	struct in_addr subnet_addr;
	struct dg_payload pload;
	int length=sizeof(IPClient);
	fd_set rset;
	int seq_num = 0;
	char* ipAddressSocket;
	struct sigaction new_action, old_action;
	struct flow_metadata *flow_data;
	char *file_name;

	/**
	 *  Reading the Input arguments from the input file.
	 */
	fp=fopen("server.in","r");
	Fscanf(fp,"%d",&portNumber);
	Fscanf(fp,"%d",&slidingWindowSize);

	printf("Server Well Known Port Number:%d\n",portNumber);

	/**
	 * Gettting the interface details.
	 */
	head = getInterfaces(portNumber,NULL,NULL,NULL);

	printf("Server Interfaces and their details\n");
	printInterfaceDetails(head);

	/**
	 * Registering signal handler for the SIGCHLD signal.
	 */
	new_action.sa_handler = sigchild_handler;
	sigemptyset(&new_action.sa_mask);
	new_action.sa_flags = 0;
	sigaction(SIGCHLD,NULL,&old_action);

	if(old_action.sa_handler != SIG_IGN){
		printf("New Action Set\n");
		sigaction(SIGCHLD,&new_action,&old_action);
	}

	FD_ZERO(&rset);

	/**
	 *  Maxfd to monitor on select.
	 */
	maxfd = getMaxFD(head)+1;

	for(; ;){

		temp = head;

		while(temp!=NULL){

			FD_SET(temp->sockfd,&rset);
			temp = temp->next;
		}

		Select(maxfd,&rset,NULL,NULL,NULL);
		temp = head;

		while(temp!=NULL){

			if(FD_ISSET(temp->sockfd,&rset)){

				memset(&pload,0,sizeof(pload));
				recvfrom(temp->sockfd,&pload,sizeof(pload),0,(SA *)&IPClient,&length);

				ipAddressSocket = getSocketAddress(IPClient);

				/*
				 *  If the client is already connected then this is datagram
				 *  is a part of retransmission hence we will ignore this
				 *  connection request.
				 */
				if(isClientConnected(head_client_address,ipAddressSocket)){
						printf("Client Already connected hence ignoring\n");
						break;
				}

				if((pid = fork()) == 0){
					flow_data = (struct flow_metadata*)malloc(sizeof(struct flow_metadata));
					flow_data->slidingWindow = pload.windowSize;
					printf("sliding window size %d\n",flow_data->slidingWindow);
					file_name = (char *)malloc(1024);
					strcpy(file_name,pload.buff);
					printf("forked a child and handled client connection and file_name: %s\n",file_name);
					doFileTransfer(temp,IPClient,flow_data,file_name);

				}else if(pid > 0){

					struct connected_client_address *client_sock_address = (struct connected_client_address*)malloc(sizeof(struct connected_client_address));

					strcpy(client_sock_address->client_sockaddress,ipAddressSocket);
					// have to use the PID for removing the entry for this list.
					sprintf(client_sock_address->child_pid,"%d",pid);

					printf("childPID %d\n",pid);

					if(head_client_address == NULL){
						head_client_address = client_sock_address;
						temp_client_address = head_client_address;
					}else{
						temp_client_address->next=client_sock_address;
						temp_client_address = client_sock_address;
					}
					temp_client_address->next = NULL;
				}
			}
			temp = temp->next;
		}
	}
}

void doFileTransfer(struct binded_sock_info *sock_info,struct sockaddr_in IPClient,
		struct flow_metadata *flow_data, char *file_name){

	char buff[MAXLINE];
	int seqNum =0;
	bool isLocal=false;
	int conn_sockfd = Socket(AF_INET,SOCK_DGRAM,0);
	char IPServer[128];
	struct sockaddr_in serverAddr;
	int length = sizeof(serverAddr);
	struct sockaddr_in networkMaskAddr;
	int optval=1;
	int portNumberRetransmit=0;
	unsigned long matchNumber=0;
	struct dg_payload pload;
	struct sigaction new_action, old_action;
	uint32_t q_sent = 0,q_acked = 0,q_rear = 0,window_size,cwnd,ssthresh;
	struct Node *headNode,*tempNode;
	FILE *fp;

	if (rttinit == 0) {
		rtt_init(&rttinfo);		/* first time we're called */
		rttinit = 1;
		rtt_d_flag = 1;
		flow_data->cwnd = 1;
		flow_data->slidingWindowEnd = flow_data->slidingWindow;
		flow_data->slidingWindowStart = 0;
	}

	window_size = flow_data->slidingWindow;
	cwnd = flow_data->cwnd;
	ssthresh = 80;

	inet_pton(AF_INET,sock_info->ip_address,&serverAddr.sin_addr);

	serverAddr.sin_port = 0;
	serverAddr.sin_family = AF_INET;
	Bind(conn_sockfd,(SA *)&serverAddr,sizeof(serverAddr));

	inet_pton(AF_INET,sock_info->network_mask,&networkMaskAddr.sin_addr);

	getClientIPAddress(&IPClient,&networkMaskAddr,&serverAddr,NULL,&matchNumber);

	if(matchNumber > 0){
		printf("Client Host is Local\n");
		setsockopt(conn_sockfd,SOL_SOCKET,MSG_DONTROUTE,&optval,sizeof(int));
	}else{
		printf("Client is not local to the server\n");
	}

	Getsockname(conn_sockfd,(SA *)&serverAddr,&length);
	memset(&pload,0,sizeof(pload));

	new_action.sa_handler = sig_alrm;
	sigemptyset(&new_action.sa_mask);
	new_action.sa_flags = 0;
	sigaction(SIGALRM,NULL,&old_action);

	if(old_action.sa_handler != SIG_IGN){
		printf("New Action Sigalrm Set\n");
		sigaction(SIGALRM,&new_action,&old_action);
	}

	sprintf(pload.buff,"%d",htons(serverAddr.sin_port));

	pload.seq_number = seqNum++;
	pload.type = PAYLOAD;
	pload.ts = rtt_ts(&rttinfo);

	if(connect(conn_sockfd,(SA *)&IPClient,sizeof(IPClient)) < 0){
		printf("Connection Error :%s",strerror(errno));
	}

	jmpParameter = 1;

	/**
	 *   Backing up th port number
	 *
	 */
	sendagain:
			pload.ts = rtt_ts(&rttinfo);
			sendto(sock_info->sockfd,(void *)&pload,sizeof(pload),0,(SA *)&IPClient,sizeof(IPClient));

			if(rttinfo.rtt_nrexmt >=1){
				printf("Server time out sending through connection socket also\n");
				sendto(conn_sockfd,(void *)&pload,sizeof(pload),0,NULL,0);
			}

			printf("wait time :%d\n",rtt_start(&rttinfo));

			alarm(rtt_start(&rttinfo)/1000);


			if (sigsetjmp(jmpbuf, jmpParameter) != 0) {
				printf("received sigalrm retransmitting\n");
				if (rtt_timeout(&rttinfo) < 0) {
					printf("Reached Maximum retransmission attempts : No response from the server giving up\n");
					rttinit = 0;	/* reinit in case we're called again */
					errno = ETIMEDOUT;
					return;
				}
				goto sendagain;
			}

	inet_ntop(AF_INET,&serverAddr.sin_addr,IPServer,sizeof(IPServer));

	printf("Server Child is running on IP-Address :%s with port number :%d\n",IPServer,ntohs(serverAddr.sin_port));

	memset(&pload,0,sizeof(pload));
	Recvfrom(conn_sockfd, &pload, sizeof(pload), 0, NULL, NULL);
	alarm(0);

	//rtt_stop(&rttinfo, rtt_ts(&rttinfo) - pload.ts);

	printf("Sending data...\n");
	int k = 0;
	jmpParameter=0;

	headNode = BuildCircularLinkedList(headNode,flow_data->slidingWindow);

	int fd = open(file_name,O_RDONLY);

	populateDataList(headNode,fd);

	tempNode = headNode;

	struct Node *temp2 = headNode;

	for(;;){

		int size,i;

		send:
			size = min(window_size,cwnd);

			alarm(rtt_start(&rttinfo)/1000);

			for(i=0;i<size;i++){

				memset(&pload,0,sizeof(pload));

			 loadagain:
					if(tempNode!=NULL){

						if(tempNode->ack == -1 || tempNode->ack > 0){
							// have to close as the file data is completed
							printf("file transfer completed\n");
							strcpy(pload.buff,"DONE");
							sendto(conn_sockfd,(void *)&pload,sizeof(pload),0,NULL,0);
							goto receive;
						}

						pload.seq_number = seqNum++;
						pload.ts=rtt_ts(&rttinfo);
						strcpy(pload.buff,tempNode->buff);
						tempNode->seqNum = pload.seq_number;
					}else{
						populateDataList(headNode,fd);
						tempNode = headNode;
						goto loadagain;
					}

				senddatagain:
					pload.ts = rtt_ts(&rttinfo);
					sendto(conn_sockfd,(void *)&pload,sizeof(pload),0,NULL,0);

					printf("Sending the data");

					printf("wait time in data :%d\n",rtt_start(&rttinfo));

					if (sigsetjmp(jmpbuf, 1) != 0) {
						printf("received sigalrm retransmitting\n");
						if (rtt_timeout(&rttinfo) < 0) {
							printf("Reached Maximum retransmission attempts : No response from the server giving up\n");
							rttinit = 0;	/* reinit in case we're called again */
							errno = ETIMEDOUT;
							return;
						}
						goto senddatagain;
					}
				}

			receive:
				Recvfrom(conn_sockfd, &pload, sizeof(pload), 0, NULL, NULL);
				alarm(0);
				rtt_stop(&rttinfo, rtt_ts(&rttinfo) - pload.ts);
				if(cwnd < ssthresh){
					cwnd*=2;
				}else{
					cwnd+=1;
				}

				if(tempNode->ack == -1 || tempNode->ack > 0){
					goto done;
				}

				if(temp2->seqNum == pload.ack-1){
					temp2->ack = temp2->seqNum;
				}


				if(pload.windowSize > 0){
					window_size = pload.windowSize;
					goto send;
				}else{
					goto receive;
				}
	}
	done:
		close(conn_sockfd);
		exit(0);
}

/**
 * Handler for SIGCHLD signal.
 */
void sigchild_handler(int signum){

	int stat;
	pid_t pid;

	while((pid = waitpid(-1,&stat,WNOHANG)) > 0){
		printf("Child Terminated with PID :%d\n",pid);
		removeClientAddrFromList(pid,&head_client_address);
	}
}

void
sig_alrm(int signo){
	printf("received the sigalrm\n");
	siglongjmp(jmpbuf,1);
}

