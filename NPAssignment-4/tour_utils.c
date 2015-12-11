/*
 * tour_utils.c
 *
 *  Created on: Nov 29, 2015
 *      Author: sujan
 */

#include "src_usp.h"

char *my_name;
bool isHostNameFilled = false;
bool joinedMulticastGrp =false;

void allocate_buffer(char **buff){

	*buff = (char *)malloc(BUFFER_SIZE*sizeof(char));
}

bool create_tour_list(int count , char **argv, struct tour_route *tour_list){

	char prev_vm[10] = "\0";
	int i=0;
	for(i=1; i < count ;i++){
		if(strcmp(prev_vm, argv[i]))
		{
			struct tour_route route;
			strcpy(route.ip_address , Gethostbyname(argv[i]));
			tour_list[i] = route;

			if(DEBUG){
				printf("Inserted in the list \n");
				printf("in presentation format %s\n",route.ip_address);
			}

			strcpy(prev_vm, argv[i]);
		}
		else
		{
			return true;
		}
	}


	insert_me_address_at_bgn(tour_list);
	insert_multicast_address_at_lst(count, tour_list);

	printf("Built the tour list at node %s\n",Gethostname());

	return false;
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

void send_icmp_echo(int sockfd, char *dest_addr_ip,struct hwaddr hw_addr,int seqNum) {

	char *buff = (char *)malloc(IP_HDR_LEN + ICMP_HDR_LEN + ICMP_DATA_LEN);

	memset(buff,'\0',IP_HDR_LEN + ICMP_HDR_LEN + ICMP_DATA_LEN);

	build_ip_header(buff, IP_HDR_LEN+ICMP_HDR_LEN+ICMP_DATA_LEN, dest_addr_ip, true);

	char *icmp_hdr = buff + IP_HDR_LEN;

	struct icmp *icmp = (struct icmp *)icmp_hdr;
	struct sockaddr_ll addr_ll;

	char data_icmp[ICMP_DATA_LEN];
	/**
	 * Setting dummy data for sending the ping
	 */
	memset(data_icmp,0xff,ICMP_DATA_LEN);

	bzero(&addr_ll,sizeof(addr_ll));

	icmp->icmp_seq = seqNum;
	icmp->icmp_type = ICMP_ECHO;
	icmp->icmp_code = 0;
	icmp->icmp_id = ICMP_IDENTIFIER;
	memcpy(icmp->icmp_data,data_icmp,ICMP_DATA_LEN);
	icmp->icmp_cksum = 0;
	icmp->icmp_cksum = in_cksum((u_short *)icmp,ICMP_HDR_LEN + ICMP_DATA_LEN);


	if(DEBUG)
		printf("Sending ICMP Echo message to %s\n",dest_addr_ip);

	char *eth_buff = (char *)malloc(ETHR_FRAME_SIZE_IP * sizeof(char));

	if(DEBUG){
		printf("Printing src  ");
		printHWADDR(my_hw_addr_head->mac_addr);
		printf("Printing dest  ");
		printHWADDR(hw_addr.sll_addr);
		printf("Length of the ethr frame %lu",sizeof(struct icmp));
	}

	char* predecessor_vm_name = malloc(10);

	strcpy(predecessor_vm_name,Gethostbyaddr(dest_addr_ip));


	/**
	 * Ping information
	 */
	printf("PING %s (%s): %d data bytes \n",
					predecessor_vm_name,dest_addr_ip,ICMP_DATA_LEN);

	build_eth_frame_ip(eth_buff, hw_addr.sll_addr, my_hw_addr_head->mac_addr, ETH0_INDEX, &addr_ll, buff, PACKET_OTHERHOST);

	if(sendto(pf_sockfd,eth_buff,ETHR_FRAME_SIZE_IP,0,
						(SA *)&addr_ll,sizeof(addr_ll)) < 0){
			printf("Error at sendto at ICMP :%s\n", strerror(errno));
			exit(-1);
	}
}

void build_ip_header(char *buff, uint16_t total_len,char *dest_addr, bool isIcmp){

	if(DEBUG){
		printf("Building the IP header\n");
	}

	struct ip *ip = (struct ip *)buff;

	ip->ip_v = IPVERSION;

	ip->ip_hl = sizeof(struct ip) >> 2;

	ip->ip_tos = 0;

	ip->ip_len = htons(total_len);

	ip->ip_id = htons(IDENTIFICATION_FIELD);

	ip->ip_off =  0;

	if(isIcmp){
		ip->ip_p = IPPROTO_ICMP;
	}
	else{
		ip->ip_p = GRP_PROTOCOL_VALUE;
	}

	ip->ip_ttl = MAX_TTL_VALUE;
	if(inet_pton(AF_INET,dest_addr,&ip->ip_dst) < 0){
		printf("Error in converting numeric format %s\n",strerror(errno));
	}

	if(inet_pton(AF_INET,Gethostbyname(Gethostname()),&ip->ip_src) < 0){
		printf("Error in converting numeric format %s\n",strerror(errno));
	}

	ip->ip_sum = 0;

	ip->ip_sum = in_cksum((u_short *)ip,total_len);//htons(0x7d9e);//in_cksum((u_short *)ip,IP_HDR_LEN);

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

	if(DEBUG)
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

	if(DEBUG)
		printf("Length %ld\n",IP_HDR_LEN + sizeof(struct tour_payload));

	return IP_HDR_LEN + sizeof(struct tour_payload);
}

void process_received_datagram(int sockfd, int udp_sockfd,int udprecvsockfd, char *buff){

	if(DEBUG)
		printf("%s has received the datagram\n",Gethostbyname(Gethostname()));

	char time_buff[100];

	memset(time_buff,0,sizeof(time_buff));

	time_t ticks = time(NULL);

	snprintf(time_buff, sizeof(time_buff), "%.24s " , ctime(&ticks));

	struct ip *ip_hdr = (struct ip *)buff;

	char src_addr[32];

	inet_ntop(AF_INET,&ip_hdr->ip_src,src_addr,32);

	printf("%s received source routing packet from %s\n",time_buff,Gethostbyaddr(src_addr));

	add_prev_node(src_addr);

	join_mcast_grp(udprecvsockfd);

	char *data = buff + IP_HDR_LEN;

	struct tour_payload payload;

	memcpy(&payload,data,sizeof(payload));

	if(DEBUG)
		print_the_payload(payload);

	if(payload.index == payload.count-SOURCE_AND_MULTICAST_COUNT){

		if(DEBUG){
			printf("I am last node in the tour\n");
			printf("Sending the multi-cast message as I am last node in the tour\n");
		}

		unsigned int time_out = TIME_OUT_FOR_MULTICAST;
		/**
		 *  Setting the five secs wait

		struct timeval t;

		t.tv_sec = TIME_OUT_FOR_MULTICAST;
		t.tv_usec = 0;

		int sockfd = socket(AF_INET,SOCK_STREAM,0);

		fd_set monitor_fds;

		FD_ZERO(&monitor_fds);
		FD_SET(sockfd,&monitor_fds);

		again:
			if(select(sockfd + 1, &monitor_fds, NULL, NULL, &t) <  0){
				if(errno == EINTR){
					printf("Received the EINTR and remaining time %ld\n",t.tv_sec);
					goto again;
				}else{
					printf("Received an error in select :%s\n",strerror(errno));
				}
			}
		*/
		again:
			if((time_out = sleep(time_out)) > 0){
				//printf("Received an interrupt - remaining time :%u\n",time_out);
				goto again;
			}
		char vm_name[20];

		strcpy(vm_name,Gethostname());

		char msg[200];

		memset(msg,'\0',200);

		strcat(msg,"This is node ");

		strcat(msg,vm_name);

		strcat(msg,".Tour has ended.Group members please identify yourselves.");

		send_multicast_msg(udp_sockfd,msg);
	}else{
		forward_the_datagram(sockfd, payload);
	}
}

void
print_the_payload(struct tour_payload payload){

	int i=0;
	printf("Index : %d and Size of list : %d  in the payload\n",payload.index,payload.count);

	printf("printing the received tour\n");

	for(i=0;i<payload.count;i++){

		printf("%s\t",payload.tour_list[i].ip_address);
	}

	printf("\n");
}

void
forward_the_datagram(int sockfd, struct tour_payload payload){

	char *buff = (char *)malloc(BUFFER_SIZE*sizeof(char));

	payload.index++;

	build_ip_header(buff,calculate_length(),payload.tour_list[payload.index].ip_address, false);

	memcpy(buff+IP_HDR_LEN, &payload, sizeof(struct tour_payload));

	struct sockaddr_in destAddr;

	destAddr.sin_family = AF_INET;

	if(inet_pton(AF_INET,getIpAddressInTourList(payload.tour_list,payload.index),&destAddr.sin_addr) < 0){
		printf("Error in converting numeric format %s\n",strerror(errno));
	}

	destAddr.sin_port = 0;


	char next_vm_name[20];

	memset(next_vm_name,'\0',20);

	strcpy(next_vm_name,Gethostbyaddr(getIpAddressInTourList(payload.tour_list,payload.index)));

	printf("%s is sending the source routing packet to next node in the tour %s\n",Gethostname(),next_vm_name);

	if(sendto(sockfd,buff,BUFFER_SIZE,0,(SA *)&destAddr,sizeof(struct sockaddr)) < 0){
		printf("Error in Sendto in %s\n",strerror(errno));
	}
}

void join_mcast_grp(int udpsendsockfd){

	if(!joinedMulticastGrp)
	{
		struct sockaddr_in multi_addr;

		build_multicast_addr(&multi_addr);

		Mcast_join(udpsendsockfd,(SA *)&multi_addr,sizeof(multi_addr),NULL,0);

		printf("%s has joined the multicast group using multicast address :%s and port number :%d\n",Gethostname(),
																									MULTICAST_ADDRESS,MULTICAST_PORT_NUMBER);

		joinedMulticastGrp = true;
	}
}

void send_multicast_msg(int udp_sockfd,char *msg)
{

	struct sockaddr_in destAddr;

	build_multicast_addr(&destAddr);

	printf("Node %s Sending: %s\n",Gethostname(),msg);

	if(sendto(udp_sockfd, msg, strlen(msg), 0,(SA *)&destAddr, sizeof(struct sockaddr_in)) < 0)
	{
		printf("Error in sending message to multi-cast address %s\n", strerror(errno));
	}

	if(DEBUG)
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

	if(DEBUG)
		printf("Came into areq\n");

	int unix_sockfd;

	struct sockaddr_un arp_addr;
	struct sockaddr_un temp_tour_addr;

	struct uds_arp_msg uds_msg;

	unix_sockfd = socket(AF_LOCAL,SOCK_STREAM,0);

	char target_ip_address[128];
//
//	char file_name[15] = "/tmp/npaXXXXXX";
//	file_name[14] = '\0';

	arp_addr.sun_family = AF_LOCAL;
	temp_tour_addr.sun_family = AF_LOCAL;

//	if(mkstemp(file_name) < 0){
//		printf("error in creating temporary file\n");
//		exit(-1);
//	}
//
//	if(DEBUG){
//		printf("temp-file_name :%s\n",file_name);
//	}
//
//	unlink(file_name);
//
//	strcpy(temp_tour_addr.sun_path, file_name);

	strcpy(arp_addr.sun_path,ARP_WELL_KNOWN_PATH_NAME);

	//Bind(unix_sockfd,(SA *)&temp_tour_addr,sizeof(temp_tour_addr));

	connect(unix_sockfd,(SA *)&arp_addr,sizeof(arp_addr));

	struct sockaddr_in *target_addr = (struct sockaddr_in *)IPaddr;

	inet_ntop(AF_INET,&target_addr->sin_addr,target_ip_address,sizeof(target_ip_address));

	printf("Calling ARP module for getting the mac-address for ip address :%s\n",target_ip_address);

	strcpy(uds_msg.target_ip_address,target_ip_address);

	uds_msg.hw_addr = *HWaddr;

	int n=0;

	if((n=sendto(unix_sockfd,&uds_msg,sizeof(uds_msg),0,NULL,0)) < 0){

		printf("Error in send to in areq %s\n",strerror(errno));
	}

	//printf("Sending an packet arp with bytes :%d\n",n);

	memset(&uds_msg, '\0',sizeof(uds_msg));

	if(DEBUG)
		printf("Send waiting in recv\n");


	struct timeval t;

	t.tv_sec = 2;
	t.tv_usec = 0;

	fd_set monitor_fds;

	FD_ZERO(&monitor_fds);
	FD_SET(unix_sockfd,&monitor_fds);

	if(Select(unix_sockfd+1,NULL,NULL,NULL,&t) < 0){

		return -1;
	}



	if(recvfrom(unix_sockfd,&uds_msg,sizeof(uds_msg),0,NULL,NULL)< 0){

		printf("Error in recv from %s\n",strerror(errno));
	}

	if(DEBUG){
		printf("Message received for areq  ");

		printHWADDR(uds_msg.hw_addr.sll_addr);
	}

	*HWaddr = uds_msg.hw_addr ;

	printf("Received the mac-address from the ARP module for ip address %s and the",target_ip_address);

	printHWADDR(HWaddr->sll_addr);

	//unlink(file_name);

	return 0;
}



void get_predecessor_mac(char *ip_addr, struct hwaddr *hw_addr){

	if(DEBUG)
		printf("came into pinging the packet\n");

	struct sockaddr_in pred_addr;

	pred_addr.sin_family = AF_INET;

	inet_pton(AF_INET, ip_addr, &pred_addr.sin_addr);

	hw_addr->sll_halen = ETH_ALEN;
	hw_addr->sll_ifindex = ETH0_INDEX;
	hw_addr->sll_hatype = ETH_HA_TYPE;

	if(areq((SA *)&pred_addr,sizeof(struct sockaddr_in), hw_addr) < 0){

		printf("Timeout:ARP failed to get the failed mac-address for ip-address :%s\n",ip_addr);
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
			printf("host name: %s\n", he->h_name);

		return he->h_name;
	}else{
		printf("gethostbyaddr error : %s for ipadress: %s\n",hstrerror(h_errno),canonical_ip_address);
		exit(-1);
	}
	return NULL;
}


/**
 *  Borrowed this method from the steven code.
 */
uint16_t
in_cksum(uint16_t *addr, int len)
{
	int				nleft = len;
	uint32_t		sum = 0;
	uint16_t		*w = addr;
	uint16_t		answer = 0;

	/*
	 * Our algorithm is simple, using a 32 bit accumulator (sum), we add
	 * sequential 16 bit words to it, and at the end, fold back all the
	 * carry bits from the top 16 bits into the lower 16 bits.
	 */
	while (nleft > 1)  {
		sum += *w++;
		nleft -= 2;
	}

		/* 4mop up an odd byte, if necessary */
	if (nleft == 1) {
		*(unsigned char *)(&answer) = *(unsigned char *)w ;
		sum += answer;
	}

		/* 4add back carry outs from top 16 bits to low 16 bits */
	sum = (sum >> 16) + (sum & 0xffff);	/* add hi 16 to low 16 */
	sum += (sum >> 16);			/* add carry */
	answer = ~sum;				/* truncate to 16 bits */
	return(answer);
}
