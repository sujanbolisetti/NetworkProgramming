/*
 * server.c
 *
 *  Created on: Oct 4, 2015
 *      Author: sujan
 */

#include "usp.h"
#include  "unpifi.h"

int main(int argc, char **argv){

	FILE *fp;
	int portNumber,slidingWindowSize;
	int sockfd;
	struct ifi_info *if_head, *if_temp;
	struct sockaddr_in *servaddr;
	struct sockaddr *sa;
	struct binded_sock_info *head, *temp;

	fp=fopen("server.in","r");
	fscanf(fp,"%d",&portNumber);
	fscanf(fp,"%d",&slidingWindowSize);

	printf("%d %d\n",portNumber,slidingWindowSize);

	for(if_head = if_temp = Get_ifi_info(AF_INET,1);
				if_temp!=NULL;if_temp = if_temp->ifi_next){

			sockfd = Socket(AF_INET,SOCK_DGRAM,0);
			servaddr = (struct sockaddr_in *) if_temp->ifi_addr;
			servaddr->sin_family = AF_INET;
			servaddr->sin_port = htons(portNumber);

			struct binded_sock_info *bsock_info = (struct binded_sock_info *)malloc(sizeof(struct binded_sock_info));

			if(head == NULL){

				head = bsock_info;
				temp = head;
			}else{

				temp->next = bsock_info;
				temp =  bsock_info;
			}

			bsock_info->sockfd = sockfd;

			inet_ntop(AF_INET,&servaddr->sin_addr,bsock_info->ip_address,sizeof(bsock_info->ip_address));

			struct sockaddr_in *netAddr = (struct sockaddr_in *)if_temp->ifi_ntmaddr;

			inet_ntop(AF_INET,&netAddr->sin_addr,bsock_info->network_mask,sizeof(bsock_info->network_mask));

			Bind(sockfd, (SA *)servaddr,sizeof(*servaddr));

			printf("presentation address %s\n",bsock_info->ip_address);

			printf("presentation address %s\n",bsock_info->ip_address);

	}

	temp->next = NULL;

	temp = head;

	while(temp!=NULL){

		printf("IP Adress %s\n",temp->ip_address);
		printf("Network Mask %s\n",temp->network_mask);
		temp=temp->next;
	}

}



