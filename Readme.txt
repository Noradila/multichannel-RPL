setCh is for other nodes
/examples/adila/simple-udp-rpl

xSetCh is for the LPBR
/examples/adila/native-border-router

************************

1. LPBR sends CH_CHANGE to entries in its Routing Table. Each node is given 60 seconds before it times out and send CH_CHANGE to the next node.

2. Node receives CH_CHANGE. It calls test1 process (to be renamed)
Uses a for loop where: 
x = 0; y = 1 is NBR_CH_CHANGE delay 0.15 second
x = 1; y = 1 is STARTPROBE delay 1 second because it needs to wait for NBRPROBE to finish
x = 0; y = 0 is CONFIRM_CH delay 0.5 second

for loop is used because the codes for those 3 messages are exactly the same but only the delay is difference.

msg.type = NBR_CH_CHANGE
msg.value = newChannel
msg.value2 = 1 (indicating it's NBR_CH_CHANGE)

call function: loopFunction(msg, y, x, nbrCh);

X = 0; Y = 1
3. Node Tree Neighbour receives NBR_CH_CHANGE in turn; 0.15 second each where one duty cycle is 0.125 second.

call function: updateRoutingTable(sender_addr, newChannel);
The function updates the Routing Table, r->nbrCh = newChannel; or Default Router List, uip_ds6_defrt_setCh(newChannel); to the newChannel so that when it needs to send something, it will use the newChannel.

ROUTING TABLE:
-------------------------------------------------------------------------
Route through	|	Nexthop (via)	|	Nexthop channel (nbrCh)	|
-------------------------------------------------------------------------
aaaa:: 		|	fe80::		|	msg->value		|
aaaa:: 		|	fe80::		|	msg->value		|
-------------------------------------------------------------------------

DEFAULT ROUTER LIST:
-----------------------------------------
Defrt IP	|	Parent ch	|
-----------------------------------------
fe80:: 		|	msg->value	|
-----------------------------------------

When NBR_CH_CHANGE has finished,node updates it's own address structure (in uip-ds6.c):

OWN ADDRESS STRUCTURE:
-------------------------------------------------------------------------
IP (own address)|	currentCh	|	prevCh			|
-------------------------------------------------------------------------
fe80:: [0]	|	-		|	-			|
aaaa:: [1]	|	msg->value	|	cc2420_get_channel()	|
-------------------------------------------------------------------------

Then,
X = 1; Y = 1
4. Node tell each Tree Neighbour to STARTPROBE delay 1 second

call function: loopFunction(msg, y, x, nbrCh);

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

Calls test1 process

X = 0; Y = 0
call function: loopFunction(msg, y, x, currentCh or prevCh) delay 0.5 second

10. Tree Neighbour receives CONFIRM_CH. Call test1 process

wait for 0.15 second before sending so that Node is on the listening channel

msg.type = GET_ACK;
msg.addrPtr = sender_addr;
msg.value = channel;

call function: updateRoutingTable(sender_addr, channel);
The function was called after receiving NBR_CH_CHANGE.
It is called again to check/update if the previous channel is choosen instead of the new channel.

11. Node receives GET_ACK from Tree Neighbour.
It updates the keepProbeResult() Ack section.

call function: keepProbeResult(sender_addr, 0 (chN), 1 (getAck));
When chN = 0, getAck is updated to 1

updates probe recv;	chN == pr->chNum
updates pkt rcv/sent; 	chN = 1
updates getAck; 	chN ! = 1; chN = 0, getAck = 1 (checkAck = getAck)
			chN ! = 1; chN != 0, checkAck = 0 

12. back in test1 process

call function: checkAckProbeResultTable(newCh);

Checks if all ACK are received. If not, it will go through CONFIRM_CH (from test1 process) again (**9).
Else, send CONFIRM_CH to LPBR.

msg.type = CONFIRM_CH;
msg.value = newChannel;
msg.addPtr = &LPBR;
msg.paddingBuf[30] = " ";

13. LPBR receives CONFIRM_CH. It updates its Neighbour Table.

NEIGHBOUR TABLE:
-------------------------------------------------------------------------
Local IP	|NBR channel	|reach	|ns	|nsC	|isR	|state	|
-------------------------------------------------------------------------
fe80::		|msg->value	|	|	|	|	|	|
fe80::		|msg->value	|	|	|	|	|	|
-------------------------------------------------------------------------

14. Back to checkAckProbeResultTable()

call function: removeProbe();
Removes all entries in probeResult_table as it is no longer needed.

