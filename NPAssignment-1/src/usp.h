/*
 * usp.h
 *
 *  Created on: Sep 15, 2015
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

#define SA struct sockaddr

#define	min(a,b)	((a) < (b) ? (a) : (b))
#define	max(a,b)	((a) > (b) ? (a) : (b))

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

#endif /* USP_H_ */
