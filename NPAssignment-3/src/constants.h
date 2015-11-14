/*
 * constants.h
 *
 *  Created on: Nov 8, 2015
 *      Author: sujan
 */

#ifndef CONSTANTS_H_
#define CONSTANTS_H_

#define SERVER_WELL_KNOWN_PATH_NAME "/tmp/sujan_sthota.dg"

#define ODR_PATH_NAME "/tmp/sujan_sthota_odr.dg"

#define MAXLINE 512

#define NUM_OF_VMS 10

#define ETH_FRAME_LEN 1518

#define DEBUG 0

#define ODR_GRP_TYPE 1800

#define ETH_HDR_LEN 14

#define R_REQ 0

#define R_REPLY 1

#define PAY_LOAD 2

#define ZERO_HOP_COUNT 0

#define R_REPLY_NOT_SENT 0

#define R_REPLY_SENT 1

#define EHTR_FRAME_SIZE sizeof(struct odr_frame) + ETH_HDR_LEN

#define INEFFICIENT_FRAME_EXISTS 1

#define EFFICIENT_FRAME_EXISTS 0

#define FRAME_NOT_EXISTS 2


//#define VM1 "130.245.156.21"
//
//#define VM2 "130.245.156.22"
//
//#define VM3 "130.245.156.23"
//
//#define VM4 "130.245.156.24"
//
//#define VM5 "130.245.156.25"
//
//#define VM6 "130.245.156.26"
//
//#define VM7 "130.245.156.27"
//
//#define VM8 "130.245.156.28"
//
//#define VM9 "130.245.156.29"
//
//#define VM10 "130.245.156.20"


#endif /* CONSTANTS_H_ */
