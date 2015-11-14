/*
 * routing_utils.c
 *
 *  Created on: Nov 13, 2015
 *      Author: sujan
 */

#include "../src/usp.h"

struct route_entry *rtable_head = NULL;
struct route_entry *rtable_tail = NULL;

void add_entry_in_rtable(char *dest_ipaddress, char *next_hp_mac_addr, int hop_count, int outg_inf_index){

	struct route_entry *r_node = (struct route_entry*)malloc(sizeof(struct route_entry));

	strcpy(r_node->dest_canonical_ipAddress,dest_ipaddress);
	strcpy(r_node->next_hop_mac_address,next_hp_mac_addr);

	r_node->hop_count = hop_count;

	// TODO : have to populate the time_out
	r_node->outg_inf_index = outg_inf_index;

	r_node->next = NULL;

	if(rtable_head ==  NULL){
		rtable_head = r_node;
		rtable_tail = rtable_head;
	}else{
		rtable_tail->next =  r_node;
		rtable_tail = r_node;
	}
}

struct route_entry* get_rentry_in_rtable(char *dest_ipAddress){

	struct route_entry *temp_rnode = rtable_head;

	while(temp_rnode != NULL){

		if(strcmp(temp_rnode->dest_canonical_ipAddress,dest_ipAddress)){
			return temp_rnode;
		}

		temp_rnode = temp_rnode->next;
	}
	return NULL;
}





