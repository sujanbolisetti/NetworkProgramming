GROUP MEMBERS:
-------------
	Sujan Bolisetti (109797364) (NetId: sbolisetti)
	Sidhartha Thota (109888134) (NetId: sthota)
-----------------------------------------------------------------------------------------------------

COMPILING THE PROJECT:
----------------------

Pre-requisites:
1. Make
2. GCC compiler

Compiling:
1. Download the tar file from the blackboard and place it in a new folder
2. Untar the tar file using the command "xxx"
3. Compile the project using "make all" command
4. Now the folder has all the necessary executables to run the project
5. Please place "server.in" and "client.in" files in this folder to successful execution of the program
6. Please also place the text file in this folder that is to be transmitted to client

RUNNING THE PROJECT:
--------------------
Before running the project, it needs to be compiled (Instructions to compile the project)
1. Open the folder in two terminals (One to run the server and another to run the client)
2. Running the server:
	a. Open one of the terminal
	b. Run the server using command "./server"
	c. On successful run, it shows port number it is running and the interface details
3. Running the client:
	a. Open the other terminal
	b. Run the client using command "./client"
	c. Now it starts the client with file transfer mechanism


-------------------------------------------------------------------------------------------------------------------------------
IMPLEMENTATION DETAILS
-------------------------------------------------------------------------------------------------------------------------------

OUR PACKET STRUCTURE:
---------------------
	(Inspired from TCP RFC)

Packet Size: 512 Bytes

Header Information: 16 Bytes.
Data-Payload: 496 Bytes.

Bits

0………						    16						   ….31 
+----------------------------------------------------+-----------------------------------------------+                                         
|                                               Sequence Number                                      |
+----------------------------------------------------------------------------------------------------+ 
|                                             Acknowledgement Number                                 |
+----------------------------------------------------------------------------------------------------+ 
|                                                Time Stamp                                          |
+----------------------------------------------------------------------------------------------------+
|                          Window Size	              | 		Type of the packet           |
+----------------------------------------------------------------------------------------------------+ 
|                                         Data (Payload) [496 Bytes]				     |	
+----------------------------------------------------------------------------------------------------+       

MODIFICATIONS OF STEVEN'S TO BOUND ONLY UNICAST ADDRESSES:
----------------------------------------------------------
	On calling get_ifi_info method with arguments “inet” and “1” (to get the aliases), we are getting the interface details. We are checking that the interface is UP and reading the address only from the ifi_addr structure (Which are essentially unicast address). Ignoring the address from ifi_brdaddr structure as they are broadcast address and we also haven’t bound the wild card address. (Actually Steven’s code already filters the broadcast address by using the flag IFF_BROADCAST flag)
Implementation of array of structures:
	We have used a linked list for holding the bonded socket information. Each node in the linked list is a structure containing:  sockfd, IP address bound to the socket, network mask for the IP address, subnet address. Kindly refer to usp.h for seeing the binded_sock_info structure.

IMPLEMENTATION OF TIME-OUT MECHANISM:
-------------------------------------
	Borrowed time out implementation from the steven’s code. Header File : unprtt.h and Source File was moved to rtt_utils.c.
Modified the Min and Max RTO values to 1000 and 3000 milliseconds and also changed the calculation from seconds to milliseconds. Also updated the MAX_RETRANSMISSIONS to 12.
Modified the rtt_info structure contents to have integer data type from float data type. This will change the calculation to integer arithmetic. Kindly refer to unprtt.h and rtt_utils.c for seeing the changes.
We are setting the alarm to RTO value before sending the data packet and recalculating the RTO after receiving the ack. The received ack contains the timestamp when the packet was sent, so based on the difference between the current time stamp and ack time stamp we will calculate the new RTO. This elegant method was mentioned in Section 22.5 in the text book and exactly from RFC 1323.


SERVER SLIDING WINDOW IMPLEMENTATION:
--------------------------------------

	We are maintaining the sender sliding window in the form of circular linked list, to which we maintain two pointers sentNode and ackNode. The sentNode -> dictates, what is the next packet to send to the client (in case of window size > 0) and ackNode dictates -> what is the next packet for which we are expecting the acknowledgement. So basically we are waiting for the last un-acked packet. In case of time out we will retransfer the last unacked packet to the client. We will refill the sliding window with the data once the open space is available (I mean we got ack’s for few packets in the sliding window then we can remove them and store new packets.)

CONGESTION CONTROL:
-------------------

	We have implemented TCP Reno’s congestion control mechanism. We had two variables for tracking the congestion. 1. cwnd (Congestion Window) 2. ssthresh (Slow Start threshold). Here cwnd value indicates segments count instead of bytes as in normal TCP.

