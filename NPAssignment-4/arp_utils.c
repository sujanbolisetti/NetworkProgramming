/*
 * arp_utils.c
 *
 *  Created on: Dec 3, 2015
 *      Author: sujan
 */

#include "src_usp.h"

//struct ip_addr_hw_addr_pr *my_hw_addr_head = NULL;
unsigned char BROADCAST_MAC_ADDRESS[6]=  {0xff,0xff,0xff,0xff,0xff,0xff};

struct arp_cache_entry *arp_cache_head = NULL;
struct arp_cache_entry *arp_cache_rear = NULL;

struct sockaddr *sa;

void create_ip_hw_mp(struct hwa_info *head){

	my_hw_addr_head = NULL;

	struct hwa_info *temp = head;

	struct ip_addr_hw_addr_pr *temp_my_hw_addr = NULL;

	if(DEBUG)
		printf("Entered creating the ip_addr_hw_addr map\n");

	while(temp!=NULL){

		struct ip_addr_hw_addr_pr *node = (struct ip_addr_hw_addr_pr *)malloc(sizeof(struct ip_addr_hw_addr_pr));

		memcpy(node->mac_addr,temp->if_haddr,ETH_ALEN);

		ETH0_INDEX =  temp->if_index;

		if(DEBUG)
			printf("Populated the mac_addr and ifi_index is :%d\n",temp->if_index);

		if ((sa = temp->ip_addr) != NULL){
			strcpy(node->ip_addr,Sock_ntop_host(sa, sizeof(struct sockaddr)));
		}

		if(DEBUG)
			printf("printing the ip_address %s\n",node->ip_addr);

		node -> next = NULL;

		if(my_hw_addr_head == NULL){
			my_hw_addr_head = node;
			temp_my_hw_addr = node;
		}else{
			temp_my_hw_addr->next = node;
			temp_my_hw_addr = node;
		}
		temp = temp->hwa_next;
	}
}

void print_my_hw_ip_mp(){

	struct ip_addr_hw_addr_pr *temp_my_hw_addr = my_hw_addr_head;

	while(temp_my_hw_addr != NULL){

		 if(DEBUG)
			 printf("         IP addr = %s\n", temp_my_hw_addr->ip_addr);

		 printHWADDR(temp_my_hw_addr->mac_addr);
		 temp_my_hw_addr = temp_my_hw_addr->next;
	}
}

void printHWADDR(char *hw_addr){

	printf(" MAC addr = ");
	char *ptr = hw_addr;
	int i = IF_HADDR;
	do {
		printf("%.2x%s", *ptr++ & 0xff, (i == 1) ? " " : ":");
	} while (--i > 0);

	printf("\n");
}


char *
sock_ntop_host(const struct sockaddr *sa, socklen_t salen)
{
    static char str[128];		/* Unix domain is largest */

	switch (sa->sa_family) {
		case AF_INET: {
			if(DEBUG)
				printf("Entered into AF_INET\n");

			struct sockaddr_in	*sin = (struct sockaddr_in *) sa;

			if (inet_ntop(AF_INET, &sin->sin_addr, str, sizeof(str)) == NULL)
				return(NULL);

			if(DEBUG)
				printf("IP_address %s\n",str);

			return(str);
		}
	}
	return NULL;
}

char *
Sock_ntop_host(const struct sockaddr *sa, socklen_t salen)
{
	char	*ptr;
	if ( (ptr = sock_ntop_host(sa, salen)) == NULL)
		printf("sock_ntop_host error %s",strerror(errno));	/* inet_ntop() sets errno */
	return(ptr);
}


struct arp_pkt* build_arp_pkt(int pkt_type, unsigned char *snd_hw_addr,
												char *snd_ip_addr, char *tgt_ip_address){

	if(DEBUG)
		printf("Building ARP _PACKET at node %s\n",Gethostname());

	struct arp_pkt *pkt = (struct arp_pkt *)malloc(sizeof(struct arp_pkt));

	pkt->hard_type =  1;
	pkt->prot_type = htons(ARP_GRP_TYPE);
	pkt->hard_size = 6;
	pkt->prot_size = 4;
	pkt->op = pkt_type;

	memcpy(pkt->snd_ethr_addr,snd_hw_addr,ETH_ALEN);
	strcpy(pkt->snd_ip_address,snd_ip_addr);
	strcpy(pkt->target_ip_address,tgt_ip_address);

	return pkt;
}

