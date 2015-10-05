/*
 * client.c
 * 		Main Client program.
 *
 *  Created on: Sep 16, 2015
 *      Author: sujan
 */

#include "usp.h"

int main(int argc, char **argv){

	struct hostent *he;
	struct in_addr **ipaddr_list;
	struct in_addr servaddr;
	char server_ipaddress[INET_ADDRSTRLEN];
	int isIpAddress = 1,i=0,user_input=0;
	struct sigaction new_action, old_action;
	pid_t childpid;
	int pfd[2];

	if(argc < 2){
		printf("Kindly enter the IpAddress/HostName of the server to connect\n");
		exit(-1);
	}


	/**
	    Checking whether the user entered argument is IPAddress/hostname.
	    Logic : If there is a character in the argument then it is characterized
	                   as hostname as IPV4 address will not have a character.
	 **/
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

		strcpy(server_ipaddress,argv[1]);
		if((he = gethostbyaddr(&servaddr, sizeof(servaddr), AF_INET))!= NULL){
			printf("The server host name: %s\n", he->h_name);
		}else{
			printf("gethostbyaddr error : %s for ipadress: %s\n",hstrerror(h_errno),argv[1]);
			exit(-1);
		}
	}else{

		if((he  = gethostbyname(argv[1])) !=NULL){
			ipaddr_list = (struct in_addr **)he->h_addr_list;
			/**
			 *  In case of the hostname we are taking only the
			 *  first address.
			 */
			strcpy(server_ipaddress,inet_ntoa(*ipaddr_list[0]));
			for(i = 0; ipaddr_list[i] != NULL; i++) {
				printf("The server host ip address: %s \n", inet_ntoa(*ipaddr_list[i]));
			}
		}else{
			printf("gethostbyname error: %s for hostname: %s\n",hstrerror(h_errno),argv[1]);
			exit(-1);
		}
	}

	/**
	 * Registering signal handler for the SIGCHLD signal.
	 */
	new_action.sa_handler = sigchild_handler;
	sigemptyset(&new_action.sa_mask);
	new_action.sa_flags = 0;
	sigaction(SIGCHLD,NULL,&old_action);

	if(old_action.sa_handler != SIG_IGN){
		sigaction(SIGCHLD,&new_action,&old_action);
	}

	for(; ;){

		childpid = 0;

		user_input=0;

		printf("Enter the service option number \n");

		printf("1. DAY_TIME\n2. ECHO\n3. QUIT \n");

		/**
		 *  Incase the parent receives SIGCHLD
		 *  we will retry the scanf;
		 */
		while(scanf(" %d",&user_input) < 0){
			if(errno == EINTR){
				continue;
			}else{
				printf("Scan failed with error : %s",strerror(errno));
				exit(-1);
			}
		}

		if(pipe(pfd) < 0){
				printf("Pipe creation failed : %s\n",strerror(errno));
				exit(-1);
		}

		switch(user_input){

		case TIME_SERVICE:
			childpid = request_service(DAYTIMECLIENTEXENAME,server_ipaddress,pfd);
			break;
		case ECHO_SERVICE:
			childpid = request_service(ECHOCLIENTEXENAME,server_ipaddress,pfd);
			break;
		case QUIT:
			printf("Quitting the client process\n");
			exit(0);
		default:
			printf("Invalid Option Number %d\n",user_input);
			break;
		}

		if(childpid != 0)
			printf("Child Terminated with PID : %d\n",childpid);
	}
	return 0;
}

/**
   This function forks a child process and run the requested service
   by the user in the child process by doing a exec.
   @params:
        prog_name: Name of the program to run
		service_ipadress : Ipadress of the server to connect.
		pfd : pipe for the communication of parent and child process created.

**/
int request_service(char* prog_name, char *server_ipaddress,int pfd[2]){

	char str[5],buff[MAXLINE];
	int n,childpid;


	if((childpid = fork()) == 0){

		close(pfd[0]);

		sprintf(str, "%d", pfd[1]);
		if((execlp("xterm","xterm","-e",prog_name,server_ipaddress,str,(char *)0)) < 0){
			exit(-1);
		}
	}

	/**
	 *  Closing write end in the parent.
	 */
	close(pfd[1]);

	while(read(pfd[0],buff,MAXLINE) > 0){
		printf("Status of the child : %s\n",buff);
		memset(buff,0,sizeof(buff));
	}

	close(pfd[0]);
	return childpid;
}

/**
 * Handler for SIGCHLD signal.
 */
void sigchild_handler(int signum){

	int stat;
	pid_t pid;

	pid = wait(&stat);
}
