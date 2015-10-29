/*
 * client.c
 *
 *  Created on: Oct 7, 2015
 *      Author: sujan
 */
#include "usp.h"
#include  "unpifi.h"

#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_RESET   "\x1b[0m"

#define	RTT_DEBUG 0

#define MAX_RESTRANSMISSION_CLIENT 10

static struct rtt_info rttinfo;
static int	rttinit = 0;
static sigjmp_buf jmpbuf;
int server_seq_num = -1;

// Thread conditions
pthread_mutex_t the_mutex;
pthread_cond_t condc, condp;
bool isFilled = false;
bool isPrinterExited = false;

struct Node *front = NULL, *buff_head = NULL, *rear = NULL;
int filled_circular_buffer_size = 0;
float prob = 0.0f;

// Stats
int num_of_packets_dropped = 0;
int num_of_acks_dropped = 0;

struct timeval timeout;


// Temporary file pointers
FILE *fp_output;
FILE *fp_output_seq;

int main(int argc,char **argv){

	FILE *fp;
	char IPServer[128];
	int portNumber,sockfd;
	char fileName[120];
	uint32_t windowSize;
	uint32_t WINDOW_SIZE;
	struct sockaddr_in *clientAddr;
	struct ifi_info *if_head,*if_temp;
	char clientIP[128];
	struct binded_sock_info *head = NULL, *temp=NULL;
	long max = 0, tempMax=0;
	struct sockaddr_in serverAddr,IPClient;
	int length = sizeof(IPClient),optval=1;
	bool isLocal=false;
	struct in_addr subnet_addr;
	unsigned long maxMatch=0;
	bool isLoopBack=false;
	char buff[PACKET_SIZE];
	int ack_seq=0;
	struct dg_payload send_pload;
	struct dg_payload recv_pload;
	struct sigaction new_action, old_action;
	uint32_t seqNum = 0;
	int randomSeed;
	int required_seq_num = -1;
	uint32_t sleepPrinterInSecs = 0;
	timeout.tv_sec = 5;
	timeout.tv_usec = 0;
	int transmission_count = 0;

	// Threading related
	pthread_t printer;
	pthread_mutex_init(&the_mutex, NULL);
	pthread_cond_init(&condc, NULL);		/* Initialize consumer condition variable */
	pthread_cond_init(&condp, NULL);		/* Initialize producer condition variable */

	fp=fopen("client.in","r");
	Fscanf(fp,"%s",IPServer);
	Fscanf(fp,"%d",&portNumber);
	Fscanf(fp,"%s",fileName);
	Fscanf(fp,"%d",&windowSize);
	Fscanf(fp,"%d",&randomSeed);
	Fscanf(fp,"%f",&prob);
	Fscanf(fp,"%u",&sleepPrinterInSecs);

	printf(ANSI_COLOR_RED   "Server IP : %s\n" ANSI_COLOR_RESET, IPServer);
	printf("Server Port : %d\n", portNumber);
	printf("Client requesting file : %s\n", fileName);
	printf("Client window size : %d\n", windowSize);
	printf("Random seed to discard a packet : %d\n", randomSeed);
	printf("Probability of discarding a packet : %f\n", prob);
	printf("Sleep time of printer thread %u\n", sleepPrinterInSecs);

	setRandomSeed(randomSeed);

	// Temporary output files
	if(DEBUG)
	{
		fp_output=fopen("output.txt", "w");
		if(fp_output == NULL)
		{
			if(DEBUG)
				printf("Error in opening a file\n");
			exit(-1);
		}

		fp_output_seq=fopen("output_seq.txt", "w");
		if(fp_output_seq == NULL)
		{
			if(DEBUG)
				printf("Error in opening a file\n");
			exit(-1);
		}
	}


	struct dg_payload data_temp_buff[windowSize];
	WINDOW_SIZE = windowSize;

	initializeTempBuff(data_temp_buff, WINDOW_SIZE);

	buff_head = BuildCircularLinkedList(windowSize);
	//printList(buff_head);
	//front = buff_head;
	//rear = buff_head;

	printf("Connecting to Server IP-Address %s\n",IPServer);

	bzero(&serverAddr,sizeof(serverAddr));

	if(inet_pton(AF_INET,IPServer,&serverAddr.sin_addr) < 0){
		printf("Inet_ptonN error\n");
	}
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(portNumber);
	IPClient.sin_family = AF_INET;
	IPClient.sin_port = 0;

	/**
	 *  Check whether the server is local/external.
	 */
	if(strcmp(IPServer, LOOPBACK_ADDRESS) == 0){
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
		printf("Server is not in local network to the client\n");
	}else{
		printf("server is in local network to the Client\n");
		setsockopt(sockfd,SOL_SOCKET,MSG_DONTROUTE,&optval,sizeof(int));
	}

	sockfd = Socket(AF_INET,SOCK_DGRAM,0);

	Bind(sockfd,(SA *)&IPClient,sizeof(IPClient));

	Getsockname(sockfd,(SA *)&IPClient,&length);

	inet_ntop(AF_INET,&IPClient.sin_addr,clientIP,sizeof(clientIP));

	printf("Client is running on IP-Address :%s with Port Number :%d\n",clientIP, ntohs(IPClient.sin_port));

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

	new_action.sa_handler = sig_alrm;
	sigemptyset(&new_action.sa_mask);
	new_action.sa_flags = 0;
	sigaction(SIGALRM,NULL,&old_action);

	if(old_action.sa_handler != SIG_IGN){
		sigaction(SIGALRM,&new_action,&old_action);
	}

	memset(&send_pload,0,sizeof(send_pload));
	strcpy(send_pload.buff,fileName);
	send_pload.type = PAYLOAD;
	send_pload.seq_number = seqNum++;
	send_pload.windowSize = windowSize;

	send_pload = convertToNetworkOrder(send_pload);

	sendagain:
		send_pload.ts = rtt_ts(&rttinfo);

		if(!is_in_limits(prob)){
			sendto(sockfd,(void *)&send_pload,sizeof(send_pload),0,NULL,0);
			printf("Client has sent the file Name to server... Waiting for the server child port number\n");
		}else{
			printf("Dropping the packet containing the fileName with seqNum :%d\n",send_pload.seq_number);
			goto sendagain;
		}

		if (sigsetjmp(jmpbuf, 1) != 0) {
			printf("Timeout occurred so retransmitting the packet with FileName....\n");
			if (rtt_timeout(&rttinfo) < 0) {
				printf("Maximum retransmission attempts done. No response from the server so giving up\n");
				rttinit = 0;	/* reinit in case we're called again */
				errno = ETIMEDOUT;
				return(-1);
			}
			goto sendagain;
		}

	memset(&recv_pload,0,sizeof(recv_pload));
	if(Recvfrom(sockfd,&recv_pload,sizeof(recv_pload),0,NULL,NULL) < 0)
	{
		alarm(0);
		close(sockfd);
		exit(0);
	}

	recv_pload = convertToHostOrder(recv_pload);
	rtt_stop(&rttinfo, rtt_ts(&rttinfo) - recv_pload.ts);
	alarm(0);

	int serverChildPortNumber = atoi(recv_pload.buff);

	printf("Received new ephemeral port number of the server child %d\n", serverChildPortNumber);
	close(sockfd);

	struct sockaddr_in newServerAddr;

	newServerAddr.sin_port = htons(serverChildPortNumber);
	newServerAddr.sin_addr.s_addr = serverAddr.sin_addr.s_addr;
	newServerAddr.sin_family = AF_INET;

	int new_sockfd = Socket(AF_INET,SOCK_DGRAM,0);

	Bind(new_sockfd,(SA *)&IPClient,sizeof(IPClient));

	if(connect(new_sockfd,(SA *)&newServerAddr,sizeof(newServerAddr)) < 0){
		printf("Connection Error :%s",strerror(errno));
	}else{
		printf("Socket is connected in client\n");
	}

	uint32_t ts = recv_pload.ts;

	sendAcknowledgement(sockfd, ts, recv_pload.seq_number + 1, WINDOW_SIZE, PORT_NUMBER);

	if(DEBUG)
	{
		printf("Sent the acknowledgment :%d to tell server that port number is received\n", recv_pload.seq_number + 1);
	}

	// Create the printer thread
	pthread_create(&printer, NULL, printData, &sleepPrinterInSecs);
	printf("Started printer thread successfully\n");

	fd_set monitor_fds;

	FD_ZERO(&monitor_fds);

	for(;;){

		printf("Client is waiting for data packet\n");
		SELECT:

			FD_SET(new_sockfd, &monitor_fds);

			timeout.tv_sec = 3;

			if(select(new_sockfd+1, &monitor_fds, NULL, NULL, &timeout) == 0){

				if(transmission_count == MAX_RESTRANSMISSION_CLIENT){

					printf("Server is not responding.... Quitting the client\n");
					exit(0);
				}

				sendAcknowledgement(new_sockfd, ts, server_seq_num + 1,
							getWindowSize(windowSize, getUsedTempBuffSize(data_temp_buff, WINDOW_SIZE)), SERVER_TIMEOUT);

				if(getWindowSize(windowSize, getUsedTempBuffSize(data_temp_buff, WINDOW_SIZE)) != 0){
						transmission_count++;
				}
				goto SELECT;
			}
			else{

			if(FD_ISSET(new_sockfd, &monitor_fds))
			{
				memset(&recv_pload,0,sizeof(recv_pload));
				if(Recvfrom(new_sockfd,&recv_pload,sizeof(recv_pload),0,NULL,NULL) < 0){
					if(errno == ECONNREFUSED){
						printf("Quitting the client\n");
					}
					exit(0);
				}

				transmission_count=0;
				recv_pload = convertToHostOrder(recv_pload);

				uint32_t ts = recv_pload.ts;
				uint32_t seq = recv_pload.seq_number + 1;

				if(recv_pload.type == WINDOW_PROBE){
					printf("Received window probe request. Sending the current window size for window probe segment\n");
					sendAcknowledgement(new_sockfd, ts, INT_MAX,
							getWindowSize(windowSize, getUsedTempBuffSize(data_temp_buff, WINDOW_SIZE)), WINDOW_PROBE);
					continue;
				}


				printf("Client has received the data packet sequence number : %d\n", recv_pload.seq_number);

				// store in other list and send ack for required packet
				if(DEBUG)
				{
					printf("Window size: %d\n", getWindowSize(windowSize, getUsedTempBuffSize(data_temp_buff, WINDOW_SIZE)));
				}

				if(getWindowSize(windowSize, getUsedTempBuffSize(data_temp_buff, WINDOW_SIZE)) == 0)
				{
					sendAcknowledgement(sockfd, ts, server_seq_num + 1, getWindowSize(windowSize, getUsedTempBuffSize(data_temp_buff, WINDOW_SIZE)), ACK);
					printf("Unable to store packet. Receive buffer is full\n");
					continue;
				}

				// drop packet if packet received is less than expected seq number
				if(server_seq_num != -1 && server_seq_num >= recv_pload.seq_number)
				{
					sendAcknowledgement(sockfd, ts, server_seq_num + 1, getWindowSize(windowSize, getUsedTempBuffSize(data_temp_buff, WINDOW_SIZE)), ACK);
					printf("Waiting for data sequence number : %d so dropping received sequence number : %d\n", server_seq_num + 1, recv_pload.seq_number);
					continue;
				}

				int type = ACK;
				if(!is_in_limits(prob))
				{
					if(required_seq_num == recv_pload.seq_number || required_seq_num == -1)
					{
						if(DEBUG)
							printf("producer trying to grab\n");
						pthread_mutex_lock(&the_mutex);
						isFilled = true;
						if(DEBUG)
							printf("Producer thread grabbed the lock\n");

						pushData(recv_pload, windowSize);
						if(DEBUG)
							printf("Pushed the data for seq num %d and type %d\n",recv_pload.seq_number, recv_pload.type);
						if(required_seq_num != -1)
						{
							while(getPacket(&recv_pload, data_temp_buff, seq, WINDOW_SIZE))
							{
								if(DEBUG)
									printf("Packet present in temporary buffer : %d type : %d\n", recv_pload.seq_number, recv_pload.type);
								pushData(recv_pload,windowSize);
								if(DEBUG)
									printf("Pushed the data for seq num %d\n",recv_pload.seq_number);
								if(recv_pload.type == FIN){
									type = FIN_ACK;
									printf("Received FIN packet from server\n");
									if(recv_pload.buff!=NULL)
										printf("*****%s**********\n",recv_pload.buff);
									closeConnection(sockfd, recv_pload, prob);
									goto cleanClose;
								}
								memset(&recv_pload, 0, sizeof(recv_pload));
								seq++;
							}
							required_seq_num = seq;
						}

						if(recv_pload.type == FIN)
						{
							type = FIN_ACK;
							printf("Received FIN packet from server\n");
							if(recv_pload.buff!=NULL)
								printf("*****%s**********\n",recv_pload.buff);
							closeConnection(new_sockfd, recv_pload, prob);
							goto cleanClose;
						}

						sendAcknowledgement(new_sockfd, ts, seq, getWindowSize(windowSize, getUsedTempBuffSize(data_temp_buff, WINDOW_SIZE)), type);

						if(DEBUG)
								printf("Filled value before :%d\n",isFilled);
						pthread_cond_signal(&condc);
						pthread_mutex_unlock(&the_mutex);
						if(DEBUG)
							printf("Producer released locks and Filled value :%d\n",isFilled);
					}
					else
					{
						storePacket(recv_pload, data_temp_buff, WINDOW_SIZE);
						if(DEBUG)
							printf("Packet stored in data_temp_buff %d\n", recv_pload.seq_number);
						sendAcknowledgement(new_sockfd, ts, required_seq_num, getWindowSize(windowSize, getUsedTempBuffSize(data_temp_buff, WINDOW_SIZE)), ACK);
					}

				}
				else
				{
					printf("Intentionally dropping packet with sequence number : %d\n", recv_pload.seq_number);
					num_of_packets_dropped++;
					if(required_seq_num == -1)
					{
						required_seq_num = recv_pload.seq_number;
					}
				}
			}
			else{
				goto SELECT;
			}
		}
	}

	cleanClose:
		printf("Waiting for printer thread to exit\n");
		pthread_join(printer, NULL);
		printf("Done with the file transfer\n");
		close(new_sockfd);
		deleteCircularLinkedList(buff_head);
		printf("Client closed successfully\n");
		printf("------------------------------------------------------------\n");
		printf("Statistics:\n------------\n");
		printf("Number of packets intentionally dropped: %d\n", num_of_packets_dropped);
		printf("Number of acknowledgments intentionally dropped: %d\n", num_of_acks_dropped);
		printf("------------------------------------------------------------\n");
}

