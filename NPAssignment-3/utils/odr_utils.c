/*
 * odr_utils.c
 *
 *  Created on: Nov 12, 2015
 *      Author: sujan
 */

#include "../src/usp.h"

//int pf_sockfd = create_pf_socket();

char BROADCAST_MAC_ADDRESS[6]=  {0xff,0xff,0xff,0xff,0xff,0xff};

struct odr_frame_node *rreq_list_head;
struct odr_frame_node *rreq_list_rear;
struct hwa_info	*hw_head;


void odr_init(){

	hw_head = get_hw_addrs();
}



int create_pf_socket(){

	return socket(PF_PACKET,SOCK_RAW,htons(ODR_GRP_TYPE));
}

unsigned int getNumOfInfs(struct hwa_info *head){

	unsigned int size;

	struct hwa_info *temp;
	temp = head;

	while(temp!=NULL){
		size++;
		temp = temp->hwa_next;
	}

	return size;
}

/**
 * Have written only for rreq.
 * Will see whether to name the method as rreq/ a generic method
 * for all the types of the packets.
 */
void send_frame_rreq(int pf_sockfd,int recv_inf_index,
		struct odr_frame *frame,char *vm_name){

	struct sockaddr_ll addr_ll;
	struct hwa_info *hw_temp;
	unsigned char dest_mac[6];
	unsigned char src_mac[6];
	int send_result = 0;

	for(hw_temp = hw_head;
			hw_temp != NULL; hw_temp = hw_temp->hwa_next){

		if(hw_temp->if_index != recv_inf_index){

			strcpy(dest_mac,BROADCAST_MAC_ADDRESS);
			strcpy(src_mac,hw_temp->if_haddr);

			bzero(&addr_ll,sizeof(addr_ll));

			void* buffer = (void*)malloc(EHTR_FRAME_SIZE);

			unsigned char* etherhead = buffer;

			unsigned char* data = buffer + ETH_HDR_LEN;

			struct ethhdr *eh = (struct ethhdr *)etherhead;

			addr_ll.sll_family   = AF_PACKET;
			addr_ll.sll_ifindex  = hw_temp->if_index;
			addr_ll.sll_pkttype  = PACKET_BROADCAST;
			addr_ll.sll_protocol = htons(ODR_GRP_TYPE);
			addr_ll.sll_halen    = ETH_ALEN;


			/*set the frame header*/

			if(DEBUG){
				printf("src addr %x %d\n",hw_temp->if_haddr[0],hw_temp->if_index);
				printf("frame_size :%lu\n",EHTR_FRAME_SIZE);
			}

			memcpy((void*)buffer, (void*)dest_mac, ETH_ALEN);
			memcpy((void*)(buffer+ETH_ALEN), (void*)src_mac, ETH_ALEN);

			printf("eth src addr %x\n",src_mac[0]);
			eh->h_proto = htons(ODR_GRP_TYPE);

			memcpy(addr_ll.sll_addr,(void*)dest_mac, ETH_ALEN);

			/**
			 *  blanking out the last two values.
			 */
			addr_ll.sll_addr[6] = 0x00;
			addr_ll.sll_addr[7] = 0x00;

			memcpy(data,frame,sizeof(struct odr_frame));

			// TODO : have to convert dest_mac into presentation format.
			printf("ODR at node %s is sending frame rreq- src: %s dest: %s"
					"from outgoing interface index :%d\n",vm_name,vm_name,dest_mac);

			if(sendto(pf_sockfd,buffer,EHTR_FRAME_SIZE,0,
							(struct sockaddr *)&addr_ll,sizeof(addr_ll)) < 0){
				printf("Error in send to %s\n",strerror(errno));
				exit(-1);
			}

		}
	}
}

/**
 *  Idea :
 *  	To parse the incoming frame.
 *  	Options:
 *  		1. Check from the payload the destination IpAddress,
 *  		if the current process has the entry in the routing table
 *  		send a rreply else broadcast the frame to all the remaining
 *  		interfaces.
 *  		2. If broadcasting further add the entry in the routing table
 *  			this constructs the reverse route to the source, increase
 *  			the hop_count.
 *
 *
 */
