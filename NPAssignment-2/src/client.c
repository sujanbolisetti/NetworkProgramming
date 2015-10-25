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
int server_seq_num = -1;

// Thread conditions
pthread_mutex_t the_mutex;
pthread_cond_t condc, condp;
bool isFilled = false;
bool isPrinterExited = false;

struct Node *front = NULL, *buff_head = NULL, *rear = NULL;
int filled_circular_buffer_size = 0;

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
	long max = 0,tempMax=0;
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
	uint32_t seqNum=0;
	float prob;
	int randomSeed;
	int required_seq_num = -1;
	uint32_t sleepPrinterInSecs = 0;

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

	setRandomSeed(randomSeed);

	struct dg_payload data_temp_buff[windowSize];
	WINDOW_SIZE = windowSize;

	initializeTempBuff(data_temp_buff, windowSize);
	buff_head = BuildCircularLinkedList(windowSize);
	printList(buff_head);
	front = buff_head;
	rear = buff_head;

	if(buff_head != NULL){
		printf("Buff was not null\n");
	}

	// Create the threads
	pthread_create(&printer, NULL, printData, &sleepPrinterInSecs);

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

	printf("Client is running on IP-Address :%s with Port Number :%d\n",clientIP,ntohs(IPClient.sin_port));

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
		printf("New Action Set\n");
		sigaction(SIGALRM,&new_action,&old_action);
	}

	memset(&send_pload,0,sizeof(send_pload));
	strcpy(send_pload.buff,fileName);
	send_pload.type = PAYLOAD;
	send_pload.seq_number = seqNum++;
	send_pload.windowSize = windowSize;

	sendagain:
		send_pload.ts = rtt_ts(&rttinfo);
		sendto(sockfd,(void *)&send_pload,sizeof(send_pload),0,NULL,0);

		printf("wait time :%d\n",rtt_start(&rttinfo));

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
	memset(&recv_pload,0,sizeof(recv_pload));
	Recvfrom(sockfd,&recv_pload,sizeof(recv_pload),0,NULL,NULL);
	alarm(0);

	int serverChildPortNumber = atoi(recv_pload.buff);

	printf("New Ephemeral PortNumber of the Server Child %d\n",serverChildPortNumber);
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

	uint32_t ts =  recv_pload.ts;
	sendAcknowledgement(sockfd, ts, recv_pload.seq_number+1, WINDOW_SIZE, ACK);
	printf("Has sent the ack :%d for the port number\n", recv_pload.seq_number+1);

	for(;;){

		printf("Client is waiting for data packet\n");
		memset(&recv_pload,0,sizeof(recv_pload));
		Recvfrom(sockfd,&recv_pload,sizeof(recv_pload),0,NULL,NULL);
		printf("Client has received the data packet %d\n", recv_pload.seq_number);

		// store in other list and send ack for required packet
		printf("Window size: %d\n", getWindowSize(windowSize, getUsedTempBuffSize(data_temp_buff, windowSize)));
		if(getWindowSize(windowSize, getUsedTempBuffSize(data_temp_buff, windowSize)) == 0)
		{
			printf("Unable to store packet. Receive buffer is full\n");
			continue;
		}

		// drop packet if packet received is less than expected seq number
		if(server_seq_num != -1 && server_seq_num >= recv_pload.seq_number)
		{
			sendAcknowledgement(sockfd, ts, server_seq_num + 1, getWindowSize(windowSize, getUsedTempBuffSize(data_temp_buff, WINDOW_SIZE)), ACK);
			printf("Discarding seqNum less than : %d  receive seqNum %d\n", server_seq_num + 1,recv_pload.seq_number);
			continue;
		}

		if(recv_pload.type == WINDOW_PROBE){
			printf("Sending the acknowledgment for window probe segment \n");
			sendAcknowledgement(sockfd, recv_pload.ts, INT_MAX,
								getWindowSize(windowSize, getUsedTempBuffSize(data_temp_buff, WINDOW_SIZE)), WINDOW_PROBE);
			continue;
		}

//		if(recv_pload.type == FIN)
//		{
//			if(required_seq_num == -1 && server_seq_num + 1 == recv_pload.seq_number){
//				printf("Received FIN from server\n");
//				sendAcknowledgement(sockfd, recv_pload.ts, recv_pload.seq_number+1,
//						getWindowSize(windowSize, getUsedTempBuffSize(data_temp_buff, WINDOW_SIZE)), FIN_ACK);
//				break;
//			}
//			else
//			{
//				continue;
//			}
//		}

		int type = ACK;
		//printf("Probability of dropping packet : %f\n", prob);
		if(!is_in_limits(prob))
		{
			if(required_seq_num == recv_pload.seq_number || required_seq_num == -1)
			{
				//printf("Producer trying to acquire lock\n");
				pthread_mutex_lock(&the_mutex);
				isFilled = true;
				printf("Producer thread grabbed the lock\n");
				pushData(recv_pload, WINDOW_SIZE);
				printf("Pushed the data for seq num %d\n",recv_pload.seq_number);

				int ts = recv_pload.ts;
				int seq = recv_pload.seq_number + 1;
				if(required_seq_num != -1)
				{
					while(getPacket(&recv_pload, data_temp_buff, seq, WINDOW_SIZE))
					{
						printf("Packet present in data_temp_buff %d type %d\n", recv_pload.seq_number, recv_pload.type);
						pushData(recv_pload,WINDOW_SIZE);
						printf("Pushed the data for seq num %d\n",recv_pload.seq_number);
						if(recv_pload.type == FIN){
							type = FIN_ACK;
							printf("Received FIN from server\n");
							break;
						}
						memset(&recv_pload, 0, sizeof(recv_pload));
						seq++;
					}

					// checking if any new packets in temporary buffer
					int new;
					if((new = isNewPacketPresent(data_temp_buff, WINDOW_SIZE)))
					{
						required_seq_num = seq;
					}
					else
					{
						required_seq_num = -1;
					}
				}
				sendAcknowledgement(sockfd, ts, seq, getWindowSize(windowSize, getUsedTempBuffSize(data_temp_buff, WINDOW_SIZE)), type);
				if(type == FIN_ACK)
				{
					break;
				}
			}
			else
			{
				storePacket(recv_pload, data_temp_buff, WINDOW_SIZE);
				printf("Packet stored in data_temp_buff %d\n", recv_pload.seq_number);
				sendAcknowledgement(sockfd, ts, required_seq_num, getWindowSize(windowSize, getUsedTempBuffSize(data_temp_buff, WINDOW_SIZE)), ACK);
			}

			//printf("Producer started releasing locks\n");
			pthread_cond_signal(&condc);
			pthread_mutex_unlock(&the_mutex);
			printf("Producer released locks\n");
		}
		else
		{
			printf("Intentionally dropping packet %d\n", recv_pload.seq_number);
			if(required_seq_num == -1)
			{
				required_seq_num = recv_pload.seq_number;
			}
		}
	}

	printf("Waiting for printer thread to exit\n");
	pthread_join(printer, NULL);
	printf("Done with the file transfer\n");
	memset(&recv_pload,0,sizeof(recv_pload));
	recvfrom(sockfd,&recv_pload,sizeof(recv_pload),0,NULL,NULL);
	printf("Server Child Closed\n");
	close(sockfd);
}

