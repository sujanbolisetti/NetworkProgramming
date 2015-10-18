/*
 * constants.h
 *
 *  Created on: Oct 7, 2015
 *      Author: sujan
 */

#ifndef CONSTANTS_H_
#define CONSTANTS_H_

#define LOOPBACK_ADDRESS "127.0.0.1"

#define MAXLINE 1024

// UDP packet types
#define ACK 1
#define PAYLOAD 2
#define FIN 4
#define FIN_ACK 8
#define USED 16


#define SLOWSTART 1
#define FASTRESTRANSMIT 2

#define SLIDING_WINDOW 40

#define MAX_DUPLICATE_ACK_COUNT 3

// States
#define SLOW_START 1
#define CONGESTION_AVOIDANCE 2
#define FAST_RECOVERY 4

// events
#define TIME_OUT 1
#define DUPLICATE_3_ACK 2
#define DUPLICATE_ACK 4
#define NEW_ACK 8







#endif /* CONSTANTS_H_ */
