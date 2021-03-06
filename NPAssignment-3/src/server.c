/*
 * server.c
 *
 *  Created on: Nov 8, 2015
 *      Author: sujan, siddhu
 */

#include "usp.h"

int main(){

	int listenfd;
	char write_buff[MAXLINE];
	struct msg_from_uds *msg;
	char my_name[128];

	memset(my_name,'\0',sizeof(my_name));
	gethostname(my_name,sizeof(my_name));

	printf("Starting the server on %s\n",my_name);

	struct sockaddr_un serverAddr,odr_addr;
	listenfd = socket(AF_LOCAL,SOCK_DGRAM,0);

	bzero(&serverAddr,sizeof(serverAddr));
	bzero(&odr_addr,sizeof(odr_addr));

	serverAddr.sun_family = AF_LOCAL;
	odr_addr.sun_family= AF_LOCAL;

	strcpy(serverAddr.sun_path,SERVER_WELL_KNOWN_PATH_NAME);
	strcpy(odr_addr.sun_path,ODR_PATH_NAME);

	unlink(SERVER_WELL_KNOWN_PATH_NAME);

	Bind(listenfd,(SA *)&serverAddr,sizeof(serverAddr));

	Connect(listenfd,(SA *)&odr_addr,sizeof(odr_addr));

	for(;;){

		msg = msg_receive(listenfd);

		if(DEBUG)
			printf("Message received in the server on port_num %d\n",msg->dest_port_num);

		time_t ticks = time(NULL);

		snprintf(write_buff, sizeof(write_buff), "%.24s\r" , ctime(&ticks));

		printf("server at node %s responding to request from %s\n",my_name,Gethostbyaddr(msg->canonical_ipAddress));

		if(DEBUG)
			printf("Server the sending the time %s\n",write_buff);

		msg_send(listenfd,msg->canonical_ipAddress,msg->dest_port_num,write_buff,msg->flag);

		printf("----------------------------------------------------------------------\n");

	}

	return 0;
}



