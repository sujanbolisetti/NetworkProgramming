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
	struct sockaddr_ll socket_address;
	fd_set rset;
	struct iovec iov[1];
	struct msghdr mh;
	char my_name[128];
	unsigned int interface_index;

	int optval=1;

	gethostname(my_name,sizeof(my_name));

	printf("Starting ODR process on %s\n",Gethostbyname(my_name));

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

	for(;;){

		FD_SET(sockfd, &rset);

		FD_SET(pf_sockfd,&rset);

		printf("Waiting for select\n");

		select(maxfd,&rset,NULL,NULL,NULL);

		if(FD_ISSET(sockfd,&rset)){

			struct reply_from_uds_client *reply = msg_receive(sockfd);

			if(is_route_exists(reply->canonical_ipAddress)){

			}else{

				/**
				 *  1. Broadcast a rreq packet to all the interfaces.
				 *  2. Wait for rreply from the destination.
				 *  3. Fill in the routing table and send the payload.
				 */



			}



//				/*our MAC address*/
//				unsigned char src_mac[6] = {0x00, 0x0c, 0x29, 0x49, 0x3f, 0x65};
//
//				/*other host MAC address*/
//				unsigned char dest_mac[6] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
//
//				void* buffer = (void*)malloc(300);
//
//				unsigned char* etherhead = buffer;
//
//				unsigned char* data = buffer + 14;
//
//				struct ethhdr *eh = (struct ethhdr *)etherhead;
//
//				int send_result = 0;
//
//				socket_address.sll_family   = AF_PACKET;
//
//				socket_address.sll_ifindex  = 2;
//
//				//socket_address.sll_hatype   = ARPHRD_ETHER;
//
//				/*target is another host*/
//				socket_address.sll_pkttype  = PACKET_BROADCAST;
//
//				socket_address.sll_protocol = htons(32765);
//
//				/*address length*/
//				socket_address.sll_halen    = ETH_ALEN;
//				/*MAC - begin*/
//				socket_address.sll_addr[0]  = 0xff;
//				socket_address.sll_addr[1]  = 0xff;
//				socket_address.sll_addr[2]  = 0xff;
//				socket_address.sll_addr[3]  = 0xff;
//				socket_address.sll_addr[4]  = 0xff;
//				socket_address.sll_addr[5]  = 0xff;
//				/*MAC - end*/
//				socket_address.sll_addr[6]  = 0x00;/*not used*/
//				socket_address.sll_addr[7]  = 0x00;/*not used*/
//
//				int j=0;
//
//				/*set the frame header*/
//				memcpy((void*)buffer, (void*)dest_mac, ETH_ALEN);
//				memcpy((void*)(buffer+ETH_ALEN), (void*)src_mac, ETH_ALEN);
//				eh->h_proto = 0x00;
//
//				mh.msg_name = (caddr_t)&socket_address;
//				mh.msg_namelen =  sizeof(struct sockaddr_ll);
//
//				iov[0].iov_base = buffer;
//				iov[0].iov_len = strlen(buffer);
//
//				mh.msg_iov = iov;
//
//				mh.msg_iovlen = 1;
//
//				mh.msg_control = NULL;
//				mh.msg_controllen = 0;
//
//				//sendmsg(pf_sockfd, &mh,0);
//				send_result = sendto(pf_sockfd, buffer, 300, 0,
//						  (struct sockaddr*)&socket_address, sizeof(struct sockaddr_ll));
//
//				if(send_result < 0){
//					printf("send to failed %s \n",strerror(errno));
//				}


		}

		if(FD_ISSET(pf_sockfd,&rset)){

			printf("Received packet from other ODR\n");

			setsockopt(pf_sockfd,IPPROTO_IP,IP_PKTINFO,&optval,sizeof(int));

			void* buffer = (void*)malloc(ETH_FRAME_LEN); /*Buffer for ethernet frame*/
			int length = 0; /*length of the received frame*/
			//recvmsg(pf_sockfd,&mh,0);
			length = recvfrom(pf_sockfd, buffer, 300, 0, NULL, NULL);

			if(length < 0){
				printf("Error in recvfrom\n");
			}

			setsockopt(pf_sockfd,IPPROTO_IP,IP_PKTINFO,&optval,sizeof(int));


			gethostname(my_name,sizeof(my_name));
			printf("message received on %s\n",
					Gethostbyname(my_name));


		}
	}
}



