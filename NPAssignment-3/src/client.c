/*
 * client.c
 *
 *  Created on: Nov 8, 2015
 *      Author: sujan
 */

#include "usp.h"

int main(){

	int sockfd,tempsockfd;
	struct sockaddr_un cli_addr,odr_addr;
	struct sockaddr_in cli_ipAddress;
	int length = sizeof(cli_ipAddress);
	char ipAddress[128];
	char file_name[15] = "/tmp/npaXXXXXX";
	file_name[14] = '\0';
	char my_name[128];
	char server_vm_name[128];

	//struct vm_info *head = populate_vminfo_structs();

	//printf("vm_address :%s\n",head->vm_ipadress);

	sockfd = socket(AF_LOCAL,SOCK_DGRAM,0);

	bzero(&cli_addr,sizeof(cli_addr));
	bzero(&odr_addr,sizeof(odr_addr));

	cli_addr.sun_family = AF_LOCAL;

	odr_addr.sun_family = AF_LOCAL;

	strcpy(odr_addr.sun_path,ODR_PATH_NAME);

	if(mkstemp(file_name) < 0){
		printf("error in creating temp-file name\n");
		exit(-1);
	}

	printf("file_name :%s\n",file_name);

	if(unlink(file_name) < 0){
		printf("strerror %s\n",strerror(errno));
	}

	strcpy(cli_addr.sun_path,file_name);

	printf("sun_path %s\n",cli_addr.sun_path);

	Bind(sockfd,(SA *)&cli_addr,sizeof(cli_addr));

	Connect(sockfd,&odr_addr,sizeof(odr_addr));

	memset(my_name,'\0',sizeof(my_name));
	gethostname(my_name,sizeof(my_name));

	printf("Host Name of the Client Node : %s\n",my_name);

	strcpy(ipAddress,Gethostbyname(my_name));

	printf("Primary IP_Address of the node : %s\n",ipAddress);

	for(;;){


		printf("Enter the server node name (vm1....vm10)\n");

		memset(server_vm_name,'\0',sizeof(server_vm_name));

		if(scanf("%s",server_vm_name) < 0){
			printf("Scanf failed with error:%s\n",strerror(errno));
		}

		printf("client node at %s sending the request to server at %s\n",my_name,server_vm_name);

		msg_send(sockfd,Gethostbyname(server_vm_name),SERVER_PORT,"",0);

		struct msg_from_uds *msg = msg_receive(sockfd);

		printf("Received the time from server %s\n",msg->msg_received);

	}


	// cleaning the file_name created
	unlink(file_name);
	return 0;
}