void
sig_alrm(int signo){
	printf("received the sig-alarm\n");
	siglongjmp(jmpbuf,1);
}

void
sendAcknowledgement(int sockfd, uint32_t ts, uint32_t seq, uint32_t windowSize, uint32_t type)
{
	struct dg_payload send_pload;
	memset(&send_pload,0,sizeof(send_pload));
	send_pload.ts=ts;
	send_pload.ack = seq;
	send_pload.windowSize = windowSize;
	send_pload.type = type;
	server_seq_num = send_pload.ack-1;
	sendto(sockfd,(void *)&send_pload,sizeof(send_pload),0,NULL,0);
	printf("Sent ack with seq num : %d of type %d with window size %d \n", send_pload.ack, type, send_pload.windowSize);
}
//
//int getPacket(struct dg_payload *pload, struct dg_payload *data_temp_buff, int seq_num, int windowSize)
//{
//	int i;
//	for(i = 0; i < windowSize; i++)
//	{
//		if(data_temp_buff[i].seq_number == seq_num)
//		{
//			pload->ack = data_temp_buff[i].ack;
//			strcpy(pload->buff, data_temp_buff[i].buff);
//			pload->type = data_temp_buff[i].type;
//			pload->seq_number = data_temp_buff[i].seq_number;
//			pload->ts = data_temp_buff[i].ts;
//			pload->windowSize = data_temp_buff[i].windowSize;
//			data_temp_buff[i].type = USED;
//			return 1;
//		}
//	}
//	return 0;
//}
//
//int storePacket(struct dg_payload pload, struct dg_payload* data_temp_buff, int windowSize)
//{
//	int i;
//	bool isPresent = false;
//	for(i = 0; i < windowSize; i++)
//	{
//		if(pload.seq_number == data_temp_buff[i].seq_number)
//		{
//			isPresent = true;
//			printf("Already present in buffer %d \n", data_temp_buff[i].seq_number);
//			break;
//		}
//	}
//
//	for(i = 0; i < windowSize && !isPresent; i++)
//	{
//		if(data_temp_buff[i].type == USED)
//		{
//			data_temp_buff[i] = pload;
//			printf("Stored buffer %d \n", data_temp_buff[i].seq_number);
//			return 1;
//		}
//	}
//
//	return 0;
//}
//
//int isNewPacketPresent(struct dg_payload* data_temp_buff, int windowSize)
//{
//	int i;
//	for(i = 0; i < windowSize; i++)
//	{
//		if(data_temp_buff[i].type != USED)
//		{
//			return 1;
//		}
//	}
//	return 0;
//}
//
//void initializeTempBuff(struct dg_payload* data_temp_buff, int windowSize)
//{
//	int i;
//	for(i = 0; i < windowSize; i++)
//	{
//		data_temp_buff[i].type = USED;
//	}
//}
//
//int getUsedTempBuffSize(struct dg_payload* data_temp_buff, int windowSize)
//{
//	int count = 0;
//	int i;
//	for(i = 0; i < windowSize; i++)
//	{
//		if(data_temp_buff[i].type != USED)
//		{
//			count++;
//		}
//	}
//	return count;
//}

