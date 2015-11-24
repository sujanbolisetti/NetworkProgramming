/*
 * common_utils.c
 *
 *  Created on: Nov 8, 2015
 *      Author: sujan, siddhu
 */

#include "../src/usp.h"

char *my_name;
bool isHostNameFilled = false;

struct port_entry port_entries[NUM_PORT_ENTRIES];

int port_num = 2;

void
msg_send(int sockfd, char* destIpAddress, int destPortNumber,
					char* message,int flag){

	char* msg_odr = build_msg_odr(sockfd, destIpAddress, destPortNumber, message, flag);

	if(write(sockfd,msg_odr,strlen(msg_odr)) <  0){
		printf("ODR at node :%s , received a error in write :%s\n",Gethostname(),strerror(errno));
	}
}

void msg_send_to_uds(int sockfd, char* destIpAddress, int destPortNumber, int src_port_num,
		char* message,int flag)
{
	char* msg_odr = build_msg_odr(sockfd, destIpAddress, destPortNumber, message, flag);
	struct sockaddr_un serverAddr;

	serverAddr.sun_family = AF_LOCAL;
	strcpy(serverAddr.sun_path,get_path_name(src_port_num));

	if(DEBUG){
		printf("In Message Send to Uds port Numbers dest %d and src %d\n",destPortNumber,src_port_num);
		printf("Path name for uds process %s\n",serverAddr.sun_path);
	}

	printf("ODR at node %s sending message to client/ server in Unix domain socket\n",Gethostname());

	if(sendto(sockfd,msg_odr,strlen(msg_odr),0,(SA*)&serverAddr,sizeof(serverAddr)) < 0){
		printf("Error in send to uds %s\n",strerror(errno));
	}
}

char* build_msg_odr(int sockfd, char* destIpAddress, int destPortNumber,
		char* message,int flag)
{
	char* msg_odr = malloc(sizeof(char)*1024);
	memset(msg_odr,'\0',sizeof(msg_odr));

	char *temp = msg_odr;

	int length=0;
	char temp_str[120];

	if(DEBUG)
		printf("%s\n",destIpAddress);

	strcpy(msg_odr,destIpAddress);

	sprintf(temp_str,"%d",destPortNumber);

	if(DEBUG)
		printf("%s\n",temp_str);

	length = strlen(destIpAddress);

	msg_odr[length] = '$';

	strcpy(msg_odr+length+1,temp_str);

	length+= 1 + strlen(temp_str);

	if(DEBUG)
		printf("******length of th port number************* %lu\n",strlen(temp_str));

	msg_odr[length] = '$';

	strcpy(msg_odr+length+1,message);

	if(DEBUG)
		printf("%s\n",message);

    length+=1+strlen(message);

	msg_odr[length] = '$';

	memset(temp_str,'\0',sizeof(temp_str));

	sprintf(temp_str,"%d",flag);

	if(DEBUG)
		printf("%s\n",temp_str);

	strcpy(msg_odr+length+1,temp_str);

	if(DEBUG){
		printf("length %d\n",length);
		printf("complete string %s\n",temp);
	}

	return temp;
}

struct msg_from_uds * msg_receive(int sockfd){

	struct msg_from_uds *reply = (struct msg_from_uds *)malloc(sizeof(struct msg_from_uds));
	char* msg_odr = (char *)malloc(sizeof(char)*1024);
	memset(msg_odr,'\0',sizeof(msg_odr));
	memset(reply, '\0', sizeof(struct msg_from_uds));
	struct sockaddr_un peerAddress;
	bzero(&peerAddress,sizeof(peerAddress));
	int src_port_num;
	int len = sizeof(struct sockaddr_un);

	again:
		if(recvfrom(sockfd, msg_odr, MSG_LEN, 0, (SA*)&peerAddress, &len) > 0){
			if(DEBUG)
				printf("messaged received : %s\n",msg_odr);
		}else{
			if(errno  == EINTR){
				goto again;
			}else{
				printf("ODR at node %s : received an error in recvfrom %s\n",Gethostname(),strerror(errno));
			}
		}

	if(DEBUG)
		printf("path_name of the client/server %s\n",peerAddress.sun_path);

	src_port_num = add_port_entry(peerAddress.sun_path);

	reply->src_port_num = src_port_num;

	int length = strlen(msg_odr);
	int i=0,k=0,dollarCount = 0,start_index=0;
	char tmp_str[120];

	while(i < length){

		if(msg_odr[i] == '$'){
			dollarCount++;
			i++;
		}else{
			tmp_str[k] = msg_odr[i];
			i++;
			k++;
			continue;
		}

		switch(dollarCount){

		case 1:
			if(reply->canonical_ipAddress == NULL){
				tmp_str[k] = '\0';
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
				strcpy(reply->msg_received,tmp_str);
				memset(tmp_str,'\0',sizeof(tmp_str));
				k=0;
			}
			break;
		case 2:
				tmp_str[k] = '\0';
				int portNumber = atoi(tmp_str);
				//printf("port number %d\n",portNumber);
				reply->dest_port_num = portNumber;
				memset(tmp_str,'\0',sizeof(tmp_str));
				k=0;

			break;
		}

	}

	reply->flag = atoi(tmp_str);

	if(DEBUG)
		printf("application pay_load parameters ip_address : %s\n dest_port_num : %d\n src_port_num :%d\n  payload : %s\n force_dsc_flag : %d\n",
								        reply->canonical_ipAddress,reply->dest_port_num,reply->src_port_num,reply->msg_received,reply->flag);

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
	//char* ip_addr = (char *)malloc(128);

