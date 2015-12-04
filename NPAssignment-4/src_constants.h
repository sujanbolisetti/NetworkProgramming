/*
 * src_constants.h
 *
 *  Created on: Nov 29, 2015
 *      Author: sujan
 */

#ifndef SRC_CONSTANTS_H_
#define SRC_CONSTANTS_H_

#define GRP_PROTOCOL_VALUE 182

#define DEBUG 1

#define MULTICAST_ADDRESS "224.0.0.224"

#define MULTICAST_PORT_NUMBER 32765

#define IDENTIFICATION_FIELD 44

#define INDEX_IN_TOUR_AT_SOURCE 1

#define SIZE_OF_TOUR_LIST 20

#define BUFFER_SIZE 1024

#define IP_HDR_LEN sizeof(struct ip)

#define MAX_TTL_VALUE 255

#define SOURCE_AND_MULTICAST_COUNT 2

#define ARP_WELL_KNOWN_PATH_NAME "/tmp/sujan_sthota.dg"

#define ARP_GRP_TYPE 1800

#define ETH_HDR_LEN 14

#define ETHR_FRAME_TYPE 0x0806

#define BACKLOG 1024

#define EHTR_FRAME_SIZE sizeof(struct arp_pkt) +14

#define ARP_REQ 1

#define ARP_REP 2

#define ETH_HA_TYPE 1

#define INVALID_RECV_IF_INDEX -1

#define INVALID_SOCK_DESC -1

#define ETH0_INDEX 2

#endif /* SRC_CONSTANTS_H_ */
