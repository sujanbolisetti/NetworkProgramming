/*
 * client.c
 *
 *  Created on: Nov 8, 2015
 *      Author: sujan, siddhu
 */

#include "usp.h"

int main(int argc, char **argv){

	int sockfd,tempsockfd;
	struct sockaddr_un cli_addr,odr_addr;
	struct sockaddr_in cli_ipAddress;
	int length = sizeof(cli_ipAddress);
	char ipAddress[128];
	char file_name[15] = "/tmp/npaXXXXXX";
	file_name[14] = '\0';
	char my_name[128];
	char server_vm_name[128];
	fd_set monitor_fds;

	sockfd = socket(AF_LOCAL,SOCK_DGRAM,0);

	bzero(&cli_addr,sizeof(cli_addr));
	bzero(&odr_addr,sizeof(odr_addr));

	cli_addr.sun_family = AF_LOCAL;

	odr_addr.sun_family = AF_LOCAL;

	strcpy(odr_addr.sun_path,ODR_PATH_NAME);

	if(mkstemp(file_name) < 0){
		printf("error in creating temporary file\n");
		exit(-1);
	}

	printf("file_name :%s\n",file_name);

	if(unlink(file_name) < 0){
		printf("str error %s\n",strerror(errno));
	}

	strcpy(cli_addr.sun_path, file_name);

	printf("sun_path %s\n",cli_addr.sun_path);

	Bind(sockfd,(SA *)&cli_addr,sizeof(cli_addr));

	Connect(sockfd,&odr_addr,sizeof(odr_addr));

	memset(my_name,'\0',sizeof(my_name));
	gethostname(my_name,sizeof(my_name));

	printf("Host Name of the Client Node : %s\n",my_name);

	strcpy(ipAddress,Gethostbyname(my_name));

	printf("Primary IP_Address of the node : %s\n",ipAddress);

	struct timeval t;

	int timeout = 3; // 3 Seconds

	for(;;)
	{
		printf("Enter the server node name (vm1....vm10)\n");

		memset(server_vm_name,'\0',sizeof(server_vm_name));

		if(scanf("%s",server_vm_name) < 0){
			printf("scanf failed with error:%s\n",strerror(errno));
		}

		t = get_time_interval(timeout);

		int force_dsc = 0;

		int i = 0;
		for (i = 0; i < 2; i++)
		{
			FD_ZERO(&monitor_fds);

			FD_SET(sockfd, &monitor_fds);

			msg_send(sockfd,Gethostbyname(server_vm_name),SERVER_PORT,"What is current time?", force_dsc);

			if(!force_dsc)
			{
				printf("Time request sent to server : %s", server_vm_name);
			}
			else
			{
				printf("Time request sent again to server with force discovery : %s", server_vm_name);
			}

			select(sockfd + 1, &monitor_fds, NULL, NULL, &t);

			if(FD_ISSET(sockfd, &monitor_fds)){
				struct msg_from_uds *msg = msg_receive(sockfd);
				printf("Received current time from server : %s\n", msg->msg_received);
				break;
			}

			if(force_dsc)
			{
				printf("Reply not received from server even after force discovery request. So, exiting the program\n");
				break;
			}

			force_dsc = 1;
		}
	}

	// cleaning the file_name created
	unlink(file_name);
	return 0;
}

// 3 Seconds
struct timeval get_time_interval(int secs)
{
    struct timeval t;
    t.tv_sec = secs;
    t.tv_usec = 0;
    return t;
}