void
sig_alrm(int signo){
	printf("received the sig-alarm\n");
	siglongjmp(jmpbuf,1);
}

void closeConnection(int sockfd, struct dg_payload pload, float prob)
{
	pthread_cond_signal(&condc);
	pthread_mutex_unlock(&the_mutex);

	if(DEBUG)
		printf("Producer released locks\n");

	struct dg_payload recv_pload;

	printf("Sending the FIN_ACK to server\n");
	fd_set monitor_fds;
	FD_ZERO(&monitor_fds);

	FD_SET(sockfd, &monitor_fds);

	FIN_STATE:
		sendAcknowledgement(sockfd, pload.ts, pload.seq_number + 1, 1, FIN_ACK);
		FD_SET(sockfd, &monitor_fds);
		timeout.tv_sec = 1;
		if(select(sockfd+1, &monitor_fds, NULL, NULL, &timeout) == 0)
		{
			goto FIN_STATE;
		}

		if(FD_ISSET(sockfd, &monitor_fds))
		{
				memset(&recv_pload,0,sizeof(recv_pload));
				if(recvfrom(sockfd,&recv_pload,sizeof(recv_pload),0,NULL,NULL) < 0){
					goto FIN_STATE;
				}else{
					recv_pload = convertToHostOrder(recv_pload);
					if(!is_in_limits(prob)){
						if(recv_pload.type == ACK){
							printf("Received the ACK for FIN_ACK packet received\n");
							return;
						}
						else{
							goto FIN_STATE;
						}
					}else{
						printf("Intentionally dropping ACK packet after FIN_ACK\n");
						printf("Retransmitting so sending the FIN_ACK to server\n");
						goto FIN_STATE;
					}
				}
		}
}

