/*
 * usp.h
 *
 *  Created on: Nov 8, 2015
 *      Author: sujan, siddhu
 */

#ifndef USP_H_
#define USP_H_

#include<sys/socket.h>

#include<sys/types.h>

#include<netinet/in.h>

#include<arpa/inet.h>

#include<errno.h>

#include<signal.h>

#include<stdio.h>

#include<stdlib.h>

#include<sys/wait.h>

#include <sys/select.h>

#include<string.h>

#include<fcntl.h>

#include <netdb.h>

#include <pthread.h>

#include<unistd.h>

#include <signal.h>

#include "constants.h"

#include <error.h>

#include <sys/un.h>

#include <linux/if_packet.h>

#include <linux/if_ether.h>

#include <linux/if_arp.h>

#include <stdbool.h>

#include "hw_addrs.h"

#define SA struct sockaddr

#define	min(a,b)	((a) < (b) ? (a) : (b))
#define	max(a,b)	((a) > (b) ? (a) : (b))

struct vm_info{
	char *vm_name;
	char *vm_ipadress;
	struct vm_info *next;
};

struct port_entry{
	int port_num;
	char path_name[108];
	int time_stamp;
};

struct msg_from_uds{
	char* msg_received;
	char* canonical_ipAddress;
	int dest_port_num;
	int src_port_num;
	int flag;
};

struct route_entry{

	char dest_canonical_ipAddress[20];
	char next_hop_mac_address[6];
	int outg_inf_index;
	int hop_count;
	long int time_stamp;
	struct route_entry *next;
};


struct odr_hdr{

	uint16_t pkt_type;
	char cn_src_ipaddr[20];
	uint16_t src_port_num;
	char cn_dsc_ipaddr[20];
	uint16_t dest_port_num;
	uint16_t hop_count;
	uint16_t force_route_dcvry;
	int broadcast_id;
	int rreq_id;
	uint16_t rreply_sent;
	uint16_t payload_len;
};

/**
 * 150 bytes as of now and may increase :P
 */
struct odr_frame{

	struct odr_hdr hdr;
	char payload[80];
};


struct odr_frame_node
{
	struct odr_frame frame;
	struct odr_frame_node *next;
};

void odr_init(char inf_mac_addr_map[MAX_INTERFACES][ETH_ALEN], int timeout);

void msg_send(int sockfd, char* destIpAddress, int destPortNumber,
					char* message,int flag);

struct msg_from_uds * msg_receive(int sockfd);

void build_port_entries();

char *
get_vm_ipaddress(char *vm_name,struct vm_info *head);

void
vm_init();

struct vm_info *
populate_vminfo_structs();

char*
get_vm_name(char *vm_ipAddress,struct vm_info *head);

char*
Gethostbyname(char *my_name);

int add_port_entry(char* path_name);

char *
Sock_ntop_host(const struct sockaddr *sa, socklen_t salen);

int is_inefficient_rreq_exists(struct odr_frame *frame);

bool remove_rreq_frame(struct odr_frame *frame);

void store_rreq_frame(struct odr_frame *frame);

void process_received_rreply_frame(int pf_sockid,int received_inf_ind,struct odr_frame *received_frame,
		char *src_mac_addr, char* dst_mac_addr, bool is_belongs_to_me);

void send_frame_rreply(int pf_sockid, struct odr_frame *frame,
			int hop_count,bool is_belongs_to_me);

void send_frame_rreq(int pf_sockfd,int recv_inf_index,
		struct odr_frame *frame);

struct odr_hdr build_odr_hdr(char *src,char *dst,int hop_count,int pkt_type,
		int broadcast_id, int rreq_id, int frc_dsc,int rreply_sent);

struct odr_hdr build_odr_payload_hdr(int rreq_id, char *src, int pkt_type, struct msg_from_uds *msg);

struct odr_frame build_odr_frame(char *src,char *dst,int hop_count,int pkt_type, int broadcast_id,
			int rreq_id, int frc_dsc,int rreply_sent,char *pay_load,struct msg_from_uds *msg);

struct odr_frame build_payload_frame(int rreq_id,char *my_ip_address, struct msg_from_uds *msg);

void build_eth_frame(void *buffer,char *dest_mac,
			char *src_mac,int inf_index,
			struct sockaddr_ll *addr_ll,struct odr_frame *frame, int eth_pkt_type);

void process_received_rreq_frame(int pf_sockid,int received_inf_ind,
			struct odr_frame *received_frame,char *src_mac_addr,char *dst_mac_addr);

bool update_routing_table(char *dest_ipaddress, char *next_hp_mac_addr, int hop_count, int outg_inf_index, int force_dsc);

void add_entry_in_rtable(char *dest_ipaddress, char *next_hp_mac_addr, int hop_count, int outg_inf_index);

struct route_entry* get_rentry_in_rtable(char *dest_ipAddress, int force_dsc);

void build_rreply_odr_frame(struct odr_frame *rrep_frame,int hop_count);

void convertToNetworkOrder(struct odr_hdr *hdr);

void printRoutingTable(struct route_entry* head);

char* Gethostname();

void fill_inf_mac_addr_map(struct hwa_info	*hw_head, char inf_mac_addr_map[MAX_INTERFACES][ETH_ALEN]);

void Sendto(int pf_sockfd, char* buffer, struct sockaddr_ll addr_ll,char *sendType);

void printHWADDR(char* src_mac_addr);

bool remove_frame(struct odr_frame *frame);

struct odr_frame * get_next_send_packet(struct odr_frame *frame);

void send_frame_payload(int pf_sockfd,struct odr_frame *frame,
			char *nxt_hop_addr,int outg_inf_index);

void build_port_entries();

char* get_inf_mac_addr(int inf_index);

void forward_frame_payload(int pf_sockfd,struct odr_frame *frame);

char* get_path_name(int dest_port_num);

void msg_send_to_uds(int sockfd, char* destIpAddress, int destPortNumber, int src_port_num,
		char* message,int flag);

char* build_msg_odr(int sockfd, char* destIpAddress, int destPortNumber,
		char* message,int flag);

long int get_present_time();

void set_route_entry_timeout(long int timeout);

int get_route_entry_timeout();

void send_frame_for_rrply(int pf_sockfd, struct odr_frame *received_frame, char *src_mac_addr, int received_inf_index);

void send_rrply_to_next_hop(int pf_sockfd, struct odr_frame *frame, char* dest_mac_addr, int outg_inf_index);

int get_new_broadcast_id();

int get_new_rreq_id();

#endif /* USP_H_ */