void build_eth_frame(void *buffer,char *dest_mac,
			char *src_mac,int inf_index,
			struct sockaddr_ll *addr_ll, struct arp_pkt *pkt, int eth_pkt_type){

	if(DEBUG)
		printf("Building ETHR FRAME at node %s\n",Gethostname());

	bzero(addr_ll,sizeof(*addr_ll));

	convertToNetworkOrder(pkt);

	unsigned char* etherhead = buffer;

	unsigned char* data = buffer + ETH_HDR_LEN;

	struct ethhdr *eh = (struct ethhdr *)etherhead;

	addr_ll->sll_family   = AF_PACKET;
	addr_ll->sll_ifindex  = inf_index;
	addr_ll->sll_pkttype  = eth_pkt_type;
	addr_ll->sll_protocol = htons(ARP_GRP_TYPE);
	addr_ll->sll_halen    = ETH_ALEN;

	memcpy((void*)buffer, (void*)dest_mac, ETH_ALEN);
	memcpy((void*)(buffer+ETH_ALEN), (void*)src_mac, ETH_ALEN);
	eh->h_proto = htons(ARP_GRP_TYPE);

	memcpy(addr_ll->sll_addr,(void*)dest_mac, ETH_ALEN);

	printf("Destination and src mac address\n");

	printHWADDR(dest_mac);

	printHWADDR(src_mac);

	/**
	 *  blanking out the last two values.
	 */
	addr_ll->sll_addr[6] = 0x00;
	addr_ll->sll_addr[7] = 0x00;

	memcpy(data,pkt,sizeof(struct arp_pkt));
}

void build_eth_frame_ip(void *buffer,char *dest_mac,
			char *src_mac,int inf_index,
			struct sockaddr_ll *addr_ll, char *pkt, int eth_pkt_type){


	if(DEBUG)
		printf("Building ETHR FRAME for ip at node %s\n",Gethostname());

	bzero(addr_ll,sizeof(*addr_ll));

	unsigned char* etherhead = buffer;

	unsigned char* data = buffer + ETH_HDR_LEN;

	struct ethhdr *eh = (struct ethhdr *)etherhead;

	addr_ll->sll_family   = AF_PACKET;
	addr_ll->sll_ifindex  = inf_index;
	addr_ll->sll_pkttype  = eth_pkt_type;
	addr_ll->sll_protocol = htons(ETH_P_IP);
	addr_ll->sll_halen    = ETH_ALEN;

	memcpy((void*)buffer, (void*)dest_mac, ETH_ALEN);
	memcpy((void*)(buffer+ETH_ALEN), (void*)src_mac, ETH_ALEN);
	eh->h_proto = htons(ETH_P_IP);

	memcpy(addr_ll->sll_addr,(void*)dest_mac, ETH_ALEN);

	if(DEBUG){
		printf("Destination and src mac address\n");

		printHWADDR(dest_mac);

		printHWADDR(src_mac);
	}

	/**
	 *  blanking out the last two values.
	 */
	addr_ll->sll_addr[6] = 0x00;
	addr_ll->sll_addr[7] = 0x00;

	if(DEBUG)
		printf("Size of the data in icmp %ld\n",sizeof(*pkt));

	memcpy(data,pkt,(IP_HDR_LEN + 8));
}

void process_arp_req(int pf_sockfd,struct arp_pkt *pkt,int recv_if_index){

	struct arp_cache_entry *arp_entry = NULL;

	if(DEBUG)
		printf("Processing the received arp_req at node %s\n",Gethostname());

	// check if this arp_req is for me. if so send a arp_reply.
	if(arp_req_is_for_me(pkt->target_ip_address)){

		if(DEBUG)
			printf("ARP_REQ belongs to me at node %s\n",Gethostname());

		update_arp_cache(pkt->snd_ethr_addr,pkt->snd_ip_address,recv_if_index,INVALID_SOCK_DESC);
		send_arp_reply(pf_sockfd,pkt);
	}
}

void process_arp_rep(struct arp_pkt *pkt){

	struct arp_cache_entry *temp;

	struct uds_arp_msg uds_msg;

	if((temp = get_hw_addr_arp_cache(pkt->snd_ip_address)) != NULL){

		if(DEBUG){
			printf("Mac address from Packet\n");
			printHWADDR(pkt->snd_ethr_addr);
		}

		memcpy(temp->hw_address,pkt->snd_ethr_addr,ETH_ALEN);

		memcpy(uds_msg.hw_addr.sll_addr, temp->hw_address,ETH_ALEN);

		uds_msg.hw_addr.sll_halen = ETH_ALEN;

		uds_msg.hw_addr.sll_hatype = ETH_HA_TYPE;

		uds_msg.hw_addr.sll_ifindex = 2;

		if(temp->cli_sockdes > 0){

			if(sendto(temp->cli_sockdes,&uds_msg,sizeof(uds_msg),0,NULL,0) < 0){

				printf("Error in sendto in uds socket %s\n",strerror(errno));
			}

			temp->cli_sockdes = INVALID_SOCK_DESC;
			close(temp->cli_sockdes);

			print_arp_cache();
		}else{

			printf("Socket Descriptor is less than zero in the sender.......**********");
		}


	}else{
		printf("Wierd the sender ip addr is not in arp_cache\n");
	}
}

