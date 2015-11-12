/*
 * odr_utils.c
 *
 *  Created on: Nov 12, 2015
 *      Author: sujan
 */

#include "../src/usp.h"

struct hwa_info	*hw_head = get_hw_addrs();

//int pf_sockfd = create_pf_socket();

char BROADCAST_MAC_ADDRESS[6]=  {'ff','ff','ff','ff','ff','ff'};

int create_pf_socket(){

	return Socket(PF_PACKET,SOCK_RAW,htons(ODR_GRP_TYPE));

}

unsigned int getNumOfInfs(struct hwa_info *head){

	unsigned int size;

	struct hwa_info *temp = head;

	while(temp!=NULL){
		size++;
		temp = temp->hwa_next;
	}

	return size;
}

/**
 * Have written only for rreq.
 * Will see whether to name the method as rreq/ a generic method
 * for all the types of the packets.
 *
 * TODO:
 * 	Append the broadcast_id and hop_count in the rreq.
 * 	ideally good to have a structure for this.
 */
void send_frame_rreq(int pf_sockfd,int recv_inf_index, int packet_type,
				struct  reply_from_uds_client *reply){

	struct sockaddr_ll addr_ll;
	struct hwa_info *hw_temp;
	unsigned char dest_mac[6];
	int send_result = 0;

	for(hw_temp = hw_head;
			hw_temp != NULL; hw_temp = hw_temp->hwa_next){

		if(hw_temp->if_index != recv_inf_index){

			strcpy(dest_mac,BROADCAST_MAC_ADDRESS);

			bzero(&addr_ll,sizeof(addr_ll));

			void* buffer = (void*)malloc(strlen(reply->canonical_ipAddress) + ETH_HDR_LEN + 1);

			unsigned char* etherhead = buffer;

			unsigned char* data = buffer + ETH_HDR_LEN;

			struct ethhdr *eh = (struct ethhdr *)etherhead;

			addr_ll.sll_family   = AF_PACKET;
			addr_ll.sll_ifindex  = hw_temp->if_index;
			addr_ll.sll_pkttype  = PACKET_BROADCAST;
			addr_ll.sll_protocol = htons(ODR_GRP_TYPE);
			addr_ll.sll_halen    = ETH_ALEN;


			/*set the frame header*/
			memcpy((void*)buffer, (void*)dest_mac, ETH_ALEN);
			memcpy((void*)(buffer+ETH_ALEN), (void*)hw_temp->if_haddr, ETH_ALEN);
			eh->h_proto = htons(ODR_GRP_TYPE);

			memcpy(addr_ll.sll_addr,(void*)dest_mac, ETH_ALEN);

			/**
			 *  blanking out the last two values.
			 */
			addr_ll.sll_addr[6] = 0x00;
			addr_ll.sll_addr[7] = 0x00;

			strcpy(data,reply->canonical_ipAddress);

			if(sendto(pf_sockfd,buffer,strlen(buffer),
							(struct sockaddr *)&addr_ll,sizeof(addr_ll)) < 0){
				printf("Error in send to %s\n",strerror(errno));
				exit(-1);
			}

		}
	}
}

/**
 *  Idea :
 *  	To parse the incoming frame.
 *  	Options:
 *  		1. Check from the payload the destination IpAddress,
 *  		if the current process has the entry in the routing table
 *  		send a rreply else broadcast the frame to all the remaining
 *  		interfaces.
 *  		2. If broadcasting further add the entry in the routing table
 *  			this constructs the reverse route to the source, increase
 *  			the hop_count.
 *
 *
 */
void process_received_frame(){



}

/**
 *  This function returns the pf_sock_ids array
 *  for all the interfaces and it also populates
 *  the interface_sockid_map which has interface_id
 *  as key and sock_id as value.
 *
 */
int* bindInterfaces(int *inf_sockid_map){

	struct hwa_info	*hw_temp = hw_head;

	struct sockaddr_ll addr_ll;
	int pf_sockfd;
	unsigned int num_of_infs = 0,i=0;

	num_of_infs = getNumOfInfs(hw_head);

	int *inf_sockfds = (int *)malloc(num_of_infs*sizeof(int));

	for(hw_temp = hw_head;
				hw_temp != NULL; hw_temp = hw_temp->hwa_next){


		bzero(&addr_ll,sizeof(addr_ll));

		addr_ll.sll_family = PF_PACKET;
		memcpy(addr_ll.sll_addr,hw_temp->if_haddr);
		addr_ll.sll_ifindex = hw_temp->if_index;
		addr_ll.sll_protocol = htons(ODR_GRP_TYPE);

		pf_sockfd = create_pf_socket();

		Bind(pf_sockfd,(struct sock_addr*)&addr_ll,sizeof(addr_ll));

		inf_sockfds[i] = pf_sockfd;

		inf_sockid_map[hw_temp->if_index] = pf_sockfd;
		i++;
	}
	return inf_sockfds;
}


bool is_route_exists(char *canonical_ipAddress){

	return false;
}



