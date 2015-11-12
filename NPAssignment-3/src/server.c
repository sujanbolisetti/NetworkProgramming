/*
 * server.c
 *
 *  Created on: Nov 8, 2015
 *      Author: sujan
 */

#include "usp.h"

int main(){

	int listenfd;
	char write_buff[MAXLINE];

	struct sockaddr_un serverAddr;
	listenfd = socket(AF_LOCAL,SOCK_DGRAM,0);

	bzero(&serverAddr,sizeof(serverAddr));
	serverAddr.sun_family = AF_LOCAL;
	strcpy(serverAddr.sun_path,SERVER_WELL_KNOWN_PATH_NAME);

	for(;;){


		time_t ticks = time(NULL);

		snprintf(write_buff, sizeof(write_buff), "%.24s\r\n" , ctime(&ticks));
	}

	return 0;
}



