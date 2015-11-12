/*
 * usp.h
 *
 *  Created on: Nov 8, 2015
 *      Author: sujan
 */

#ifndef USP_H_
#define USP_H_

#include<sys/socket.h>

#include<sys/types.h>

#include<netinet/in.h>

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

#include "constants.h"

#include <error.h>

#include	<sys/un.h>

#include <linux/if_packet.h>

#include <linux/if_ether.h>

#include <linux/if_arp.h>

#define SA struct sockaddr

#define	min(a,b)	((a) < (b) ? (a) : (b))
#define	max(a,b)	((a) > (b) ? (a) : (b))

struct vm_info{
	char *vm_name;
	char *vm_ipadress;
	struct vm_info *next;
};

struct reply_from_ODR{
	char* msg_received;
	char* canonical_ipAddress;
	int* portNumber;
};

void msg_send(int sockfd, char* destIpAddress, int destPortNumber,
					char* message,int flag);

struct reply_from_ODR * msg_receive(int sockfd);

char *
get_vm_ipaddress(char *vm_name,struct vm_info *head);

void
vm_init();

struct vm_info *
populate_vminfo_structs();

char*
get_vm_name(char *vm_ipAddress,struct vm_info *head);

char*
Gethostbyname(char *my_name);

char *
Sock_ntop_host(const struct sockaddr *sa, socklen_t salen);





#endif /* USP_H_ */
