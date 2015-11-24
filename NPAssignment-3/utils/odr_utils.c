/*
 * odr_utils.c
 *
 *  Created on: Nov 12, 2015
 *      Author: sujan, siddhu
 */

#include "../src/usp.h"

unsigned char BROADCAST_MAC_ADDRESS[6]=  {0xff,0xff,0xff,0xff,0xff,0xff};

struct odr_frame_node *next_to_list_head;
struct odr_frame_node *next_to_list_rear;
void *buffer;
struct hwa_info	*hw_head;

/**
 *  Initialization method for initializing all odr helpful structures.
 */
void odr_init(char inf_mac_addr_map[MAX_INTERFACES][ETH_ALEN]){
	hw_head = get_hw_addrs();
	fill_inf_mac_addr_map(hw_head, inf_mac_addr_map);
	buffer = (void*)malloc(EHTR_FRAME_SIZE);
	build_port_entries();
}

/**
 *  Helper method for creating the pf_socket.
 */
int create_pf_socket(){

	return socket(PF_PACKET,SOCK_RAW,htons(ODR_GRP_TYPE));
}

/**
 *  Helper method for creating the Ethernet buffer.
 */
void* create_eth_buffer(){
	return (void*)malloc(EHTR_FRAME_SIZE);
}

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

			printf("ODR at node %s is sending packet with type %s - src: %s dest ",
															Gethostname(),get_packet_type(frame->hdr.pkt_type),Gethostbyaddr(frame->hdr.cn_src_ipaddr));
			printHWADDR(BROADCAST_MAC_ADDRESS);

			Sendto(pf_sockfd, buffer, addr_ll,"R_REQ");
		}
	}
}

void process_received_rreply_frame(int pf_sockid,int received_inf_ind,struct odr_frame *received_frame,
		char *src_mac_addr, char* dst_mac_addr, bool is_belongs_to_me){

	bool route_updated = update_routing_table(received_frame->hdr.cn_src_ipaddr, src_mac_addr,
			received_frame->hdr.hop_count, received_inf_ind, received_frame->hdr.force_route_dcvry, received_frame->hdr.pkt_type,received_frame->hdr.broadcast_id);
	if(route_updated || is_belongs_to_me)
	{
		send_frame_rreply(pf_sockid, received_frame, INVALID_HOP_COUNT, is_belongs_to_me);
	}
	else
	{
		printf("Discarding R_RPLY as R_RPLY has already been sent\n");
	}
}

void process_received_rreq_frame(int pf_sockid,int received_inf_ind,
			struct odr_frame *received_frame,char *src_mac_addr,char *dst_mac_addr){

	struct odr_frame org_received_frame = *received_frame;
	struct route_entry *rt;
	int result = -1;

	bool route_updated = update_routing_table(org_received_frame.hdr.cn_src_ipaddr, src_mac_addr,
									org_received_frame.hdr.hop_count, received_inf_ind, received_frame->hdr.force_route_dcvry,
									received_frame->hdr.pkt_type,received_frame->hdr.broadcast_id);

	if((rt = get_rentry_in_rtable(received_frame->hdr.cn_dsc_ipaddr, received_frame->hdr.force_route_dcvry,
							received_frame->hdr.pkt_type)) != NULL){

		if(DEBUG)
			printf("Routing entery for destination exists. Hence sending a r-reply\n");

		printf("ODR at node %s has a route to %s in its routing table\n",Gethostname(),Gethostbyaddr(received_frame->hdr.cn_dsc_ipaddr));

		if(!received_frame->hdr.rreply_sent){
			send_frame_rreply(pf_sockid,received_frame, rt->hop_count,false);
		}else{
			printf("ODR at node %s received a rreq packet with R_reply sent flag hence not sending a r_reply\n",Gethostname());
		}

		if(route_updated){

			if(DEBUG)
				printf("Updated routing table\n");

			org_received_frame.hdr.rreply_sent = R_REPLY_SENT;
			send_frame_rreq(pf_sockid, received_inf_ind, &org_received_frame);
		}
	}else{

		if(route_updated){

			if(DEBUG)
				printf("No Route for the destination hence forwarding the r_req_packet\n");

			send_frame_rreq(pf_sockid,received_inf_ind,received_frame);
		}
	}
}

