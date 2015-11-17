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

	printf("Started adding routing entry\n");
	struct route_entry *r_node = (struct route_entry*)malloc(sizeof(struct route_entry));

	printf("started address strcpy :%s\n",dest_ipaddress);
	strcpy(r_node->dest_canonical_ipAddress,dest_ipaddress);

	printf("Next Hop in Added_route ");
	printHWADDR(next_hp_mac_addr);
	memcpy((void *)r_node->next_hop_mac_address,(void *)next_hp_mac_addr,ETH_ALEN);
	printf("End address strcpy\n");

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
	printf("Added routing entry\n");
}

struct route_entry* get_rentry_in_rtable(char *dest_ipAddress){

	printf("Entered get_routing_entry\n");
	struct route_entry *temp_rnode = rtable_head;

	//TODO Have to check stale entries.
	while(temp_rnode != NULL){

		if(!strcmp(temp_rnode->dest_canonical_ipAddress,dest_ipAddress)){
			return temp_rnode;
		}

		temp_rnode = temp_rnode->next;
	}
	return NULL;
}

bool update_routing_table(char *dest_ipaddress, char *next_hp_mac_addr, int hop_count, int outg_inf_index){

	printf("Entered updating routing table\n");
	struct route_entry *rnode_entry = get_rentry_in_rtable(dest_ipaddress);

	if(rnode_entry != NULL)
	{
		printf("Updating routing table\n");
		if(hop_count < rnode_entry->hop_count)
		{
			rnode_entry->hop_count = hop_count;
			rnode_entry->outg_inf_index = outg_inf_index;
			memcpy(rnode_entry->next_hop_mac_address, next_hp_mac_addr,ETH_ALEN);
			printRoutingTable(rtable_head);
			return true;
		}
		else
		{
			printRoutingTable(rtable_head);
			return false;
		}
	}
	else
	{
		printf("Adding routing table\n");
		printf("Next Hop Before Add ");
		printHWADDR(next_hp_mac_addr);
		add_entry_in_rtable(dest_ipaddress, next_hp_mac_addr, hop_count, outg_inf_index);
		printRoutingTable(rtable_head);
		return true;
	}

}

void printRoutingTable(struct route_entry* head)
{
	struct route_entry* temp = head;
	printf("------------------------ROUTING TABLE--------------------------------\n");
	while(temp != NULL)
	{
		printf("Dest address: %s\t", temp->dest_canonical_ipAddress);
		printf("Hop count: %d\t", temp->hop_count);
		printf("Next Hop");
		printHWADDR(temp->next_hop_mac_address);
		printf("Interface index: %d\t",temp->outg_inf_index);
		printf("Time stamp: %d\t", temp->time_stamp);
		printf("\n");
		temp = temp -> next;
	}
	printf("----------------------------------------------------------------------\n");
}

