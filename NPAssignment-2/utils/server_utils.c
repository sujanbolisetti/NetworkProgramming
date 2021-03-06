/*
 * server_utils.c
 *
 *  Created on: Oct 19, 2015
 *       Authors: sujan, sidhartha
 */
#include "../src/usp.h"

int Send_Packet(int conn_sockfd,int seq, char *buff, int type, uint32_t ts,char *client_addr){

	struct dg_payload send_pload;

	memset(&send_pload,0,sizeof(send_pload));
	send_pload.seq_number = seq;
	send_pload.type = type;
	int n;

	switch(type){
		case FIN:
			if(buff != NULL){
				strcpy(send_pload.buff,buff);
			}
			if(DEBUG)
				printf("Completed File Transfer and waiting for Acks...\n");

			break;
		case ACK:
			printf("(%s) Sending an ACK ....\n",client_addr);
			break;
		case WINDOW_PROBE:
			printf("(%s) Sending an window_probe ....\n",client_addr);
			break;
		case PAYLOAD:
			strcpy(send_pload.buff,buff);
			break;
	}

	send_pload.ts = ts;

	send_pload = convertToNetworkOrder(send_pload);

	if((n = sendto(conn_sockfd,(void *)&send_pload,sizeof(send_pload),0,NULL,0)) < 0){
		printf("Error in send : %s\n",strerror(errno));
		exit(0);
	}
	return seq;
}

char* printCongestionState(int congestion_state_id){

	switch(congestion_state_id){
	case SLOW_START:
		return "Slow Start";
	case CONGESTION_AVOIDANCE:
		return "Congestion Avoidance";
	case FAST_RECOVERY:
		return "Fast Recovery";
	}
}

