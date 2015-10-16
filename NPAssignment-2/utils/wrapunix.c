/*
 * wrapunix.c
 *
 *  Created on: Oct 5, 2015
 *      Author: sujan
 */

#include	"../src/usp.h"

int
Ioctl(int fd, int request, void *arg)
{
	int		n;

	if ( (n = ioctl(fd, request, arg)) == -1)
		printf("ioctl error %s",strerror(errno));
	return(n);	/* streamio of I_LIST returns value */
}

void
Gettimeofday(struct timeval *tv, void *foo)
{
	if (gettimeofday(tv, foo) == -1)
		printf("gettimeday error \n");
	return;
}

