/*
 * client.c
 *
 *  Created on: Sep 16, 2015
 *      Author: sujan
 */

#include "usp.h"

int main(int argc, char **argv){


	struct hostent *he;
	struct in_addr **ipaddr_list;
	struct in_addr servaddr;
	char host_name[INET_ADDRSTRLEN];
	int isIpAddress = 1,i=0,user_input=0;

	if(argc < 2){
		printf("Kindly enter the IpAddress of the server to connect\n");
		exit(-1);
	}

	for(i=0;i<strlen(argv[1]);i++){

		char c = argv[1][i];
		if((c > 64 && c < 91) || (c > 96 && c < 123)){
			isIpAddress  = 0;
			break;
		}
	}

	if(isIpAddress){

		if((inet_pton(AF_INET,argv[1],&servaddr)) <  0){
			printf("%s\n",strerror(errno));
		}

		if((he = gethostbyaddr(&servaddr, sizeof(servaddr), AF_INET))!= NULL){
			printf("The server host is %s\n", he->h_name);
		}else{
			printf("gethostbyaddr error : %s for ipadress: %s\n",hstrerror(h_errno),argv[1]);
			exit(-1);
		}

	}else{

		if((he  = gethostbyname(argv[1])) !=NULL){
			ipaddr_list = (struct in_addr **)he->h_addr_list;

			for(i = 0; ipaddr_list[i] != NULL; i++) {
				printf("The server host ip address is %s \n", inet_ntop(he->h_addrtype, *ipaddr_list[i], host_name,sizeof(host_name)));
			}
		}else{
			printf("gethostbyname error: %s for hostname: %s\n",hstrerror(h_errno),argv[1]);
			exit(-1);
		}



	}

	for(; ;){

		printf("Enter the service name \n");

		printf("1. ECHO\n2. DAY_TIME\n3. QUIT \n");

		scanf(" %d", &user_input);

		switch(user_input){
		case 1:
			printf("Not implemented\n");
			break;
		case 2:
			printf("Implemented\n");
			break;
		case 3:
			printf("Quiting the parent process\n");
			exit(0);
		}
	}
	return 0;
}
