/*
 * tour_utils.c
 *
 *  Created on: Nov 29, 2015
 *      Author: sujan
 */

#include "src_usp.h"

char *my_name;
bool isHostNameFilled = false;

void allocate_buffer(char **buff){

	*buff = (char *)malloc(BUFFER_SIZE*sizeof(char));
}

void create_tour_list(int count , char **argv, struct tour_route *tour_list){

	//tour_list = (struct tour_route*) malloc(sizeof(struct tour_route) * SIZE_OF_TOUR_LIST);

	int i=0;
	for(i=1; i < count ;i++){
		struct tour_route route;

		strcpy(route.ip_address ,Gethostbyname(argv[i]));

		tour_list[i] = route;
		if(DEBUG){
			printf("Inserted in the list \n");
			printf("in presentation format %s\n",route.ip_address);
		}
	}

	insert_me_address_at_bgn(tour_list);
	insert_multicast_address_at_lst(count, tour_list);
}

void insert_me_address_at_bgn(struct tour_route *tour_list){

	struct tour_route route;
	strcpy(route.ip_address,Gethostbyname(Gethostname()));

	if(tour_list != NULL){

		if(DEBUG){
			printf("Inserted in the list at bg\n");
			printf("in presentation format %s\n",route.ip_address);
		}

		tour_list[0] = route;
	}
}

void insert_multicast_address_at_lst(int count, struct tour_route *tour_list){

	if(DEBUG){
		printf("Inserting at the last in the list\n");
	}

	struct in_addr multi_addr;

	if(inet_pton(AF_INET,MULTICAST_ADDRESS,&multi_addr) < 0){
		printf("Error in converting into presentation format\n");
		exit(0);
	}

	struct tour_route route;
	strcpy(route.ip_address,MULTICAST_ADDRESS);
	route.port_number = htons(MULTICAST_PORT_NUMBER);
	tour_list[count] = route;

	if(DEBUG){
		printf("Ending at the last in the list\n");
	}
}

char*
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
		return inet_ntoa(*ipaddr_list[0]);
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


void build_ip_header(char *buff,uint16_t total_len,char *dest_addr){

	if(DEBUG){
		printf("Building the IP_Address\n");
	}

	struct ip *ip = (struct ip *)buff;

	ip->ip_v = IPVERSION;

	ip->ip_hl = sizeof(struct ip) >> 2;

	ip->ip_tos = 0;

	ip->ip_len = htons(total_len);

	ip->ip_id = htons(IDENTIFICATION_FIELD);

	ip->ip_off =  0;

	ip->ip_ttl = MAX_TTL_VALUE;

	ip->ip_p = GRP_PROTOCOL_VALUE;


	if(inet_pton(AF_INET,dest_addr,&ip->ip_dst) < 0){
		printf("Error in converting numeric format %s\n",strerror(errno));
	}

	if(inet_pton(AF_INET,Gethostbyname(Gethostname()),&ip->ip_src) < 0){
		printf("Error in converting numeric format %s\n",strerror(errno));
	}
}

void populate_data_in_datagram(char *buff, uint16_t index,uint16_t count, struct tour_route *tour_list){

	if(DEBUG){
		printf("Populating the data in the datagram\n");
	}

	char *data = buff + IP_HDR_LEN;

	struct tour_payload payload;

	payload.index = index;

	payload.count = count;

	memcpy(payload.tour_list,tour_list,(sizeof(struct tour_route)*SIZE_OF_TOUR_LIST));

	print_the_payload(payload);

	// TODO
	//convertToNetworkOrder(&payload);

	memcpy(data,&payload,sizeof(struct tour_payload));
}


char*
getIpAddressInTourList(struct tour_route *tour_list, uint16_t index){

	if(DEBUG)
		printf("Getting the IpAddress in the tourList\n");

	/**
	 *  Two checks may be unnecessary will have
	 *  to check.
	 */
	if(index < SIZE_OF_TOUR_LIST){

		if(DEBUG){
			printf("In getting the IpAddress bg\n");
			printf("in presentation format %d  %s\n",index,tour_list[index].ip_address);
		}

		return tour_list[index].ip_address;
	}

	return NULL;
}

uint16_t
calculate_length(){

	printf("Length %ld\n",IP_HDR_LEN + sizeof(struct tour_payload));

	return IP_HDR_LEN + sizeof(struct tour_payload);
}

void process_received_datagram(int sockfd, char *buff){

	printf("%s has received the datagram\n",Gethostbyname(Gethostname()));

	struct ip *ip_hdr = (struct ip *)buff;

	char *data = buff + IP_HDR_LEN;

	struct tour_payload payload;

	memcpy(&payload,data,sizeof(payload));

	print_the_payload(payload);

	if(payload.index == payload.count-SOURCE_AND_MULTICAST_COUNT){
		printf("I am last node in the tour\n");
	}else{
		forward_the_datagram(sockfd, payload);
	}
}

void
print_the_payload(struct tour_payload payload){

	int i=0;

	printf("Index : %d and Size of list : %d  in received payload\n",payload.index,payload.count);

	printf("printing the received tour\n");

	for(i=0;i<payload.count;i++){

		printf("%s\t",payload.tour_list[i].ip_address);
	}

	printf("\n");
}

void
forward_the_datagram(int sockfd, struct tour_payload payload){

	char *buff = (char *)malloc(BUFFER_SIZE*sizeof(char));

	//allocate_buffer(&buff);

	payload.index++;

	build_ip_header(buff,calculate_length(),payload.tour_list[payload.index].ip_address);

	memcpy(buff+IP_HDR_LEN, &payload, sizeof(struct tour_payload));

	struct sockaddr_in destAddr;

	destAddr.sin_family = AF_INET;

	if(inet_pton(AF_INET,getIpAddressInTourList(payload.tour_list,payload.index),&destAddr.sin_addr) < 0){
		printf("Error in converting numeric format %s\n",strerror(errno));
	}

	destAddr.sin_port = 0;

	if(sendto(sockfd,buff,BUFFER_SIZE,0,(SA *)&destAddr,sizeof(struct sockaddr)) < 0){
		printf("Error in Sendto in %s\n",strerror(errno));
	}
}

void join_mcast_grp(int udpsock){

	struct sockaddr_in multi_addr;

	bzero(&multi_addr,sizeof(struct sockaddr_in));
	/**
	 * Joining the multicast address
	 */
	multi_addr.sin_family = AF_INET;

	inet_pton(AF_INET,MULTICAST_ADDRESS,&multi_addr.sin_addr);

	multi_addr.sin_port = MULTICAST_PORT_NUMBER;

	Mcast_join(udpsock,(SA *)&multi_addr,sizeof(multi_addr),NULL,0);
}
