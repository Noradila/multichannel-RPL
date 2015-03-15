/*
 * Copyright (c) 2011, Swedish Institute of Computer Science.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * This file is part of the Contiki operating system.
 *
 */

#include "contiki.h"
#include "lib/random.h"
#include "sys/ctimer.h"
#include "sys/etimer.h"
#include "net/uip.h"
#include "net/uip-ds6.h"
#include "net/uip-debug.h"

#include "sys/node-id.h"

#include "simple-udp.h"
#include "servreg-hack.h"

#include <stdio.h>
#include <string.h>

#include "lib/list.h"
#include "lib/memb.h"

#define UDP_PORT 1234
#define SERVICE_ID 190

#define UIP_REXMIT    0

//#define SEND_INTERVAL		(60 * CLOCK_SECOND)
//#define SEND_INTERVAL		(310 * CLOCK_SECOND)
//#define SEND_INTERVAL		(450 * CLOCK_SECOND)
#define SEND_INTERVAL		(600 * CLOCK_SECOND)
#define SEND_TIME		(random_rand() % (SEND_INTERVAL))
//#define SEND_TIME		(20 * CLOCK_SECOND)

struct probeResult {
  struct probeResult *next;
  uip_ipaddr_t pAddr;
  uint8_t chNum;
  uint8_t rxValue;

  uint8_t checkAck;
};

LIST(probeResult_table);
MEMB(probeResult_mem, struct probeResult, 5);

struct sentRecv {
  struct sentRecv *next;
  uip_ipaddr_t sendToAddr;
  uint8_t noSent;
  uint8_t noRecv;
};

LIST(sentRecv_table);
MEMB(sentRecv_mem, struct sentRecv, 2); //for now, only sent to LPBR

//uint8_t currentVal;
uint8_t changeTo;
static uip_ipaddr_t holdAddr;
uint8_t x;
uint8_t y;

//uint8_t noOfEntry = 0;

//uip_ipaddr_t nextHopAddr;

uint8_t keepType;
//extern uint8_t keepListNo = 0;
//extern uint8_t nbrNo = 0;

uint8_t sum;
uint8_t divide;

//uip_ipaddr_t toParent;
  uip_ipaddr_t sendTo1;

static struct simple_udp_connection unicast_connection;

//the application specific event value
static process_event_t event_data_ready;

enum {
	CH_CHANGE,
	NBR_CH_CHANGE,
	STARTPROBE,
	NBRPROBE,
	PROBERESULT,
	CONFIRM_CH,
	GET_ACK,
SENTRECV
};

struct unicast_message {
	uint8_t type;
	uint8_t value;
	//uint8_t holdV;
	uint8_t value2;

	uip_ipaddr_t address;
	uip_ipaddr_t *addrPtr; 

	char paddingBuf[30];
};

