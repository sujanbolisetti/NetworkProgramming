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
static bool isMetaDataTransfer=true;


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

	int seqNum =0;
	bool isLocal=false;
	int conn_sockfd = Socket(AF_INET,SOCK_DGRAM,0);
	char IPServer[128];
	struct sockaddr_in serverAddr;
	int length = sizeof(serverAddr);
	struct sockaddr_in networkMaskAddr;
	int optval=1,state=SLOW_START,fin_flag = 0;
	int portNumberRetransmit=0;
	unsigned long matchNumber=0;
	struct dg_payload pload;
	struct sigaction new_action, old_action;
	uint32_t q_sent = 0,q_acked = 0,q_rear = 0,window_size,cwnd,ssthresh;
	struct Node *headNode=NULL,*sentNode=NULL,*ackNode = NULL;;
	FILE *fp;
	bool queueFull=false;
	bool isRetransmit=false;
	sigset_t sigset_alrm;
	bool isWindowSizeZero = false;
	uint32_t persistent_timeout = 1;

	if (rttinit == 0) {
		rtt_init(&rttinfo);		/* first time we're called */
		rttinit = 1;
		rtt_d_flag = 1;
	}

	/**
	 * Initializing variables
	 */
	window_size = flow_data->slidingWindow;
	cwnd = 1;
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

	sigemptyset(&sigset_alrm);
	sigaddset(&sigset_alrm,SIGALRM);

	/**
	 *  Setting the sigalarm
	 */
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

	/**
	 *   Backing up the port number
	 */
	sendagain:
		pload.ts = rtt_ts(&rttinfo);
		sendto(sock_info->sockfd,(void *)&pload,sizeof(pload),0,(SA *)&IPClient,sizeof(IPClient));

		if(rttinfo.rtt_nrexmt >=1){
			printf("Server time out sending through connection socket also\n");
			sendto(conn_sockfd,(void *)&pload,sizeof(pload),0,NULL,0);
		}
		alarm(rtt_start(&rttinfo)/1000);

		if (sigsetjmp(jmpbuf, 1) != 0) {
			printf("received sigalrm retransmitting\n");
			if (rtt_timeout(&rttinfo) < 0) {
				printf("Reached Maximum retransmission attempts : No response from the server giving up\n");
				rttinit = 0;	/* reinit in case we're called again */
				errno = ETIMEDOUT;
				return;
			}
			if(isMetaDataTransfer){
				printf("Meta data retransmit-1\n");
				goto sendagain;
			}else{
				printf("FileData Retransfer-1\n");
				isRetransmit=true;
				goto senddatagain;
			}
		}

	inet_ntop(AF_INET,&serverAddr.sin_addr,IPServer,sizeof(IPServer));

	printf("Server Child is running on IP-Address :%s with port number :%d\n",IPServer,ntohs(serverAddr.sin_port));

	memset(&pload,0,sizeof(pload));
	Recvfrom(conn_sockfd, &pload, sizeof(pload), 0, NULL, NULL);
	printf("Received Ack for port Number packet\n");
	alarm(0);

	printf("Sending File data...\n");
	isMetaDataTransfer = false;
	int k = 0;

	/**
	 *   Building the sliding window list.
	 */
	headNode = BuildCircularLinkedList(flow_data->slidingWindow);

	int fd = open(file_name,O_RDONLY);

	/**
	 *  Ackindex -> 1
	 *  Initial CWND -> 1
	 */
	sentNode = headNode;
	ackNode = headNode;
	populateDataList(sentNode,fd,1,ackNode,headNode);

	int size,i,duplicateAckCount=0,lastAckReceived=0;

	for(;;){

		send:
			size = min(window_size,cwnd);

			if(ackNode != NULL && !fin_flag){
				queueFull = populateDataList(sentNode,fd,size,ackNode,headNode);
			}

			alarm(rtt_start(&rttinfo)/1000);

			printf("RTO : %d\n",rtt_start(&rttinfo)/1000);

			/**
			 *  We will not sent any packets for three conditions:
			 *  1. Window Size sent by the receiver is zero.
			 *  2. Received a FIN/FIN_ACK from receiver.
			 *  3. The queue is full.
			 */
			for(i=0;i<size && !fin_flag && !queueFull;i++){

				/*
				memset(&pload,0,sizeof(pload));
				pload.seq_number = seqNum++;

				if(sentNode->type == FIN){
					printf("file transfer completed and waiting for ACKs.......\n");
			     	pload.type = FIN;
			     	fin_flag = 1;
				 }else{
					 strcpy(pload.buff,sentNode->buff);
				  }
				sentNode->seqNum = pload.seq_number;
				*/

				senddatagain:

				 	 if (sigsetjmp(jmpbuf, 1) != 0) {
						printf("received sigalrm retransmitting\n");
						if (rtt_timeout(&rttinfo) < 0) {
							printf("Reached Maximum retransmission attempts : No response from the server giving up\n");
							rttinit = 0;	/* reinit in case we're called again */
							errno = ETIMEDOUT;
							return;
						}
						if(isMetaDataTransfer){
							printf("Meta data retransmit\n");
							goto sendagain;
						}else if(window_size > 0){
							printf("FileData Retransfer-2\n");
							congestion_control(TIME_OUT,&cwnd,&ssthresh,&duplicateAckCount,&state);
							isRetransmit=true;
							goto senddatagain;
						}else if(window_size == 0){
							printf("Window Size equals to zero\n");
							Send_Packet(conn_sockfd,INT_MAX,NULL,ackNode->type,rtt_ts(&rttinfo));
							goto receive;
						}
					}

					if(isRetransmit){
						alarm(rtt_start(&rttinfo)/1000);
						//memset(&pload,0,sizeof(pload));
						//sentNode->
						Send_Packet(conn_sockfd,ackNode->seqNum,ackNode->buff,ackNode->type,rtt_ts(&rttinfo));
						//pload.seq_number = ackNode->seqNum;
						printf("Ack Data in retransmit : %d %d\n",ackNode->seqNum,ackNode->ind);
						//strcpy(pload.buff,ackNode->buff);
						//pload.type = PAYLOAD;
					}else{
						sentNode->seqNum = Send_Packet(conn_sockfd,seqNum++,sentNode->buff,sentNode->type,rtt_ts(&rttinfo));
						if(sentNode->type == FIN){
							fin_flag = 1;
						}
						printf("Sent a packet with seqNumber :%d\n",sentNode->seqNum);
						sentNode = sentNode->next;
					}

					//pload.ts = rtt_ts(&rttinfo);
					//sendto(conn_sockfd,(void *)&pload,sizeof(pload),0,NULL,0);

					if(isRetransmit){
						isRetransmit = false;
						break;
					}

				}

			receive:
				printf("waiting for ack\n");
				memset(&pload,0,sizeof(pload));
				sigprocmask(SIG_UNBLOCK,&sigset_alrm,NULL);
				Recvfrom(conn_sockfd, &pload, sizeof(pload), 0, NULL, NULL);
				sigprocmask(SIG_BLOCK,&sigset_alrm,NULL);
				printf("received the ack with %d %d\n",pload.ack,pload.windowSize);
				rtt_stop(&rttinfo, rtt_ts(&rttinfo) - pload.ts);

				if(pload.type == WINDOW_PROBE){
					goto window_size_update;
				}

				if(!lastAckReceived || (lastAckReceived!=pload.ack)){
					congestion_control(NEW_ACK,&cwnd,&ssthresh,&duplicateAckCount,&state);
					printf("last received ack :%d\n",pload.ack);
					lastAckReceived = pload.ack;
				}else if(lastAckReceived==pload.ack){
					printf("Entered last received ack loop\n");
					if(duplicateAckCount == MAX_DUPLICATE_ACK_COUNT){
						printf("entered duplicate ack loop\n");
						congestion_control(DUPLICATE_3_ACK,&cwnd,&ssthresh,&duplicateAckCount,&state);
						if(window_size > 0){
							isRetransmit=true;
							goto senddatagain;
						}else{
							goto receive;
						}
					}else{
						congestion_control(DUPLICATE_ACK,&cwnd,&ssthresh,&duplicateAckCount,&state);
					}
				}


				if(pload.type==FIN_ACK){
					//TODO: have to send ack and wait TIME_WAIT
					printf("Received FIN_ACK from client\n");
//					memset(&pload,0,sizeof(pload));
//					pload.type = ACK;
//					sendto(conn_sockfd,(void *)&pload,sizeof(pload),0,NULL,0);
					Send_Packet(conn_sockfd,seqNum++,NULL,ACK,rtt_ts(&rttinfo));
					goto done;
				}

				/**
				 * We will move the ackNode in case the ackNode seqNumber is less
				 * than the received acknowledgement number and also the ackNode
				 * should not cross the sentNode.
				 */
				while(ackNode->seqNum <= pload.ack-1 &&
								ackNode != sentNode){
					printf("Received the ack packet with ack number :%d, skipping :%d\n",pload.ack,ackNode->seqNum);
					ackNode->ack = ackNode->seqNum;
					ackNode= ackNode->next;
					rtt_newpack(&rttinfo);
				}


			window_size_update:
				window_size = pload.windowSize;

				if(pload.windowSize > 0){
					if(isWindowSizeZero){
						reset_persistent_timeout_value(&persistent_timeout);
						isWindowSizeZero=false;
					}
					printf("window size :%d\n",window_size);
					goto send;
				}else if(!pload.windowSize){
					printf("Entered window size zero\n");
					isWindowSizeZero = true;
					// setting the alarm for persistent_timer to do window probes.
					printf("time-out :%d\n",persistent_timeout);
					alarm(persistent_timeout); // In seconds.
					increment_persistent_timeout_value(&persistent_timeout);
					goto receive;
				}
	}
	done:
		alarm(0);
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

