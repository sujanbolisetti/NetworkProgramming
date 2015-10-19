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

#include <setjmp.h>

#include<string.h>

#include<fcntl.h>

#include <netdb.h>

#include <pthread.h>

#include<unistd.h>

#include <signal.h>

# include <sys/ioctl.h>

#include<math.h>

#include	"unprtt.h"

// #include <sys/sockio.h>

#ifdef	HAVE_PTHREAD_H
# include	<pthread.h>
#endif

#include <stdbool.h>

#include "constants.h"

#define PACKET_SIZE 512

#define SA struct sockaddr

#define	min(a,b)	((a) < (b) ? (a) : (b))
#define	max(a,b)	((a) > (b) ? (a) : (b))

#define	FILE_MODE	(S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)

struct binded_sock_info{
	int sockfd;
	char ip_address[128];
	char network_mask[128];
	char subnet_adress[128];
	struct binded_sock_info *next;
};

struct connected_client_address{
	char child_pid[256];
	char client_sockaddress[256];
	struct connected_client_address *next;
};

struct dg_payload{
	uint32_t type;
	uint32_t seq_number;
	uint32_t ack;
	uint32_t ts;
	uint32_t windowSize;
	char buff[PACKET_SIZE];
};

struct flow_metadata{
	uint32_t slidingWindow;
	uint32_t slidingWindowStart;
	uint32_t slidingWindowEnd;
	uint32_t cwnd;
	uint32_t ssthresh;
};

struct Node{
	int ack;
	char buff[PACKET_SIZE];
	int seqNum;
	int ind;
	struct Node *next;
	uint32_t type;
};

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

void sig_alrm(int signo);

char *
Sock_ntop_host(const struct sockaddr *sa, socklen_t salen);

void Getpeername(int sockfd,struct sockaddr *sa, int* length);

void Getsockname(int sockfd,struct sockaddr *sa, int* length);

void doFileTransfer(struct binded_sock_info *sock_info,struct sockaddr_in IPClient,
		struct flow_metadata *flow_data, char *file_name);

void Fscanf(FILE *fp, char *format, void *data);

void getClientIPAddress(struct sockaddr_in *clientAddr,struct sockaddr_in *networkAddr, struct sockaddr_in *serverAddr,
			struct sockaddr_in *IPClient,unsigned long* maxMatch);

struct binded_sock_info* getInterfaces(int portNumber,unsigned long *maxMatch,
			struct sockaddr_in *serverAddr,struct sockaddr_in *IPClient);

int getMaxFD(struct binded_sock_info *head);

void printInterfaceDetails(struct binded_sock_info *head);

int
Ioctl(int fd, int request, void *arg);

void Recvfrom(int , struct dg_payload *, ssize_t , int , struct sockaddr *, socklen_t *);
bool Sendto(int , struct dg_payload *, ssize_t , int ,  struct sockaddr *, socklen_t );

void removeClientAddrFromList(int child_pid, struct connected_client_address **head);

char* getSocketAddress(struct sockaddr_in IPClient);

bool isClientConnected(struct connected_client_address *head,char *ipAddressSocket);

bool populateDataList(struct Node *sent,int fd,
			int windowSize,struct Node* ackNode,struct Node *headNode);

struct Node * BuildCircularLinkedList(int size);
void printList(struct Node *head);

unsigned int rand_interval(unsigned int, unsigned int);

unsigned int is_in_limits(float);

void
congestion_control(int how,int *cwnd, int *ssthresh, int *duplicateAck,int *state);

// UDP packet types
enum PACKET_TYPE
{
	ack,
	pay_load
};

enum CONGESTION_TYPE{
	slowstart,
	fast_
};

#endif /* USP_H_ */
