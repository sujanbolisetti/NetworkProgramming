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
struct connected_client_address *head_client_address=NULL;

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


	if(argc < 2){
		printf("Kindly enter the input file name\n");
		exit(0);
	}

	/**
	 *  Reading the Input arguments from the input file.
	 */
	fp=fopen(argv[1],"r");
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
						continue;
				}

				printf("Came inside socket\n");

				if((pid = fork()) == 0){
					printf("forked a child and handled client connection\n");
					doFileTransfer(temp,IPClient);

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
						temp_client_address->next = NULL;
					}
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
	char IPServer[128];
	struct sockaddr_in serverAddr;
	int length = sizeof(serverAddr);
	struct sockaddr_in networkMaskAddr;
	int optval=1;
	unsigned long matchNumber=0;
	struct dg_payload pload;

	inet_pton(AF_INET,sock_info->ip_address,&serverAddr.sin_addr);

	serverAddr.sin_port = 0;
	serverAddr.sin_family = AF_INET;

	Bind(conn_sockfd,(SA *)&serverAddr,sizeof(serverAddr));

	inet_pton(AF_INET,sock_info->network_mask,&networkMaskAddr.sin_addr);

	getClientIPAddress(&IPClient,&networkMaskAddr,&serverAddr,NULL,&matchNumber);

	if(matchNumber > 0){
		printf("Client Host is Local\n");
		setsockopt(conn_sockfd,SOL_SOCKET,MSG_DONTROUTE,&optval,sizeof(int));
	}

	Getsockname(conn_sockfd,(SA *)&serverAddr,&length);
	memset(&pload,0,sizeof(pload));
	pload.portNumber = htons(serverAddr.sin_port);
	pload.seq_number = get_seq_num();
	pload.type = PAYLOAD;

	sendto(sock_info->sockfd,(void *)&pload,sizeof(pload),0,(SA *)&IPClient,sizeof(IPClient));

	if(connect(conn_sockfd,(SA *)&IPClient,sizeof(IPClient)) < 0){
		printf("Connection Error :%s",strerror(errno));
	}

	inet_ntop(AF_INET,&serverAddr.sin_addr,IPServer,sizeof(IPServer));

	printf("Server Child is running on IP-Address :%s with port number :%d\n",IPServer,ntohs(serverAddr.sin_port));

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
	close(conn_sockfd);
	printf("Exiting child\n");
	exit(0);
}

int get_seq_num(){
	seq_num++;
	return seq_num;
}

/**
 * Handler for SIGCHLD signal.
 */
void sigchild_handler(int signum){

	int stat;
	pid_t pid;

	printf("Came into signal handler\n");
//	wait(&pid);
	while((pid = waitpid(-1,&stat,WNOHANG)) > 0){
		printf("Child Terminated with PID :%d\n",pid);
		removeClientAddrFromList(pid,&head_client_address);
	}
}

