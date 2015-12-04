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

void process_received_datagram(int sockfd, int udp_sockfd, char *buff){

	printf("%s has received the datagram\n",Gethostbyname(Gethostname()));

	struct ip *ip_hdr = (struct ip *)buff;

	char *data = buff + IP_HDR_LEN;

	struct tour_payload payload;

	memcpy(&payload,data,sizeof(payload));

	print_the_payload(payload);

	if(payload.index == payload.count-SOURCE_AND_MULTICAST_COUNT){
		printf("I am last node in the tour\n");
		printf("Sending the multi-cast message as I am last node in the tour\n");
		send_multicast_msg(udp_sockfd);
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

void join_mcast_grp(int udpsendsockfd){

	struct sockaddr_in multi_addr;

	build_multicast_addr(&multi_addr);

	Mcast_join(udpsendsockfd,(SA *)&multi_addr,sizeof(multi_addr),NULL,0);
}

void send_multicast_msg(int udp_sockfd)
{
	char msg[128] = "Hello Guys, We are part of one group\n";

	struct sockaddr_in destAddr;

	build_multicast_addr(&destAddr);

	if(sendto(udp_sockfd, msg, strlen(msg), 0,(SA *)&destAddr, sizeof(struct sockaddr_in)) < 0)
	{
		printf("Error in sending message to multi-cast address %s\n", strerror(errno));
	}
	printf("Sent multicast message\n");
}

void build_multicast_addr(struct sockaddr_in *multi_addr){

	bzero(multi_addr,sizeof(struct sockaddr_in));

	multi_addr->sin_family = AF_INET;
	multi_addr->sin_port = htons(MULTICAST_PORT_NUMBER);

	if(inet_pton(AF_INET,MULTICAST_ADDRESS,&multi_addr->sin_addr) < 0){
		printf("Error in converting numeric format %s\n",strerror(errno));
	}
}

/**
 *  API for getting the Mac Address
 */
int areq (struct sockaddr *IPaddr, socklen_t sockaddrlen, struct hwaddr *HWaddr){

	// create a unix domain socket with temp sun path name.
	// connect to the arp process on the well known arp is running.
	// wait for a reply with a time out set.

	printf("Came into areq\n");
	int unix_sockfd;

	struct sockaddr_un arp_addr;
	struct sockaddr_un temp_tour_addr;

	struct uds_arp_msg uds_msg;

	unix_sockfd = socket(AF_LOCAL,SOCK_STREAM,0);

	char target_ip_address[128];

	char file_name[15] = "/tmp/npaXXXXXX";
	file_name[14] = '\0';

	arp_addr.sun_family = AF_LOCAL;
	temp_tour_addr.sun_family = AF_LOCAL;

	if(mkstemp(file_name) < 0){
		printf("error in creating temporary file\n");
		exit(-1);
	}

	if(DEBUG){
		printf("temp-file_name :%s\n",file_name);
	}

	if(unlink(file_name) < 0){
		printf("str error %s\n",strerror(errno));
	}

	strcpy(temp_tour_addr.sun_path, file_name);

	strcpy(arp_addr.sun_path,ARP_WELL_KNOWN_PATH_NAME);

	Bind(unix_sockfd,(SA *)&temp_tour_addr,sizeof(temp_tour_addr));

	connect(unix_sockfd,(SA *)&arp_addr,sizeof(arp_addr));

	struct sockaddr_in *target_addr = (struct sockaddr_in *)IPaddr;

	inet_ntop(AF_INET,&target_addr->sin_addr,target_ip_address,sizeof(target_ip_address));

	printf("target ip address :%s\n",target_ip_address);

	strcpy(uds_msg.target_ip_address,target_ip_address);

	uds_msg.hw_addr = *HWaddr;

	if(sendto(unix_sockfd,&uds_msg,sizeof(uds_msg),0,NULL,0) < 0){

		printf("Error in send to in areq %s\n",strerror(errno));
	}

	memset(&uds_msg, '\0',sizeof(uds_msg));

	printf("Send waiting in recv\n");

	if(recvfrom(unix_sockfd,&uds_msg,sizeof(uds_msg),0,NULL,NULL)< 0){

		printf("Error in recv from %s\n",strerror(errno));
	}

	printf("Message received for areq  ");

	printHWADDR(uds_msg.hw_addr.sll_addr);

	return 0;
}



void ping_predecessor(){

	printf("came into pinging the packet\n");

	struct sockaddr_in pred_addr;

	pred_addr.sin_family = AF_INET;

	inet_pton(AF_INET,"130.245.156.21",&pred_addr.sin_addr);

	struct hwaddr hw_addr;

	hw_addr.sll_halen = ETH_ALEN;
	hw_addr.sll_ifindex = ETH0_INDEX;
	hw_addr.sll_hatype = ETH_HA_TYPE;

	areq((SA *)&pred_addr,sizeof(struct sockaddr_in),&hw_addr);
}


