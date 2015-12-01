/*
 * src_tour.c
 *
 *  Created on: Nov 29, 2015
 *      Author: sujan
 */

#include "src_usp.h"

int main(int argc, char **argv){

	int rt,pg,pf,udpsock;

	int on = 1,maxfd;

	struct sockaddr_in multi_addr;

	struct ip *ip;

	char *buff = (char *)malloc(1024);

	fd_set tour_fds;

	bool SOURCE_IN_TOUR = true;

	/**
	 *  Current index in the source route
	 */
	uint32_t current_index = 1;

	if(argc < 2){
		//printf("Kindly enter the routing path\n");
		SOURCE_IN_TOUR = false;
	}

	/**
	 *  Creating the route traversal sockets.
	 */
	rt = Socket(AF_INET,SOCK_RAW,GRP_PROTOCOL_VALUE);

	if(setsockopt(rt,IPPROTO_IP,IP_HDRINCL,&on,sizeof(int)) < 0){
		printf("Error in Setting the sockopt %s\n",strerror(errno));
	}

	/**
	 *  Creating ping socket.
	 */
	pg = Socket(AF_INET,SOCK_RAW,IPPROTO_ICMP);

	/**
	 *  Creating a PF_SOCKET for sending the ping messages.
	 */
	pf = Socket(PF_PACKET,SOCK_RAW,ETH_P_IP);


	/**
	 * Creating a UDP socket
	 */
	udpsock = Socket(AF_INET,SOCK_DGRAM,0);


	if(SOURCE_IN_TOUR){
		/**
		 *  Create a tour_list
		 */
		struct tour_route *head = create_tour_list(argc,argv);


		if(DEBUG){
			printf("Completed the creation of tour list\n");
		}

		bzero(&multi_addr,sizeof(struct sockaddr_in));
		/**
		 * Joining the multicast address
		 */
		multi_addr.sin_family = AF_INET;

		inet_pton(AF_INET,MULTICAST_ADDRESS,&multi_addr.sin_addr);

		multi_addr.sin_port = MULTICAST_PORT_NUMBER;

		Mcast_join(udpsock,(SA *)&multi_addr,sizeof(multi_addr),NULL,0);

		uint16_t total_len = calculate_length(argc+1);

		build_ip_header(buff,INDEX_IN_TOUR_AT_SOURCE,total_len);

		populate_data_in_datagram(buff,argc,INDEX_IN_TOUR_AT_SOURCE);

		struct sockaddr_in destAddr;

		destAddr.sin_family = AF_INET;

		destAddr.sin_addr.s_addr = getIpAddressInTourList(current_index);

		if(sendto(rt,buff,sizeof(buff),0,(SA *)&destAddr,sizeof(destAddr)) < 0){
			printf("Error in Sendto in %s\n",strerror(errno));
		}
	}

	for(;;){

		FD_ZERO(&tour_fds);

		FD_SET(rt, &tour_fds);

		//FD_SET(pg,&tour_fds);

		maxfd = rt+1;

		if(DEBUG){
			printf("Waiting for the select\n");
		}

		Select(maxfd,&tour_fds,NULL,NULL,NULL);

		if(DEBUG){
			printf("Came after the select\n");
		}

		memset(buff,'\0',1024);

		if(FD_ISSET(rt,&tour_fds)){

			if(recvfrom(rt,buff,sizeof(buff),0,NULL,NULL) < 0){

				printf("Received a packet in  %s\n",Gethostname());
			}
		}

		if(FD_ISSET(pg,&tour_fds)){

			printf("Came into ping socket\n");

			if(recvfrom(pg,buff,sizeof(buff),0,NULL,NULL) < 0){

				printf("Received packet in packet socket\n");
			}
		}
	}
	return 0;
}




