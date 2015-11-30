/*
 * tour_utils.c
 *
 *  Created on: Nov 29, 2015
 *      Author: sujan
 */

#include "src_usp.h"

char *my_name;
bool isHostNameFilled = false;
struct tour_route tour_list[SIZE_OF_TOUR_LIST];
//struct tour_route *tour_list_rear = NULL;

struct tour_route * create_tour_list(int count , char **argv){

	int i=0;

	for(i=1; i < count ;i++){
		struct tour_route route;
		route.ip_address = Gethostbyname(argv[i]);
		tour_list[i] = route;
		if(DEBUG){
			struct in_addr tempAddr;
			tempAddr.s_addr = route.ip_address;
			printf("Inserted in the list \n");
			printf("in presentation format %s\n",inet_ntoa(tempAddr));
		}
	}

	insert_my_address_at_bgn();
	insert_multicast_address_at_lst(count);
	return tour_list;
}

void insert_my_address_at_bgn(){

	struct tour_route route;
	route.ip_address = Gethostbyname(Gethostname());

	if(tour_list != NULL){

		if(DEBUG){
			struct in_addr tempAddr;
			tempAddr.s_addr = route.ip_address;
			printf("Inserted in the list at bg\n");
			printf("in presentation format %s\n",inet_ntoa(tempAddr));
		}

		//node->next = tour_list_head;
		//tour_list_head = node;

		tour_list[0] = route;
	}
}

void insert_multicast_address_at_lst(int count){

	if(DEBUG){
		printf("Inserting at the last in the list\n");
	}

	struct in_addr multi_addr;

	if(inet_pton(AF_INET,MULTICAST_ADDRESS,&multi_addr) < 0){
		printf("Error in converting into presentation format\n");
		exit(0);
	}

	//struct tour_route *node = (struct tour_route *)malloc(sizeof(struct tour_route));
	//node->ip_address = multi_addr.s_addr;
	//node->port_number = MULTICAST_PORT_NUMBER;
	//node->next = NULL;

	struct tour_route route;
	route.ip_address = multi_addr.s_addr;
	route.port_number = MULTICAST_PORT_NUMBER;
	tour_list[count] = route;

	if(DEBUG){
			printf("Ending at the last in the list\n");
		}

}


uint32_t
Gethostbyname(char *my_name){

	struct hostent *he;
	struct in_addr **ipaddr_list;

	if(DEBUG){
		printf("In Get host my name :%s\n",my_name);
	}

	if((he  = gethostbyname(my_name)) !=NULL){
		ipaddr_list = (struct in_addr **)he->h_addr_list;
		/**
		 *  In case of the hostname we are taking only the
		 *  first address as that is the canonical IpAddress
		 *  we are considering.
		 */

		//strcpy(ip_addr,inet_ntoa(*ipaddr_list[0]));
		return ipaddr_list[0]->s_addr;
	}else{
		printf("gethostbyname error: %s for hostname: %s\n",hstrerror(h_errno),my_name);
		exit(-1);
	}
}


char* Gethostname(){

	if(!isHostNameFilled){
		my_name = (char *)malloc(20*sizeof(char));
		gethostname(my_name,sizeof(my_name));
		isHostNameFilled = true;
	}
	return my_name;
}


void build_ip_header(char *buff, uint16_t index,int total_len){

	if(DEBUG)
		printf("Building the IP_Address\n");

	struct ip *ip = (struct ip *)buff;

	ip->ip_v = htons(IPVERSION);

	ip->ip_hl = htons(sizeof(struct ip) >> 2);

	ip->ip_tos = 0;

	//TODO : Have to set the total length.
	ip->ip_len = htons(total_len);

	ip->ip_id = htons(IDENTIFICATION_FIELD);

	ip->ip_off =  0;

	ip->ip_ttl = htons(1);

	ip->ip_src.s_addr = htons(Gethostbyname(Gethostname()));

	ip->ip_dst.s_addr = htons(getIpAddressInTourList(index));
}

void populate_data_in_datagram(char *buff, uint16_t index,uint16_t count){

	if(DEBUG){
		printf("Populating the data in the datagram\n");
	}

	char *ptr = buff + sizeof(struct ip);

	*((uint16_t *) ptr) = htons(index+1);

	ptr+=2;

	*((uint16_t *) ptr) = htons(count);

	ptr+=2;

	memcpy(ptr,tour_list,sizeof(tour_list));
}


uint32_t getIpAddressInTourList(uint16_t index){

//	while(temp != NULL){
//
//		if(index == count){
//			return temp->ip_address;
//		}
//
//		temp = temp->next;
//		count++;
//	}


	if(DEBUG)
		printf("Getting the IpAddress in the tourList\n");

	/**
	 *  Two checks may be unnecessary will have
	 *  to check.
	 */
	if(tour_list != NULL && index < SIZE_OF_TOUR_LIST){

		if(DEBUG){
			struct in_addr tempAddr;
			tempAddr.s_addr = tour_list[index].ip_address;
			printf("In getting the IpAddress bg\n");
			printf("in presentation format %s\n",inet_ntoa(tempAddr));
		}


		return tour_list[index].ip_address;
	}


	return 0;
}

int
calculate_length(int tour_list_len){

	return tour_list_len*sizeof(struct tour_route) + 4 + sizeof(struct ip);
}

