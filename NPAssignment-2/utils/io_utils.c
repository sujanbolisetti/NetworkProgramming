/*
 * io_utils.c
 *
 *  Created on: Oct 10, 2015
 *       Authors: sujan, sidhartha
 */

#include "../src/usp.h"

void Fscanf(FILE *fp, char *format, void *data){

	if(fscanf(fp,format,data) < 0){
		printf("Error in reading the file : %s",strerror(errno));
	}
}


unsigned long getClientIPAddress(struct sockaddr_in *clientAddr,struct sockaddr_in *networkAddr, struct sockaddr_in *serverAddr,
			struct sockaddr_in *IPClient,unsigned long maxMatch){

	unsigned long clientNetworkPortion = clientAddr->sin_addr.s_addr & networkAddr->sin_addr.s_addr;

	unsigned long serverNetworkPortion = serverAddr->sin_addr.s_addr & networkAddr->sin_addr.s_addr;

	if(clientNetworkPortion == serverNetworkPortion){
		if(clientNetworkPortion > maxMatch){
			if(IPClient!=NULL){
				IPClient->sin_addr.s_addr = clientAddr->sin_addr.s_addr;
			}
			maxMatch = clientNetworkPortion;
		}
	}
	return maxMatch;
}

void removeClientFromList(int child_pid, struct connected_client_address **head)
{
	char sChild_pid[256];
	sprintf(sChild_pid,"%d",child_pid);
	struct connected_client_address *prev = *head;
	struct connected_client_address *temp = (*head) -> next;
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
			}
			prev = temp;
			temp = temp -> next;
	}
}
