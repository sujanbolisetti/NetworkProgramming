/*
 * common_utils.c
 *
 *  Created on: Nov 8, 2015
 *      Author: sujan
 */

#include "../src/usp.h"

#define concat(a,b) {a##b}

char *my_name;
bool isHostNameFilled = false;

struct port_entry port_entries[NUM_PORT_ENTRIES];

int port_num = 2;


void
msg_send(int sockfd, char* destIpAddress, int destPortNumber,
					char* message,int flag){

	char* msg_odr = malloc(sizeof(char)*1024);

	char *temp = msg_odr;

	int length=0;
	char temp_str[120];

	printf("%s\n",destIpAddress);

	strcpy(msg_odr,destIpAddress);

	sprintf(temp_str,"%d",destPortNumber);

	printf("%s\n",temp_str);

	length = strlen(destIpAddress);

	msg_odr[length] = '$';

	strcpy(msg_odr+length+1,temp_str);

	length += strlen(temp_str);

	msg_odr[length] = '$';

	strcpy(msg_odr+length+1,message);

	printf("%s\n",message);

	if(strlen(message)){
		length+=strlen(message);
	}else{
		length+=1;
	}

	msg_odr[length] = '$';

	memset(temp_str,'\0',sizeof(temp_str));

	sprintf(temp_str,"%d",flag);

	printf("%s\n",temp_str);

	strcpy(msg_odr+length+1,temp_str);

	printf("length %d\n",length);
	printf("complete string %s\n",temp);

	if(write(sockfd,temp,strlen(temp)) <  0){
		printf("write failed with error :%s\n",strerror(errno));
	}
}


struct reply_from_uds * msg_receive(int sockfd){

	struct reply_from_uds *reply = (struct reply_from_uds *)malloc(sizeof(struct reply_from_uds));
	char* msg_odr = (char *)malloc(sizeof(char)*1024);
	memset(reply, '\0', sizeof(struct reply_from_uds));
	struct sockaddr_un peerAddress;
	bzero(&peerAddress,sizeof(peerAddress));
	int port_number;
	int len = sizeof(struct sockaddr_un);

	//bzero(reply,);
	if(recvfrom(sockfd, msg_odr, MSG_LEN, 0, (SA*)&peerAddress, &len) > 0){
		printf("messaged received %s\n",msg_odr);
	}


	printf("path_name of the client %s\n",peerAddress.sun_path);

	port_number = add_port_entry(peerAddress.sun_path);


	int length = strlen(msg_odr);
	int i=0,k=0,dollarCount = 0,start_index=0;
	char tmp_str[120];

	while(i < length){

		if(msg_odr[i] == '$'){
			dollarCount++;
			i++;
		}else{
//			if(DEBUG)
				//printf("character %c\n",msg_odr[i]);

			tmp_str[k] = msg_odr[i];
			i++;
			k++;
			continue;
		}

		switch(dollarCount){

		case 1:
			if(reply->canonical_ipAddress == NULL){
				tmp_str[k] = '\0';

				//printf("ip-address %s\n",tmp_str);
				reply->canonical_ipAddress = malloc(sizeof(char)*128);
				strcpy(reply->canonical_ipAddress,tmp_str);
				memset(tmp_str,'\0',sizeof(tmp_str));
				k=0;
			}
			break;
		case 3:
			if(reply->msg_received ==  NULL){
				tmp_str[k] = '\0';
				reply->msg_received = malloc(sizeof(char)*1024);

				if(!tmp_str[0]){

					if(DEBUG)
						printf("Entered into populating the portNumber in the msg field\n");

					char port_str[10];
					sprintf(port_str,"%d",port_number);
					strcpy(reply->msg_received,port_str);
				}
				else{
					strcpy(reply->msg_received,tmp_str);
				}
				memset(tmp_str,'\0',sizeof(tmp_str));
				k=0;
			}
			break;
		case 2:
			//if(reply->portNumber ==  NULL)
			{
				tmp_str[k] = '\0';
				int portNumber = atoi(tmp_str);
				//printf("port number %d\n",portNumber);
				reply->portNumber = portNumber;
				memset(tmp_str,'\0',sizeof(tmp_str));
				k=0;
			}
			break;
		}

	}

	reply->flag = atoi(tmp_str);

