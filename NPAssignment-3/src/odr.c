/*
 * odr.c
 *
 *  Created on: Nov 9, 2015
 *      Author: sujan, siddhu
 */

#include "usp.h"

char inf_mac_addr_map[MAX_INTERFACES][ETH_ALEN];
long int route_entry_timeout = 30; // default taken as 30 seconds
int broadcast_id = 0;

/*
 *  Used to retrieve the saved packet
 */
int r_req_id = 0;

int main(int argc, char **argv){

	int sockfd,pf_sockfd,maxfd;
	struct sockaddr_un odr_addr;
	struct sockaddr_ll addr_ll;
	fd_set rset;
	struct iovec iov[1];
	struct msghdr mh;
	char my_name[128];
	unsigned int interface_index;

	struct odr_frame frame;
	char my_ip_address[20];
	struct route_entry *rt;


	if(argc < 2){
		printf("Kindly enter the staleness parameter\n");
		exit(0);
	}else{
		populate_staleness_parameter(argv[1]);
	}

	gethostname(my_name,sizeof(my_name));

	strcpy(my_ip_address,Gethostbyname(my_name));

	printf("Starting ODR process on %s\n",my_name);

	sockfd = socket(AF_LOCAL,SOCK_DGRAM,0);
	pf_sockfd = create_pf_socket();

	if(pf_sockfd < 0){
		printf("error in PF_Socket creation :%s\n",strerror(errno));
	}

	odr_addr.sun_family = AF_LOCAL;

	strcpy(odr_addr.sun_path,ODR_PATH_NAME);

	unlink(ODR_PATH_NAME);

	Bind(sockfd,(SA *)&odr_addr,sizeof(odr_addr));

	FD_ZERO(&rset);

	FD_SET(sockfd, &rset);
	FD_SET(pf_sockfd,&rset);

	maxfd = max(sockfd,pf_sockfd)+1;

	odr_init(inf_mac_addr_map);

	for(;;){

		FD_SET(sockfd, &rset);

		FD_SET(pf_sockfd,&rset);

		if(DEBUG)
			printf("Waiting for select\n");

		select(maxfd,&rset,NULL,NULL,NULL);

		if(FD_ISSET(sockfd,&rset)){

			struct msg_from_uds *reply = msg_receive(sockfd);

			printf("ODR at node %s has received a message on unix domain socket from application program\n",my_name);

			struct odr_frame payload_frame;

			payload_frame = build_payload_frame(get_new_rreq_id(),my_ip_address,reply);

			if((rt = get_rentry_in_rtable(reply->canonical_ipAddress, reply->flag, PAY_LOAD)) != NULL){

				printf("ODR at node %s has a route to %s in its routing table\n",Gethostname(),Gethostbyaddr(reply->canonical_ipAddress));
				send_frame_payload(pf_sockfd,&payload_frame,rt->next_hop_mac_address,rt->outg_inf_index);

			}else{

				/**
				 *  1. Broadcast a rreq packet to all the interfaces.
				 *  2. Wait for rreply from the destination.
				 *  3. Fill in the routing table and send the payload.
				 */
				store_next_to_send_frame(&payload_frame);

				frame = build_odr_frame(my_ip_address,reply->canonical_ipAddress,
						ZERO_HOP_COUNT,R_REQ, get_new_broadcast_id(), payload_frame.hdr.rreq_id,reply->flag,R_REPLY_NOT_SENT,NULL,NULL);

				if(DEBUG){
						printf("broad_cast id %d  r_req id %d\n", r_req_id-1, broadcast_id-1);
				}

				send_frame_rreq(pf_sockfd,-1,&frame);
			}
		}

		if(FD_ISSET(pf_sockfd,&rset)){

			printf("%s received packet from other ODR\n",my_name);

			void* buffer = (void*)malloc(EHTR_FRAME_SIZE);
			int addr_len = sizeof(addr_ll);
			unsigned char src_mac_addr[6];
			unsigned char dest_mac_addr[6];


			if(recvfrom(pf_sockfd, buffer, EHTR_FRAME_SIZE, 0,(SA*)&addr_ll,&addr_len) < 0){
				printf("Error in recv_from :%s\n",strerror(errno));
				exit(0);
			}

			memcpy((void *)dest_mac_addr,(void *)buffer,ETH_ALEN);
			memcpy((void *)src_mac_addr,(void *)(buffer+ETH_ALEN),ETH_ALEN);

			struct odr_frame *received_frame  = (struct odr_frame *)(buffer + ETH_HDR_LEN);

			if(DEBUG)
				printf("Before host conversion odr pkt_type and r_reqqId: %d : %d\n",received_frame->hdr.pkt_type,received_frame->hdr.rreq_id);

			convertToHostOrder(&(received_frame->hdr));

			if(DEBUG)
				printf("After host conversion odr pkt_type and r_reqid : %d : %d\n",received_frame->hdr.pkt_type,received_frame->hdr.rreq_id);


			if(DEBUG)
			{
				printf("Packet came to mac-addr ");
				printHWADDR(dest_mac_addr);
				printf("Packet destined from mac-addr ");
				printHWADDR(src_mac_addr);
			}

			if(addr_ll.sll_pkttype == PACKET_BROADCAST)
			{
				memcpy((void *)dest_mac_addr,(void *)inf_mac_addr_map[addr_ll.sll_ifindex],ETH_ALEN);
				if(DEBUG)
					printf("After updating dest addr for broadcast packet the interface index: %d\n",addr_ll.sll_ifindex);

				printHWADDR(dest_mac_addr);
			}

			if(DEBUG)
				printf("Packet type (Broadcast/otherhost) from sockaddr_ll %d\n", addr_ll.sll_pkttype);

			if(DEBUG)
				printf("Packet destined ip-address %s\n", received_frame->hdr.cn_dsc_ipaddr);

			printf("ODR at node %s has received a packet with type %d with destination %s\n", my_name, received_frame->hdr.pkt_type,
																								Gethostbyaddr(received_frame->hdr.cn_dsc_ipaddr));

			received_frame->hdr.hop_count++;

			if(is_frame_belongs_to_me(received_frame->hdr.cn_dsc_ipaddr, my_ip_address))
			{

				struct odr_frame *frame_to_send;

				switch(received_frame->hdr.pkt_type){
					case R_REQ:
						if(DEBUG)
							printf("Received my R_REQ with id %d\n", received_frame->hdr.rreq_id);

						send_frame_for_rreq(pf_sockfd, received_frame, src_mac_addr, addr_ll.sll_ifindex);
						break;

					case R_REPLY:
						if(DEBUG)
							printf("Received my R_RPLY destined to me\n");

						send_frame_for_rrply(pf_sockfd, received_frame, src_mac_addr, addr_ll.sll_ifindex);
						break;

					case PAY_LOAD:
						if(DEBUG)
							printf("Received pay_load destined to me %s\n", my_name);

						update_routing_table(received_frame->hdr.cn_src_ipaddr, src_mac_addr, received_frame->hdr.hop_count,
								addr_ll.sll_ifindex, received_frame->hdr.force_route_dcvry, received_frame->hdr.pkt_type,received_frame->hdr.broadcast_id);

						if(DEBUG)
							printf("Sending the message to the server/ client\n");

						msg_send_to_uds(sockfd,received_frame->hdr.cn_src_ipaddr,received_frame->hdr.src_port_num,received_frame->hdr.dest_port_num,
										received_frame->payload,received_frame->hdr.force_route_dcvry);
						break;

					default:
						printf("Wrong packet type received %d\n", received_frame->hdr.pkt_type);
					}
			}
			else
			{
				switch(received_frame->hdr.pkt_type){

				case R_REQ:
					if(DEBUG)
						printf("Received R_REQ and processing to see whether a route exist else forwarding\n");

					process_received_rreq_frame(pf_sockfd,addr_ll.sll_ifindex,
															received_frame,src_mac_addr,dest_mac_addr);
					break;
				case R_REPLY:
					if(DEBUG)
						printf("Received R_RPLY and forwarding same\n");

					process_received_rreply_frame(pf_sockfd,addr_ll.sll_ifindex,
															received_frame,src_mac_addr,dest_mac_addr,false);
					break;
				case PAY_LOAD:

					if(DEBUG)
						printf("Received PAY_LOAD and forwarding same\n");

					update_routing_table(received_frame->hdr.cn_src_ipaddr, src_mac_addr, received_frame->hdr.hop_count,
							addr_ll.sll_ifindex, received_frame->hdr.force_route_dcvry, received_frame->hdr.pkt_type,received_frame->hdr.broadcast_id);
					forward_frame_payload(pf_sockfd, received_frame);
					break;
				default:
					printf("Wrong packet type received %d\n", received_frame->hdr.pkt_type);
				}
			}
		}
	}

	/**
	 * Freeing the routing table
	 */
	freeRoutingTable();
}

int is_frame_belongs_to_me(char* dest_ip_addr, char* my_ip_addr)
{
	return !strcmp(dest_ip_addr, my_ip_addr);
}

char* get_inf_mac_addr(int inf_index)
{
	if(inf_index < MAX_INTERFACES)
		return inf_mac_addr_map[inf_index];
	else
	{
		if(DEBUG)
			printf("inf_index out of limit\n");
		return NULL;
	}
}

int get_route_entry_timeout()
{
	return route_entry_timeout;
}

void set_route_entry_timeout(long int timeout)
{
	route_entry_timeout = timeout;

	if(DEBUG)
		printf("Staleness parameter set as %ld seconds\n", timeout);
}

int get_new_broadcast_id()
{
	if(DEBUG)
		printf("Getting the broadcast Id :%d\n",broadcast_id);

	return broadcast_id++;
}

int get_new_rreq_id()
{
	return r_req_id++;
}

void populate_staleness_parameter(char *staleness_param)
{

	int timeout = atoi(staleness_param);

	if(timeout < 0)
	{
		printf("Kindly enter postive staleness parameter\n");
		exit(0);
	}

	set_route_entry_timeout(timeout);
}