void
congestion_control(int how,int *cwnd, int *ssthresh, int *duplicateAck,int *state)
{
	switch(how){

		case TIME_OUT:
			if(*state == SLOW_START){
					*ssthresh = *cwnd/2+1;
					*cwnd = 1;
					*duplicateAck =0;
			}else if(*state == CONGESTION_AVOIDANCE || *state == FAST_RECOVERY){
				*ssthresh = *cwnd/2;
				*cwnd = 1;
				*state = SLOW_START;
				*duplicateAck =0;
			}
			break;
		case DUPLICATE_3_ACK:

			if(*state == SLOW_START || *state == CONGESTION_AVOIDANCE){
				double d = (double)(*cwnd);
				*ssthresh = *cwnd/2 + 1;//ceil(d/2.0);
				*cwnd = *ssthresh + 3;
				*state = FAST_RECOVERY;
				*duplicateAck =0;
			}
			break;
		case DUPLICATE_ACK:
			if(*state == FAST_RECOVERY){
				*cwnd = *cwnd+1;
			}else{
				*duplicateAck = *duplicateAck +1;
			}
			break;
		case NEW_ACK:
			if(*state == SLOW_START){
				*cwnd=*cwnd+1;
				if(*cwnd >= *ssthresh)
					*state = CONGESTION_AVOIDANCE;
			}else if(*state == CONGESTION_AVOIDANCE){
				*cwnd=*cwnd+1;
			}else if(*state == FAST_RECOVERY){
				*cwnd = *ssthresh;
				*state = CONGESTION_AVOIDANCE;
			}
			*duplicateAck =0;
			break;
	}

	printf("cwnd : %d ssthresh :%d state : %d\n",*cwnd,*ssthresh,*state);
}

