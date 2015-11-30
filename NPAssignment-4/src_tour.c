/*
 * src_tour.c
 *
 *  Created on: Nov 29, 2015
 *      Author: sujan
 */

#include "src_usp.h"

int main(int argc, char **argv){

	int rt,pg,pf,udpsock;

	int on = 1;

	struct sockaddr_in multi_addr;

	struct ip *ip;

	char *buff = (char *)malloc(1024);

	/**
	 *  Current index in the source route
	 */
	uint32_t current_index = 1;

	if(argc < 2){
		printf("Kindly enter the routing path\n");
		exit(0);
	}

	/**
	 *  Creating the route traversal sockets.
	 */
	rt = Socket(AF_INET,SOCK_RAW,GRP_PROTOCOL_VALUE);

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

	/**
	 *  Create a tour_list
	 */
	struct tour_route *head = create_tour_list(argc,argv);


	if(DEBUG){
		printf("Completed the creation of tour list\n");
	}

	setsockopt(rt,IPPROTO_IP,IP_HDRINCL,&on,sizeof(int));

	bzero(&multi_addr,sizeof(struct sockaddr_in));
	/**
	 * Joining the multicast address
	 */
	multi_addr.sin_family = AF_INET;

	inet_pton(AF_INET,MULTICAST_ADDRESS,&multi_addr.sin_addr);

	multi_addr.sin_port = MULTICAST_PORT_NUMBER;

	Mcast_join(udpsock,(SA *)&multi_addr,sizeof(multi_addr),NULL,0);

	int total_len = calculate_length(argc+1);

	build_ip_header(buff,INDEX_IN_TOUR_AT_SOURCE,total_len);

	populate_data_in_datagram(buff,argc,INDEX_IN_TOUR_AT_SOURCE);

	struct sockaddr_in destAddr;

	destAddr.sin_family = AF_INET;

	destAddr.sin_addr.s_addr = getIpAddressInTourList(current_index);

	sendto(rt,buff,sizeof(buff),0,(SA *)&destAddr,sizeof(destAddr));

	return 0;
}