	if((he  = gethostbyname(my_name)) !=NULL){
		ipaddr_list = (struct in_addr **)he->h_addr_list;
		/**
		 *  In case of the hostname we are taking only the
		 *  first address as that is the canonical IpAddress
		 *  we are considering.
		 */

		//strcpy(ip_addr,inet_ntoa(*ipaddr_list[0]));
		return inet_ntoa(*ipaddr_list[0]);
	}else{
		printf("gethostbyname error: %s for hostname: %s\n",hstrerror(h_errno),my_name);
		exit(-1);
	}
}

char*
Gethostbyaddr(char* canonical_ip_address){

	struct hostent *he;
	struct in_addr ipAddr;
	//char* vm_name = (char *)malloc(20);

	if((inet_pton(AF_INET,canonical_ip_address,&ipAddr)) <  0){
				printf("%s\n",strerror(errno));
	}

	if((he = gethostbyaddr(&ipAddr, sizeof(ipAddr), AF_INET))!= NULL){
		if(DEBUG)
			printf("The server host name: %s\n", he->h_name);
		//strcpy(vm_name,he->h_name);
		return he->h_name;
	}else{
		printf("gethostbyaddr error : %s for ipadress: %s\n",hstrerror(h_errno),canonical_ip_address);
		exit(-1);
	}
	return NULL;
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

/**
 * Helper method to convert into network order.
 */
void convertToNetworkOrder(struct odr_hdr *hdr){

	if(DEBUG)
		printf("came into network order conversion\n");

	hdr->broadcast_id = htonl(hdr->broadcast_id);
	hdr->hop_count = htons(hdr->hop_count);
	hdr->pkt_type = htons(hdr->pkt_type);
	hdr->force_route_dcvry = htons(hdr->force_route_dcvry);
	hdr->rreply_sent = htons(hdr->rreply_sent);
	hdr->dest_port_num = htons(hdr->dest_port_num);
	hdr->src_port_num = htons(hdr->src_port_num);
	hdr->rreq_id = htonl(hdr->rreq_id);
	hdr->payload_len = htons(hdr->payload_len);
}

/**
 *  Helper method to convert into host order.
 */
void convertToHostOrder(struct odr_hdr *hdr){

	if(DEBUG)
		printf("came into host order conversion \n");

	hdr->broadcast_id = ntohl(hdr->broadcast_id);
	hdr->hop_count = ntohs(hdr->hop_count);
	hdr->pkt_type = ntohs(hdr->pkt_type);
	hdr->force_route_dcvry = ntohs(hdr->force_route_dcvry);
	hdr->rreply_sent = ntohs(hdr->rreply_sent);
	hdr->dest_port_num = ntohs(hdr->dest_port_num);
	hdr->src_port_num = ntohs(hdr->src_port_num);
	hdr->rreq_id = ntohl(hdr->rreq_id);
	hdr->payload_len = ntohs(hdr->payload_len);
}


void printHWADDR(char *hw_addr){

	printf("hw addr = ");
	char *ptr = hw_addr;
	int i = IF_HADDR;
	do {
		printf("%.2x%s", *ptr++ & 0xff, (i == 1) ? " " : ":");
	} while (--i > 0);

	printf("\n");
}

int add_port_entry(char* path_name)
{

	if(DEBUG)
		printf("path_name received in add_port_entry %s\n",path_name);

	int i = 0;
	// Checking the path_name is present or not.
	for(i=1;i < NUM_PORT_ENTRIES;i++){

		if(!strcmp(path_name,port_entries[i].path_name)){
			if(DEBUG){
				printf("got a matched entry in add port num with index :%d\n",i);
			}
			return i;
		}
	}

	struct port_entry port_entr;

	port_num++;
	port_entr.port_num = port_num;
	strcpy(port_entr.path_name,path_name);

	port_entries[port_num] = port_entr;

	if(DEBUG)
		printf("Inserted the port entry with index %d\n",port_num);
	// have to insert a time stamp.
	return port_num;
}

void build_port_entries()
{
	struct port_entry port_entr;

	port_entr.port_num = SERVER_PORT;
	strcpy(port_entr.path_name, SERVER_WELL_KNOWN_PATH_NAME);
	port_entries[SERVER_PORT] = port_entr;

	bzero(&port_entr, sizeof(port_entr));

	port_entr.port_num = ODR_PORT;
	strcpy(port_entr.path_name, ODR_PATH_NAME);
	port_entries[ODR_PORT] = port_entr;
}

/**
 *  Wrappe method for sending the packet.
 */
void Sendto(int pf_sockfd, char* buffer, struct sockaddr_ll addr_ll,char *sendType)
{
	if(sendto(pf_sockfd,buffer,EHTR_FRAME_SIZE,0,
					(SA *)&addr_ll,sizeof(addr_ll)) < 0){
		printf("ODR at node %s : In sending %s received an error %s\n",Gethostname(),sendType, strerror(errno));
		exit(-1);
	}
}

/**
 *  Helper method for getting the path name for port_name.
 */
char* get_path_name(int dest_port_num){

	return port_entries[dest_port_num].path_name;
}

/**
 * Helper method to return the pkt_type
 */
char* get_packet_type(int pkt_type){

	switch(pkt_type){

	case 0:
		return "R_REQ";
	case 1:
		return "R_RPLY";
	case 2:
		return "PAY_LOAD";
	default:
		printf("Pkt_type :%d\n",pkt_type);
		return "Unknown";
	}
}