int getWindowSize(uint32_t windowSize, int temp_buff_size)
{
	int size = windowSize - filled_circular_buffer_size - temp_buff_size-1;
	return size < 0 ? 0 : size;
}

bool printDataBuff()
{
	bool isFinReceived = false;
	pthread_mutex_lock(&the_mutex);
	while (!isFilled)
	{
		pthread_cond_wait(&condc, &the_mutex);
	}
	printf("Entered printer thread with bool %d\n", isFilled);
	printf("Printer thread acquired thread server_seq_num %d \n", server_seq_num);
	isFinReceived = popData();
	isFilled = false;
	pthread_cond_signal(&condp);
	pthread_mutex_unlock(&the_mutex);
	return isFinReceived;
}

void* printData(void *ptr)
{
	uint32_t sleepTime = *(uint32_t *)ptr;
	while(!printDataBuff())
	{
		usleep(sleepTime*1000);
	}
	isPrinterExited = true;
	printf("Printer thread exited\n");
	return NULL;
}

void pushData(struct dg_payload pload, int windowSize)
{
	if((front == buff_head && rear->ind == windowSize - 1) || front == rear->next)
	{
		return;
	}

	memset(rear->buff, 0, PACKET_SIZE);
	strcpy(rear->buff, pload.buff);
	rear->seqNum = pload.seq_number;
	rear->type = pload.type;
	rear = rear->next;
	filled_circular_buffer_size++;
}

bool popData()
{
	if(front == rear)
	{
		return false;
	}

	while(front != rear)
	{
		//printf("Data packet seq number is %d and data is %s\n", front->seqNum, front->buff);
		printf("Printing Data packet seq number is %d\n", front->seqNum);
		front = front -> next;
		filled_circular_buffer_size--; // Window size
		if(front -> type == FIN)
		{
			return true;
		}
	}
	return false;
}