void
sendAcknowledgement(int sockfd, uint32_t ts, uint32_t ack, uint32_t windowSize, uint32_t type)
{
	struct dg_payload send_pload;
	memset(&send_pload,0,sizeof(send_pload));
	bool is_probe_req = false;
	bool is_port_number_packet = false;
	bool is_server_timeout = false;

	send_pload.ts=ts;
	send_pload.ack = ack;
	send_pload.windowSize = windowSize;
	send_pload.type = type;
	switch(type)
	{
		case WINDOW_PROBE:
			send_pload.type = type;
			send_pload.windowSize = windowSize;
			send_pload.ts=ts;
			is_probe_req = true;
			break;
		case PORT_NUMBER:
			is_port_number_packet = true;
			send_pload.type = ACK;
			break;
		case SERVER_TIMEOUT:
			is_server_timeout = true;
			send_pload.type = ACK;
			break;
		case ACK:
			send_pload.type = ACK;
			server_seq_num = send_pload.ack-1;
			break;
	}

	if(is_probe_req || is_port_number_packet || is_server_timeout || !is_in_limits(prob))
	{
		send_pload = convertToNetworkOrder(send_pload);
		if(sendto(sockfd,(void *)&send_pload,sizeof(send_pload),0,NULL,0) < 0){
			printf("Error on send : %s\n",strerror(errno));
		}
		printf("Sent acknowledgment with sequence number : %d with window size %d \n", ntohl(send_pload.ack), ntohs(send_pload.windowSize));
	}
	else
	{
		num_of_acks_dropped++;
		printf("Intentionally not sending acknowledgment for sequence number: %d\n", ack-1);
	}
}

