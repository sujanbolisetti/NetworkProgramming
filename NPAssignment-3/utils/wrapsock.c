/*
 * wrapsock.c
 *
 *  Created on: Nov 8, 2015
 *      Author: sujan, siddhu
 */

#include "../src/usp.h"

void
Bind(int fd, const struct sockaddr *sa, socklen_t salen)
{
	if (bind(fd, sa, salen) < 0){
		printf("bind error %s\n",strerror(errno));
		exit(0);
	}
}

void
Connect(int fd, const struct sockaddr *sa, socklen_t salen)
{
	if (connect(fd, sa, salen) < 0){
		printf("connect error %s\n",strerror(errno));
		exit(0);
	}
}



