GROUP MEMBERS:
	Sujan Bolisetti (109797364) (NetId: sbolisetti)
	Sidhartha Thota (109888134) (NetId: sthota)

Our Packet Structure:
	(Inspired TCP RFC)

Packet Size: 512 Bytes

Header Information: 16 Bytes.
Data-Payload: 496 Bytes.

Bits

0………						    16						   ….31 
+----------------------------------------------------+-----------------------------------------------+                                         
						Sequence Number					     |
+----------------------------------------------------------------------------------------------------+ 
                                              Acknowledgement Number                                 |
+----------------------------------------------------------------------------------------------------+ 
                                                 Time Stamp                                          |
+----------------------------------------------------------------------------------------------------+
                           Window Size	              | 		Type of the packet           |
+----------------------------------------------------------------------------------------------------+ 
                                          Data (Payload) [496 Bytes]				     |	
+----------------------------------------------------------------------------------------------------+       



MODIFICATIONS OF STEVEN'S TO BOUND ONLY UNICAST ADDRESSES:
	On calling get_ifi_info method with arguments “inet” and “1” (to get the aliases), we are getting the interface details. We are checking that the interface is UP and reading the address only from the ifi_addr structure (Which are essentially unicast address). Ignoring the address from ifi_brdaddr structure as they are broadcast address and we also haven’t bound the wild card address. (Actually Steven’s code already filters the broadcast address by using the flag IFF_BROADCAST flag)
Implementation of array of structures:
	We have used a linked list for holding the bonded socket information. Each node in the linked list is a structure containing:  sockfd, IP address bound to the socket, network mask for the IP address, subnet address. Kindly refer to usp.h for seeing the binded_sock_info structure.

IMPLEMENTATION OF TIME-OUT MECHANISM:
	Borrowed time out implementation from the steven’s code. Header File : unprtt.h and Source File was moved to rtt_utils.c.
Modified the Min and Max RTO values to 1000 and 3000 milliseconds and also changed the calculation from seconds to milliseconds. Also updated the MAX_RETRANSMISSIONS to 12.
Modified the rtt_info structure contents to have integer data type from float data type. This will change the calculation to integer arithmetic. Kindly refer to unprtt.h and rtt_utils.c for seeing the changes.
We are setting the alarm to RTO value before sending the data packet and recalculating the RTO after receiving the ack. The received ack contains the timestamp when the packet was sent, so based on the difference between the current time stamp and ack time stamp we will calculate the new RTO. This elegant method was mentioned in Section 22.5 in the text book and exactly from RFC 1323.


SENDING SLIDING WINDOW IMPLEMENTATION:

	We are maintaining the sender sliding window in the form of circular linked list, to which we maintain two pointers sentNode and ackNode. The sentNode -> dictates, what is the next packet to send to the client (in case of window size > 0) and ackNode dictates -> what is the next packet for which we are expecting the acknowledgement. So basically we are waiting for the last un-acked packet. In case of time out we will retransfer the last unacked packet to the client. We will refill the sliding window with the data once the open space is available (I mean we got ack’s for few packets in the sliding window then we can remove them and store new packets.) 

CONGESTION CONTROL:

	We have implemented TCP Reno’s congestion control mechanism. We had two variables for tracking the congestion. 1. cwnd (Congestion Window) 2. ssthresh (Slow Start threshold). Here cwnd value indicates segments count instead of bytes as in normal TCP.

Initial values of cwnd = 1 and ssthresh = receiver_window_size/2

Initially the sender (server) will be in slow start phase where his cwnd will be incremented by one after receiving an ack (This is an exponential increase). Once the sender crosses the ssthresh the cwnd will increase linearly (We are increasing cwnd by 1 after every three acks received) and this phase is congestion avoidance phase.  We also have implemented Fast Retransmit/Fast Recovery phase in which the sender will recognize the failure of the packet by receiving three duplicate acks. For further details kindly refer to file src/server.c and method_name is congestion_control.

The sender will always send the min(cwnd, received_window_size) count of packets to the client. The sender will update the receiver window size from the acknowledgement received from the client.  

Persitent Mode:
	The sender will enter into persistent mode when the receiver window locks(the receiver window is full)