	if(!DEBUG)
		printf("reply parameters : %s %d %s	%d\n",reply->canonical_ipAddress,(reply->portNumber),reply->msg_received,reply->flag);

	return reply;
}


char* Gethostname(){

	if(!isHostNameFilled){
		my_name = (char *)malloc(20*sizeof(char));
		gethostname(my_name,sizeof(my_name));
		isHostNameFilled = true;
	}
	return my_name;
}

char*
Gethostbyname(char *my_name){

	struct hostent *he;
	struct in_addr **ipaddr_list;

	if((he  = gethostbyname(my_name)) !=NULL){
		ipaddr_list = (struct in_addr **)he->h_addr_list;
		/**
		 *  In case of the hostname we are taking only the
		 *  first address as that is the canonical IpAddress
		 *  we are considering.
		 */
		return inet_ntoa(*ipaddr_list[0]);
	}else{
		printf("gethostbyname error: %s for hostname: %s\n",hstrerror(h_errno),my_name);
		exit(-1);
	}
}

void fill_inf_mac_addr_map(struct hwa_info	*hw_head, char inf_mac_addr_map[MAX_INTERFACES][ETH_ALEN])
{
	struct hwa_info	*temp = hw_head;
	char   *ptr;
	int i =0;
	while(temp != NULL)
	{
		if(DEBUG)
		{
			printf("Interface index %d\t",temp->if_index);
			printHWADDR(temp->if_haddr);

		}
		memcpy((void *)inf_mac_addr_map[temp->if_index],(void *)temp->if_haddr,ETH_ALEN);
		if(DEBUG){
			printf("In fill In Map");
			printHWADDR(inf_mac_addr_map[temp->if_index]);
		}
		temp = temp -> hwa_next;
	}
}

void convertToNetworkOrder(struct odr_hdr *hdr){

	printf("came into network order conversion\n");

	hdr->broadcast_id = htonl(hdr->broadcast_id);
	hdr->hop_count = htons(hdr->hop_count);
	hdr->pkt_type = htons(hdr->pkt_type);
	hdr->force_route_dcvry = htons(hdr->force_route_dcvry);
	hdr->rreply_sent = htons(hdr->rreply_sent);
}

void convertToHostOrder(struct odr_hdr *hdr){

	printf("came into host order conversion\n");

	hdr->broadcast_id = ntohl(hdr->broadcast_id);
	hdr->hop_count = ntohs(hdr->hop_count);
	hdr->pkt_type = ntohs(hdr->pkt_type);
	hdr->force_route_dcvry = ntohs(hdr->force_route_dcvry);
	hdr->rreply_sent = ntohs(hdr->rreply_sent);
}


void printHWADDR(char *hw_addr){

	printf(" HW addr = ");
	char *ptr = hw_addr;
	int i = IF_HADDR;
	do {
		printf("%.2x%s", *ptr++ & 0xff, (i == 1) ? " " : ":");
	} while (--i > 0);

	printf("\n");
}

int add_port_entry(char* path_name)
{

	int i = 0;
	// Checking the path_name is present or not.
	for(i=0;i < NUM_PORT_ENTRIES;i++){

		if(!strcmp(path_name,port_entries[i].path_name)){
			return i;
		}
	}

	struct port_entry port_entr;

	port_num++;
	port_entr.port_num = port_num;
	strcpy(port_entr.path_name,path_name);
	// have to insert a time stamp.
	return port_num;
}

void build_port_entries()
{
	struct port_entry port_entr;

	port_entr.port_num = 0;
	strcpy(port_entr.path_name, SERVER_WELL_KNOWN_PATH_NAME);
	port_entries[SERVER_PORT] = port_entr;

	bzero(&port_entr, sizeof(port_entr));

	port_entr.port_num = 1;
	strcpy(port_entr.path_name, ODR_PATH_NAME);
	port_entries[ODR_PORT] = port_entr;
}

void Sendto(int pf_sockfd, char* buffer, struct sockaddr_ll addr_ll,char *sendType)
{
	if(sendto(pf_sockfd,buffer,EHTR_FRAME_SIZE,0,
					(SA *)&addr_ll,sizeof(addr_ll)) < 0){
		printf("Error in %s send error type  %s\n",sendType, strerror(errno));
		exit(-1);
	}
}