/*---------------------------------------------------------------------------*/
PROCESS(unicast_sender_process, "Unicast sender example process");
PROCESS(test1, "test");
AUTOSTART_PROCESSES(&unicast_sender_process, &test1);
/*---------------------------------------------------------------------------*/
static void updateRoutingTable(uip_ipaddr_t *addr, uint8_t msgValue) {
  //static uip_ds6_route_t *r;
  //uip_ipaddr_t nextHopIP;

  static uip_ds6_nbr_t *nbr;

  for(nbr = nbr_table_head(ds6_neighbors); nbr != NULL;
    nbr = nbr_table_next(ds6_neighbors,nbr)) {
    if(addr->u8[13] == nbr->ipaddr.u8[13]) {
      nbr->nbrCh = msgValue;
      //printf("UPDATE NBR TABLE ");
      //uip_debug_ipaddr_print(&nbr->ipaddr);
      //printf(" update nbr->nbrCh %d ", nbr->nbrCh);
      //printf("\n\n");
    }
  }
}
/*---------------------------------------------------------------------------*/
static void removeProbe() {
  struct probeResult *pr, *r;

  r = NULL;
  for(pr = list_head(probeResult_table); pr != NULL; pr = pr->next) {
    if(r == NULL) {
      pr = pr->next;
    }
    else {
      r->next = pr->next;
    }
    pr->next = NULL;
    return;
  }
  r = pr;

  /*for(pr = list_head(probeResult_table); pr != NULL; pr = pr->next) {
    printf("READ FROM REMOVEPROBE pr->pAddr ");
    uip_debug_ipaddr_print(&pr->pAddr);
    printf(" pr->chNum %d pr->rxValue %d", pr->chNum, pr->rxValue);
    printf("\n");
  }*/
}
/*---------------------------------------------------------------------------*/
static void checkAckProbeResultTable(uint8_t theChannel) {
  struct probeResult *pr;
  struct unicast_message msg2;
  uint8_t confirmToLPBR = 0;

  //uip_ipaddr_t sendTo1;
  uip_ip6addr(&sendTo1, 0xaaaa, 0, 0, 0, 0x212, 0x7401, 0x0001, 0x0101);

  //changeTo = theChannel;
  msg2.value = theChannel;

  for(pr = list_head(probeResult_table); pr != NULL; pr = pr->next) {
    //! if checkAck == 0, do retransmission from CONFIRM_CH
    if((pr->checkAck) == 0 && (pr->pAddr.u8[13] != 1)) {
      msg2.type = CONFIRM_CH;
      msg2.value = theChannel;
      msg2.addrPtr = &pr->pAddr;
      msg2.value2 = 1;
      //msg2.value2 = 0; //y==0, for() run only once in test1

//[TO DO]
      //process_post(&test1, event_data_ready, &msg2);
      break;
    }
    if(pr->pAddr.u8[13] == 1) {
      confirmToLPBR = 1; //no need to sent to LPBR again since it is in NT
    }
  }

  if(confirmToLPBR == 0) {
    msg2.type = CONFIRM_CH;
    msg2.value = theChannel;
    msg2.addrPtr = &sendTo1;
    msg2.paddingBuf[30] = " ";

//    printf("To LPBR Sending CONFIRM_CH to ");
//    uip_debug_ipaddr_print(msg2.addrPtr);
//    printf(" channel %d\n", msg2.value);

    simple_udp_sendto(&unicast_connection, &msg2, sizeof(msg2), msg2.addrPtr);
    removeProbe();
  }
}
/*---------------------------------------------------------------------------*/
static void readProbeResult() {
  struct probeResult *pr;
  struct unicast_message msg2;
  sum = 0;
  divide = 0;

  //uip_ipaddr_t sendTo1;
  uip_ip6addr(&sendTo1, 0xaaaa, 0, 0, 0, 0x212, 0x7401, 0x0001, 0x0101);

  /*for(pr = list_head(probeResult_table); pr != NULL; pr = pr->next) {
    printf("keeprobe c %d r %d ack %d ", pr->chNum, pr->rxValue, pr->checkAck);
    uip_debug_ipaddr_print(&pr->pAddr);
    printf("\n\n");
  }*/

  for(pr = list_head(probeResult_table); pr != NULL; pr = pr->next) {
   if(pr->chNum != 0) {
    printf("Sending PROBERESULT to ");
    uip_debug_ipaddr_print(&sendTo1);
    printf("\n");

    msg2.type = PROBERESULT;
    msg2.address = pr->pAddr;
    msg2.value = pr->chNum;
    msg2.value2 = pr->rxValue; 

    //! Sending PROBERESULT to LPBR without deciding the change yet
    simple_udp_sendto(&unicast_connection, &msg2, sizeof(msg2) + 1, &sendTo1);

    sum = sum + pr->rxValue;
    divide++;
   }
   //nbrNo++;
  }

  //keepListNo = divide;
  msg2.type = CONFIRM_CH;
  //msg2.value2 = 0;

  if((sum/divide) >= ((sum/divide)/2)) {
    msg2.value = uip_ds6_if.addr_list[1].currentCh;
  }
  else {
    msg2.value = uip_ds6_if.addr_list[1].prevCh;
  }
  process_post(&test1, event_data_ready, &msg2);

//!!!!!!!!!!CONFIRM_CH IS TO BE SENT TO ALL NBR!!!!!!
}
/*---------------------------------------------------------------------------*/
static void keepProbeResult(const uip_ipaddr_t *prAddr, uint8_t chN, uint8_t getAck) {
  struct probeResult *pr;

  /*for(pr = list_head(probeResult_table); pr != NULL; pr = pr->next) {
    printf("%d keeprobe c %d r %d ack %d ", chN, pr->chNum, pr->rxValue, pr->checkAck);
    uip_debug_ipaddr_print(&pr->pAddr);
    printf("\n\n");
  }*/

  for(pr = list_head(probeResult_table); pr != NULL; pr = pr->next) {
    if(uip_ipaddr_cmp(prAddr, &pr->pAddr)) {
      if(chN != 1 && getAck != 2) {
	if(pr->chNum == 0) {
	  if(getAck == 1) {
	    pr->checkAck = getAck;
	    return;
	  }
	  else {
	    pr->chNum = chN;
	    pr->rxValue = 1;
	    return;
	  }
	}
        if(chN == pr->chNum) {
	  pr->rxValue = (pr->rxValue) + 1;
	  return;
        }

        if(chN == 0) {
	  pr->checkAck = getAck;
	  return;
        }
        else {
	  pr->checkAck = 0;
        }
      }
      //?keep track of sent/received if chN == 1 (chN == 0 indicates GET_ACK received)
      /*else { 
        pr->rxValue = (pr->rxValue) + 1;
      }*/
    }
  }

  pr = memb_alloc(&probeResult_mem);
  if(pr != NULL) {
    pr->rxValue = 0;
    pr->chNum = chN;
    uip_ipaddr_copy(&pr->pAddr, prAddr);
    list_add(probeResult_table, pr);
  }
}
/*---------------------------------------------------------------------------*/
static void keepSentRecv(const uip_ipaddr_t *sendToAddr, uint8_t pktSent, uint8_t pktRecv) {
  struct sentRecv *sr; 

  /*for(sr = list_head(sentRecv_table); sr != NULL; sr = sr->next) {
    printf("keepSentRecv s: %d r: %d ", sr->noSent, sr->noRecv);
    uip_debug_ipaddr_print(&sr->sendToAddr);
    printf("\n");
  }*/

  for(sr = list_head(sentRecv_table); sr != NULL; sr = sr->next) {
    if(uip_ipaddr_cmp(sendToAddr, &sr->sendToAddr)) {
      if(pktSent == 1) {
	sr->noSent = sr->noSent + 1;
	return;
      }
      if(pktRecv == 1) {
	sr->noRecv = sr->noRecv + 1;
	return;
      }
      if(sr->noSent == 10) {
	//tell LPBR to check the channel condition
      }
    }
  }

  sr = memb_alloc(&sentRecv_mem);
  if(sr != NULL) {
    sr->noSent = pktSent;
    sr->noRecv = pktRecv;
    uip_ipaddr_copy(&sr->sendToAddr, sendToAddr);
    list_add(sentRecv_table, sr);
  }
}
/*---------------------------------------------------------------------------*/
/*static void keepSentRecv(const uip_ipaddr_t *prAddr, uint8_t pktSent, uint8_t pktRecv) {
  struct probeResult *pr;
  struct unicast_message msg2;

  //uip_ipaddr_t sendTo1G;
  uip_ip6addr(&sendTo1, 0xaaaa, 0, 0, 0, 0x212, 0x7401, 0x0001, 0x0101);

//rxValue is received; chNum is sent
  for(pr = list_head(probeResult_table); pr != NULL; pr = pr->next) {
    if(uip_ipaddr_cmp(prAddr, &pr->pAddr)) {
      if(pktSent == 1) {
	pr->chNum = pr->chNum + 1;
	return;
      }
      if(pktRecv == 1) {
	pr->rxValue = pr->rxValue + 1;
	return;
      }

      //has sent 10 packets
      if(pr->chNum == 10 || pr->rxValue == 10) {
	
        msg2.type = SENTRECV;
	if(pr->chNum == 10) {
          msg2.value = pr->chNum;
	}
	else {
	  msg2.value = pr->rxValue;
	}
        msg2.addrPtr = &sendTo1;
        msg2.paddingBuf[30] = " ";

        printf("Sending SENTRECV to ");
        uip_debug_ipaddr_print(msg2.addrPtr);
        printf(" sent %d\n", msg2.value);

        simple_udp_sendto(&unicast_connection, &msg2, sizeof(msg2), msg2.addrPtr);

	if(pr->chNum == 10) {
          pr->chNum = 0;
	}
	else {
	  pr->rxValue = 0;
	}
	return;
      }
    }
  }

  pr = memb_alloc(&probeResult_mem);
  if(pr != NULL) {
    pr->rxValue = pktRecv;
    pr->chNum = pktSent;
    uip_ipaddr_copy(&pr->pAddr, prAddr);
    list_add(probeResult_table, pr);
  }
}
/*---------------------------------------------------------------------------*/
static void
receiver(struct simple_udp_connection *c,
         const uip_ipaddr_t *sender_addr,
         uint16_t sender_port,
         const uip_ipaddr_t *receiver_addr,
         uint16_t receiver_port,
         const uint8_t *data,
         uint16_t datalen)
{
  struct unicast_message *msg;
  struct unicast_message msg2;
  msg = data;

  struct probeResult *pr;

  static uip_ds6_route_t *r;

  static uip_ds6_nbr_t *nbr;

  //uip_ipaddr_t sendTo1G;
  //uip_ip6addr(&sendTo1, 0xaaaa, 0, 0, 0, 0x212, 0x7401, 0x0001, 0x0101);

  if(msg->type == CH_CHANGE) {
    printf("%d: %d received CH_CHANGE from ", cc2420_get_channel(), msg->value);
    uip_debug_ipaddr_print(sender_addr);
    printf("\n");  

    //put all nbr in probeResult table (to keep track of all ACKs
    for(nbr = nbr_table_head(ds6_neighbors); nbr != NULL;
      nbr = nbr_table_next(ds6_neighbors,nbr)) {
      keepProbeResult(&nbr->ipaddr, 0, 2);
    }

    msg2.type = NBR_CH_CHANGE;
    msg2.value = msg->value;

    process_post_synch(&test1, event_data_ready, &msg2);
  }//end if(msg->type == CH_CHANGE)

  else if(msg->type == NBR_CH_CHANGE) {
    printf("%d: %d received NBR_CH_CHANGE from ", cc2420_get_channel(), msg->value);
    uip_debug_ipaddr_print(sender_addr);
    printf("\n");

    msg2.value = msg->value;
    msg2.addrPtr = sender_addr;

    //? updates the routing table r->nbrCh = msg->value;
    updateRoutingTable(msg2.addrPtr, msg2.value);
  }//end if(msg->type == NBR_CH_CHANGE)

  else if(msg->type == STARTPROBE) {
    //printf("RECEIVED STARTPROBE\n\n");

    msg2.type = NBRPROBE;
    msg2.addrPtr = sender_addr;
    msg2.value = msg->value;

    process_post_synch(&test1, event_data_ready, &msg2);
  }

  else if(msg->type == NBRPROBE) {
//printf("Q3 %d\n\n", list_length(n->queued_packet_list));
    //printf("%d %d / %d: %d received %d NBRPROBE from ", NETSTACK_RADIO.receiving_packet(), NETSTACK_RADIO.pending_packet(), cc2420_get_channel(), msg->value, msg->value2);
    //uip_debug_ipaddr_print(sender_addr);
    //printf("\n");

    keepProbeResult(sender_addr, msg->value, 0);
  }

  else if(msg->type == CONFIRM_CH) {
    //printf("%d: Received CONFIRM_CH from ", cc2420_get_channel());
    //uip_debug_ipaddr_print(sender_addr);
    //printf("\n");
 
    msg2.type = GET_ACK;
    msg2.addrPtr = sender_addr;
    msg2.value = msg->value;

    process_post_synch(&test1, event_data_ready, &msg2);
  }

  else if(msg->type == GET_ACK) {
    msg2.value = msg->value;
//    printf("Received GET ACK from ");
//    uip_debug_ipaddr_print(sender_addr);
//    printf("\n");
    //noOfEntry++;
    //printf("%d: GET ACK BACK %d/%d nbr %d\n", cc2420_get_channel(), noOfEntry, keepListNo, nbrNo);

    keepProbeResult(sender_addr, 0, 1);

    //if(noOfEntry == keepListNo) {
/*    if(noOfEntry == nbrNo) {
      msg2.type = CONFIRM_CH;
      msg2.value = changeTo;
      msg2.addrPtr = &sendTo1G;
      msg2.paddingBuf[30] = " ";

      //printf("Sending CONFIRM_CH to ");
      //uip_debug_ipaddr_print(msg2.addrPtr);
      //printf(" channel %d\n", msg2.value);

      simple_udp_sendto(&unicast_connection, &msg2, sizeof(msg2), msg2.addrPtr);
    }*/
  }

  else {
  printf("Data received on port %d from port %d with length %d\n",
         receiver_port, sender_port, datalen);

  //keepSentRecv(sender_addr, 0, 1);

//printf("TEST ");
//uip_debug_ipaddr_print(sender_addr);
//printf("\n\n");

  //keepProbeResult(sender_addr, 1, 0);
  }
}
/*---------------------------------------------------------------------------*/
static void
set_global_address(void)
{
  uip_ipaddr_t ipaddr;
  int i;
  uint8_t state;

  uip_ip6addr(&ipaddr, 0xaaaa, 0, 0, 0, 0, 0, 0, 0);
  uip_ds6_set_addr_iid(&ipaddr, &uip_lladdr);
  uip_ds6_addr_add(&ipaddr, 0, ADDR_AUTOCONF);

  printf("IPv6 addresses: ");
  for(i = 0; i < UIP_DS6_ADDR_NB; i++) {
    state = uip_ds6_if.addr_list[i].state;
    if(uip_ds6_if.addr_list[i].isused &&
       (state == ADDR_TENTATIVE || state == ADDR_PREFERRED)) {
      uip_debug_ipaddr_print(&uip_ds6_if.addr_list[i].ipaddr);
      printf("\n");
    }
  }
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(unicast_sender_process, ev, data)
{
  static struct etimer periodic_timer;
  static struct etimer send_timer;
  uip_ipaddr_t *addr;

  //uip_ipaddr_t sendTo1;
  uip_ip6addr(&sendTo1, 0xaaaa, 0, 0, 0, 0x212, 0x7401, 0x0001, 0x0101);

  //static uip_ds6_route_t *r;

  //struct unicast_message *msg;
  //struct unicast_message msg2;
  //msg = data;

  static struct etimer time;

  //static uip_ds6_nbr_t *nbr;

  PROCESS_BEGIN();

  servreg_hack_init();

  set_global_address();

  simple_udp_register(&unicast_connection, UDP_PORT,
                      NULL, UDP_PORT, receiver);

  etimer_set(&periodic_timer, SEND_INTERVAL);
  while(1) {
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));
    etimer_reset(&periodic_timer);
    etimer_set(&send_timer, SEND_TIME);

    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&send_timer));
    addr = servreg_hack_lookup(SERVICE_ID);

    static unsigned int message_number;
    char buf[20];

    /*for(nbr = nbr_table_head(ds6_neighbors); nbr != NULL;
      nbr = nbr_table_next(ds6_neighbors,nbr)) {
      //printf("NBR TABLE ");
      uip_debug_ipaddr_print(&nbr->ipaddr);
      printf(" channel %d\n", nbr->nbrCh);
    }*/

    //keepSentRecv(&sendTo1, 1, 0);

