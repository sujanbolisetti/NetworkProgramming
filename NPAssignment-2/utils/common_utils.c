/*
 * io_utils.c
 *
 *  Created on: Oct 10, 2015
 *      Authors: sujan, sidhartha
 */

#include "../src/usp.h"
#include "../src/unpifi.h"


void Fscanf(FILE *fp, char *format, void *data){

	if(fscanf(fp,format,data) < 0){
		printf("Error in reading the file : %s",strerror(errno));
		exit(0);
	}
}

bool isServerInSameHost(struct binded_sock_info *head,char *IPServer){

	struct binded_sock_info *temp = head;

	while(temp!=NULL){

		if(strcmp(IPServer,temp->ip_address)==0){
			return true;
		}
		temp= temp->next;
	}
	return false;
}

void isClientLocal(struct sockaddr_in *clientAddr,struct sockaddr_in *networkAddr, struct sockaddr_in *serverAddr,
			struct sockaddr_in *IPClient,unsigned long *maxMatch){

	unsigned long clientNetworkPortion = clientAddr->sin_addr.s_addr & networkAddr->sin_addr.s_addr;

	unsigned long serverNetworkPortion = serverAddr->sin_addr.s_addr & networkAddr->sin_addr.s_addr;

	if(clientNetworkPortion == serverNetworkPortion){
		if(clientNetworkPortion > *maxMatch){
			if(IPClient!=NULL){
				IPClient->sin_addr.s_addr = clientAddr->sin_addr.s_addr;
			}
			*maxMatch = clientNetworkPortion;
		}
	}
}

struct binded_sock_info* getInterfaces(int portNumber,unsigned long *maxMatch,
			struct sockaddr_in *serverAddr,struct sockaddr_in *IPClient){

	struct binded_sock_info *head=NULL, *temp=NULL;
	struct ifi_info *if_head, *if_temp;
	int sockfd;
	struct sockaddr_in *addr;

	for(if_head = if_temp = Get_ifi_info(AF_INET,1);
					if_temp!=NULL;if_temp = if_temp->ifi_next){

			if((if_temp->ifi_flags & IFF_UP)){
					//&& !((if_temp->ifi_flags & IFF_BROADCAST)
					//|| (if_temp->ifi_flags & IFF_MULTICAST))) {

				addr = (struct sockaddr_in *) if_temp->ifi_addr;
				addr->sin_family = AF_INET;
				addr->sin_port = htons(portNumber);

				struct binded_sock_info *bsock_info = (struct binded_sock_info *)malloc(sizeof(struct binded_sock_info));

				if(head == NULL){

					head = bsock_info;
					temp = head;
				}else{

					temp->next = bsock_info;
					temp =  bsock_info;
				}

				inet_ntop(AF_INET,&addr->sin_addr,bsock_info->ip_address,sizeof(bsock_info->ip_address));

				struct sockaddr_in *netAddr = (struct sockaddr_in *)if_temp->ifi_ntmaddr;

				if(serverAddr!=NULL){
					isClientLocal(addr,netAddr,serverAddr,IPClient,maxMatch);
				}

				inet_ntop(AF_INET,&netAddr->sin_addr,bsock_info->network_mask,sizeof(bsock_info->network_mask));

				struct in_addr subnet_addr;
				subnet_addr.s_addr =  netAddr->sin_addr.s_addr & addr->sin_addr.s_addr;

				inet_ntop(AF_INET,&subnet_addr,bsock_info->subnet_adress,sizeof(bsock_info->subnet_adress));

				/**
				 *  Maximum Matching is required on Client side to identify whether
				 *  the server is in the same network. If it is NULL then it is server
				 *  hence we can bind the socket.
				 */
				if(maxMatch==NULL){
					sockfd = Socket(AF_INET,SOCK_DGRAM,0);
					bsock_info->sockfd = sockfd;
					Bind(sockfd, (SA *)addr,sizeof(struct sockaddr_in));
				}

			}
		}

		if(temp!=NULL)
			temp->next = NULL;

		return head;
}

void printInterfaceDetails(struct binded_sock_info *head){

	struct binded_sock_info *temp = head;

	while(temp!=NULL){

			printf("IP Adress %s\n",temp->ip_address);
			printf("Network Mask %s\n",temp->network_mask);
			printf("Subet Address %s\n",temp->subnet_adress);
			printf("--------------------------------------------------\n");
			temp=temp->next;
		}
}

int getMaxFD(struct binded_sock_info *head){

	struct binded_sock_info *temp = head;
	int maxfd = 0;

	while(temp!=NULL){

		if(temp->sockfd > maxfd){
			maxfd = temp->sockfd;
		}
		temp = temp->next;
	}
	return maxfd;
}

