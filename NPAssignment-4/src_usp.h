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

/* include mcast_join1 */
//	#include	<net/if.h>

#define SA struct sockaddr

#define	min(a,b)	((a) < (b) ? (a) : (b))
#define	max(a,b)	((a) > (b) ? (a) : (b))

struct tour_route{
	char ip_address[32];
	uint16_t port_number;
};

struct tour_payload{

	uint16_t index;
	uint16_t count;
	struct tour_route tour_list[SIZE_OF_TOUR_LIST];
};

void
create_tour_list(int count , char **argv, struct tour_route *tour_list);

void
insert_me_address_at_bgn(struct tour_route *tour_list);

void
insert_multicast_address_at_lst(int count,struct tour_route *tour_list);

char*
Gethostbyname(char *my_name);

char* Gethostname();

void build_ip_header(char *buff,uint16_t total_len,char *dest_addr);

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

void allocate_buffer(char *buff);

#endif /* SRC_USP_H_ */