keepSentRecv(&sendTo1, 1, 0);

    printf("Sending unicast to ");
    uip_debug_ipaddr_print(&sendTo1);
    printf("\n");
    sprintf(buf, "Message %d", message_number);
    message_number++;
    simple_udp_sendto(&unicast_connection, buf, strlen(buf) + 1, &sendTo1);

      //? to be called after sending the PROBE INFO to LPBR - empty the list
      //? QUICK HACK
      //for(q = 1; q <= 3; q++) {
        //removeProbe();
      //}
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(test1, ev, data)
{
  static struct etimer time;
  struct unicast_message *msg;
  struct unicast_message msg2;
  msg = data;

  static uip_ds6_route_t *r;

  //uip_ipaddr_t sendTo1;
  uip_ip6addr(&sendTo1, 0xfe80, 0, 0, 0, 0x212, 0x7401, 0x0001, 0x0101);

  //uip_ipaddr_t sendTo1G;
  //uip_ip6addr(&sendTo1G, 0xaaaa, 0, 0, 0, 0x212, 0x7401, 0x0001, 0x0101);

  static struct ctimer timer;
  struct probeResult *pr;

  //uint8_t newVal;
  //uip_ipaddr_t toParent;

  static uip_ds6_nbr_t *nbr;

uint8_t ww = 0;
uint8_t delayTime = 0;
  PROCESS_BEGIN();

  while(1) {
    PROCESS_WAIT_EVENT_UNTIL(ev == event_data_ready);

    //if(msg->type == NBR_CH_CHANGE || msg->type == STARTPROBE || msg->type == CONFIRM_CH) {
    if(msg->type == NBR_CH_CHANGE || msg->type == STARTPROBE) {

      msg2.type = msg->type;
      msg2.value = msg->value;
      changeTo = msg2.value;
      keepType = msg2.type;

      //! for padding as shortest packet size is 43 bytes (defined in contikimac.c)
      //msg2.paddingBuf[30] = " ";

      for(x = 0; x <=1; x++) {
        //! sending to ALL TREE NBR and PARENT should be done within 1 seconds maximum

        for(nbr = nbr_table_head(ds6_neighbors); nbr != NULL;
          nbr = nbr_table_next(ds6_neighbors,nbr)) {

	  if(!uip_ipaddr_cmp(uip_ds6_defrt_choose(), &sendTo1) && 
	    (uip_ipaddr_cmp(uip_ds6_defrt_choose(), &nbr->ipaddr))) {

	    msg2.type = keepType;
            msg2.value = changeTo;
            msg2.paddingBuf[30] = " ";
            msg2.addrPtr = &nbr->ipaddr;

            ww = 1;
	  }
	  else {
            for(r = uip_ds6_route_head(); r != NULL; 
	      r = uip_ds6_route_next(r)) {

              if(nbr->ipaddr.u8[11] == r->ipaddr.u8[11]){

		msg2.type = keepType;
                msg2.value = changeTo;
                msg2.paddingBuf[30] = " ";
                msg2.addrPtr = &nbr->ipaddr;

                ww = 1;
	      }//END IF
            }//END RT
          }//END ELSE

          if(ww == 1) {
	    if(x == 0) {
	    //if(keepType == NBR_CH_CHANGE) {
	      msg2.type = NBR_CH_CHANGE;
	      printf("%d Sending NBR CH CHANGE %d to tree neighbour ", nbr->nbrCh, msg2.value);
	      //delayTime = 1;
	      //delayTime = 0.15; //takes 1 sec but it doesn't matter since it will be in queue
	    }
	    else if(x == 1) {
	    //if(keepType == STARTPROBE) {
	      msg2.type = STARTPROBE;
	      printf("%d Sending STARTPROBE %d to tree neighbour ", nbr->nbrCh, msg2.value);
	      //delayTime = 1;
	    }

            uip_debug_ipaddr_print(msg2.addrPtr);
            printf("\n");
	    simple_udp_sendto(&unicast_connection, &msg2, sizeof(msg2), msg2.addrPtr);	

	    //etimer_set(&time, delayTime * CLOCK_SECOND);
	    etimer_set(&time, 1 * CLOCK_SECOND);
	    PROCESS_YIELD_UNTIL(etimer_expired(&time));

	    if(uip_ds6_if.addr_list[1].currentCh != changeTo) {
    	      uip_ds6_if.addr_list[1].currentCh = changeTo;
	    }
            ww = 0;
          }
        }//END NT
      }//END FOR X==1

      if(keepType == NBR_CH_CHANGE || keepType == STARTPROBE) {
      //if(keepType == STARTPROBE) {
        readProbeResult();
      }

/*      if(keepType == CONFIRM_CH) {
	printf("\n\nFINISH CONFIRM_CH\n\n");
	//checkAckProbeResultTable(changeTo);

    /*msg2.type = CONFIRM_CH;
    msg2.value = changeTo;
    msg2.addrPtr = &sendTo1G;
    msg2.paddingBuf[30] = " ";

    printf("Sending CONFIRM_CH to ");
    uip_debug_ipaddr_print(msg2.addrPtr);
    printf(" channel %d\n", msg2.value);

    simple_udp_sendto(&unicast_connection, &msg2, sizeof(msg2), msg2.addrPtr);
*/
//removeProbe();
//call removeProbe()
//      }
    }

    if(msg->type == NBRPROBE) {

      msg2.type = NBRPROBE;
      msg2.addrPtr = msg->addrPtr;
      msg2.value = msg->value;

      changeTo = msg2.value;
      uip_ipaddr_copy(&holdAddr, msg2.addrPtr);

      //printf("IN NBRPROBE\n\n");
      etimer_set(&time, 0.125 * CLOCK_SECOND);
      PROCESS_YIELD_UNTIL(etimer_expired(&time));

      //printf("AFTER 0.125s\n\n");
      y = 1;
      //! for padding as shortest packet size is 43 bytes (defined in contikimac.c)
      msg2.paddingBuf[30] = " ";
      for(x = 1; x <= 8; x++) {
      msg2.type = NBRPROBE;
	msg2.value = changeTo;
	msg2.value2 = y;
	msg2.addrPtr = &holdAddr;
        msg2.paddingBuf[30] = " ";

	//@
        //cc2420_set_channel(msg2.value);
// 	printf("%d %d Sending %d NBRPROBE %d to sender ", cc2420_get_channel(), sizeof(msg2), msg2.value2, msg2.value);
//	uip_debug_ipaddr_print(msg2.addrPtr);
//	printf("\n");

	y++;
	simple_udp_sendto(&unicast_connection, &msg2, sizeof(msg2), msg2.addrPtr);
      }
      //! the last few packets might be still be sending
      //!cc2420_set_channel(changeTo);
      etimer_set(&time, 0.15 * CLOCK_SECOND);
      //etimer_set(&time, 1 * CLOCK_SECOND);
      PROCESS_YIELD_UNTIL(etimer_expired(&time));
    }//end if(msg->type == NBRPROBE)

    if(msg->type == CONFIRM_CH) {
      msg2.type = msg->type;
      msg2.value = msg->value;
      changeTo = msg2.value;
      keepType = msg2.type;

      msg2.value2 = msg->value2;
      y = msg2.value2;

      /*if(y == 1) {
        msg2.addrPtr = msg->addrPtr;
	msg2.type = CONFIRM_CH;
	printf("%d Sending CONFIRM CH to all neighbours ", nbr->nbrCh);
	uip_debug_ipaddr_print(msg2.addrPtr);
	printf("\n");
	simple_udp_sendto(&unicast_connection, &msg2, sizeof(msg2), msg2.addrPtr);	
	
	etimer_set(&time, 0.5 * CLOCK_SECOND);
	PROCESS_YIELD_UNTIL(etimer_expired(&time));

	y = 0;
	break;
      }

      else {*/
      for(nbr = nbr_table_head(ds6_neighbors); nbr != NULL;
        nbr = nbr_table_next(ds6_neighbors,nbr)) {

	//msg2.type = keepType;
        msg2.value = changeTo;
        msg2.paddingBuf[30] = " ";
        msg2.addrPtr = &nbr->ipaddr;

	//if(nbr->ipaddr.u8[13] == 1) {
	  //printf("no need to send to LPBR\n");
	//}

	msg2.type = CONFIRM_CH;
	printf("%d Sending CONFIRM CH to all neighbours ", nbr->nbrCh);
        uip_debug_ipaddr_print(msg2.addrPtr);
        printf("\n");
	simple_udp_sendto(&unicast_connection, &msg2, sizeof(msg2), msg2.addrPtr);	

	etimer_set(&time, 0.7 * CLOCK_SECOND);
	PROCESS_YIELD_UNTIL(etimer_expired(&time));
      }
      //}

      //printf("FINISHED CONFIRM_CH\n\n");
      checkAckProbeResultTable(changeTo); //check to retransmit CONFIRM_CH
      //removeProbe(); //should be here?

  //struct probeResult *pr;

  /*for(pr = list_head(probeResult_table); pr != NULL; pr = pr->next) {
    printf("keeprobe c %d r %d ack %d ", pr->chNum, pr->rxValue, pr->checkAck);
    uip_debug_ipaddr_print(&pr->pAddr);
    printf("\n\n");
  }*/

    }

    if(msg->type == GET_ACK) {
      
      msg2.addrPtr = msg->addrPtr;
      uip_ipaddr_copy(&holdAddr, msg2.addrPtr);
      msg2.value = msg->value;

      changeTo = msg2.value;

      updateRoutingTable(msg2.addrPtr, msg2.value);

      //printf("SET 0.15S TIMER\n");
      etimer_set(&time, 0.15 * CLOCK_SECOND);
      PROCESS_YIELD_UNTIL(etimer_expired(&time));

      msg2.type = GET_ACK;
      msg2.addrPtr = &holdAddr;

      msg2.value = changeTo;

      //!cc2420_set_channel(msg2.value);
      printf("Sending GET_ACK back %d ", changeTo);
      //cc2420_set_channel(uip_ds6_defrt_ch());
      uip_debug_ipaddr_print(msg2.addrPtr);
      printf("\n");

      //@
      //cc2420_set_channel(changeTo);
      simple_udp_sendto(&unicast_connection, &msg2, sizeof(msg2), msg2.addrPtr);
    }
  }//end while(1)

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
