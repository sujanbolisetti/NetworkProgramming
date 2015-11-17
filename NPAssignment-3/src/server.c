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
	struct reply_from_uds *reply;

	struct sockaddr_un serverAddr,odr_addr;
	listenfd = socket(AF_LOCAL,SOCK_DGRAM,0);

	bzero(&serverAddr,sizeof(serverAddr));
	bzero(&odr_addr,sizeof(odr_addr));

	serverAddr.sun_family = AF_LOCAL;
	odr_addr.sun_family= AF_LOCAL;

	strcpy(serverAddr.sun_path,SERVER_WELL_KNOWN_PATH_NAME);
	strcpy(odr_addr.sun_path,ODR_PATH_NAME);

	Bind(listenfd,(SA *)&serverAddr,sizeof(serverAddr));

	Connect(listenfd,(SA *)&odr_addr,sizeof(odr_addr));

	for(;;){

		reply = msg_receive(listenfd);

		time_t ticks = time(NULL);

		snprintf(write_buff, sizeof(write_buff), "%.24s\r\n" , ctime(&ticks));

		msg_send(listenfd,reply->canonical_ipAddress,reply->portNumber,write_buff,reply->flag);
	}

	return 0;
}



