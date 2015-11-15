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

	length+=strlen(message);

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

struct reply_from_uds_client * msg_receive(int sockfd){

	struct reply_from_uds_client *reply = (struct reply_from_uds_client *)malloc(sizeof(struct reply_from_uds_client));
	char* msg_odr = (char *)malloc(sizeof(char)*1024);
	memset(reply, '\0', sizeof(struct reply_from_uds_client));

	//bzero(reply,);
	if(read(sockfd,msg_odr,1024) > 0){
		printf("messaged received %s\n",msg_odr);
	}

	int length = strlen(msg_odr);
	int i=0,k=0,dollarCount = 0,start_index=0;
	char tmp_str[120];

	while(i < length){

		if(msg_odr[i] == '$'){
			dollarCount++;
			i++;
		}else{
			if(DEBUG)
				printf("character %c\n",msg_odr[i]);

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
				strcpy(reply->msg_received,tmp_str);
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

void fill_inf_mac_addr_map(struct hwa_info	*hw_head, char inf_mac_addr_map[10][20])
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
		strcpy(inf_mac_addr_map[temp->if_index],temp->if_haddr);
		temp = temp -> hwa_next;
	}
}

void convertToNetworkOrder(struct odr_hdr *hdr){

	hdr->broadcast_id = htonl(hdr->broadcast_id);
	hdr->hop_count = htons(hdr->hop_count);
	hdr->pkt_type = htons(hdr->pkt_type);
	hdr->force_route_dcvry = htons(hdr->force_route_dcvry);
	hdr->rreply_sent = htons(hdr->rreply_sent);
}


void printHWADDR(char *hw_addr){

	printf("         HW addr = ");
	char *ptr = hw_addr;
	int i = IF_HADDR;
	do {
		printf("%.2x%s", *ptr++ & 0xff, (i == 1) ? " " : ":");
	} while (--i > 0);

	printf("\n");
}


//char*
//get_vm_name(char *vm_ipAddress,struct vm_info *head){
//
//	struct vm_info *temp = head;
//
//	while(temp!=NULL){
//
//		if(!strcmp(temp->vm_ipadress,vm_ipAddress)){
//
//			return temp->vm_name;
//		}
//		temp = temp->next;
//	}
//
//	return NULL;
//
//}
//
//char *
//get_vm_ipaddress(char *vm_name,struct vm_info *head){
//
//	struct vm_info *temp = head;
//
//	while(temp!=NULL){
//
//		printf("vm-name :%s\n",temp->vm_name);
//		if(!strcmp(temp->vm_name,vm_name)){
//
//			return temp->vm_ipadress;
//		}
//		temp = temp->next;
//	}
//
//	return NULL;
//}
//
//void
//vm_init(){
//
//  int i=0;
//
//  for(i=0;i<NUM_OF_VMS;i++){
//	sprintf(vm_names[i],"vm%d",i+1);
//  }
//
//  strcpy(vm_names[10],"sujan-VirtualBox");
//
//  strcpy(vm_ipAddresses[0],"130.245.156.21");
//  strcpy(vm_ipAddresses[1],"130.245.156.22");
//  strcpy(vm_ipAddresses[2],"130.245.156.23");
//  strcpy(vm_ipAddresses[3],"130.245.156.24");
//  strcpy(vm_ipAddresses[4],"130.245.156.25");
//  strcpy(vm_ipAddresses[5],"130.245.156.26");
//  strcpy(vm_ipAddresses[6],"130.245.156.27");
//  strcpy(vm_ipAddresses[7],"130.245.156.28");
//  strcpy(vm_ipAddresses[8],"130.245.156.29");
//  strcpy(vm_ipAddresses[9],"130.245.156.20");
//  strcpy(vm_ipAddresses[10],"127.0.0.1");
//}
//
//
//struct vm_info *
//populate_vminfo_structs(){
//
//	vm_init();
//
//	struct vm_info *head=NULL, *tmp=NULL;
//
//	int i=0;
//
//	for(i=0;i<=NUM_OF_VMS;i++){
//
//		struct vm_info *new_vm = (struct vm_info *)malloc(sizeof(struct vm_info));
//
//		new_vm->vm_name = vm_names[i];
//		new_vm->vm_ipadress = vm_ipAddresses[i];
//
//		if(head== NULL){
//			head = new_vm;
//			tmp = head;
//		}else{
//			tmp->next = new_vm;
//			tmp=new_vm;
//		}
//	}
//
//	tmp->next = NULL;
//
//	return head;
//}



