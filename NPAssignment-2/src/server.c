/*
 * server.c
 *
 *  Created on: Oct 4, 2015
 *      Author: sujan
 */

#include "usp.h"
#include  "unpifi.h"

#define	RTT_DEBUG 0

/**
 *  Client Address structure to identify duplicate client requests
 */
struct connected_client_address *head_client_address=NULL;

/**
 *  RTT info structures.
 */
struct rtt_info   rttinfo;
int	rttinit = 0;

int congestion_avoidance = 0;

int timeout_restransmission_count = 0;
int dup_ack_3_count  = 0;

int sentpackets_count_first_time = 0;

/**
 *  Signal handler structure.
 */
sigjmp_buf jmpbuf;
bool isMetaDataTransfer=true;


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


	printf("Printing the input parameters from server.in file\n");
	printf("--------------------------------------------------\n");
	printf("Server Well Known Port Number:%d\n",portNumber);
	printf("Server sliding window size :%d\n",slidingWindowSize);
	printf("--------------------------------------------------\n");


	/**
	 * Gettting the interface details.
	 */
	head = getInterfaces(portNumber,NULL,NULL,NULL);

	printf("Server Interfaces and their details\n");
	printf("--------------------------------------------------\n");
	printInterfaceDetails(head);


	/**
	 * Registering signal handler for the SIGCHLD signal.
	 */
	new_action.sa_handler = sigchild_handler;
	sigemptyset(&new_action.sa_mask);
	new_action.sa_flags = 0;
	sigaction(SIGCHLD,NULL,&old_action);

	if(old_action.sa_handler != SIG_IGN){
		if(DEBUG)
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
				Recvfrom(temp->sockfd,&pload,sizeof(pload),0,(SA *)&IPClient,&length);

				ipAddressSocket = getSocketAddress(IPClient);

				printf("Server Socket running on IpAddress:PortNumber - %s:%d has received a Connection fromm the client running on IpAddress:PortNumber - %s\n",temp->ip_address,portNumber,ipAddressSocket);

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
					flow_data->slidingWindow = slidingWindowSize;
					pload = convertToHostOrder(pload);
					flow_data->receiver_window = pload.windowSize;
					if(DEBUG)
						printf("receiver window size %d\n",flow_data->receiver_window);

					file_name = (char *)malloc(1024);
					strcpy(file_name,pload.buff);
					printf("forked a child for handling client connection(sending the file with name): %s\n",file_name);
					doFileTransfer(temp,IPClient,flow_data,file_name);

				}else if(pid > 0){

					struct connected_client_address *client_sock_address = (struct connected_client_address*)malloc(sizeof(struct connected_client_address));

					strcpy(client_sock_address->client_sockaddress,ipAddressSocket);

					// have to use the PID for removing the entry for this list.
					sprintf(client_sock_address->child_pid,"%d",pid);

					if(DEBUG)
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
	int conn_sockfd;
	char IPServer[128];
	struct sockaddr_in serverAddr;
	int length = sizeof(serverAddr);
	struct sockaddr_in networkMaskAddr;
	int optval=1,state=SLOW_START,fin_flag =  0;
	int portNumberRetransmit=0;
	unsigned long matchNumber=0;
	struct dg_payload pload;
	struct sigaction new_action, old_action;
	uint32_t window_size,cwnd,ssthresh,sliding_window;
	struct Node *headNode=NULL,*sentNode=NULL,*ackNode = NULL;
	bool queueFull=false;
	bool isRetransmit=false;
	sigset_t sigset_alrm;
	bool isWindowSizeZero = false,fileNotExists=false,isTimeWaitState = false;
	uint32_t persistent_timeout = 1;


	/**
	 *  Creating the new connection socket.
	 */
	conn_sockfd = Socket(AF_INET,SOCK_DGRAM,0);

	if (rttinit == 0) {
		rtt_init(&rttinfo);		/* first time we're called */
		rttinit = 1;
		rtt_d_flag = 1;
	}

	/**
	 * Initializing variables
	 */
	window_size = flow_data->receiver_window;
	sliding_window = flow_data->slidingWindow;
	cwnd = 1;
	ssthresh = window_size/2;

	inet_pton(AF_INET,sock_info->ip_address,&serverAddr.sin_addr);

	serverAddr.sin_port = 0;
	serverAddr.sin_family = AF_INET;
	Bind(conn_sockfd,(SA *)&serverAddr,sizeof(serverAddr));

	inet_pton(AF_INET,sock_info->network_mask,&networkMaskAddr.sin_addr);

	getClientIPAddress(&IPClient,&networkMaskAddr,&serverAddr,NULL,&matchNumber);

	if(matchNumber > 0){
		printf("------------Client Host is Local Network as the Server. Hence setting the socket option MSG_DONTROUTE------------\n");
		setsockopt(conn_sockfd,SOL_SOCKET,MSG_DONTROUTE,&optval,sizeof(int));
	}else{
		printf("------------Client Host is not in the Local Network as the Server-------------\n");
	}

	Getsockname(conn_sockfd,(SA *)&serverAddr,&length);

	printf("Server Child is running on IP-Address :%s with port number :%d\n",IPServer,ntohs(serverAddr.sin_port));

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
		if(DEBUG)
			printf("New Action Sigalrm Set\n");
		sigaction(SIGALRM,&new_action,&old_action);
	}

	sprintf(pload.buff,"%d",htons(serverAddr.sin_port));

	pload.seq_number = seqNum++;
	pload.type = PAYLOAD;
	pload.ts =  rtt_ts(&rttinfo);

	/**
	 *  Connecting the new socket to IPClient.
	 */
	if(connect(conn_sockfd,(SA *)&IPClient,sizeof(IPClient)) < 0){
		printf("Connection Error :%s",strerror(errno));
	}

	/**
	 *   Backing up the port number
	 */
	sendagain:
		pload.ts = rtt_ts(&rttinfo);
		pload = convertToNetworkOrder(pload);

		printf("Sending the server's new ephemeral port number to the client\n");
		sendto(sock_info->sockfd,(void *)&pload,sizeof(pload),0,(SA *)&IPClient,sizeof(IPClient));

		if(rttinfo.rtt_nrexmt >=1){
			printf("Server timed out Hence sending through listening socket as well as connection socket\n");
			sendto(conn_sockfd,(void *)&pload,sizeof(pload),0,NULL,0);
		}
		alarm(rtt_start(&rttinfo)/1000);

		signalHandling:
			if (sigsetjmp(jmpbuf, 1) != 0) {
				printf("received a sigalrm\n");
				if (rtt_timeout(&rttinfo) < 0) {
					printf("Reached Maximum retransmission attempts : No response from the client so giving up\n");
					rttinit = 0;	/* reinit in case we're called again */
					errno = ETIMEDOUT;
					exit(0);
				}
				if(isMetaDataTransfer){
					printf("Retransmistting...\n");
					goto sendagain;
				}else{
					if(DEBUG)
						printf("FileData Retransfer\n");
					timeout_restransmission_count++;
					isRetransmit=true;
					goto senddatagain;
				}
			}

	inet_ntop(AF_INET,&serverAddr.sin_addr,IPServer,sizeof(IPServer));

	memset(&pload,0,sizeof(pload));

	Recvfrom(conn_sockfd, &pload, sizeof(pload), 0, NULL, NULL);
	printf("Received an Ack from the client for the port Number packet\n");
	alarm(0);
	printf("Connection Established Succesfully\n");
	printf("-------------Sending File data...-------------\n");

	int k = 0;
	isMetaDataTransfer = false;
	/* Closing the listening socket in the child */
	close(sock_info->sockfd);

	/**
	 *   Building the sliding window list.
	 */
	headNode = BuildCircularLinkedList(sliding_window);

	int fd;
	if((fd = open(file_name,O_RDONLY)) < 0){
		printf("***************Error in opening the file :%s\n*********************",strerror(errno));
		FIN_STATE:
				printf("Closing the connection with the client\n");
				Send_Packet(conn_sockfd,seqNum++,"Error in opening the file : No Such file or directory exists",FIN,rtt_ts(&rttinfo));
				fileNotExists = true;
				alarm(rtt_start(&rttinfo)/1000);

				while(1){
					memset(&pload,0,sizeof(pload));
					Recvfrom(conn_sockfd, &pload, sizeof(pload), 0, NULL, NULL);
					pload = convertToHostOrder(pload);
					/* Resetting the alarm */
					alarm(0);
					if(pload.type == FIN_ACK){
						/* Sending the final ACK */
						Send_Packet(conn_sockfd,seqNum++,NULL,ACK,rtt_ts(&rttinfo));
						isTimeWaitState = true;
						/* Considering the Maximum MSL as 2 sec */
						/* After 2 seconds we will conclude the ACK is delivered correctly*/
						alarm(2);
					}else{
						goto FIN_STATE;
					}

					if (sigsetjmp(jmpbuf, 1) != 0) {
							if(fileNotExists){
								if(isTimeWaitState){
									printf("Client has closed the connection hence the closing the server child\n");
									goto done;
								}
								printf("File Not Exisits.. Performing cleaner close\n");
								goto FIN_STATE;
							}
					}
				}
		}


	/**
	 *  Ackindex -> 1
	 *  Initial CWND -> 1
	 */
	sentNode = headNode;
	ackNode = headNode;

	/**
	 *  Opening the file.
	 */
	FILE *fp = fopen(file_name,"r");

	int size,i,duplicateAckCount=0,lastAckReceived=0;

	setRearNode(headNode);


	bool receiveWindowFull = false;
	bool windowEmpty = false;
	bool isFull=false;

	for(;;){

		send:
			size = min(window_size,cwnd);

			if(!fin_flag && !isFull){
				isFull = populateDataList(&fp,ackNode,isFull);
			}

			alarm(rtt_start(&rttinfo)/1000);

			printf("Retransmission Timeout : %d Seconds\n",rtt_start(&rttinfo)/1000);


			if(sentNode->next == headNode && ackNode->next == headNode){
					receiveWindowFull = false;
					windowEmpty = true;
			}

//			if(receiveWindowFull){
//				printf("Sender Sliding is Full..... Hence locking the Window\n");
//			}

			/**
			 *  We will not sent any packets for these conditions:
			 *  1. Window Size sent by the receiver is zero.
			 *  2. Received a FIN/FIN_ACK from receiver.
			 *
			 */
			for(i=0;i<size && !fin_flag &&
				sentNode->next != ackNode && !receiveWindowFull;i++){

				if(sentNode->next == headNode && !windowEmpty){
					receiveWindowFull = true;
				}else{
					windowEmpty = false;
				}

				senddatagain:
				 	 if (sigsetjmp(jmpbuf, 1) != 0) {
						printf("received sigalrm retransmitting\n");
						if (rtt_timeout(&rttinfo) < 0) {
							printf("Reached Maximum retransmission attempts : No response from the server giving up\n");
							rttinit = 0;	/* reinit in case we're called again */
							errno = ETIMEDOUT;
							exit(0);
						}
						if(isMetaDataTransfer){
							printf("Meta data retransmit\n");
							goto sendagain;
						}else if(isTimeWaitState){
							printf("Client has closed the connection hence the closing the server child\n");
							goto done;
						}else if(window_size > 0){
							printf("TimeOut....Retransmitting\n");
							congestion_control(TIME_OUT,&cwnd,&ssthresh,&duplicateAckCount,&state);
							isRetransmit=true;
							goto senddatagain;
						}else if(isWindowSizeZero){
							printf("Persistent Mode... Window Size equals to zero ...Sending window probe\n");
							Send_Packet(conn_sockfd,INT_MAX,NULL,WINDOW_PROBE,rtt_ts(&rttinfo));
							goto receive;
						}
					}

					if(isRetransmit){
						alarm(rtt_start(&rttinfo)/1000);
						timeout_restransmission_count++;
						Send_Packet(conn_sockfd,ackNode->seqNum,ackNode->buff,ackNode->type,rtt_ts(&rttinfo));
						printf("Retransmiting the Data Packet with SeqNum : %d\n",ackNode->seqNum);
					}else{
						sentNode->seqNum = Send_Packet(conn_sockfd,seqNum++,sentNode->buff,sentNode->type,rtt_ts(&rttinfo));
						if(DEBUG)
							printf("send payload time stamp :%u\n",rtt_ts(&rttinfo));
						if(sentNode->type == FIN){
							fin_flag = 1;
						}
						sentpackets_count_first_time++;
						printf("Sent a packet with seqNumber : %d and  ReceiverWindowsize %d\n",sentNode->seqNum, window_size);
						sentNode = sentNode->next;
					}

					if(isRetransmit){
						isRetransmit = false;
						break;
					}

				}

			receive:
				printf("Waiting for Ack...\n");
				memset(&pload,0,sizeof(pload));
				sigprocmask(SIG_UNBLOCK,&sigset_alrm,NULL);
				if(Recvfrom(conn_sockfd, &pload, sizeof(pload), 0, NULL, NULL) < 0){
					goto done;
				}
				sigprocmask(SIG_BLOCK,&sigset_alrm,NULL);
				pload = convertToHostOrder(pload);
				if(DEBUG)
					printf("received payload time stamp :%u\n",pload.ts);

				rtt_stop(&rttinfo, rtt_ts(&rttinfo) - pload.ts);

				if(pload.type == WINDOW_PROBE){
					printf("Received the Ack n response for Window Probe :%d\n",pload.windowSize);
					goto window_size_update;
				}else{
					printf("Received the ack with Number : %d and Window Size %d\n",pload.ack,pload.windowSize);
				}

				if(!lastAckReceived || (lastAckReceived!=pload.ack)){
					congestion_control(NEW_ACK,&cwnd,&ssthresh,&duplicateAckCount,&state);
					if(DEBUG)
						printf("last received ack :%d\n",pload.ack);

					lastAckReceived = pload.ack;
				}else if(lastAckReceived==pload.ack){
					if(DEBUG)
						printf("Entered last received ack loop :%d\n",pload.ack);
					if(duplicateAckCount == MAX_DUPLICATE_ACK_COUNT){
						printf("#########entered duplicate ack loop##########\n");
						congestion_control(DUPLICATE_3_ACK,&cwnd,&ssthresh,&duplicateAckCount,&state);
						dup_ack_3_count++;
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
					printf("Received FIN_ACK from client\n");
					Send_Packet(conn_sockfd,seqNum++,NULL,ACK,rtt_ts(&rttinfo));
					alarm(0);
					isTimeWaitState=true;
					/*setting the alarm for last ACK*/
					alarm(2);
					goto receive;
				}

				//int diff = pload.ack-1- ackNode->seqNum;

				/**
				 * We will move the ackNode in case the ackNode seqNumber is less
				 * than the received acknowledgement number and also the ackNode
				 * should not cross the sentNode.
				 */
				while(pload.ack > 0 && ackNode->seqNum <= pload.ack-1 && ackNode != sentNode ){
					printf("Received the ack packet with ack number : %d so skipping packet with seq number: %d\n", pload.ack,ackNode->seqNum);
					ackNode->ack = ackNode->seqNum;
					ackNode= ackNode->next;
					if(DEBUG){
						printf("Ack Node now pointing to :%d\n",ackNode->seqNum);
					}
					rtt_newpack(&rttinfo);

					if(ackNode->next == headNode){
						receiveWindowFull=false;
					}
				}

//			if(diff > 1){
//				printf("Received Cummulative Ack :%d hence skipping %d packets\n",pload.ack,diff);
//			}

			window_size_update:
				window_size = pload.windowSize;

				if(pload.windowSize > 0){
					if(isWindowSizeZero){
						reset_persistent_timeout_value(&persistent_timeout);
						alarm(0);
						isWindowSizeZero=false;
					}

					if(DEBUG)
						printf("window size :%d\n",window_size);

					goto send;
				}else if(!pload.windowSize){
					if(DEBUG)
						printf("Entered window size zero\n");
					isWindowSizeZero = true;
					// setting the alarm for persistent_timer to do window probes.
					if(DEBUG)
						printf("time-out :%d\n",persistent_timeout);

					alarm(persistent_timeout); // In seconds.
					increment_persistent_timeout_value(&persistent_timeout);
					goto receive;
				}
	}
	done:
		alarm(0);
		close(conn_sockfd);
		fclose(fp);
		printf("Statistics\n");
		printf("--------------------------------------------------------------------\n");
		printf("Number of packets sent from the sender sliding window :%d\n",sentpackets_count_first_time);
		printf("Number of retransissions due to TimeOuts Count :%d\n",timeout_restransmission_count);
		printf("Number of times retransmission due 3 DUP ACKs :%d\n",dup_ack_3_count);
		printf("--------------------------------------------------------------------\n");
		fflush(stdout);
		deleteCircularLinkedList(headNode);
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

	if(DEBUG)
		printf("received the sigalrm\n");
	siglongjmp(jmpbuf,1);
}

