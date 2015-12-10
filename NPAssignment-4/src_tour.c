/*
 * src_tour.c
 *
 *  Created on: Nov 29, 2015
 *      Author: sujan
 */

/***
 *  TODO: Have to stop the alarm after receiving the multicast message -> done.
 *  	  On the last node we have to wait five secs before sending the mutilcast message to all the peers.
 */

#include "src_usp.h"

bool ALRM_SET = false;

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

		sigaction(SIGALRM, &sig_alrm_action, &old_action);
	}

	struct hwa_info *head = Get_hw_addrs();

	create_ip_hw_mp(head);

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
	pf_sockfd = Socket(PF_PACKET,SOCK_RAW,htons(ETH_P_IP));

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

		memset(buff,'\0',BUFFER_SIZE);

		// TODO : Have to check the identification field in arp
		if(FD_ISSET(rt,&tour_fds)){

			if(recvfrom(rt,buff,BUFFER_SIZE,0,NULL,NULL) < 0){

				printf("Received a packet in  %s\n",Gethostname());
			}

			/**
			 *  We should restrict to join if we have already joined.
			 */
			struct ip *ip = (struct ip *)buff;

			if(ip->ip_id == ntohs(IDENTIFICATION_FIELD)){

				if(DEBUG){
					printf("Identification field is matching\n");
				}

				join_mcast_grp(udprecvsockfd);
				/*
				 *  Sending the ping to the predecessor.
				 */
				sig_alrm_handler(SIGALRM);
				process_received_datagram(rt, udpsock, buff);
			}else{

				if(DEBUG){
					printf("Discarding the packet as the identification field in the ip packet didn't match %d\n",ip->ip_id);
				}
			}
		}

		if(FD_ISSET(pg, &tour_fds)){

			if(DEBUG)
				printf("Came into ping socket\n");

			struct sockaddr_in sock_addr;
			int len;

			int n=0;
			bzero(buff, BUFFER_SIZE);
			if((n=recvfrom(pg,buff,BUFFER_SIZE,0,(SA*) &sock_addr,&len)) < 0){

				printf("Error in recv-from :%s\n",strerror(errno));
			}


			char p_addr[32];
			inet_ntop(AF_INET, &sock_addr.sin_addr, p_addr, 32);

			if(DEBUG){
				printf("Before Src Ip Address and length : %s %d\n",p_addr,n);
			}

			struct ip *ip = (struct ip *)buff;

			inet_ntop(AF_INET,&ip->ip_src, p_addr, 32);

			if(DEBUG){
				printf("Src Ip Address and length : %s %d\n",p_addr,n);
				printf("Destination addr :%u\n",ip->ip_p);
			}

			struct icmp *icmp = (struct icmp *)(buff + IP_HDR_LEN);

			if(DEBUG){
				printf("identifier %u\n",icmp->icmp_id);
				printf("ICMP type %u\n",icmp->icmp_type);
			}


			/**
			 *  Checking the packet type and identification field.
			 */
			if(ntohs(icmp->icmp_id) == ICMP_IDENTIFIER && icmp->icmp_type == ICMP_ECHO)
			{
				//printf("Received ping from next node %s\n", p_addr);

				printf("%d bytes from %s (%s): icmp_seq:%u, ttl=%d\n",
																	ICMP_DATA_LEN,Gethostbyaddr(p_addr),p_addr,icmp->icmp_seq,ip->ip_ttl);
			}


		}

		if(FD_ISSET(udprecvsockfd, &tour_fds))
		{
			char udp_msg[BUFFER_SIZE];
			bzero(&udp_msg, BUFFER_SIZE);

			if(DEBUG)
				printf("Received packet on UDP socket\n");

			if(recvfrom(udprecvsockfd,udp_msg,BUFFER_SIZE,0,NULL,NULL) < 0){

				printf("Error in recv-from of udp socket :%s\n",strerror(errno));
			}

			printf("Node %s Received %s\n",Gethostname(),udp_msg);

			char vm_name[20];

			strcpy(vm_name,Gethostname());

			char msg[200];

			memset(msg,'\0',200);

			strcat(msg,"Node ");

			strcat(msg,vm_name);

			strcat(msg," I am a member of the group.");



			if(!ALRM_SET){

				send_multicast_msg(udpsock,msg);

				alarm(0);
				/*
				 *  Setting the sig-alarm for termination.
				 */
				sig_alrm_action.sa_handler = sig_alrm_handler_termination;
				sigemptyset(&sig_alrm_action.sa_mask);
				sig_alrm_action.sa_flags = 0;
				sigaction(SIGALRM, NULL, &old_action);

				if(old_action.sa_handler != SIG_IGN){

					if(DEBUG)
						printf("New sig alrm action set\n");

					sigaction(SIGALRM, &sig_alrm_action, &old_action);
				}
				ALRM_SET = true;
				alarm(5);
			}
		}
	}
	return 0;
}

void sig_alrm_handler(int signum)
{
	if(DEBUG){
		printf("Received a sig-alrm\n");
	}

	send_icmp_echoes();
	alarm(1);
	return;
}

void sig_alrm_handler_termination(int signum)
{
	printf("Node at %s Exiting the tour application\n",Gethostname());
	exit(0);
}
