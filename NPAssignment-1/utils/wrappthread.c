/*
 * wrappthread.c
 *
 *  Created on: Sep 17, 2015
 *      Author: sujan
 */

#include "../src/usp.h"

void
Pthread_create(pthread_t *tid, const pthread_attr_t *attr,
			   void * (*func)(void *), void *arg)
{
	int		n;

	if ( (n = pthread_create(tid, attr, func, arg)) == 0)
		return;
	errno = n;
	printf("pthread create error %s\n",strerror(errno));
}

void
Pthread_detach(pthread_t tid)
{
	int		n;

	if ( (n = pthread_detach(tid)) == 0)
		return;
	errno = n;
	printf("pthread detach error %s\n",strerror(errno));
}