/**
 *  Function to handle congestion window.
 */

void
congestion_control(int how,int *cwnd, int *ssthresh, int *duplicateAck,int *state)
{
	switch(how){

		case TIME_OUT:
			if(*state == SLOW_START){
					*ssthresh = (*cwnd/2)+1;
					*cwnd = 1;
					*duplicateAck =0;
					 congestion_avoidance = 0;
			}else if(*state == CONGESTION_AVOIDANCE || *state == FAST_RECOVERY){
				*ssthresh = (*cwnd/2) + 1;
				*cwnd = 1;
				*state = SLOW_START;
				*duplicateAck =0;
				 congestion_avoidance = 0;
			}
			break;
		case DUPLICATE_3_ACK:

			if(*state == SLOW_START || *state == CONGESTION_AVOIDANCE){
				double d = (double)(*cwnd);
				*ssthresh = *cwnd/2 + 1;//ceil(d/2.0);
				*cwnd = *ssthresh + 3;
				*state = FAST_RECOVERY;
				*duplicateAck =0;
				 congestion_avoidance = 0;
			}
			break;
		case DUPLICATE_ACK:
			if(*state == FAST_RECOVERY){
				*cwnd = *cwnd+1;
				congestion_avoidance = 0;
			}else{
				*duplicateAck = *duplicateAck +1;
				congestion_avoidance = 0;
			}
			break;
		case NEW_ACK:
			if(*state == SLOW_START){
				*cwnd=*cwnd+1;
				if(*cwnd >= *ssthresh)
					*state = CONGESTION_AVOIDANCE;
					congestion_avoidance = 0;
			}else if(*state == CONGESTION_AVOIDANCE){
				if(congestion_avoidance == 3){
					*cwnd=*cwnd + 1;
					congestion_avoidance = 0;
				}else{
					congestion_avoidance++;
				}
			}else if(*state == FAST_RECOVERY){
				*cwnd = *ssthresh;
				*state = CONGESTION_AVOIDANCE;
				 congestion_avoidance = 0;
			}
			*duplicateAck =0;
			break;
	}

	printf("----------cwnd : %d ssthresh :%d state : %s--------\n",*cwnd,*ssthresh,printCongestionState(*state));
}

