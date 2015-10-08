/*
 * client.c
 *
 *  Created on: Oct 7, 2015
 *      Author: sujan
 */
#include "usp.h"
#include  "unpifi.h"

int main(int argc,char **argv){

	FILE *fp;
	char IPServer[128];
	int portNumber,sockfd;
	char fileName[120];
	struct sockaddr_in *clientAddr;
	struct ifi_info *if_head,*if_temp;
	char clientIP[128];
	struct binded_sock_info *head, *temp;
	long networkMasks[1024];
	long max = 0,tempMax=0;
	struct sockaddr_in serverAddr,IPClient;
	int length = 0;

	fp=fopen("client.in","r");
	fscanf(fp,"%s",IPServer);
	fscanf(fp,"%d",&portNumber);
	fscanf(fp,"%s",fileName);

	inet_pton(AF_INET,IPServer,&serverAddr.sin_addr);
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(portNumber);
	IPClient.sin_family = AF_INET;
	IPClient.sin_port = htons(0);


	for(if_head = if_temp = Get_ifi_info(AF_INET,1);
						if_temp!=NULL;if_temp = if_temp->ifi_next){

		clientAddr = (struct sockaddr_in *) if_temp->ifi_addr;

		struct binded_sock_info *bsock_info = (struct binded_sock_info *)malloc(sizeof(struct binded_sock_info));

		if(head == NULL){
			head = bsock_info;
			temp = head;
		}else{
			temp->next = bsock_info;
			temp =  bsock_info;
		}

		inet_ntop(AF_INET,&clientAddr->sin_addr,bsock_info->ip_address,sizeof(bsock_info->ip_address));

		struct sockaddr_in *netAddr = (struct sockaddr_in *)if_temp->ifi_ntmaddr;

		if((tempMax= (netAddr->sin_addr.s_addr & serverAddr.sin_addr.s_addr)) > 0){

		if(tempMax > max){
			max = tempMax;
			IPClient.sin_addr = clientAddr->sin_addr;
			}
		}

		inet_ntop(AF_INET,&netAddr->sin_addr,bsock_info->network_mask,sizeof(bsock_info->network_mask));
	}

	temp->next = NULL;

	temp = head;

	while(temp!=NULL){
		printf("IP Adress %s\n",temp->ip_address);
		printf("Network Mask %s\n",temp->network_mask);
		temp=temp->next;
	}

	if(strcmp(IPServer,LOOPBACK_ADDRESS) == 0){
		inet_pton(AF_INET,IPServer,&IPClient.sin_addr);
		printf("Address %s\n",IPServer);
	}

	sockfd = Socket(AF_INET,SOCK_DGRAM,0);

	Bind(sockfd,(SA *)&IPClient,sizeof(IPClient));

	getsockname();
}