bool isClientConnected(struct connected_client_address *head,char *ipAddressSocket){

	struct connected_client_address *temp = head;
	bool addressExists =  false;

	while(temp!=NULL){
		if(strcmp(temp->client_sockaddress,ipAddressSocket) == 0){
			addressExists = true;
		}
		temp = temp->next;
	}
	return addressExists;
}


char* getSocketAddress(struct sockaddr_in IPClient){

	char portNumber[24];
	char *ipAddressSocket = (char *)malloc(10*sizeof(char));

	inet_ntop(AF_INET,&IPClient.sin_addr,ipAddressSocket,128);

	sprintf(portNumber,"%d",ntohs(IPClient.sin_port));

	strcat(ipAddressSocket,":");
	strcat(ipAddressSocket,portNumber);

	return ipAddressSocket;
}

void removeClientAddrFromList(int child_pid, struct connected_client_address **head){
	char sChild_pid[256];
	sprintf(sChild_pid,"%d",child_pid);
	struct connected_client_address *prev = *head;
	struct connected_client_address *temp = *head;
	int present = 0;

	if(strcmp(temp->child_pid, sChild_pid) == 0)
	{
			*head = (*head) -> next;
			free(prev);
			return;
	}

	while(temp != NULL)
	{
			if(strcmp(temp->child_pid, sChild_pid) == 0)
			{
					prev -> next = temp -> next;
					free(temp);
					return;
			}
			prev = temp;
			temp = temp -> next;
	}
}

struct Node * BuildCircularLinkedList(int size){

	struct Node *new_node,*temp,*head=NULL;
	int i;
	if(DEBUG)
		printf("came into BuildCircularLinkedList with size :%d\n",size);

	for(i=0;i<size;i++){
		new_node = (struct Node *)malloc(sizeof(struct Node));
		new_node->ack =0;
		new_node->ind = i;

		if(head == NULL){
			head = new_node;
			temp = new_node;
		}else{
			temp->next = new_node;
			temp = new_node;
		}
	}

	if(size>0){
		temp->next = head;
	}

	if(DEBUG)
		printList(head);

	return head;
}

void printList(struct Node *head){

	printf("Printing circular list : ");
	struct Node *temp = head;
	printf("%d ", temp->ind);
	temp = temp -> next;

	if(temp == NULL){
		return;
	}
	else{
		while(temp != head)
		{
			printf("%d ", temp->ind);
			temp = temp->next;
		}
	}
	printf("\n");
}


/**
 *  Helper method to delete the circular linked list.
 */
void deleteCircularLinkedList(struct Node *head){

	struct Node *temp = head;
	struct Node *next;

	if(DEBUG)
		printf("Freeing Circular Linked List\n");

	while(temp->next!=head){

		next = temp->next;
		free(temp);
		temp = next;
	}

	free(temp);

}



int count=0;
struct Node *rearNode = NULL;

void setRearNode(struct Node *head){
	rearNode = head;
}

bool populateDataList(FILE **fp,struct Node* ackNode,bool isFull){

	char buff[PACKET_SIZE];
	int n;

	while(rearNode->next != ackNode){

		again:
			count++;
			memset(buff,0,PACKET_SIZE);
			if((n=fread(buff,1,PACKET_SIZE-1,*fp)) < 0){
				if(errno == EINTR){
					//printf("came into EINTR\n");
					goto again;
				}else{
					printf("Fread error :%s\n",strerror(errno));
					break;
				}
			}

			if(n>0){
				buff[n] = '\0';
				rearNode->type = PAYLOAD;
				memset(rearNode->buff,0,sizeof(rearNode->buff));
				strcpy(rearNode->buff,buff);
				if(DEBUG)
					printf("%s\n",rearNode->buff);
				rearNode->ack = 0;
				rearNode = rearNode->next;
			}

			if(feof(*fp)){

				if(DEBUG){
					printf("**************EOF reached\n");
					printf("value of n : %d\n",n);
				}

				memset(rearNode->buff,0,sizeof(rearNode->buff));
				rearNode->type = FIN;

				if(DEBUG){
					printf("Changed to FIN %s\n",buff);
					printf("********** value of count :%d #######\n",count);
				}
				return true;
			}

			if(DEBUG){
				printf("value of n : %d\n",n);
			}
	}
	return false;
}

struct dg_payload convertToHostOrder(struct dg_payload pload){

	pload.ack =  ntohl(pload.ack);
	pload.seq_number = ntohl(pload.seq_number);
	pload.ts = ntohl(pload.ts);
	pload.type = ntohs(pload.type);
	pload.windowSize = ntohs(pload.windowSize);

	return pload;
}

struct dg_payload convertToNetworkOrder(struct dg_payload pload){

	pload.ack =  htonl(pload.ack);
	pload.seq_number = htonl(pload.seq_number);
	pload.ts = htonl(pload.ts);
	pload.type = htons(pload.type);
	pload.windowSize = htons(pload.windowSize);

	return pload;
}