int getWindowSize(uint32_t windowSize, int temp_buff_size)
{
	if(DEBUG)
	{
		printf("Receiver window size : %d\n", windowSize);
		printf("Data buffer size : %d\n", filled_circular_buffer_size);
		printf("Data temp buffer size : %d\n", temp_buff_size);
	}

	int size = windowSize - (filled_circular_buffer_size + 1) - temp_buff_size;
	return size < 0 ? 0 : size;
}

bool printDataBuff()
{
	bool isFinReceived = false;
	if(DEBUG)
		printf("consumer trying to grab\n");
	pthread_mutex_lock(&the_mutex);
	if(DEBUG)
		printf("consumer grabbed mutex isFilled value : %d\n",isFilled);

	while (!isFilled)
	{
		pthread_cond_wait(&condc, &the_mutex);
	}

	if(DEBUG)
		printf("Printer thread acquired thread server_seq_num %d \n", server_seq_num);

	isFinReceived = popData();
	if(isFinReceived){
		printf("FIN Received\n");
	}
	isFilled = false;
	pthread_cond_signal(&condp);
	pthread_mutex_unlock(&the_mutex);
	return isFinReceived;
}

void* printData(void *ptr)
{
	uint32_t sleepTime = *(uint32_t *)ptr;
	uint32_t threadSleep=0;

	if(DEBUG)
		printf("Sleep time in thread %u\n", sleepTime);

	while(!printDataBuff())
	{
		threadSleep = (sleepTime * (-1 * (log((double)(rand()/(double)RAND_MAX)))));

		if(threadSleep < 100){
			threadSleep = 100;
		}else if(threadSleep > 1000){
			threadSleep = 1000;
		}

		if(DEBUG)
		{
			printf("Sleep time is %u and double %f\n", threadSleep, log((double)(rand()/(double)RAND_MAX)));
		}

		usleep(threadSleep*1000);
	}
	isPrinterExited = true;
	return NULL;
}

