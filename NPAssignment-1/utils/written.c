/*
 * Written.c
 * 		wrapper function on write syscall.
 * 		Code was taken from Steven's code base.
 *
 *  Created on: Sep 17, 2015
 *      Author: sujan
 */

#include "../src/usp.h"

/* Write "n" bytes to a descriptor. */
ssize_t
writen(int fd, const void *vptr, size_t n)
{
	size_t		nleft;
	ssize_t		nwritten;
	const char	*ptr;

	ptr = vptr;
	nleft = n;
	while (nleft > 0) {
		if ( (nwritten = write(fd, ptr, nleft)) <= 0) {
			if (nwritten < 0 && errno == EINTR)
				nwritten = 0;		/* and call write() again */
			else
				return(-1);			/* error */
		}

		nleft -= nwritten;
		ptr   += nwritten;
	}
	return(n);
}
/* end writen */

void
Writen(int fd, void *ptr, size_t nbytes)
{
	if (writen(fd, ptr, nbytes) != nbytes)
		printf("write error %s\n", strerror(errno));
}

