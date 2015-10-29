/*
 * constants.h
 *
 *  Created on: Oct 7, 2015
 *       Authors: sujan, sidhartha
 */

#ifndef CONSTANTS_H_
#define CONSTANTS_H_

#define LOOPBACK_ADDRESS "127.0.0.1"

#define PACKET_SIZE 496

// UDP packet types
#define ACK 1
#define PAYLOAD 2
#define FIN 4
#define FIN_ACK 8
#define USED 16
#define WINDOW_PROBE 32
#define PORT_NUMBER 64
#define SERVER_TIMEOUT 128

#define SLIDING_WINDOW 40 // In segments.

#define MAX_DUPLICATE_ACK_COUNT 3 // In segments.

// Congestion Control states.
#define SLOW_START 1
#define CONGESTION_AVOIDANCE 2
#define FAST_RECOVERY 4

// Events.
#define TIME_OUT 1
#define DUPLICATE_3_ACK 2
#define DUPLICATE_ACK 4
#define NEW_ACK 8

#define PERSISTENT_TIMER_MAX_TIMEOUT 60 // In seconds.

#define PERSISTENT_TIMER_MIN_TIMEOUT 1 // In seconds.

#define DEBUG 0

#endif /* CONSTANTS_H_ */
