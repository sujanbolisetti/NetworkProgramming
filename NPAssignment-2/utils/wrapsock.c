/*
 * wrapsock.c
 *
 *  Created on: Oct 5, 2015
 *      Author: sujan
 */

#include "../src/usp.h"

int
Socket(int family, int type, int protocol)
{
	int		n;

	if ( (n = socket(family, type, protocol)) < 0)
		printf("socket creation error %s\n",strerror(errno));
	return(n);
}

/**
 *  changed accept to handle errno EINTR also.
 */
int
Accept(int fd, struct sockaddr *sa, socklen_t *salenptr)
{
	int		n;

again:
	if ( (n = accept(fd, sa, salenptr)) < 0) {
#ifdef	EPROTO
		if (errno == EPROTO || errno == ECONNABORTED)
#else
		if (errno == ECONNABORTED || errno == EINTR)
#endif
			goto again;
		else
			printf("accept error %s\n",strerror(errno));
	}
	return(n);
}

void
Bind(int fd, const struct sockaddr *sa, socklen_t salen)
{
	if (bind(fd, sa, salen) < 0)
		printf("bind error %s\n",strerror(errno));
}

void
Listen(int fd, int backlog)
{
	char	*ptr;

	/*4can override 2nd argument with environment variable */
	if ( (ptr = getenv("LISTENQ")) != NULL)
		backlog = atoi(ptr);

	if (listen(fd, backlog) < 0)
		printf("listen error %s\n",strerror(errno));
}

void
Connect(int fd, const struct sockaddr *sa, socklen_t salen)
{
	if (connect(fd, sa, salen) < 0)
		printf("connect error %s\n",strerror(errno));
}

int
Select(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds,
       struct timeval *timeout)
{
	int		n;

	again:
		if ( (n = select(nfds, readfds, writefds, exceptfds, timeout)) < 0){
			if(errno == EINTR){
				goto again;
			}else{
				printf("select error %s\n",strerror(errno));
			}
		}
	return(n);		/* can return 0 on timeout */
}

void Getpeername(int sockfd,struct sockaddr *sa, int* length){

	if(getpeername(sockfd, sa, length) < 0 ){
		printf("get peer name error %s\n",strerror(errno));
	}
}

void Getsockname(int sockfd,struct sockaddr *sa, int* length){

	if(getsockname(sockfd, sa, length) < 0 ){
		printf("get sock name error %s\n",strerror(errno));
	}
}

bool Sendto(int socket, struct dg_payload* pload, ssize_t size, int flags, struct sockaddr *sa, socklen_t socklen)
{
	struct dg_payload ack;
	printf("Packet sent seq number:");
	sendto(socket, (void *) pload, size, flags, sa, socklen);
	printf("%d\n", pload->seq_number);

	memset(&ack, 0, sizeof(ack));
	recvfrom(socket, &ack, sizeof(ack), 0, NULL, NULL);
	printf("Ack received seq number %d\n", ack.seq_number);
	if(ack.type == ACK && ack.seq_number == pload->seq_number)
	{
		return true;
	}

	return false;
}

void sendAck(int socket, int seq_num, struct sockaddr *sa)
{
	struct dg_payload ack;
	ack.type = ACK;
	ack.seq_number = seq_num;
	sendto(socket, (void *) &ack, sizeof(ack), 0, sa, sizeof(*sa));
	printf("Ack sent to seq number: %d\n", ack.seq_number);
}

int Recvfrom(int socket, struct dg_payload *pload, ssize_t size, int flags, struct sockaddr *sa, socklen_t *socklen)
{
	int n;
	again:
		if((n=recvfrom(socket, pload, size, flags, sa, socklen)) < 0){
			if(errno == EINTR){
				printf("Received an EINTR in recvfrom\n");
				goto again;
			}else{
				printf("Error in recvfrom : %s\n", strerror(errno));
				return n;
			}
		}
	return 0;
}