void process_received_frame(){



}

/**
 *  This function returns the pf_sock_ids array
 *  for all the interfaces and it also populates
 *  the interface_sockid_map which has interface_id
 *  as key and sock_id as value.
 *
 */
int* bindInterfaces(int *inf_sockid_map){

	struct hwa_info	*hw_temp = hw_head;

	struct sockaddr_ll addr_ll;
	int pf_sockfd;
	unsigned int num_of_infs = 0,i=0;

	num_of_infs = getNumOfInfs(hw_head);

	int *inf_sockfds = (int *)malloc(num_of_infs*sizeof(int));

	for(hw_temp = hw_head;
				hw_temp != NULL; hw_temp = hw_temp->hwa_next){


		bzero(&addr_ll,sizeof(addr_ll));

		addr_ll.sll_family = PF_PACKET;
		memcpy(addr_ll.sll_addr,hw_temp->if_haddr,8);
		addr_ll.sll_ifindex = hw_temp->if_index;
		addr_ll.sll_protocol = htons(ODR_GRP_TYPE);

		pf_sockfd = create_pf_socket();

		Bind(pf_sockfd,(struct sock_addr*)&addr_ll,sizeof(addr_ll));

		inf_sockfds[i] = pf_sockfd;

		inf_sockid_map[hw_temp->if_index] = pf_sockfd;
		i++;
	}
	return inf_sockfds;
}


bool is_route_exists(char *canonical_ipAddress){
	return false;
}


void store_rreq_frame(struct odr_frame *frame)
{
	struct odr_frame_node *frame_node;
	frame_node = (struct odr_frame_node*)malloc(sizeof(struct odr_frame_node));
	frame_node->frame = *frame;
	frame_node->next = NULL;

	if(rreq_list_head == NULL)
	{
		rreq_list_head = frame_node;
		rreq_list_rear = rreq_list_head;
		return;
	}

	rreq_list_rear -> next = frame_node;
	rreq_list_rear = frame_node;
	return;
}

void remove_rreq_frame(struct odr_frame *frame)
{
	struct odr_frame_node *temp = rreq_list_head;

	if(rreq_list_head == NULL)
		return;

	if(temp -> frame.hdr.broadcast_id == frame->hdr.broadcast_id &&
			(strcmp(temp -> frame.hdr.cn_src_ipaddr, frame->hdr.cn_src_ipaddr) == 0))
	{
		rreq_list_head = rreq_list_head -> next;
		free(temp);
		return;
	}

	struct odr_frame_node *prev = temp;
	temp = temp -> next;
	while(temp != NULL)
	{
		if(temp -> frame.hdr.broadcast_id == frame->hdr.broadcast_id &&
					(strcmp(temp -> frame.hdr.cn_src_ipaddr, frame->hdr.cn_src_ipaddr) == 0))
		{
			if(rreq_list_rear == temp)
			{
				rreq_list_rear = prev;
			}
			prev -> next = temp -> next;
			free(temp);
			return;
		}
		prev = temp;
		temp = temp -> next;
	}
}

int is_inefficient_rreq_exists(struct odr_frame *frame)
{
	struct odr_frame_node *temp = rreq_list_head;
	while(temp != NULL)
	{
		if(!strcmp(temp -> frame.hdr.cn_src_ipaddr, frame->hdr.cn_src_ipaddr) &&
				temp -> frame.hdr.broadcast_id == frame->hdr.broadcast_id)
		{
			printf("Source addr and broadcast_id matching\n");
			if(temp -> frame.hdr.hop_count > frame->hdr.hop_count)
			{
				if("Existing rreq hop_count is greater than new rreq hop_count. So updating frame in list\n")
				temp -> frame = *frame;
				return INEFFICIENT_R_REQ_EXISTS;
			}else{
				return EFFICIENT_R_REQ_EXISTS;
			}
		}
	}
	return R_REQ_NOT_EXISTS;
}

