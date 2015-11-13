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

	odr_init();

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
			int length = 0;
			int addr_len = sizeof(addr_ll);

			length = recvfrom(pf_sockfd, buffer, EHTR_FRAME_SIZE, 0,(SA*)&addr_ll,&addr_len);

			if(length < 0){
				printf("Error in recvfrom %s\n",strerror(errno));
			}

			struct odr_frame *received_frame  = (struct odr_frame *)(buffer + ETH_HDR_LEN);

			if(DEBUG)
				printf("ip-address %s\n",received_frame->hdr.cn_dsc_ipaddr);

			if(is_frame_belongs_to_me(received_frame->hdr.cn_dsc_ipaddr, my_ip_address))
			{
				printf("Its my packet\n");
				// TODO: send R_REPLY_SENT
				build_rreply_frame(received_frame);
				exit(0);
			}
			else
			{
				received_frame->hdr.hop_count++;

				int result = is_inefficient_rreq_exists(received_frame);

				if(result == INEFFICIENT_R_REQ_EXISTS || result == R_REQ_NOT_EXISTS){

					if(result == R_REQ_NOT_EXISTS){
						store_rreq_frame(received_frame);
					}
					// TODO: populate R_REPLY_SENT flag

					send_frame_rreq(pf_sockfd,addr_ll.sll_ifindex,received_frame);
				}else{
					printf("Not flooding rreq as efficient one's exists\n");
				}
			}


			gethostname(my_name,sizeof(my_name));

			printf("message received on %s\n",my_ip_address);

		}
	}
}

int is_frame_belongs_to_me(char* dest_ip_addr, char* my_ip_addr)
{
	return !strcmp(dest_ip_addr, my_ip_addr);
}



