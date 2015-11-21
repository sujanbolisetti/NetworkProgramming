/*
 * routing_utils.c
 *
 *  Created on: Nov 13, 2015
 *      Author: sujan, siddhu
 */

#include "../src/usp.h"

struct route_entry *rtable_head = NULL;
struct route_entry *rtable_tail = NULL;

long int get_present_time()
{
	return time(NULL);
}

void add_entry_in_rtable(char *dest_ipaddress, char *next_hp_mac_addr, int hop_count, int outg_inf_index,
		int broadcastid){

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
	r_node->broadcast_id = broadcastid;

	r_node->next = NULL;

	if(rtable_head ==  NULL){
		rtable_head = r_node;
		rtable_tail = rtable_head;
	}else{
		rtable_tail->next =  r_node;
		rtable_tail = r_node;
	}
}

struct route_entry* get_rentry_in_rtable(char *dest_ipAddress, int force_dsc, int pkt_type){

	printf("Entered get_routing_entry\n");

	// if route entry timeout is 0
	if(!get_route_entry_timeout() || (force_dsc && (pkt_type == R_REQ || pkt_type == PAY_LOAD)))
	{
		printf("Returning route for %s as null because of force discovery or route entry timeout\n", dest_ipAddress);
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
				remove_route_entry(temp_rnode);
				return NULL;
			}
		}

		temp_rnode = temp_rnode->next;
	}

	return NULL;
}

bool update_routing_table(char *dest_ipaddress, char *next_hp_mac_addr, int hop_count, int outg_inf_index, int force_dsc, int pkt_type,
			int broadcast_id){

	printf("Entered updating routing table\n");

	if(!get_route_entry_timeout()){
		printf("Time out is zero hence not adding/updating the entry in the routing table\n");
		return false;
	}

	/**
	 * Sending force discovery as false because we are updating the routing entry here else
	 * we will create duplicate entries for same destination.
	 */
	struct route_entry *rnode_entry = get_rentry_in_rtable(dest_ipaddress, false, pkt_type);

	if(rnode_entry != NULL)
	{
		printf("Updating routing table\n");

		if(broadcast_id >= rnode_entry -> broadcast_id  || force_dsc){

			if((broadcast_id > rnode_entry -> broadcast_id) ||
					(broadcast_id == rnode_entry -> broadcast_id && hop_count < rnode_entry->hop_count))
			{
				rnode_entry->hop_count = hop_count;
				rnode_entry->outg_inf_index = outg_inf_index;
				rnode_entry->time_stamp = get_present_time();
				rnode_entry->broadcast_id = broadcast_id;
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
		}else{
			printf("Received a rreq/reply with lower broadcastid / less efficient route hence not updating the routing table\n");
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

		add_entry_in_rtable(dest_ipaddress, next_hp_mac_addr, hop_count, outg_inf_index,broadcast_id);
		printRoutingTable(rtable_head);
		return true;
	}
}

void remove_route_entry(struct route_entry *rt){

	struct route_entry *temp = rtable_head;

		if(rtable_head == NULL)
			return;

		if(!strcmp(temp->dest_canonical_ipAddress,rt->dest_canonical_ipAddress))
		{
			rtable_head = rtable_head -> next;
			free(temp);
			return;
		}

		struct route_entry *prev = temp;
		temp = temp -> next;
		while(temp != NULL)
		{
			if(!strcmp(temp->dest_canonical_ipAddress,rt->dest_canonical_ipAddress))
			{
				if(rtable_tail == temp)
				{
					rtable_tail = prev;
				}
				prev -> next = temp -> next;
				free(temp);
				return;
			}
			prev = temp;
			temp = temp -> next;
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
		printf("Broadcast -id :%d\t",temp->broadcast_id);
		printf("Time stamp: %ld\t", temp->time_stamp);
		printf("\n");
		temp = temp -> next;
	}
	printf("----------------------------------------------------------------------\n");
}

