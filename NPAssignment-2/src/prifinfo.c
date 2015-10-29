/*
 * prifinfo.c
 *
 *  Created on: Oct 5, 2015
 *       Authors: sujan, sidhartha
 */

#include	"unpifi.h"

int
main(int argc, char **argv)
{
	struct ifi_info	*ifi, *ifihead;
	struct sockaddr	*sa;
	u_char			*ptr;
	int				i, family, doaliases;

	if (argc != 4){
		printf("usage: prifinfo <inet4|inet6> <doaliases>");
		exit(1);
	}

	if (strcmp(argv[1], "inet4") == 0)
		family = AF_INET;
#ifdef	IPv6
	else if (strcmp(argv[1], "inet6") == 0)
		family = AF_INET6;
#endif
	else{
		printf("<Invalid Address Family>");
		exit(1);
	}
	doaliases = atoi(argv[2]);

	for (ifihead = ifi = Get_ifi_info(family, doaliases);
		 ifi != NULL; ifi = ifi->ifi_next) {
		printf("%s: ", ifi->ifi_name);
		if (ifi->ifi_index != 0)
			printf("(%d) ", ifi->ifi_index);
		printf("<");
/* *INDENT-OFF* */
		if (ifi->ifi_flags & IFF_UP)			printf("UP ");
		if (ifi->ifi_flags & IFF_BROADCAST)		printf("BCAST ");
		if (ifi->ifi_flags & IFF_MULTICAST)		printf("MCAST ");
		if (ifi->ifi_flags & IFF_LOOPBACK)		printf("LOOP ");
		if (ifi->ifi_flags & IFF_POINTOPOINT)	printf("P2P ");
		printf(">\n");
/* *INDENT-ON* */

		if ( (i = ifi->ifi_hlen) > 0) {
			ptr = ifi->ifi_haddr;
			do {
				printf("%s%x", (i == ifi->ifi_hlen) ? "  " : ":", *ptr++);
			} while (--i > 0);
			printf("\n");
		}
		if (ifi->ifi_mtu != 0)
			printf("  MTU: %d\n", ifi->ifi_mtu);

		if ( (sa = ifi->ifi_addr) != NULL){

			struct sockaddr_in	*serv = (struct sockaddr_in *) sa;

			printf("Address : %u ",serv->sin_addr.s_addr);

			printf("  IP addr: %s\n",
						Sock_ntop_host(sa, sizeof(*sa)));

			struct sockaddr_in servaddr;

			bzero(&servaddr,sizeof(servaddr));

			if((inet_pton(AF_INET,"127.0.0.1",&servaddr.sin_addr)) <  0){
						printf("error %s\n",strerror(errno));
			}

			if(servaddr.sin_addr.s_addr & serv->sin_addr.s_addr){
				printf("They are matching\n");
			}else{
				printf("they are not matching\n");
			}

			printf("Address to numeric : %u \n",servaddr.sin_addr.s_addr);


		}

		if ( (sa = ifi->ifi_ntmaddr) != NULL)
			printf(" network mask: %s\n",
						Sock_ntop_host(sa, sizeof(*sa)));

		if ( (sa = ifi->ifi_brdaddr) != NULL)
			printf("  broadcast addr: %s\n",
						Sock_ntop_host(sa, sizeof(*sa)));
		if ( (sa = ifi->ifi_dstaddr) != NULL)
			printf("  destination addr: %s\n",
						Sock_ntop_host(sa, sizeof(*sa)));
	}
	free_ifi_info(ifihead);
	exit(0);
}