Initial values of cwnd = 1 and ssthresh = receiver_window_size/2

Initially the sender (server) will be in slow start phase where his cwnd will be incremented by one after receiving an ack (This is an exponential increase). Once the sender crosses the ssthresh the cwnd will increase linearly (We are increasing cwnd by 1 after every three acks received) and this phase is congestion avoidance phase.  We also have implemented Fast Retransmit/Fast Recovery phase in which the sender will recognize the failure of the packet by receiving three duplicate acks. For further details kindly refer to file src/server.c and method_name is congestion_control.

The sender will always send the min(cwnd, received_window_size) count of packets to the client. The sender will update the receiver window size from the acknowledgement received from the client.  

PERSITENT MODE:
---------------

	The sender will enter into persistent mode when the receiver window locks(the receiver window is full). The client will send an ack when the receiver window increases, but there might me a chance that this ack is lost and acks are not retransimtted as they are not backed by a timer. So the sender and receiver can go into a deadlock. To handle this situation, the sender(Server) will send packets of type WINDOW_PROBE, starting from 1 sec and does a exponential back(Max 4 secs) and never gives up. So, as in the previous even if the ack is lost from the client, the server can get window size update surely at some later time and continue the transfer again. We have used packet type WINDOW PROBE to identify window probe packets. 

CLEAN CLOSE:
------------

----------------------------------------------------------------------------------------
CLIENT SIDE
----------------------------------------------------------------------------------------

RECEIVER SLIDING WINDOW:
------------------------
We have implemented receiver window using a buffer in the form of Circular linked list which has all the packets in sequence. This Circular linked list has size equal to the value provided in the client.in file. 

We have two (2) threads running in the client. These two threads share a common Circular linked list.
1) Receiver thread : This thread receives the packets from the server and puts the packet in the shared circular linked list.
2) Printer thread : This thread periodically prints the packets in shared circular linked list. The periodicity is based on μ value provided in the client.in file.

Client accepts all the packets based on random function and probability provided. Client also drops sending acks based on random function and probability provided

USAGE OF TEMPORARY BUFFER:
--------------------------
We have also used temporary buffer (not shared) to store the packets when we get packets other than expected packet. This is NOT an ADDITIONAL BUFFER. The size of receiver window never exceeds its capacity due to this buffer. We also use the packets in this buffer while calculating the Receiver Window Size (see below). 
  
For example: Server sent the sequence of packets in the order 31, 32, 33, 34, 35, 36 ...

1. Receiver accepts the 31 and placed in shared circular list and sends ack 32.
2. Receiver accepts the 32 and placed in shared circular list and sends ack 33.
3. Receiver dropped the 33 due to intentional dropping (using random function based on seed and probability provided in client.in file) and doesn't send any ack as it dropped the packet.
4. Receiver accepts the 34 and but places in temporary buffer and sends ack 33.
5. Receiver accepts the 35 and but places in temporary buffer and sends ack 33.
6. Receiver accepts the 36 and but places in temporary buffer and sends ack 33.

Now when server sent the 33 packet. It places 33 packet in the shared list. Now searches for 34 packet if it is present in temporary buffer if present then places in shared list. similar thing happens to 35 and 36 packets also. As temporary buffer don't have 37 packet it sends ack with 37.

Server receives 37 ack and considers that 34, 35 and 36 packets are reached to client and sends from 37 packet. Ack 37 acts as Cumulative ack here. This way number of retransmissions of packets.

CALCULATION OF RECEIVER WINDOW SIZE:
------------------------------------
Receiver Window size = Actual receiver window size (provided in client.in)  - Number of packets in the main data buffer (shared) - Number of packets in the temporary data buffer (not shared)

For example:
Receiver window size: 40
Data packets yet to be read by printer thread: 15
Data packets in the temporary buffer: 4

Then receiver window size would be 20 (40 - 15 - 5).

READING/ WRITING DATA FROM/ TO SHARED CIRCULAR LINKED LIST:
----------------------------------------------
We can see Receiver and Printer thread as Producer and Consumer because of shared Circular linked list. Reciever tries to put the data buffer in shared list (if empty slot present) and Printer tries to get and prints the data buffer in shared list (if any slot is filled). Receiver puts the data into list using rear pointer until the list is full. Printer gets the data using front pointer and prints to terminal until list is empty. 

Either one of them at a time can only access the the list. This condition is fulfilled using mutex and conditional variables. The thread which has the lock on mutex can access the circular linked list. When the Receiver thread grabbed the lock, puts the data buffer into list and then signal (using conditional wait) to the Printer after completion of its task. Similarly when the Printer thread grabbed the lock, pop the data buffer and then signal to the reciever after completion of its task.
