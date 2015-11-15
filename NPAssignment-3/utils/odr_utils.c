/*
 * odr_utils.c
 *
 *  Created on: Nov 12, 2015
 *      Author: sujan
 */

#include "../src/usp.h"

//int pf_sockfd = create_pf_socket();

unsigned char BROADCAST_MAC_ADDRESS[6]=  {0xff,0xff,0xff,0xff,0xff,0xff};

struct odr_frame_node *rreq_list_head;
struct odr_frame_node *rreq_list_rear;
struct odr_frame_node *rreply_list_head;
struct odr_frame_node *rreply_list_rear;
void *buffer;


struct hwa_info	*hw_head;


void odr_init(char inf_mac_addr_map[10][20]){
	hw_head = get_hw_addrs();
	fill_inf_mac_addr_map(hw_head, inf_mac_addr_map);
	buffer = (void*)malloc(EHTR_FRAME_SIZE);
}



int create_pf_socket(){

	return socket(PF_PACKET,SOCK_RAW,htons(ODR_GRP_TYPE));
}

void* create_eth_buffer(){
	return (void*)malloc(EHTR_FRAME_SIZE);
}

unsigned int getNumOfInfs(struct hwa_info *head){

	unsigned int size;

	struct hwa_info *temp;
	temp = head;

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
 */
void send_frame_rreq(int pf_sockfd,int recv_inf_index,
		struct odr_frame *frame){

	struct sockaddr_ll addr_ll;
	struct hwa_info *hw_temp;
	unsigned char dest_mac[6];
	unsigned char src_mac[6];
	int send_result = 0;

	for(hw_temp = hw_head;
			hw_temp != NULL; hw_temp = hw_temp->hwa_next){

		if(hw_temp->if_index != recv_inf_index){

//			strcpy(dest_mac,BROADCAST_MAC_ADDRESS);
//			strcpy(src_mac,hw_temp->if_haddr);

//			bzero(&addr_ll,sizeof(addr_ll));


			memset(buffer,'\0',sizeof(buffer));
			build_eth_frame(buffer,BROADCAST_MAC_ADDRESS,
						hw_temp->if_haddr,hw_temp->if_index,
						&addr_ll,frame,PACKET_BROADCAST);

//			unsigned char* etherhead = buffer;
//
//			unsigned char* data = buffer + ETH_HDR_LEN;
//
//			struct ethhdr *eh = (struct ethhdr *)etherhead;
//
//			addr_ll.sll_family   = AF_PACKET;
//			addr_ll.sll_ifindex  = hw_temp->if_index;
//			addr_ll.sll_pkttype  = PACKET_BROADCAST;
//			addr_ll.sll_protocol = htons(ODR_GRP_TYPE);
//			addr_ll.sll_halen    = ETH_ALEN;
//
//
//			/*set the frame header*/
//
//			if(DEBUG){
//				printf("src addr %x %d\n",hw_temp->if_haddr[0],hw_temp->if_index);
//				printf("frame_size :%lu\n",EHTR_FRAME_SIZE);
//			}
//
//			memcpy((void*)buffer, (void*)dest_mac, ETH_ALEN);
//			memcpy((void*)(buffer+ETH_ALEN), (void*)src_mac, ETH_ALEN);
//
//			printf("eth src addr %x\n",src_mac[0]);
//			eh->h_proto = htons(ODR_GRP_TYPE);
//
//			memcpy(addr_ll.sll_addr,(void*)dest_mac, ETH_ALEN);

			/**
			 *  blanking out the last two values.
//			 */
//			addr_ll.sll_addr[6] = 0x00;
//			addr_ll.sll_addr[7] = 0x00;
//
//			memcpy(data,frame,sizeof(struct odr_frame));

			// TODO : have to convert dest_mac into presentation format.
			printf("ODR at node %s is sending frame rreq- src: %s dest_mac :%s\n",
															Gethostname(),Gethostname(),"have to print the mac_address of dest");

			if(sendto(pf_sockfd,buffer,EHTR_FRAME_SIZE,0,
							(struct sockaddr *)&addr_ll,sizeof(addr_ll)) < 0){
				printf("Error in R_REQ send to %s\n",strerror(errno));
				exit(-1);
			}

		}
	}
}

/**
 *  RReply has to be forwarded until it reaches the src_ip address.
 *  1.If the destination address in the rreply matches the vm's canonical IP Address
 *    then we need to find the matching rreq and delete it and send the application
 *    payload.
 *  2. If the destination address doesn't match the ip address of the vm's then search
 *     routing_entry in the routing table, if exists forward the frame to the out-going interface
 *  3. Else create a rreq for the destination and then send the rreply for that destination.
 */
void process_received_rreply_frame(int pf_sockid,int received_inf_ind,struct odr_frame *received_frame,
		char *src_mac_addr, char* dst_mac_addr, bool is_belongs_to_me){

	update_routing_table(received_frame->hdr.cn_src_ipaddr, src_mac_addr, received_frame->hdr.hop_count,received_inf_ind);
	if(remove_rreq_frame(received_frame) || is_belongs_to_me)
	{
		send_frame_rreply(pf_sockid, received_frame,INVALID_HOP_COUNT,dst_mac_addr,is_belongs_to_me);
	}
	else
	{
		printf("Discarding R_RPLY as corresponding R_REQ doesn't exist\n");
	}
}



//void send_frame_with_route_entry(struct odr_frame *received_frame,struct route_entry *rt,
//			int pf_sockid,char *src_mac_address, char *my_mac_addr,int inf_index){
//
//	struct sockaddr_ll addr_ll;
//
//	switch(received_frame->hdr.pkt_type){
//
//	case R_REQ:
//		//TODO : Have to implement rreply sent.
//		build_rreply_odr_frame(received_frame,rt->hop_count);
//		memset(buffer,'\0',sizeof(buffer));
//		build_eth_frame(buffer,src_mac_address,
//				my_mac_addr,inf_index,
//								&addr_ll,received_frame);
//
//		if(sendto(pf_sockid,buffer,EHTR_FRAME_SIZE,0,
//									(struct sockaddr *)&addr_ll,sizeof(addr_ll)) < 0){
//			printf("Error in R_Reply Send %s\n",strerror(errno));
//			exit(-1);
//		}
//		break;
//	case R_REPLY:
//		add_entry_in_rtable(received_frame->hdr.cn_src_ipaddr,
//				src_mac_address,received_frame->hdr.hop_count,inf_index);
//
//		memset(buffer,'\0',sizeof(buffer));
//		build_eth_frame(buffer,rt->next_hop_mac_address,
//										src_mac_address,rt->outg_inf_index,
//										&addr_ll,received_frame, PACKET_BROADCAST);
//
//		if(sendto(pf_sockid,buffer,EHTR_FRAME_SIZE,0,
//									(struct sockaddr *)&addr_ll,sizeof(addr_ll)) < 0){
//			printf("Error in R_Req Send %s\n",strerror(errno));
//			exit(-1);
//		}
//
//	}
//
//}

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
void process_received_rreq_frame(int pf_sockid,int received_inf_ind,
			struct odr_frame *received_frame,char *src_mac_addr,char *dst_mac_addr){

	struct odr_frame org_received_frame = *received_frame;
	struct route_entry *rt;
	int result = -1;

	result =  is_inefficient_frame_exists(received_frame);

	if(result == EFFICIENT_FRAME_EXISTS){
			return;
	}

	/**
	 *  We will forward the frame only in case if the rreq was new/
	 *  its a better rreq than the currently saved rreq for the brodcast-id
	 *  and sender ip address.
	 */
	if(result == FRAME_NOT_EXISTS || result == INEFFICIENT_FRAME_EXISTS){

		if((rt = get_rentry_in_rtable(received_frame->hdr.cn_dsc_ipaddr)) != NULL){
			send_frame_rreply(pf_sockid,received_frame, rt->hop_count,dst_mac_addr,false);
			if(update_routing_table(org_received_frame.hdr.cn_src_ipaddr, src_mac_addr, org_received_frame.hdr.hop_count, received_inf_ind)){
				org_received_frame.hdr.rreply_sent = 1;
				send_frame_rreq(pf_sockid, received_inf_ind, &org_received_frame);
			}
		}else{

			if(result == FRAME_NOT_EXISTS){
				store_rreq_frame(received_frame);
			}

			update_routing_table(org_received_frame.hdr.cn_src_ipaddr, src_mac_addr, org_received_frame.hdr.hop_count, received_inf_ind);
			send_frame_rreq(pf_sockid,received_inf_ind,received_frame);
		}
	}
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
		memcpy(addr_ll.sll_addr,hw_temp->if_haddr,8);
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


void store_rreq_frame(struct odr_frame *frame)
{
	struct odr_frame_node *frame_node;
	frame_node = (struct odr_frame_node*)malloc(sizeof(struct odr_frame_node));
	frame_node->frame = *frame;
	frame_node->next = NULL;

	if(rreq_list_head == NULL)
	{
		rreq_list_head = frame_node;
		rreq_list_rear = rreq_list_head;
	}else{
		rreq_list_rear -> next = frame_node;
		rreq_list_rear = frame_node;
	}

}

bool remove_rreq_frame(struct odr_frame *frame)
{
	struct odr_frame_node *temp = rreq_list_head;

	if(rreq_list_head == NULL)
		return false;

	if(temp -> frame.hdr.broadcast_id == frame->hdr.broadcast_id &&
			(strcmp(temp -> frame.hdr.cn_src_ipaddr, frame->hdr.cn_dsc_ipaddr) == 0))
	{
		rreq_list_head = rreq_list_head -> next;
		free(temp);
		return true;
	}

	struct odr_frame_node *prev = temp;
	temp = temp -> next;
	while(temp != NULL)
	{
		if(temp -> frame.hdr.broadcast_id == frame->hdr.broadcast_id &&
					(strcmp(temp -> frame.hdr.cn_src_ipaddr, frame->hdr.cn_src_ipaddr) == 0))
		{
			if(rreq_list_rear == temp)
			{
				rreq_list_rear = prev;
			}
			prev -> next = temp -> next;
			free(temp);
			return true;
		}
		prev = temp;
		temp = temp -> next;
	}
	return false;
}

int is_inefficient_frame_exists(struct odr_frame *frame)
{
	struct odr_frame_node *temp = rreq_list_head;

	while(temp != NULL)
	{
		if(!strcmp(temp -> frame.hdr.cn_src_ipaddr, frame->hdr.cn_src_ipaddr) &&
				temp -> frame.hdr.broadcast_id == frame->hdr.broadcast_id)
		{
			printf("Source addr and broadcast_id matching\n");
			if(temp -> frame.hdr.hop_count > frame->hdr.hop_count)
			{
				if("Existing rreq hop_count is greater than new rreq hop_count. So updating frame in list\n")
				temp -> frame = *frame;
				return INEFFICIENT_FRAME_EXISTS;
			}else{
				return EFFICIENT_FRAME_EXISTS;
			}
		}
	}
	return FRAME_NOT_EXISTS;
}

void send_frame_rreply(int pf_sockid, struct odr_frame *frame,int hop_count,char *src_mac_addr, bool is_belongs_to_me)
{
	struct route_entry *rt;
	struct sockaddr_ll addr_ll;

	char dst_addr[20];

	if(frame->hdr.pkt_type == R_REQ)
	{
		strcpy(dst_addr,frame->hdr.cn_src_ipaddr);

		if(is_belongs_to_me){
			if(DEBUG)
				printf("Updating ZERO hop count\n");
			build_rreply_odr_frame(frame, ZERO_HOP_COUNT);
		}else{
			if(DEBUG)
				printf("Updating received hop count %d\n", hop_count);
			build_rreply_odr_frame(frame, hop_count);
		}
	}
	else if(frame->hdr.pkt_type == R_REPLY)
	{
		strcpy(dst_addr,frame->hdr.cn_dsc_ipaddr);
	}

	if(DEBUG)
		printf("Dest address %s\n",dst_addr);
	if((rt = get_rentry_in_rtable(dst_addr)) != NULL){

		if(DEBUG)
			printf("route entry exists\n");

		memset(buffer,'\0',sizeof(buffer));
		build_eth_frame(buffer,rt->next_hop_mac_address,
				src_mac_addr,rt->outg_inf_index,
						&addr_ll,frame, PACKET_OTHERHOST);

		if(DEBUG)
			printf("built frame\n");

		if(sendto(pf_sockid,buffer,EHTR_FRAME_SIZE,0,
											(struct sockaddr *)&addr_ll,sizeof(addr_ll)) < 0){
			printf("Error in R_RPLY Send %s\n",strerror(errno));
			exit(-1);
		}
	}
	else
	{
		// Generate R_REQ and then send R_RPLY
		printf("Wierd no entry exists\n");
	}
}
