/*
 * server.c
 *
 *  Created on: Oct 4, 2015
 *       Authors: sujan, sidhartha
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

				printf("Server socket (%s:%d) received a connection from client (%s)\n",temp->ip_address,portNumber,ipAddressSocket);


				/*
				 *  If the client is already connected then this is datagram
				 *  is a part of retransmission hence we will ignore this
				 *  connection request.
				 *
				 */
				if(isClientConnected(head_client_address,ipAddressSocket)){
						printf("Client Already connected hence ignoring\n");
						break;
				}

				printf("(%s:%d) Forking a child for handling client connection (sending the file with name): %s\n",temp->ip_address,portNumber,pload.buff);

				fflush(stdout);

				if((pid = fork()) == 0){
					flow_data = (struct flow_metadata*)malloc(sizeof(struct flow_metadata));
					flow_data->slidingWindow = slidingWindowSize;
					pload = convertToHostOrder(pload);
					flow_data->receiver_window = pload.windowSize;
					if(DEBUG)
						printf("receiver window size %d\n",flow_data->receiver_window);

					file_name = (char *)malloc(1024);
					strcpy(file_name,pload.buff);
					doFileTransfer(temp,IPClient,flow_data,file_name,ipAddressSocket);

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
				}else{
					printf("Child creation failed\n");
					exit(0);
				}
			}
			temp = temp->next;
		}
	}
}