void pushData(struct dg_payload pload, int windowSize)
{
	if(front == NULL){
		front = buff_head;
	}

	if(rear == NULL){
		rear = buff_head;
	}else{
		rear = rear->next;
	}

	if((front == buff_head && rear->ind == windowSize - 1) || front == rear->next)
	{
		return;
	}

	memset(rear->buff, 0, PACKET_SIZE);
	strcpy(rear->buff, pload.buff);
	rear->seqNum = pload.seq_number;
	rear->type = pload.type;
	filled_circular_buffer_size++;
}

bool popData()
{
	if(DEBUG){
		printf("***********Entered the pop data**********\n");
	}
	while(front != rear || front -> type == FIN){

		if(front -> type == FIN)
		{

			if(DEBUG)
			{
				printf("Popped the FIN packet in printer thread\n");
				fflush(fp_output);
				fclose(fp_output);
				fclose(fp_output_seq);
			}

			return true;
		}

		if(!DEBUG)
		{
			printf("\nPrinting data packet sequence number is %d\n", front->seqNum);
			printf("%s", front->buff);
		}

		fflush(stdout);

		if(DEBUG)
		{
			fprintf(fp_output, "%s\n", front->buff);
			fprintf(fp_output_seq, "Printing Data packet sequence number is %d\n", front->seqNum);
			printf("Printing data packet sequence number is %d\n", front->seqNum);
		}

		filled_circular_buffer_size--; // Window size
		front = front -> next;
	}

	if(front == rear && front != NULL && rear != NULL)
	{
		if(!DEBUG)
		{
			printf("\nPrinting data packet sequence number is %d\n", front->seqNum);
			printf("%s", front->buff);
		}

		filled_circular_buffer_size--;
		front = NULL;
		rear = NULL;
		return false;
	}

	printf("\n");
	return false;
}
