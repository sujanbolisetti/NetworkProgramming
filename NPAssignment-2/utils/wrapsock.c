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

	if ( (n = select(nfds, readfds, writefds, exceptfds, timeout)) < 0)
		printf("select error %s\n",strerror(errno));
	return(n);		/* can return 0 on timeout */
}




