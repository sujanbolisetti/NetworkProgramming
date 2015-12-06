/*
 * src_tour.c
 *
 *  Created on: Nov 29, 2015
 *      Author: sujan
 */

#include "src_usp.h"

int main(int argc, char **argv){

	int rt,pg,udpsock,udprecvsockfd;

	int on = 1,maxfd;

	struct sockaddr_in multi_addr;

	struct ip *ip;

	char *buff = (char *)malloc(BUFFER_SIZE*sizeof(char));

	fd_set tour_fds;

	bool SOURCE_IN_TOUR = true;

	struct tour_route tour_list[SIZE_OF_TOUR_LIST];

	struct sigaction old_action;
	struct sigaction sig_alrm_action;

	sig_alrm_action.sa_handler = sig_alrm_handler;
	sigemptyset(&sig_alrm_action.sa_mask);
	sig_alrm_action.sa_flags = 0;
	sigaction(SIGALRM, NULL, &old_action);

	if(old_action.sa_handler != SIG_IGN){

		if(DEBUG)
			printf("New sig alrm action set\n");

		sigaction(SIGCHLD, &sig_alrm_action, &old_action);
	}


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
	pf_sockfd = Socket(PF_PACKET,SOCK_RAW,ETH_P_IP);


	/**
	 * Creating a UDP socket for sending
	 */
	udpsock = Socket(AF_INET,SOCK_DGRAM,0);

	/**
	 * Creating a UDP socket for receiving
	 */
	udprecvsockfd = Socket(AF_INET,SOCK_DGRAM,0);


	build_multicast_addr(&multi_addr);

	Bind(udprecvsockfd,(SA *)&multi_addr,sizeof(struct sockaddr_in));

	if(SOURCE_IN_TOUR){

		//allocate_buffer(&buff);
		/**
		 *  Create a tour_list
		 */
		bool is_list_bad = create_tour_list(argc,argv,tour_list);

		if(is_list_bad)
		{
			printf("Input VMs list is having consecutive duplicate, please check and start program again\n");
			return 0;
		}

		if(DEBUG){
			printf("Completed the creation of tour list\n");
		}

		join_mcast_grp(udprecvsockfd);

		uint16_t total_len = calculate_length();

		build_ip_header(buff,total_len,tour_list[INDEX_IN_TOUR_AT_SOURCE].ip_address, false);

		populate_data_in_datagram(buff,INDEX_IN_TOUR_AT_SOURCE,argc+1,tour_list);

		struct sockaddr_in destAddr;

		destAddr.sin_family = AF_INET;

		if(inet_pton(AF_INET,getIpAddressInTourList(tour_list,INDEX_IN_TOUR_AT_SOURCE),&destAddr.sin_addr) < 0){
			printf("Error in converting numeric format %s\n",strerror(errno));
		}

		destAddr.sin_port = 0;

		if(sendto(rt,buff,BUFFER_SIZE,0,(SA *)&destAddr,sizeof(struct sockaddr)) < 0){
			printf("Error in Sendto in %s\n",strerror(errno));
		}
	}

	for(;;){

		FD_ZERO(&tour_fds);

		FD_SET(rt, &tour_fds);

		FD_SET(pg, &tour_fds);

		FD_SET(udprecvsockfd, &tour_fds);

		maxfd = max(max(rt, udprecvsockfd), pg) + 1;

		if(DEBUG){
			printf("Waiting for the select\n");
		}

		Select(maxfd,&tour_fds,NULL,NULL,NULL);

		if(DEBUG){
			printf("Came after the select\n");
		}

		memset(buff,'\0',1024);

		if(FD_ISSET(rt,&tour_fds)){

			if(recvfrom(rt,buff,1024,0,NULL,NULL) < 0){

				printf("Received a packet in  %s\n",Gethostname());
			}

			join_mcast_grp(udprecvsockfd);
			process_received_datagram(rt, udpsock, buff);
			alarm(1);
		}

		if(FD_ISSET(pg, &tour_fds)){

			printf("Came into ping socket\n");
			struct sockaddr_in sock_addr;
			int len;

			bzero(buff, sizeof(buff));
			if(recvfrom(pg,buff,sizeof(buff),0,(SA*) &sock_addr,&len) < 0){

				printf("Error in recv-from :%s\n",strerror(errno));
			}

			char p_addr[32];
			inet_ntop(AF_INET, &sock_addr.sin_addr, p_addr, 32);

			struct icmp *icmp = (struct icmp*)buff + IP_HDR_LEN;
			if(icmp->icmp_id == ICMP_IDENTIFIER)
			{
				printf("Received ping from next node %s\n", p_addr);
			}
		}

		if(FD_ISSET(udprecvsockfd, &tour_fds))
		{
			char udp_msg[BUFFER_SIZE];
			bzero(&udp_msg, BUFFER_SIZE);
			printf("Received packet on UDP socket\n");

			if(recvfrom(udprecvsockfd,udp_msg,BUFFER_SIZE,0,NULL,NULL) < 0){

				printf("Error in recv-from of udp socket :%s\n",strerror(errno));
			}

			printf("Received message on udp socket: %s\n", udp_msg);
		}
	}
	return 0;
}

void sig_alrm_handler(int signum)
{
	send_icmp_echoes();
	alarm(1);
	return;
}
