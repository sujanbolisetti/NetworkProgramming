/*
 * io_utils.c
 *
 *  Created on: Oct 10, 2015
 *      Author: sujan
 */

#include "../src/usp.h"
#include "../src/unpifi.h"

void Fscanf(FILE *fp, char *format, void *data){

	if(fscanf(fp,format,data) < 0){
		printf("Error in reading the file : %s",strerror(errno));
	}
}

void getClientIPAddress(struct sockaddr_in *clientAddr,struct sockaddr_in *networkAddr, struct sockaddr_in *serverAddr,
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

			if(if_temp->ifi_flags & IFF_UP){

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
					getClientIPAddress(addr,netAddr,serverAddr,IPClient,maxMatch);
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

		temp->next = NULL;

		return head;
}

void printInterfaceDetails(struct binded_sock_info *head){

	struct binded_sock_info *temp = head;

	while(temp!=NULL){

			printf("IP Adress %s\n",temp->ip_address);
			printf("Network Mask %s\n",temp->network_mask);
			printf("Subet Address %s\n",temp->subnet_adress);
			printf("----------------------------------------\n");
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


