#include "src_usp.h"

struct icmp_tour_node *icmp_prev_node_list_head;
struct icmp_tour_node *icmp_prev_node_list_rear;

void add_prev_node(char *ip_addr)
{
	if(!is_prev_node_in_list(ip_addr))
	{
		struct icmp_tour_node *temp = (struct icmp_tour_node *)malloc(sizeof(struct icmp_tour_node));
		strcpy(temp->ip_address, ip_addr);
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
		printf("%s already present in the ping list\n", ip_addr);
	}
}

bool is_prev_node_in_list(char* ip_addr)
{
	struct icmp_tour_node *temp = icmp_prev_node_list_head;
	while(!temp)
	{
		if(!strcmp(temp->ip_address, ip_addr)){
			return true;
		}
	}
	return false;
}

void send_icmp_echoes()
{
	void* buffer = (void*)malloc(ETHR_FRAME_SIZE);
	struct icmp_tour_node *temp = icmp_prev_node_list_head;
	while(!temp)
	{
		bzero(buffer, ETHR_FRAME_SIZE);
		struct hwaddr hw_addr;
		get_predecessor_mac(temp->ip_address, &hw_addr);
		send_icmp_echo(pf_sockfd, buffer, temp->ip_address, hw_addr.sll_addr);
	}
}
