/*
 * odr.c
 *
 *  Created on: Nov 9, 2015
 *      Author: sujan
 */

#include "usp.h"

int main(){

	int sockfd,pf_sockfd,maxfd;
	struct sockaddr_un odr_addr;
	struct sockaddr_ll addr_ll;
	fd_set rset;
	struct iovec iov[1];
	struct msghdr mh;
	char my_name[128];
	unsigned int interface_index;
	int broadcast_id=0;
	struct odr_frame frame;
	char my_ip_address[20];
	char inf_mac_addr_map[10][20];

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

			struct reply_from_uds_client *reply = msg_receive(sockfd);

			// have to exclude this even when force_route_discovery.
			if(is_route_exists(reply->canonical_ipAddress)){

			}else{

				/**
				 *  1. Broadcast a rreq packet to all the interfaces.
				 *  2. Wait for rreply from the destination.
				 *  3. Fill in the routing table and send the payload.
				 */

				frame = build_odr_frame(my_ip_address,reply->canonical_ipAddress,
						ZERO_HOP_COUNT,R_REQ,broadcast_id++,reply->flag,R_REPLY_NOT_SENT,NULL);

				send_frame_rreq(pf_sockfd,-1,&frame);
			}
		}

		if(FD_ISSET(pf_sockfd,&rset)){

			printf("%s received packet from other ODR\n",my_name);

			void* buffer = (void*)malloc(EHTR_FRAME_SIZE);
			int addr_len = sizeof(addr_ll);
			unsigned char src_mac_addr[8];
			unsigned char dest_mac_addr[8];

			if(recvfrom(pf_sockfd, buffer, EHTR_FRAME_SIZE, 0,(SA*)&addr_ll,&addr_len) < 0){
				printf("Error in recv_from :%s\n",strerror(errno));
				exit(0);
			}

			memcpy((void *)dest_mac_addr,(void *)buffer,ETH_ALEN);
			memcpy((void *)src_mac_addr,(void *)(buffer+ETH_ALEN),ETH_ALEN);

			struct odr_frame *received_frame  = (struct odr_frame *)(buffer + ETH_HDR_LEN);

			printHWADDR(dest_mac_addr);
			printHWADDR(src_mac_addr);

			if(addr_ll.sll_pkttype == PACKET_BROADCAST)
			{
				memcpy((void *)dest_mac_addr,(void *)inf_mac_addr_map[addr_ll.sll_ifindex],ETH_ALEN);
				printf("After updating dest addr for broadcast packet");
				printHWADDR(dest_mac_addr);
			}

			printf("packet type from sockaddr_ll %d\n",addr_ll.sll_pkttype);

			if(DEBUG)
				printf("ip-address %s\n",received_frame->hdr.cn_dsc_ipaddr);

			if(is_frame_belongs_to_me(received_frame->hdr.cn_dsc_ipaddr, my_ip_address))
			{
				printf("%s has received a packet with type %d which belongs it\n",my_name,received_frame->hdr.pkt_type);
				// TODO: send R_REPLY_SENT
				// have to send application payload/r_reply based on the
				// type of the frame received.
				//build_rreply_frame(received_frame);

				received_frame->hdr.hop_count++;

				switch(received_frame->hdr.pkt_type){

					case R_REQ:
						if(DEBUG)
							printf("Recieved my R_REQ destined to me\n");
						process_received_rreply_frame(pf_sockfd,addr_ll.sll_ifindex,
																				received_frame,src_mac_addr,dest_mac_addr, true);
						break;
					case R_REPLY:
						printf("TODO: have to sent the data-payload\n");
						break;
					}
			}
			else
			{
				received_frame->hdr.hop_count++;
				switch(received_frame->hdr.pkt_type){

				case R_REQ:
					process_received_rreq_frame(pf_sockfd,addr_ll.sll_ifindex,
															received_frame,src_mac_addr,dest_mac_addr);
					break;
				case R_REPLY:
					process_received_rreply_frame(pf_sockfd,addr_ll.sll_ifindex,
															received_frame,src_mac_addr,dest_mac_addr,false);
					break;
				}

			}
		}
	}
}

int is_frame_belongs_to_me(char* dest_ip_addr, char* my_ip_addr)
{
	return !strcmp(dest_ip_addr, my_ip_addr);
}