void send_arp_req(int pf_sockfd,char *target_ip_addr){

	printf("Sending the arp_req at source node %s for ip_address %s\n",Gethostname(),target_ip_addr);

	struct arp_pkt *pkt;

	struct sockaddr_ll addr_ll;

	char *buff = (char*)malloc(ETHR_FRAME_SIZE);

	pkt = build_arp_pkt(ARP_REQ,my_hw_addr_head->mac_addr,my_hw_addr_head->ip_addr,target_ip_addr);

	build_eth_frame(buff,BROADCAST_MAC_ADDRESS,my_hw_addr_head->mac_addr,ETH0_INDEX,&addr_ll,pkt,PACKET_BROADCAST);

	Sendto(pf_sockfd,buff,addr_ll,"ARP_REQ");
}

void send_arp_reply(int pf_sockfd,struct arp_pkt *pkt){

	char *buff = (char *)malloc(ETHR_FRAME_SIZE);

	struct sockaddr_ll addr_ll;

	unsigned char temp_dest_mac[6];

	unsigned char temp_ip_addr[128];

	strcpy(temp_ip_addr,pkt->snd_ip_address);

	memcpy(temp_dest_mac,pkt->snd_ethr_addr,ETH_ALEN);

	strcpy(pkt->snd_ip_address,my_hw_addr_head->ip_addr);

	memcpy(pkt->snd_ethr_addr,my_hw_addr_head->mac_addr,ETH_ALEN);

	strcpy(pkt->target_ip_address,temp_ip_addr);

	memcpy(pkt->target_ethr_addr,temp_dest_mac,ETH_ALEN);

	pkt->op = ARP_REP;

	build_eth_frame(buff,pkt->target_ethr_addr,my_hw_addr_head->mac_addr,ETH0_INDEX,&addr_ll,pkt,PACKET_OTHERHOST);

	Sendto(pf_sockfd,buff,addr_ll,"ARP_REPLY");

}


void update_arp_cache(unsigned char *eth_addr,char *ip_address,int recv_if_index,int cli_sock_desc){

	struct arp_cache_entry *cache_entry = (struct arp_cache_entry *)malloc(sizeof(struct arp_cache_entry));

	printf("%s is updating its ARP_CACHE\n",Gethostname());

	if(eth_addr != NULL){
		memcpy(cache_entry->hw_address,eth_addr,ETH_ALEN);
	}

	strcpy(cache_entry->ip_address,ip_address);

	cache_entry->sll_hatype = ETH_HA_TYPE;

	cache_entry->sll_ifindex = recv_if_index;

	cache_entry->cli_sockdes = cli_sock_desc;

	cache_entry->next = NULL;

	if(arp_cache_head == NULL){
		arp_cache_head = cache_entry;
		arp_cache_rear = cache_entry;
	}else{

		if(get_hw_addr_arp_cache(ip_address) == NULL){
			arp_cache_rear->next = cache_entry;
			arp_cache_rear= cache_entry;
		}
	}

}


bool arp_req_is_for_me(char *ip_address){

	struct ip_addr_hw_addr_pr *temp = my_hw_addr_head;
	while(temp!=NULL){

		if(!strcmp(temp->ip_addr,ip_address)){
			return true;
		}
	}

	return false;

}

struct arp_cache_entry* get_hw_addr_arp_cache(char *ip_address){

	struct arp_cache_entry *temp = arp_cache_head;

	while(temp!=NULL){

		if(!strcmp(temp->ip_address,ip_address)){
			if(DEBUG)
				printf("IpAddress is in arp cache\n");

			return temp;
		}
		temp= temp->next;
	}

	return NULL;
}


void convertToNetworkOrder(struct arp_pkt *pkt){

	pkt->hard_type =  htons(pkt->hard_type);
	pkt->prot_type = htons(pkt->prot_type);
	pkt->hard_size = htons(pkt->hard_size);
	pkt->prot_size = htons(pkt->prot_size);
	pkt->op = htons(pkt->op);
}

void convertToHostOrder(struct arp_pkt *pkt){

	pkt->hard_type =  ntohs(pkt->hard_type);
	pkt->prot_type = ntohs(pkt->prot_type);
	pkt->hard_size = ntohs(pkt->hard_size);
	pkt->prot_size = ntohs(pkt->prot_size);
	pkt->op = ntohs(pkt->op);
}

/**
 *  Wrapper method for sending the packet.
 */
void Sendto(int pf_sockfd, char* buffer, struct sockaddr_ll addr_ll,char *sendType)
{
	if(sendto(pf_sockfd,buffer,ETHR_FRAME_SIZE,0,
					(SA *)&addr_ll,sizeof(addr_ll)) < 0){
		printf("%s error in sending packet type %s due to %s\n",Gethostname(),sendType, strerror(errno));
		exit(-1);
	}

	if(DEBUG)
		printf("Sent a pkt type %s\n",sendType);
}

void print_arp_cache(){

	struct arp_cache_entry *temp = arp_cache_head;

	while(temp != NULL){

		printf("IP Address :%s ",temp->ip_address);

		printf("Mac Address ");
		printHWADDR(temp->hw_address);
		temp = temp->next;
	}

}
