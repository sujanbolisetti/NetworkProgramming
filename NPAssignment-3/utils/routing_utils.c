/*
 * routing_utils.c
 *
 *  Created on: Nov 13, 2015
 *      Author: sujan
 */

#include "../src/usp.h"

struct route_entry *rtable_head = NULL;
struct route_entry *rtable_tail = NULL;

int get_present_time()
{
	return time(NULL);
}

void add_entry_in_rtable(char *dest_ipaddress, char *next_hp_mac_addr, int hop_count, int outg_inf_index){

	struct route_entry *r_node = (struct route_entry*)malloc(sizeof(struct route_entry));

	strcpy(r_node->dest_canonical_ipAddress,dest_ipaddress);

	if(DEBUG)
	{
		printf("Next Hop in Added_route ");
		printHWADDR(next_hp_mac_addr);
	}

	memcpy((void *)r_node->next_hop_mac_address,(void *)next_hp_mac_addr,ETH_ALEN);

	r_node->hop_count = hop_count;

	// TODO : have to populate the time_out
	r_node->outg_inf_index = outg_inf_index;
	r_node->time_stamp = get_present_time();

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

	printf("Entered get_routing_entry\n");

	// if route entry timeout is 0
	if(!get_route_entry_timeout())
	{
		return NULL;
	}

	struct route_entry *temp_rnode = rtable_head;
	int present_time = get_present_time();

	//TODO Have to check stale entries.
	while(temp_rnode != NULL){

		if(!strcmp(temp_rnode->dest_canonical_ipAddress,dest_ipAddress)){

			// Checking staleness of route entry
			if((present_time - temp_rnode -> time_stamp) < get_route_entry_timeout())
			{
				return temp_rnode;
			}
			else
			{
				printf("Found stale route entry for destination ip-addr %s\n", dest_ipAddress);
				return NULL;
			}
		}

		temp_rnode = temp_rnode->next;
	}

	return NULL;
}

bool update_routing_table(char *dest_ipaddress, char *next_hp_mac_addr, int hop_count, int outg_inf_index, int force_dsc){

	printf("Entered updating routing table\n");
	struct route_entry *rnode_entry = get_rentry_in_rtable(dest_ipaddress);

	if(rnode_entry != NULL)
	{
		printf("Updating routing table\n");
		if(hop_count < rnode_entry->hop_count || force_dsc)
		{
			rnode_entry->hop_count = hop_count;
			rnode_entry->outg_inf_index = outg_inf_index;
			rnode_entry->time_stamp = get_present_time();
			memcpy(rnode_entry->next_hop_mac_address, next_hp_mac_addr,ETH_ALEN);
			printRoutingTable(rtable_head);

			if(force_dsc)
				printf("Updated routing entry for destination ip-addr %s because of force route discovery\n", dest_ipaddress);
			else
				printf("Updated routing entry for destination ip-addr %s because of better route found\n", dest_ipaddress);

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
		printf("Added new routing entry for destination ip-addr %s\n", dest_ipaddress);
		if(DEBUG)
		{
			printf("Next Hop Before Add ");
			printHWADDR(next_hp_mac_addr);
		}

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

