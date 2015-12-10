/*
 * src_usp.h
 *
 *  Created on: Nov 29, 2015
 *      Author: sujan
 */

#ifndef SRC_USP_H_
#define SRC_USP_H_

#include<sys/socket.h>

#include<sys/types.h>

#include<netinet/in.h>

#include<netinet/ip.h>

#include<netinet/ip_icmp.h>

#include<arpa/inet.h>

#include<errno.h>

#include<signal.h>

#include<stdio.h>

#include<stdlib.h>

#include<sys/wait.h>

#include <sys/select.h>

#include<string.h>

#include<fcntl.h>

#include <netdb.h>

#include <pthread.h>

#include<unistd.h>

#include <signal.h>

#include <sys/un.h>

#include <linux/if_packet.h>

#include <linux/if_ether.h>

#include <linux/if_arp.h>

#include "src_constants.h"

#include <stdbool.h>

#include "src_hw_addr.h"

/* include mcast_join1 */
//	#include	<net/if.h>

#define SA struct sockaddr

#define	min(a,b)	((a) < (b) ? (a) : (b))
#define	max(a,b)	((a) > (b) ? (a) : (b))

struct ip_addr_hw_addr_pr *my_hw_addr_head;

struct tour_route{
	char ip_address[32];
	uint16_t port_number;
};

struct tour_payload{

	uint16_t index;
	uint16_t count;
	struct tour_route tour_list[SIZE_OF_TOUR_LIST];
};

struct ip_addr_hw_addr_pr{

	char ip_addr[128];
	unsigned char mac_addr[6];
	struct ip_addr_hw_addr_pr *next;
};

struct arp_cache_entry{

	char ip_address[128];
	unsigned char hw_address[6];
	uint16_t sll_ifindex;
	uint16_t sll_hatype;
	int cli_sockdes;

	struct arp_cache_entry *next;
};

struct arp_pkt{

	uint16_t iden_field;
	uint16_t hard_type;
	uint16_t prot_type;
	uint8_t hard_size;
	uint8_t prot_size;
	uint16_t op;
	unsigned char snd_ethr_addr[6];
	char snd_ip_address[128];
	unsigned char target_ethr_addr[6];
	char target_ip_address[128];
};

struct hwaddr {

	int sll_ifindex;
	unsigned short sll_hatype;
	unsigned char sll_halen;
	unsigned char sll_addr[8];

};

struct uds_arp_msg{

	char target_ip_address[128];
	struct hwaddr hw_addr;
};

struct icmp_tour_node {
	char ip_address[128];
	int seqNum;
	struct icmp_tour_node *next;
};

int pf_sockfd;
int ETH0_INDEX;

void convertToNetworkOrder(struct arp_pkt *pkt);

void convertToHostOrder(struct arp_pkt *pkt);

bool
create_tour_list(int count , char **argv, struct tour_route *tour_list);

void
insert_me_address_at_bgn(struct tour_route *tour_list);

void
insert_multicast_address_at_lst(int count,struct tour_route *tour_list);

char*
Gethostbyname(char *my_name);

char* Gethostname();

void build_ip_header(char *buff, uint16_t total_len,char *dest_addr, bool isIcmp);

void
populate_data_in_datagram(char *buff, uint16_t index,uint16_t count, struct tour_route *tour_list);

char*
getIpAddressInTourList(struct tour_route *tour_list, uint16_t index);

uint16_t
calculate_length();

void
print_the_payload(struct tour_payload payload);

void
forward_the_datagram(int sockfd, struct tour_payload payload);

void allocate_buffer(char **buff);

void process_received_datagram(int sockfd, int udp_sockfd, char *buff);

void send_multicast_msg(int udp_sockfd,char *msg);

void build_multicast_addr(struct sockaddr_in *multi_addr);

char *
Sock_ntop_host(const struct sockaddr *sa, socklen_t salen);

void printHWADDR(char *hw_addr);

void process_arp_rep(struct arp_pkt *pkt);

void process_arp_req(int pf_sockfd,struct arp_pkt *pkt,int recv_if_index);

void build_eth_frame(void *buffer,char *dest_mac,
			char *src_mac,int inf_index,
			struct sockaddr_ll *addr_ll,struct arp_pkt *pkt, int eth_pkt_type);

void send_arp_reply(int pf_sockfd,struct arp_pkt *pkt);

void update_arp_cache(unsigned char *eth_addr,char *ip_address,int recv_if_index,int cli_sock_desc);

void Sendto(int pf_sockfd, char* buffer, struct sockaddr_ll addr_ll,char *sendType);

bool arp_req_is_for_me(char *ip_address);

struct arp_cache_entry* get_hw_addr_arp_cache(char *ip_address);

int areq (struct sockaddr *IPaddr, socklen_t sockaddrlen, struct hwaddr *HWaddr);

int
Select(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds,
       struct timeval *timeout);

void ping_predecessor();

void sig_alrm_handler(int signum);

// ICMP related
void add_prev_node(char *ip_addr);

bool is_prev_node_in_list(char* ip_addr);

void send_icmp_echoes();

void build_eth_frame_ip(void *buffer,char *dest_mac,
			char *src_mac,int inf_index,
			struct sockaddr_ll *addr_ll, char *pkt, int eth_pkt_type);

uint16_t
checksum (uint16_t *addr, int len);

char*
Gethostbyaddr(char* canonical_ip_address);

void send_icmp_echo(int sockfd, char *dest_addr_ip,struct hwaddr hw_addr,int seqNum);

uint16_t
in_cksum(uint16_t *addr, int len);

void print_arp_cache();

void sig_alrm_handler_termination(int signum);

void printHWADDR_ARP(char *hw_addr);

#endif /* SRC_USP_H_ */
