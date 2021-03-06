/*
 * src_arp.c
 *
 *  Created on: Nov 29, 2015
 *      Author: sujan
 */

#include "src_usp.h"

int main(){

	struct hwa_info *head = Get_hw_addrs();

	fd_set arp_fds;

	struct uds_arp_msg uds_msg;

	struct arp_cache_entry *arp_entry = NULL;

	struct sockaddr_ll addr_ll;

	char *recvbuff = (char *)malloc(BUFFER_SIZE);

	//printf("Got the hw_addr's\n");

	printf("Starting the arp-module at node :%s\n",Gethostname());

	create_ip_hw_mp(head);

	print_my_hw_ip_mp();

	int unix_sockfd,pf_sockfd;

	struct sockaddr_un arp_addr,cli_uds_addr;

	unix_sockfd = socket(AF_LOCAL,SOCK_STREAM,0);

	pf_sockfd = socket(PF_PACKET,SOCK_RAW,htons(ARP_GRP_TYPE));

	arp_addr.sun_family = AF_LOCAL;

	strcpy(arp_addr.sun_path,ARP_WELL_KNOWN_PATH_NAME);

	unlink(ARP_WELL_KNOWN_PATH_NAME);

	Bind(unix_sockfd,(SA *)&arp_addr,sizeof(struct sockaddr_un));

	listen(unix_sockfd,BACKLOG);

	for(;;){

		FD_ZERO(&arp_fds);

		FD_SET(unix_sockfd,&arp_fds);

		FD_SET(pf_sockfd, &arp_fds);

		int maxfd = max(unix_sockfd,pf_sockfd) + 1;

		if(DEBUG)
			printf("Waiting in select\n");

		Select(maxfd,&arp_fds,NULL,NULL,NULL);

		if(FD_ISSET(unix_sockfd,&arp_fds)){

			int cli_uds_connfd = Accept(unix_sockfd, (SA *)NULL, NULL);

			printf("Received a packet from Unix Domain Socket\n");

			memset(recvbuff,0,BUFFER_SIZE);

			//printf("Before Recv_from  \n");

			if(recvfrom(cli_uds_connfd,recvbuff,BUFFER_SIZE,0,NULL,NULL) < 0 ){
				printf("Error in Recv from %s\n",strerror(errno));
			}

			//printf("After Recv_from -1 \n");
			bzero(&uds_msg,sizeof(uds_msg));
			//printf("After Recv_from -2 \n");



			//printf("After Recv_from -3 \n");

			memcpy(&uds_msg,recvbuff,strlen(recvbuff));

			//printf("received target_ip_address request %s\n",uds_msg.target_ip_address);

			if((arp_entry = get_hw_addr_arp_cache(uds_msg.target_ip_address)) != NULL){

				printf("ARP ENTRY exists for ip_address in arp_cache %s\n",uds_msg.target_ip_address);

				printf("Sending the requested mac-address for ip-address %s in unix domain socket \n",uds_msg.target_ip_address);

				bzero(&uds_msg,sizeof(uds_msg));

				uds_msg.hw_addr.sll_halen = ETH_ALEN;

				memcpy(uds_msg.hw_addr.sll_addr,arp_entry->hw_address, ETH_ALEN);

				if(sendto(cli_uds_connfd,&uds_msg,sizeof(uds_msg),0,NULL,0) < 0){
					printf("Error in Sendto in arp module %s\n",strerror(errno));
				}

			}else{

				printf("ARP ENTRY does not exists for ip_address in arp_cache %s\n",uds_msg.target_ip_address);

				update_arp_cache(NULL,uds_msg.target_ip_address,ETH0_INDEX,cli_uds_connfd);
				send_arp_req(pf_sockfd,uds_msg.target_ip_address);
			}
		}

		//TODO : have to check the identification field before processing the frame.
		if(FD_ISSET(pf_sockfd,&arp_fds)){

			void* buffer = (void*)malloc(ETHR_FRAME_SIZE);

			bzero(&addr_ll,sizeof(addr_ll));

			int addr_len = sizeof(addr_ll);
			unsigned char src_mac_addr[6];
			unsigned char dest_mac_addr[6];

			if(recvfrom(pf_sockfd, buffer, ETHR_FRAME_SIZE, 0,(SA *)&addr_ll,&addr_len) < 0){
				printf("Error in recv_from :%s\n",strerror(errno));
				exit(0);
			}

			memcpy((void *)dest_mac_addr,(void *)buffer,ETH_ALEN);
			memcpy((void *)src_mac_addr,(void *)(buffer+ETH_ALEN),ETH_ALEN);

			struct arp_pkt *pkt  = (struct arp_pkt *)(buffer + ETH_HDR_LEN);

			convertToHostOrder(pkt);

			if(pkt->iden_field == ARP_IDEN_FIELD){
				switch(pkt->op){

					case ARP_REQ:
						process_arp_req(pf_sockfd,pkt,addr_ll.sll_ifindex);
						break;
					case ARP_REP:
						printf("Node at %s received an ARP_REP\n",Gethostname());
						process_arp_rep(pkt);
						break;
				}
			}else{
				printf("Ignoring the ARP message as the identification field didn't match\n");
			}

		}

	}

	return 0;
}


