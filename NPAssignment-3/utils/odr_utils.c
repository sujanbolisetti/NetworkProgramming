/*
 * odr_utils.c
 *
 *  Created on: Nov 12, 2015
 *      Author: sujan, siddhu
 */

#include "../src/usp.h"

//int pf_sockfd = create_pf_socket();

unsigned char BROADCAST_MAC_ADDRESS[6]=  {0xff,0xff,0xff,0xff,0xff,0xff};

struct odr_frame_node *rreq_list_head;
struct odr_frame_node *rreq_list_rear;
struct odr_frame_node *next_to_list_head;
struct odr_frame_node *next_to_list_rear;
void *buffer;


// TODO: have to check for force_route discovery.
// when r_reply sent flag is set we should not send r_reply by corresponding nodes.

struct hwa_info	*hw_head;


void odr_init(char inf_mac_addr_map[MAX_INTERFACES][ETH_ALEN]){
	hw_head = get_hw_addrs();
	fill_inf_mac_addr_map(hw_head, inf_mac_addr_map);
	buffer = (void*)malloc(EHTR_FRAME_SIZE);
	build_port_entries();
	//set_route_entry_timeout(timeout);
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

			memset(buffer,'\0',sizeof(buffer));
			build_eth_frame(buffer,BROADCAST_MAC_ADDRESS,
						hw_temp->if_haddr,hw_temp->if_index,
						&addr_ll,frame,PACKET_BROADCAST);

			printf("ODR at node %s is sending frame rreq- src: %s dest ",
															Gethostname(),frame->hdr.cn_src_ipaddr);
			printHWADDR(BROADCAST_MAC_ADDRESS);

			Sendto(pf_sockfd, buffer, addr_ll,"R_REQ");
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

	update_routing_table(received_frame->hdr.cn_src_ipaddr, src_mac_addr,
			received_frame->hdr.hop_count, received_inf_ind, received_frame->hdr.force_route_dcvry, received_frame->hdr.pkt_type);
	if(remove_rreq_frame(received_frame) || is_belongs_to_me)
	{
		send_frame_rreply(pf_sockid, received_frame, INVALID_HOP_COUNT, is_belongs_to_me);
	}
	else
	{
		printf("Discarding R_RPLY as corresponding R_REQ doesn't exist\n");
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
void process_received_rreq_frame(int pf_sockid,int received_inf_ind,
			struct odr_frame *received_frame,char *src_mac_addr,char *dst_mac_addr){

	struct odr_frame org_received_frame = *received_frame;
	struct route_entry *rt;
	int result = -1;

	printf("Before function call\n");
	result =  is_inefficient_frame_exists(received_frame);
	printf("R_REQ status %d\n", result);

	if(result == EFFICIENT_FRAME_EXISTS){
		printf("Efficient R_REQ already sent so no action\n");
		return;
	}

	/**
	 *  We will forward the frame only in case if the rreq was new/
	 *  its a better rreq than the currently saved rreq for the brodcast-id
	 *  and sender ip address.
	 */
	if(result == FRAME_NOT_EXISTS || result == INEFFICIENT_FRAME_EXISTS){
		printf("R_REQ status %d. Forwarding R_REQ\n", result);
		if((rt = get_rentry_in_rtable(received_frame->hdr.cn_dsc_ipaddr, received_frame->hdr.force_route_dcvry,
				received_frame->hdr.pkt_type)) != NULL){
			printf("Routing entry exists for dest ip addr %s", received_frame->hdr.cn_dsc_ipaddr);
			bool route_updated = update_routing_table(org_received_frame.hdr.cn_src_ipaddr, src_mac_addr,
								org_received_frame.hdr.hop_count, received_inf_ind, received_frame->hdr.force_route_dcvry, received_frame->hdr.pkt_type);
			send_frame_rreply(pf_sockid,received_frame, rt->hop_count,false);
			if(route_updated){
				printf("Updated routing table\n");
				org_received_frame.hdr.rreply_sent = R_REPLY_SENT;
				send_frame_rreq(pf_sockid, received_inf_ind, &org_received_frame);
			}
		}else{

			if(result == FRAME_NOT_EXISTS){
				printf("R_REQ frame not exists. Forwarding R_REQ in route not exist\n");
				store_rreq_frame(received_frame);
			}

			update_routing_table(org_received_frame.hdr.cn_src_ipaddr, src_mac_addr, org_received_frame.hdr.hop_count,
					received_inf_ind, received_frame->hdr.force_route_dcvry, received_frame->hdr.pkt_type);
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

void store_next_to_send_frame(struct odr_frame *frame){

	struct odr_frame_node *frame_node;
	frame_node = (struct odr_frame_node*)malloc(sizeof(struct odr_frame_node));
	frame_node->frame = *frame;
	frame_node->next = NULL;

	if(next_to_list_head == NULL)
	{
		printf("storing the frame with r_req %d\n",frame_node->frame.hdr.rreq_id);
		next_to_list_head = frame_node;
		next_to_list_rear = next_to_list_head;
	}else{
		next_to_list_rear -> next = frame_node;
		next_to_list_rear = frame_node;
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
		temp = temp->next;
	}

	return FRAME_NOT_EXISTS;
}

void send_frame_rreply(int pf_sockfd, struct odr_frame *frame,
			int hop_count,bool is_belongs_to_me)
{

	struct route_entry *rt;
	struct sockaddr_ll addr_ll;

	char dst_addr[20];

	if(frame->hdr.pkt_type == R_REQ)
	{
		strcpy(dst_addr,frame->hdr.cn_src_ipaddr);

		if(is_belongs_to_me){
			if(DEBUG)
				printf("Updating ZERO hop count as R_RPLY is starting by me\n");
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
		printf("Dest address in send_fram_rrepy %s\n",dst_addr);
	if((rt = get_rentry_in_rtable(dst_addr, frame->hdr.force_route_dcvry, frame->hdr.pkt_type)) != NULL){

		char src_mac_addr[ETH_ALEN];

		memcpy(src_mac_addr,get_inf_mac_addr(rt->outg_inf_index),ETH_ALEN);

		printf("sending src_mac_addr in rreply\n");
		printHWADDR(src_mac_addr);

		if(DEBUG)
			printf("route entry exists\n");

		memset(buffer,'\0',sizeof(buffer));
		build_eth_frame(buffer,rt->next_hop_mac_address,
				src_mac_addr,rt->outg_inf_index,
						&addr_ll,frame, PACKET_OTHERHOST);

		if(DEBUG)
			printf("built frame\n");

		Sendto(pf_sockfd, buffer, addr_ll, "R_RPLY");
	}
	else
	{
		// Generate R_REQ and then send R_RPLY
		printf("No route entry exists. So starting R_REQ for R_RPLY\n");

		frame->hdr.rreq_id = get_new_rreq_id();
		store_next_to_send_frame(frame);

		struct odr_frame outg_frame;

		printf("Sending R_REQ and my ip addr %s\n", Gethostbyname(Gethostname()));
		outg_frame = build_odr_frame(Gethostbyname(Gethostname()), frame->hdr.cn_dsc_ipaddr,
					ZERO_HOP_COUNT,R_REQ, get_new_broadcast_id(), frame->hdr.rreq_id, NO_FORCE_DSC, R_REPLY_NOT_SENT, NULL, NULL);

		send_frame_rreq(pf_sockfd, -1, &outg_frame);
	}
}

struct odr_frame build_payload_frame(int rreq_id,char *my_ip_address, struct msg_from_uds *msg){

	if(DEBUG){
		printf("Building the payload frame with destination ip_address %s\n",msg->canonical_ipAddress);
	}

	return build_odr_frame(my_ip_address,msg->canonical_ipAddress,
								ZERO_HOP_COUNT,PAY_LOAD,INVALID_ENTRY,rreq_id,NO_FORCE_DSC,R_REPLY_NOT_SENT,msg->msg_received,msg);

}


/**
 *
 */
void send_frame_payload(int pf_sockfd,struct odr_frame *frame,
			char *nxt_hop_addr,int outg_inf_index){

	struct sockaddr_ll addr_ll;

	bzero(&addr_ll,sizeof(addr_ll));

	char src_mac_addr[ETH_ALEN];

	memcpy(src_mac_addr,get_inf_mac_addr(outg_inf_index),ETH_ALEN);

	printf("Set src_mac_addr in payload\n");
	printHWADDR(src_mac_addr);

	memset(buffer,'\0',sizeof(buffer));
	build_eth_frame(buffer,nxt_hop_addr,
						src_mac_addr,outg_inf_index,
								&addr_ll,frame, PACKET_OTHERHOST);

	printf("Sending the frame in ODR\n");

	printf("Sending payload frame to mac-addr");
	printHWADDR(nxt_hop_addr);

	printf("Sending payload from this interface %d mac-address ", outg_inf_index);
	printHWADDR(src_mac_addr);

	printf("Sending the payload frame and outg inf :%s %d\n",frame->hdr.cn_dsc_ipaddr,outg_inf_index);

	Sendto(pf_sockfd, buffer, addr_ll,"PAY_LOAD");
}

void forward_frame_payload(int pf_sockfd,struct odr_frame *frame)
{
	struct route_entry* rt;
	if((rt = get_rentry_in_rtable(frame->hdr.cn_dsc_ipaddr, frame->hdr.force_route_dcvry, frame->hdr.pkt_type)) != NULL)
	{
		if(DEBUG)
			printf("Found route entry to forward payload for dest ip addr %s\n", frame->hdr.cn_dsc_ipaddr);
		send_frame_payload(pf_sockfd, frame, rt->next_hop_mac_address, rt->outg_inf_index);
	}
	else
	{
		if(DEBUG)
		{
			printf("No route exist in routing table for dest ip address %s\n", frame->hdr.cn_dsc_ipaddr);
			printf("Starting R_REQ for payload\n");
		}

		frame->hdr.rreq_id = get_new_rreq_id();
		store_next_to_send_frame(frame);

		struct odr_frame outg_frame;
		printf("Sending R_REQ and my ip addr %s", Gethostbyname(Gethostname()));
		outg_frame = build_odr_frame(Gethostbyname(Gethostname()), frame->hdr.cn_dsc_ipaddr,
					ZERO_HOP_COUNT, R_REQ, get_new_broadcast_id(), frame->hdr.rreq_id, NO_FORCE_DSC, R_REPLY_NOT_SENT, NULL, NULL);

		send_frame_rreq(pf_sockfd, -1, &outg_frame);
	}
}

/**
 *
 */
struct odr_frame * get_next_send_packet(struct odr_frame *frame){

	struct odr_frame_node *temp = next_to_list_head;

	while(temp != NULL){

		if(DEBUG)
				printf("retrieving the payload/ r_reply %d frame with rreq id : %d ?= %d\n", frame->hdr.pkt_type,
						temp->frame.hdr.rreq_id, frame->hdr.rreq_id);

		printf("temp frame.hdr.cn_dsc_ipaddr %s ?= ",temp->frame.hdr.cn_dsc_ipaddr);
		printf("frame hdr.cn_src_ipaddr %s\n ",frame->hdr.cn_src_ipaddr);

		if(temp->frame.hdr.rreq_id == frame->hdr.rreq_id
				&& !strcmp(temp-> frame.hdr.cn_dsc_ipaddr, frame->hdr.cn_src_ipaddr)){

			if(DEBUG)
				printf("retrieving the payload frame with rreq id and found matching rreq_id : %d\n", temp->frame.hdr.rreq_id);

			temp->frame.hdr.force_route_dcvry = 0;
			return &(temp->frame);
		}
		temp = temp->next;
	}
	return NULL;
}

bool remove_frame(struct odr_frame *frame)
{
	struct odr_frame_node *temp = next_to_list_head;

	if(next_to_list_head == NULL)
		return false;

	if(temp->frame.hdr.rreq_id == frame->hdr.rreq_id
					&& !strcmp(temp-> frame.hdr.cn_dsc_ipaddr, frame->hdr.cn_src_ipaddr))
	{
		next_to_list_head = next_to_list_head -> next;

		if(DEBUG)
			printf("retrieving the payload frame with rreq id in removing the head : %d\n",temp->frame.hdr.rreq_id);

		free(temp);
		return true;
	}

	struct odr_frame_node *prev = temp;
	temp = temp -> next;
	while(temp != NULL)
	{
		if(temp->frame.hdr.rreq_id == frame->hdr.rreq_id
						&& !strcmp(temp-> frame.hdr.cn_src_ipaddr,frame->hdr.cn_dsc_ipaddr))
		{

			if(DEBUG)
				printf("retrieving the payload frame with rreq id in removing after head : %d\n",temp->frame.hdr.rreq_id);

			if(next_to_list_rear == temp)
			{
				next_to_list_rear = prev;
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

void send_frame_for_rrply(int pf_sockfd, struct odr_frame *received_frame, char *src_mac_addr, int received_inf_index)
{
	update_routing_table(received_frame->hdr.cn_src_ipaddr, src_mac_addr,
									received_frame->hdr.hop_count, received_inf_index, received_frame->hdr.force_route_dcvry, received_frame->hdr.pkt_type);

	struct odr_frame* frame_to_send =  get_next_send_packet(received_frame);

	if(frame_to_send->hdr.pkt_type == R_REPLY)
	{
		printf("Got a next frame to send R_RPLY\n");
		send_rrply_to_next_hop(pf_sockfd, frame_to_send, src_mac_addr, received_inf_index);
	}
	else if(frame_to_send->hdr.pkt_type == PAY_LOAD)
	{
		printf("Got a next frame to send PAY_LOAD\n");
		send_frame_payload(pf_sockfd, frame_to_send, src_mac_addr, received_inf_index);
	}
	remove_frame(received_frame);
}

void send_rrply_to_next_hop(int pf_sockfd, struct odr_frame *frame, char* dest_mac_addr, int outg_inf_index)
{
	printf("Sending packet to next hop because of no route entry in routing table %s\n", dest_mac_addr);
	struct sockaddr_ll addr_ll;
	memset(buffer, '\0', sizeof(buffer));
	build_eth_frame(buffer, dest_mac_addr, get_inf_mac_addr(outg_inf_index),
			outg_inf_index, &addr_ll, frame, PACKET_OTHERHOST);

	Sendto(pf_sockfd, buffer, addr_ll, "R_REPLY");
}

void send_frame_for_rreq(int pf_sockfd,struct odr_frame* received_frame,char* src_mac_addr,int received_inf_index){

	update_routing_table(received_frame->hdr.cn_src_ipaddr, src_mac_addr,
							received_frame->hdr.hop_count,received_inf_index, received_frame->hdr.force_route_dcvry,received_frame->hdr.pkt_type);

	printf("Received a rreq destined to me hence sending rreply to next-hop\n");

	build_rreply_odr_frame(received_frame, ZERO_HOP_COUNT);

	send_rrply_to_next_hop(pf_sockfd,received_frame,src_mac_addr,received_inf_index);
}


