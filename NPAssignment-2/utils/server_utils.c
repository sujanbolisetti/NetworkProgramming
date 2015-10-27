/*
 * server_utils.c
 *
 *  Created on: Oct 19, 2015
 *      Author: sujan
 */
#include "../src/usp.h"

int Send_Packet(int conn_sockfd,int seq, char *buff, int type, uint32_t ts){

	struct dg_payload send_pload;

	memset(&send_pload,0,sizeof(send_pload));
	send_pload.seq_number = htonl(seq);
	send_pload.type = htons(type);
	int n;

	switch(type){
		case FIN:
			printf("Completed File Transfer and waiting for Acks...\n");
			break;
		case ACK:
			printf("Sending an ACK ....\n");
			break;
		case WINDOW_PROBE:
			printf("Sending an window_probe ....\n");
			break;
		case PAYLOAD:
			strcpy(send_pload.buff,buff);
			break;
	}

	send_pload.ts = htonl(ts);
	if((n = sendto(conn_sockfd,(void *)&send_pload,sizeof(send_pload),0,NULL,0)) < 0){
		printf("Error in send : %s\n",strerror(errno));
		exit(0);
	}
	return seq;
}



