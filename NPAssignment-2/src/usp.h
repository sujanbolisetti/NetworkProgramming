/*
 * usp.h
 *
 *  Created on: Oct 4, 2015
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

#include <sys/ioctl.h>

#include <stdbool.h>

#include "constants.h"

struct binded_sock_info{

	int sockfd;
	char ip_address[128];
	char network_mask[128];
	char subnet_adress[128];
	struct binded_sock_info *next;

};

struct connected_client_address{
	char client_sockaddress[256];
	struct connected_client_address *next;
};

struct dg_payload{
	int type;
	int seq_number;
	int portNumber;
	char buff[MAXLINE];
};

#define SA struct sockaddr

#define	min(a,b)	((a) < (b) ? (a) : (b))
#define	max(a,b)	((a) > (b) ? (a) : (b))

#define	FILE_MODE	(S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)

int		Accept(int, SA *, socklen_t *);
void	Bind(int, const SA *, socklen_t);
void	Connect(int, const SA *, socklen_t);
void	Listen(int, int);

int 	request_service(char* prog_name, char *host_ipaddress,int pfd[2]);

void    Pthread_create(pthread_t *, const pthread_attr_t *,
                                           void * (*)(void *), void *);

char* create_error_message(char *error_msg,int errorno);

void
Pthread_detach(pthread_t tid);

void* time_service(void *arg);

void* echo_service(void* arg);

void sigchild_handler(int signum);

char *
Sock_ntop_host(const struct sockaddr *sa, socklen_t salen);

void Getpeername(int sockfd,struct sockaddr *sa, int* length);

void Getsockname(int sockfd,struct sockaddr *sa, int* length);

void doFileTransfer(struct binded_sock_info *sock_info,struct sockaddr_in);

// UDP packet types
enum PACKET_TYPE
{
	ack,
	pay_load
};

#endif /* USP_H_ */
