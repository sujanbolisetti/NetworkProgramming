/*
 * utils_wrapsock.c
 *
 *  Created on: Nov 29, 2015
 *      Author: sujan
 */

#include "src_usp.h"

int
Socket(int family, int type, int protocol)
{
	int		n;

	if ( (n = socket(family, type, protocol)) < 0)
		printf("socket creation error %s\n",strerror(errno));
	return(n);
}

int
Select(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds,
       struct timeval *timeout)
{
	int		n;

	again:
		if ( (n = select(nfds, readfds, writefds, exceptfds, timeout)) < 0){

			if(errno == EINTR){
				printf("Received an interrupt\n");
				goto again;
			}else{
				printf("select error %s\n",strerror(errno));
			}
		}

	return(n);		/* can return 0 on timeout */
}

void
Bind(int fd, const struct sockaddr *sa, socklen_t salen)
{
	if (bind(fd, sa, salen) < 0)
		printf("bind error %s\n",strerror(errno));
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
Connect(int fd, const struct sockaddr *sa, socklen_t salen)
{
	if (connect(fd, sa, salen) < 0){
		printf("connect error in connecting to odr : %s kindly start the odr process before starting the client/server\n",strerror(errno));
		exit(0);
	}
}
