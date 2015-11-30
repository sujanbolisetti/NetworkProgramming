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

struct tour_route{

	in_addr_t ip_address;
	uint16_t port_number;
};

struct tour_route*
create_tour_list(int count , char **argv);

uint32_t
Gethostbyname(char *my_name);

void insert_my_address_at_bgn();

void insert_multicast_address_at_lst();

uint32_t
Gethostbyname(char *my_name);

char* Gethostname();

void build_ip_header(char *buff, uint16_t index,int total_len);

void populate_data_in_datagram(char *buff, uint16_t index,uint16_t count);

uint32_t getIpAddressInTourList(uint16_t index);

int
calculate_length(int count);

#endif /* SRC_USP_H_ */