void doFileTransfer(struct binded_sock_info *sock_info,struct sockaddr_in IPClient,
		struct flow_metadata *flow_data, char *file_name,char *client_addr){

	int seqNum =0;
	bool isLocal=false;
	int conn_sockfd;
	char IPServer[128];
	char ipAddress_client[128];
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

	serverAddr.sin_port = 0;
	serverAddr.sin_family = AF_INET;

	inet_pton(AF_INET,sock_info->network_mask,&networkMaskAddr.sin_addr);

	inet_ntop(AF_INET,&IPClient.sin_addr,ipAddress_client,sizeof(ipAddress_client));

	if(strcmp(ipAddress_client, LOOPBACK_ADDRESS) == 0){
		printf("(%s) Client is in the same host.Hence assigning loop back address to server\n",client_addr);
		inet_pton(AF_INET,LOOPBACK_ADDRESS,&serverAddr.sin_addr);
	}else{

		isClientLocal(&IPClient,&networkMaskAddr,&serverAddr,NULL,&matchNumber);

		if(matchNumber > 0){
			printf("(%s) Client Host is in Local Network to the Server. Hence setting the socket option MSG_DONTROUTE\n",client_addr);
			setsockopt(conn_sockfd,SOL_SOCKET,MSG_DONTROUTE,&optval,sizeof(int));
		}else{
			printf("(%s) Client Host is not in Local Network to the Server\n",client_addr);
		}

		inet_pton(AF_INET,sock_info->ip_address,&serverAddr.sin_addr);
	}

	Bind(conn_sockfd,(SA *)&serverAddr,sizeof(serverAddr));

	Getsockname(conn_sockfd,(SA *)&serverAddr,&length);

	printf("(%s) Server Child is running on IP-Address : %s with port number : %d\n",client_addr,sock_info->ip_address,ntohs(serverAddr.sin_port));

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
		printf("(%s) Connection Error :%s",client_addr,strerror(errno));
	}

	/**
	 *   Backing up the port number
	 */
	sendagain:
		pload.ts = rtt_ts(&rttinfo);
		pload = convertToNetworkOrder(pload);

		printf("(%s) Sending the server's new ephemeral port number %d to the client\n",client_addr,ntohs(serverAddr.sin_port));
		sendto(sock_info->sockfd,(void *)&pload,sizeof(pload),0,(SA *)&IPClient,sizeof(IPClient));

		if(rttinfo.rtt_nrexmt >=1){
			printf("(%s) Server timed out Hence sending through listening socket as well as connection socket\n",client_addr);
			sendto(conn_sockfd,(void *)&pload,sizeof(pload),0,NULL,0);
		}
		alarm(rtt_start(&rttinfo)/1000);

		signalHandling:
			if (sigsetjmp(jmpbuf, 1) != 0) {

				if(DEBUG)
					printf("received a sigalrm\n");

				if (rtt_timeout(&rttinfo) < 0) {
					printf("(%s) Reached Maximum retransmission attempts : No response from the client so giving up\n",client_addr);
					rttinit = 0;	/* reinit in case we're called again */
					errno = ETIMEDOUT;
					exit(0);
				}
				if(isMetaDataTransfer){
					printf("(%s) Retransmistting...\n",client_addr);
					goto sendagain;
				}else{
					if(DEBUG)
						printf("(%s) FileData Retransfer\n",client_addr);
					timeout_restransmission_count++;
					isRetransmit=true;
					goto senddatagain;
				}
			}

	inet_ntop(AF_INET,&serverAddr.sin_addr,IPServer,sizeof(IPServer));

	memset(&pload,0,sizeof(pload));

	Recvfrom(conn_sockfd, &pload, sizeof(pload), 0, NULL, NULL);
	printf("(%s) Received an ack from the client for the port number packet\n",client_addr);
	alarm(0);
	printf("(%s) Connection Established Succesfully\n",client_addr);

	int k = 0;
	isMetaDataTransfer = false;
	/* Closing the listening socket in the child */
	close(sock_info->sockfd);

	/**
	 *   Building the sliding window list.
	 */
	headNode = BuildCircularLinkedList(sliding_window);

	int temp_seq = seqNum++;
	int fd;
	if((fd = open(file_name,O_RDONLY)) < 0){
		printf("(%s) *********************Error in opening the file : %s*********************",client_addr,strerror(errno));
		printf("(%s) Closing the connection with the client\n",client_addr);
		FIN_STATE:
				Send_Packet(conn_sockfd,temp_seq,"Client requested file doesn't exist at server",FIN,rtt_ts(&rttinfo),client_addr);
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
						Send_Packet(conn_sockfd,temp_seq+1,NULL,ACK,rtt_ts(&rttinfo),client_addr);
						isTimeWaitState = true;
						/* Considering the Maximum MSL as 2 sec */
						/* After 4 seconds we will conclude the ACK is delivered correctly*/
						alarm(4);
					}else{
						goto FIN_STATE;
					}

					if (sigsetjmp(jmpbuf, 1) != 0) {
							if(fileNotExists){
								if(isTimeWaitState){
									printf("(%s) Client has closed the connection hence the closing the server child\n",client_addr);
									goto done;
								}

								if(DEBUG)
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
	bool skippingFlag = false;

	printf("(%s) -------------Starting the file data transfer-------------\n",client_addr);

	for(;;){

		send:
			size = min(window_size,cwnd);

			if(!fin_flag && !isFull){
				isFull = populateDataList(&fp,ackNode,isFull);
			}

			alarm(rtt_start(&rttinfo)/1000);

			printf("(%s) Retransmission Timeout : %d Seconds\n",client_addr,rtt_start(&rttinfo)/1000);


			if(sentNode->next == headNode && ackNode->next == headNode){
					receiveWindowFull = false;
					windowEmpty = true;
			}

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

				 		 if(DEBUG)
				 			 printf("received sigalrm retransmitting\n");

						if (rtt_timeout(&rttinfo) < 0) {
							printf("(%s) Reached maximum retransmission attempts : No response from the client so giving up\n",client_addr);
							rttinit = 0;	/* reinit in case we're called again */
							errno = ETIMEDOUT;
							exit(0);
						}
						if(isMetaDataTransfer){
							if(DEBUG)
								printf("Meta data retransmit\n");
							goto sendagain;
						}else if(isTimeWaitState){
							printf("(%s) Client has closed the connection hence the closing the server child\n",client_addr);
							goto done;
						}else if(window_size > 0){
							printf("(%s) TimeOut occurred\n",client_addr);
							congestion_control(TIME_OUT,&cwnd,&ssthresh,&duplicateAckCount,&state,client_addr);
							isRetransmit=true;
							goto senddatagain;
						}else if(isWindowSizeZero){
							printf("(%s) Persistent Mode... Window Size equals to zero... so sending window probe\n",client_addr);
							Send_Packet(conn_sockfd,INT_MAX,NULL,WINDOW_PROBE,rtt_ts(&rttinfo),client_addr);
							goto receive;
						}
					}

					if(isRetransmit){
						alarm(rtt_start(&rttinfo)/1000);
						timeout_restransmission_count++;
						Send_Packet(conn_sockfd,ackNode->seqNum,ackNode->buff,ackNode->type,rtt_ts(&rttinfo),client_addr);
						printf("(%s) Retransmiting the Data Packet with sequence number : %d\n",client_addr,ackNode->seqNum);
					}else{
						sentNode->seqNum = Send_Packet(conn_sockfd,seqNum++,sentNode->buff,sentNode->type,rtt_ts(&rttinfo),client_addr);
						if(DEBUG)
							printf("(%s) send payload time stamp :%u\n",client_addr,rtt_ts(&rttinfo));
						if(sentNode->type == FIN){
							printf("Sending FIN packet\n");
							fin_flag = 1;
						}
						sentpackets_count_first_time++;
						printf("(%s) Sent packet with sequence number : %d\n",client_addr,sentNode->seqNum);
						sentNode = sentNode->next;
					}

					if(isRetransmit){
						isRetransmit = false;
						break;
					}

				}

			receive:
				if(!isTimeWaitState){
					printf("(%s) Waiting for Ack...\n",client_addr);
				}
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
					printf("(%s) Received a window size for Window Probe : %d\n",client_addr,pload.windowSize);
					goto window_size_update;
				}

				/**
				 * We will move the ackNode in case the ackNode seqNumber is less
				 * than the received acknowledgement number and also the ackNode
				 * should not cross the sentNode.
				 */
				printf("(%s) Received an packet with acknowledgement number : %d with receiver window size %d\n ",
																				client_addr,pload.ack,pload.windowSize);

				if(!skippingFlag)
						skippingFlag=true;

				while(pload.ack > 0 && ackNode->seqNum <= pload.ack-1
						&& ackNode != sentNode && pload.type == ACK){
					//printf("(%s:%d) Received the packet with acknowledgement number : %d so skipping packet with seq number: %d\n", pload.ack,ackNode->seqNum);


					if(skippingFlag){
						printf("(%s) so skipping packets with sequence number ",client_addr);
						skippingFlag = false;
					}


					printf("%d ",ackNode->seqNum);
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
				printf("\n");


				if(!lastAckReceived || (lastAckReceived!=pload.ack)){
					congestion_control(NEW_ACK,&cwnd,&ssthresh,&duplicateAckCount,&state,client_addr);
					if(DEBUG)
						printf("last received ack :%d\n",pload.ack);

					lastAckReceived = pload.ack;
				}else if(lastAckReceived==pload.ack){
					if(DEBUG)
						printf("Entered last received ack loop :%d\n",pload.ack);
					if(duplicateAckCount >= MAX_DUPLICATE_ACK_COUNT){
						printf("(%s) Received 3 duplicates acknowledgements for packet : %d\n",client_addr,lastAckReceived);
						congestion_control(DUPLICATE_3_ACK,&cwnd,&ssthresh,&duplicateAckCount,&state,client_addr);
						dup_ack_3_count++;
						if(window_size > 0){
							if(isWindowSizeZero){
								isWindowSizeZero = false;
								alarm(0);
							}
							isRetransmit=true;
							goto senddatagain;
						}else{
							goto receive;
						}
					}else{
						congestion_control(DUPLICATE_ACK,&cwnd,&ssthresh,&duplicateAckCount,&state,client_addr);
					}
				}

				if(pload.type==FIN_ACK){
					printf("(%s) Received FIN_ACK from client\n",client_addr);
					Send_Packet(conn_sockfd,seqNum++,NULL,ACK,rtt_ts(&rttinfo),client_addr);
					alarm(0);
					isTimeWaitState=true;
					/*setting the alarm for last ACK*/
					alarm(4);
					if (sigsetjmp(jmpbuf, 1) != 0) {
						goto done;
					}
					goto receive;
				}

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

					printf("(%s) Received a window size zero. So, server is entering into persisitent mode\n",client_addr);

					alarm(persistent_timeout); // In seconds.
					increment_persistent_timeout_value(&persistent_timeout);
					goto receive;
				}
	}
	done:
		alarm(0);
		close(conn_sockfd);
		fclose(fp);

		if(isTimeWaitState)
			printf("(%s) Completed file transfer successfully\n",client_addr);

		printf("(%s) Statistics\n",client_addr);
		printf("--------------------------------------------------------------------\n");
		printf("(%s) Number of packets sent (Excluding retransmissions) : %d\n",client_addr,sentpackets_count_first_time);
		printf("(%s) Number of retransmissions due to Time-outs : %d\n",client_addr,timeout_restransmission_count);
		printf("(%s) Number of retransmission due to 3 duplicate acks (Fast Retransmit) : %d\n",client_addr,dup_ack_3_count);
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
		fflush(stdout);
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
congestion_control(int how,int *cwnd, int *ssthresh, int *duplicateAck,int *state,char *client_addr)
{
	printf("----------\n(%s) Congestion window calculation:\n",client_addr);
	switch(how){

		case TIME_OUT:
			printf("(%s) Time-out occurred\n",client_addr);
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
			printf("(%s) Received 3 duplicate acknowledgements--FAST RETRANSMIT\n",client_addr);
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
			printf("(%s) Received a duplicate acknowledgement\n",client_addr);
			if(*state == FAST_RECOVERY){
				*cwnd = *cwnd+1;
				congestion_avoidance = 0;
			}else{
				*duplicateAck = *duplicateAck +1;
				congestion_avoidance = 0;
			}
			break;
		case NEW_ACK:
			printf("(%s) Received a new successful acknowledgement",client_addr);
			if(*state == SLOW_START){
				*cwnd=*cwnd+1;
				if(*cwnd >= *ssthresh){
					printf(" (cwnd >= ssthresh)");
					*state = CONGESTION_AVOIDANCE;
					congestion_avoidance = 0;
				}
			}else if(*state == CONGESTION_AVOIDANCE){
				if(congestion_avoidance == 2){
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
			printf("\n");
			break;
	}

	printf("(%s) CWND : %d ssthresh : %d current congestion-state : %s\n----------\n",client_addr,*cwnd,*ssthresh,printCongestionState(*state));
}