void store_next_to_send_frame(struct odr_frame *frame){

	struct odr_frame_node *frame_node;
	frame_node = (struct odr_frame_node*)malloc(sizeof(struct odr_frame_node));
	frame_node->frame = *frame;
	frame_node->next = NULL;

	if(next_to_list_head == NULL)
	{
		if(DEBUG)
			printf("storing the frame with r_req %d\n",frame_node->frame.hdr.rreq_id);

		next_to_list_head = frame_node;
		next_to_list_rear = next_to_list_head;
	}else{
		next_to_list_rear -> next = frame_node;
		next_to_list_rear = frame_node;
	}

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

		printf("ODR at node %s has a route to %s in its routing table\n",Gethostname(),Gethostbyaddr(dst_addr));

		if(DEBUG){
			printf("sending src_mac_addr in rreply\n");
			printHWADDR(src_mac_addr);
		}

		if(DEBUG)
			printf("route entry exists\n");

		memset(buffer,'\0',sizeof(buffer));
		build_eth_frame(buffer,rt->next_hop_mac_address,
				src_mac_addr,rt->outg_inf_index,
						&addr_ll,frame, PACKET_OTHERHOST);

		if(DEBUG)
			printf("built frame\n");

		printf("ODR at node %s is sending packet with type %s - src: %s dest ",
																	Gethostname(),get_packet_type(frame->hdr.pkt_type),Gethostbyaddr(frame->hdr.cn_src_ipaddr));


		printHWADDR(rt->next_hop_mac_address);

		Sendto(pf_sockfd, buffer, addr_ll, "R_RPLY");
	}
	else
	{
		if(DEBUG)
			printf("No route entry exists. So starting R_REQ for R_RPLY\n");

		store_next_to_send_frame(frame);

		struct odr_frame outg_frame;

		if(DEBUG)
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
 * sending the payload frame.
 */
void send_frame_payload(int pf_sockfd,struct odr_frame *frame,
			char *nxt_hop_addr,int outg_inf_index){

	struct sockaddr_ll addr_ll;

	bzero(&addr_ll,sizeof(addr_ll));

	char src_mac_addr[ETH_ALEN];

	memcpy(src_mac_addr,get_inf_mac_addr(outg_inf_index),ETH_ALEN);

	if(DEBUG){
		printf("Set src_mac_addr in payload\n");
		printHWADDR(src_mac_addr);
	}

	memset(buffer,'\0',sizeof(buffer));

	if(DEBUG){
		printf("src ip-address in payload %s\n",Gethostbyaddr(frame->hdr.cn_src_ipaddr));
		printf("dest ip-address in payload %s\n",Gethostbyaddr(frame->hdr.cn_dsc_ipaddr));
	}

	char *src = (char *)malloc(sizeof(char)*20);

	strcpy(src,Gethostbyaddr(frame->hdr.cn_src_ipaddr));

	char *dst =  (char *)malloc(sizeof(char)*20);

	strcpy(dst,Gethostbyaddr(frame->hdr.cn_dsc_ipaddr));

	printf("ODR at node %s sending msg type : %s - src : %s - dest : %s\n",
										Gethostname(),get_packet_type(frame->hdr.pkt_type),src,
										dst);

	build_eth_frame(buffer,nxt_hop_addr,
						src_mac_addr,outg_inf_index,
								&addr_ll,frame, PACKET_OTHERHOST);

	if(DEBUG){
		printf("Sending the frame in ODR\n");
		printf("Sending payload frame to mac-addr");
		printHWADDR(nxt_hop_addr);
	}

	if(DEBUG){
		printf("Sending payload from this interface %d mac-address ", outg_inf_index);
		printHWADDR(src_mac_addr);
		printf("Sending the payload frame and outg inf :%s %d\n",frame->hdr.cn_dsc_ipaddr,outg_inf_index);
	}

	Sendto(pf_sockfd, buffer, addr_ll,"PAY_LOAD");
}

void forward_frame_payload(int pf_sockfd,struct odr_frame *frame)
{
	struct route_entry* rt;
	if((rt = get_rentry_in_rtable(frame->hdr.cn_dsc_ipaddr, frame->hdr.force_route_dcvry, frame->hdr.pkt_type)) != NULL)
	{
		printf("ODR at node %s has a route to %s in its routing table\n",Gethostname(),Gethostbyaddr(frame->hdr.cn_dsc_ipaddr));

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

		store_next_to_send_frame(frame);

		struct odr_frame outg_frame;

		printf("ODR at node %s forwarding packet with type %s with dest %s", Gethostname(),
									get_packet_type(frame->hdr.pkt_type),Gethostbyaddr(frame->hdr.cn_dsc_ipaddr));

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

		if(DEBUG){
			printf("temp frame.hdr.cn_dsc_ipaddr %s ?= ",temp->frame.hdr.cn_dsc_ipaddr);
			printf("frame hdr.cn_src_ipaddr %s\n ",frame->hdr.cn_src_ipaddr);
		}

		if(!strcmp(temp-> frame.hdr.cn_dsc_ipaddr, frame->hdr.cn_src_ipaddr)){

			if(DEBUG)
				printf("retrieving the payload frame with rreq id and found matching rreq_id : %d\n", temp->frame.hdr.rreq_id);

			temp->frame.hdr.force_route_dcvry = 0;
			return &(temp->frame);
		}
		temp = temp->next;
	}
	return NULL;
}

/**
 *  Helper method for removing the saved frames.
 */
bool remove_frame(struct odr_frame *frame)
{
	struct odr_frame_node *temp = next_to_list_head;

	if(next_to_list_head == NULL)
		return false;

	if(!strcmp(temp-> frame.hdr.cn_dsc_ipaddr, frame->hdr.cn_src_ipaddr))
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
		if(!strcmp(temp-> frame.hdr.cn_dsc_ipaddr, frame->hdr.cn_src_ipaddr))
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
	bool route_updated = update_routing_table(received_frame->hdr.cn_src_ipaddr, src_mac_addr,
									received_frame->hdr.hop_count, received_inf_index, received_frame->hdr.force_route_dcvry,
									received_frame->hdr.pkt_type,received_frame->hdr.broadcast_id);

	struct odr_frame* frame_to_send;

	if(DEBUG)
		printf("Entered into the send_frame_for_rrply\n");

	while((frame_to_send =  get_next_send_packet(received_frame)) != NULL)
	{
		if(frame_to_send->hdr.pkt_type == R_REPLY)
		{
			if(DEBUG)
				printf("Got a next frame to send R_RPLY\n");

			if(route_updated || !get_route_entry_timeout())
			{
				send_rrply_to_next_hop(pf_sockfd, frame_to_send, src_mac_addr, received_inf_index);
			}
		}
		else if(frame_to_send->hdr.pkt_type == PAY_LOAD)
		{
			if(DEBUG)
				printf("Got a next frame to send PAY_LOAD\n");

			send_frame_payload(pf_sockfd, frame_to_send, src_mac_addr, received_inf_index);
		}
		remove_frame(received_frame);
	}
}

void send_rrply_to_next_hop(int pf_sockfd, struct odr_frame *frame, char* dest_mac_addr, int outg_inf_index)
{
	if(DEBUG){
		printf("Sending packet to next hop because of no route entry in routing table ");
		printHWADDR(dest_mac_addr);
	}

	printf("ODR at node %s is sending frame hdr - src: %s dest ",
																Gethostname(),Gethostbyaddr(frame->hdr.cn_src_ipaddr));

	printHWADDR(dest_mac_addr);

	struct sockaddr_ll addr_ll;

	memset(buffer, '\0', sizeof(buffer));
	build_eth_frame(buffer, dest_mac_addr, get_inf_mac_addr(outg_inf_index),
			outg_inf_index, &addr_ll, frame, PACKET_OTHERHOST);

	Sendto(pf_sockfd, buffer, addr_ll, "R_REPLY");
}

void send_frame_for_rreq(int pf_sockfd,struct odr_frame* received_frame,char* src_mac_addr,int received_inf_index){

	bool route_updated = update_routing_table(received_frame->hdr.cn_src_ipaddr, src_mac_addr,
							received_frame->hdr.hop_count,received_inf_index, received_frame->hdr.force_route_dcvry,
							received_frame->hdr.pkt_type,received_frame->hdr.broadcast_id);


	if(route_updated)
	{
		if(DEBUG)
			printf("Received a rreq destined to me hence sending rreply to next-hop\n");

		build_rreply_odr_frame(received_frame, ZERO_HOP_COUNT);

		send_rrply_to_next_hop(pf_sockfd,received_frame,src_mac_addr,received_inf_index);
	}
}


