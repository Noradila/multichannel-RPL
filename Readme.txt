setCh is for other nodes
/examples/adila/simple-udp-rpl

xSetCh is for the LPBR
/examples/adila/native-border-router

********************2 HOPS COLOURING********************

2 hops colouring is done at LPBR
1. LPBR checks if the node is LPBR neighbours, else, go to 4.
2. If it is, LPBR checks if the new channel value is the same as LPBR channel (1 hop - LPBR).
3. If it is not, LPBR checks with all LPBR neighbours (excluding the node) (2 hops - LPBR + all LPBR neighbours)

1/4. LPBR checks if the node is other tree neighbour neighbour.
5. If tree neighbour neighbour is the same as the node address, LPBR checks the tree neighbour channel with the new channel (1 hop).

6. If it is not, LPBR checks if the tree neighbour is LPBR neighbour. Else, go to 8.
7. If it is, the new channel is checked with LPBR channel (2 hops - other node + LPBR).

6/8. If tree neighbour is not LPBR neighbour, check if tree neighbour address is in other tree neighbour neighbour.
9. Check if the new tree neighbour channel is the same as new channel.
10. If it is not, the new channel can be used (2 hops - other node + other node).

********************CHANNEL CHANGES*********************

1. LPBR sends CH_CHANGE to entries in its Routing Table. Each node is given 60 seconds before it times out and send CH_CHANGE to the next node.

2. Node receives CH_CHANGE. It calls keepProbeResult(ip, channel, getAck) where it keeps the list of neighbours, channel, probe result and ACK for telling the neighbours about the final change.
Only ACK will be used by other neighbours that are not tree neighbours.

It calls test1 process (to be renamed)
Uses a for loop where: 
x = 0 is NBR_CH_CHANGE delay 1 second
x = 1 is STARTPROBE delay 1 second because it needs to wait for NBRPROBE to finish

for loop is used because the codes for those 2 messages are exactly the same but only the delay is difference.

msg.type = NBR_CH_CHANGE
msg.value = newChannel

3. Node Tree Neighbour receives NBR_CH_CHANGE in turn; 1 second each

call function: updateRoutingTable(sender_addr, newChannel);
The function updates the Neighbour Table, nbr->nbrCh = newChannel so that when it needs to send something, it will use the newChannel.

NEIGHBOUR TABLE:
-------------------------------------------------
NBR		|	Nexthop channel (nbrCh)	|
-------------------------------------------------
fe80:: 		|	msg->value		|
fe80:: 		|	msg->value		|
-------------------------------------------------

When NBR_CH_CHANGE has finished, node updates it's own address structure (in uip-ds6.c):

OWN ADDRESS STRUCTURE:
-------------------------------------------------------------------------
IP (own address)|	currentCh	|	prevCh			|
-------------------------------------------------------------------------
fe80:: [0]	|	-		|	-			|
aaaa:: [1]	|	msg->value	|	cc2420_get_channel()	|
-------------------------------------------------------------------------

Then,
X = 1
4. Node tell each Tree Neighbour to STARTPROBE delay 1 second

msg.type = STARTPROBE;
msg.value = newChannel;

********STARTPROBE 1 second delay STARTS

5. Tree Neighbour receives STARTPROBE. It calls test1 process

msg.type = NBRPROBE;
msg.addrPtr = sender_addr;
msg.value = newChannel;
msg.paddingBuf[30] = " ";

Tree Neighbour waits for 0.125 second so that Node has changed to the new channel (listening channel).
Tree Neighbour sends NBRPROBE (no delay in between).
Add delay 0.15 second after the last NBRPROBE packet if the last packet is still being sent (supposedly, it should be [[[(4 * 0.125)? 1?]]] second because retransmission is done for the whole duty cycle if it is not received)

6. Node receives NBRPROBE

call function: keepProbeResult(sender_addr, newChannel, 0 (notACK packet))
probeResult_table is updated.

PROBERESULT TABLE:
-------------------------------------------------------------------------
NBR Address	|	New Channel	|	Probe Received	|Ack	|
-------------------------------------------------------------------------
fe80::		|	msg->value	|	x++		|1	|
...		|	...		|	...		|0	|
-------------------------------------------------------------------------

********STARTPROBE 1 second delay EXPIRES

7. Node sends PROBERESULT from probeResult_table to LPBR.
call function: readProbeResult();

msg.type = PROBERESULT;
msg.address = LPBR;
msg.value = chNum;
msg.value2 = rxValue;

8. LPBR receives PROBERESULT from sender_addr via &msg.address. LPBR updates it's table of channels and quality of the channels. It doesn't care if the channel is the new choosen channel or not yet.

call function: keepLpbrList(sender_addr, &msg->address, chNum, rxValue);

LPBR LIST TABLE:
-------------------------------------------------------------------------
Route through	|Nexthop|Channel|Probe Received	|Battery|Sent	|Recv	|
-------------------------------------------------------------------------
aaaa::		|fe80::	|chNum	|rxValue	|TBA	|TBA	|TBA	|
...		|...	|...	|		|	|	|	|
-------------------------------------------------------------------------

**9. After sending PROBERESULT, still in readProbeResult().
The function calculates the number of probe messages received for all nodes.
If:
(received/number of nodes) >= (received/number of nodes)/2
use the new channel
if not, go back to the previous channel;

msg.type = CONFIRM_CH;
msg.value = uip_ds6_if.addr_list[1].prevCh (or currentCh);
msg.value2 = 0; //indicates x = 0; y = 0

10. Calls test1 process. It goes through Neighbour table loop and send to all neighbours in range. Delay 0.5 second

11. Neighbour receives CONFIRM_CH. Call test1 process

call function: updateRoutingTable(sender_addr, channel);
The function was called after receiving NBR_CH_CHANGE.
It is called again to check/update if the previous channel is choosen instead of the new channel.
wait for 0.15 second before sending so that Node is on the listening channel

msg.type = GET_ACK;
msg.addrPtr = sender_addr;
msg.value = channel;

12. Node receives GET_ACK from Neighbour.
It updates the keepProbeResult() Ack section.

call function: keepProbeResult(sender_addr, 0 (chN), 1 (getAck));
When chN = 0, getAck is updated to 1

////updates probe recv;	chN == pr->chNum
updates pkt rcv/sent; 	chN = 1
updates getAck; 	chN ! = 1; chN = 0, getAck = 1 (checkAck = getAck)
			chN ! = 1; chN != 0, checkAck = 0 

[TO REDO][[[[13. back in test1 process CONFIRM_CH
call function: checkAckProbeResultTable(newCh);

Checks if all ACK are received. If not, it will go through CONFIRM_CH (from test1 process) again (**9).
Else, send CONFIRM_CH to LPBR.

msg.type = CONFIRM_CH;
msg.value = newChannel;
msg.addPtr = &LPBR;
msg.paddingBuf[30] = " ";]]]]]]

14. LPBR receives CONFIRM_CH. It updates its Neighbour Table.

NEIGHBOUR TABLE:
-------------------------------------------------------------------------
Local IP	|NBR channel	|reach	|ns	|nsC	|isR	|state	|
-------------------------------------------------------------------------
fe80::		|msg->value	|	|	|	|	|	|
fe80::		|msg->value	|	|	|	|	|	|
-------------------------------------------------------------------------

15. Back to checkAckProbeResultTable()

call function: removeProbe();
Removes all entries in probeResult_table as it is no longer needed.

