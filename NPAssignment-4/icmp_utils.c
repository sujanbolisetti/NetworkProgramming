#include "src_usp.h"

struct icmp_tour_node *icmp_prev_node_list_head=NULL;
struct icmp_tour_node *icmp_prev_node_list_rear=NULL;

void add_prev_node(char *ip_addr)
{
	if(!is_prev_node_in_list(ip_addr))
	{
		struct icmp_tour_node *temp = (struct icmp_tour_node *)malloc(sizeof(struct icmp_tour_node));
		strcpy(temp->ip_address, ip_addr);
		temp->seqNum = 0;
		temp->next = NULL;

		if(DEBUG){
			printf("Adding the predecessor in the icmp_list :%s\n",Gethostbyaddr(ip_addr));
		}

		if(icmp_prev_node_list_head == NULL)
		{
			icmp_prev_node_list_head = temp;
			icmp_prev_node_list_rear = temp;
		}
		else
		{
			icmp_prev_node_list_rear->next = temp;
			icmp_prev_node_list_rear = temp;
		}
	}
	else
	{
		printf("%s is already present in the pinging nodes list hence no-op\n", Gethostbyaddr(ip_addr));
	}
}

bool is_prev_node_in_list(char* ip_addr)
{
	struct icmp_tour_node *temp = icmp_prev_node_list_head;
	while(temp!=NULL)
	{
		if(!strcmp(temp->ip_address, ip_addr)){

			if(DEBUG)
				printf("Predecessor is already present in the list %s\n",Gethostbyaddr(ip_addr));

			return true;
		}
		temp = temp->next;
	}

	//printf("prev node doesn't exist in the list\n");
	return false;
}

void send_icmp_echoes()
{
	struct icmp_tour_node *temp = icmp_prev_node_list_head;
	struct hwaddr hw_addr;
	while(temp!= NULL)
	{
		if(DEBUG)
		{
			printf("Sending ICMP echoes\n");
		}

		bzero(&hw_addr,sizeof(hw_addr));
		get_predecessor_mac(temp->ip_address, &hw_addr);

		if(DEBUG)
		{
			printf("Received a Mac address ");
			printHWADDR(hw_addr.sll_addr);
		}

		send_icmp_echo(pf_sockfd,temp->ip_address, hw_addr,temp->seqNum++);
		temp = temp->next;
	}
}
