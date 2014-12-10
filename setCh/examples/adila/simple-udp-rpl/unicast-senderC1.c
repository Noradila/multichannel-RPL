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

//#define SEND_INTERVAL		(60 * CLOCK_SECOND)
#define SEND_INTERVAL		(300 * CLOCK_SECOND)
#define SEND_TIME		(random_rand() % (SEND_INTERVAL))

struct probeResult {
  struct probeResult *next;
  uip_ipaddr_t pAddr;
  uint8_t chNum;
  uint8_t rxValue;
};

LIST(probeResult_table);
MEMB(probeResult_mem, struct probeResult, 5);

uint8_t currentVal;
uint8_t changeTo;
static uip_ipaddr_t holdAddr;
uint8_t x;
uint8_t y;

static struct simple_udp_connection unicast_connection;

//the application specific event value
static process_event_t event_data_ready;

enum {
	TRIGGER,
	CH_CHANGE,
	NBR_CH_CHANGE,
	NBRPROBE,
	PROBERESULT
};

struct unicast_message {
	uint8_t type;
	uint8_t value;
	uint8_t holdV;

	uip_ipaddr_t address;
	uip_ipaddr_t *addrPtr; 

	char paddingBuf[30];
};

/*---------------------------------------------------------------------------*/
PROCESS(unicast_sender_process, "Unicast sender example process");
PROCESS(test1, "test");
AUTOSTART_PROCESSES(&unicast_sender_process, &test1);
/*---------------------------------------------------------------------------*/
static void removeProbe() {
  struct probeResult *pr;

  for(pr = list_head(probeResult_table); pr != NULL; pr = pr->next) {
    list_remove(probeResult_table, pr);
    memb_free(&probeResult_mem, pr);
  }
}
/*---------------------------------------------------------------------------*/
static void readProbe() {
  struct probeResult *pr;

  for(pr = list_head(probeResult_table); pr != NULL; pr = pr->next) {
    printf("READPROBE pktRx %d, chNum %d, ip ", pr->rxValue, pr->chNum);
    uip_debug_ipaddr_print(&pr->pAddr);
    printf("\n");
  }
}
/*---------------------------------------------------------------------------*/
static void keepProbeResult(const uip_ipaddr_t *prAddr, uint8_t chN) {
  struct probeResult *pr;

  for(pr = list_head(probeResult_table); pr != NULL; pr = pr->next) {
    if(uip_ipaddr_cmp(prAddr, &pr->pAddr)) {
      if(chN == pr->chNum) {
	pr->rxValue = (pr->rxValue) + 1;
	return;
      }
    }
  }

  pr = memb_alloc(&probeResult_mem);
  if(pr != NULL) {
    pr->rxValue = 1;
    pr->chNum = chN;
    uip_ipaddr_copy(&pr->pAddr, prAddr);
    list_add(probeResult_table, pr);
  }
}
/*---------------------------------------------------------------------------*/
static void decideChChange(void *ptr) {
 printf("\n\nAFTER 1 SEC CALL DECIDECHCHANGE\n\n");
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

  if(msg->type == CH_CHANGE) {
    printf("%d received CH_CHANGE from ", msg->value);
    uip_debug_ipaddr_print(sender_addr);
    printf("\n");

    msg2.type = NBR_CH_CHANGE;
    msg2.value = msg->value;

    process_post_synch(&test1, event_data_ready, &msg2);
  }//end if(msg->type == CH_CHANGE)

  else if(msg->type == NBR_CH_CHANGE) {
    printf("%d received NBR_CH_CHANGE from ", msg->value);
    uip_debug_ipaddr_print(sender_addr);
    printf("\n");

    msg2.type = NBRPROBE;
    msg2.value = msg->value;
    msg2.addrPtr = sender_addr;

    //process_post_synch(&test1, event_data_ready, &msg2);
    process_post(&test1, event_data_ready, &msg2);
  }//end if(msg->type == NBR_CH_CHANGE)

  else if(msg->type == NBRPROBE) {
    //printf("%d received NBRPROBE from ", msg->value);
    //uip_debug_ipaddr_print(sender_addr);
    //printf("\n");

    keepProbeResult(sender_addr, msg->value);
  }

  else {
  printf("Data received on port %d from port %d with length %d\n",
         receiver_port, sender_port, datalen);
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

  uip_ipaddr_t sendTo1;
  uip_ip6addr(&sendTo1, 0xaaaa, 0, 0, 0, 0x212, 0x7401, 0x0001, 0x0101);

  static uip_ds6_route_t *r;


uint8_t q;
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

    for(r = uip_ds6_route_head(); r != NULL; 
	r = uip_ds6_route_next(r)) {
	printf("%d ROUTE: ", r->nbrCh);
	uip_debug_ipaddr_print(&r->ipaddr);
	printf(" via ");
	uip_debug_ipaddr_print(uip_ds6_route_nexthop(r));
	printf("\n");
    }

//printf("DEFAULT ROUTE ");
//uip_debug_ipaddr_print(uip_ds6_defrt_choose());
//printf("\n");

//printf("SET DEFAULT ROUTE ");
//uip_ds6_defrt_ch();
//printf("\n");

//uip_ds6_if.addr_list[1].currentCh = 15;
/*printf("  OWNCH %d ", uip_ds6_if.addr_list[1].currentCh);
*/


      readProbe();

      printf("Sending unicast to ");
      uip_debug_ipaddr_print(&sendTo1);
      printf("\n");
      sprintf(buf, "Message %d", message_number);
      message_number++;
      simple_udp_sendto(&unicast_connection, buf, strlen(buf) + 1, &sendTo1);

      //? to be called after sending the PROBE INFO to LPBR - empty the list
      //? QUICK HACK
      for(q = 1; q <= 3; q++) {
        removeProbe();
      }
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

  uip_ipaddr_t sendTo1;
  uip_ip6addr(&sendTo1, 0xfe80, 0, 0, 0, 0x212, 0x7401, 0x0001, 0x0101);

  uip_ipaddr_t sendTo1G;
  uip_ip6addr(&sendTo1G, 0xaaaa, 0, 0, 0, 0x212, 0x7401, 0x0001, 0x0101);

  uip_ipaddr_t nextHopAddr;
  uip_ip6addr(&nextHopAddr, 0, 0, 0, 0, 0, 0, 0, 0);

  static struct ctimer timer;

//uint8_t calculateProbe = 0;

  PROCESS_BEGIN();

  while(1) {
    PROCESS_WAIT_EVENT_UNTIL(ev == event_data_ready);

    if(msg->type == NBR_CH_CHANGE) {
      msg2.type = NBR_CH_CHANGE;
      msg2.value = msg->value;

ctimer_set(&timer, 0.5 * CLOCK_SECOND, decideChChange, NULL);

      for(r = uip_ds6_route_head(); r != NULL; 
	r = uip_ds6_route_next(r)) {

	//! check to ensure it doesn't repeat the same nexthop neighbour
	if(!uip_ipaddr_cmp(&nextHopAddr, uip_ds6_route_nexthop(r))) {

	  printf("Sending channel change %d to tree neighbour ", msg2.value);
	  uip_debug_ipaddr_print(uip_ds6_route_nexthop(r));
	  printf("\n");	

	  msg2.addrPtr = uip_ds6_route_nexthop(r);
	  //! keep nextHop address so that it won't send to the same address again
	  uip_ipaddr_copy(&nextHopAddr, uip_ds6_route_nexthop(r));

	  //? change to the neighbour channel cc2420_set_channel(r->nbrCh)
	  simple_udp_sendto(&unicast_connection, &msg2, sizeof(msg2), msg2.addrPtr);
	}
      }

      if(!uip_ipaddr_cmp(uip_ds6_defrt_choose(), &sendTo1)) {
	msg2.addrPtr = uip_ds6_defrt_choose();
	printf("Sending channel change %d to parent ", msg2.value);
	uip_debug_ipaddr_print(uip_ds6_defrt_choose());
	printf("\n");

	simple_udp_sendto(&unicast_connection, &msg2, sizeof(msg2), msg2.addrPtr);
      }

      //? set the node OWN currentCh so that it will wake up to it's own new listening channel
      //? the new currentCh is NOT yet confirm; keep the previous channel to prevCh
      //# uip_ds6_if.addr_list[1].prevCh = cc2420_get_channel();	
      //# uip_ds6_if.addr_list[1].currentCh = msg2.value;
      uip_ds6_if.addr_list[1].prevCh = 18;

      /*etimer_set(&time, 1 * CLOCK_SECOND);
      //PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&time));

      if(etimer_expired(&time)) {
      printf("\n\nAFTER TIME 1 SEC EXPIRED!\n\n");
      }*/

    }//end if(msg->type == NBR_CH_CHANGE)

    if(msg->type == NBRPROBE) {
      msg2.type = NBRPROBE;
      msg2.addrPtr = msg->addrPtr;
      msg2.value = msg->value;

      changeTo = msg2.value;
      uip_ipaddr_copy(&holdAddr, msg2.addrPtr);

      //? updates the routing table r->nbrCh = msg->value;
      //? keeps the value in changeTo so that it doesn't need to go through the table again
      /*for(r = uip_ds6_route_head(); r != NULL; 
	r = uip_ds6_route_next(r)) {

	printf("CHECK ROUTE: ");
	uip_debug_ipaddr_print(&r->ipaddr);
	printf(" via ");
	uip_debug_ipaddr_print(uip_ds6_route_nexthop(r));
	printf(" nbrCh %d", r->nbrCh);
	printf("\n");

      }*/
      y = 1;
      //! for padding as shortest packet size is 43 bytes (defined in contikimac.c)
      msg2.paddingBuf[30] = " ";
      for(x = 1; x <= 5; x++) {
      msg2.type = NBRPROBE;
	msg2.value = changeTo;
	//msg2.value = y;
	msg2.addrPtr = &holdAddr;

 	//printf("%d Sending NBRPROBE %d to sender ", sizeof(msg2), msg2.value);
	//uip_debug_ipaddr_print(msg2.addrPtr);
	//printf("\n");

	y++;
	simple_udp_sendto(&unicast_connection, &msg2, sizeof(msg2), msg2.addrPtr);

	//etimer_set(&time, 0.05 * CLOCK_SECOND);
        //PROCESS_YIELD_UNTIL(etimer_expired(&time));
      }

      //? timeout if ENDPROBE is not received - time should be the time success probing happen
      //? etimer_set(&time (to be defined), 0.05 * CLOCK_SECOND (need to check the time);
      //? PROCESS_YIELD_UNTIL(etimer_expired(&time));

    }//end if(msg->type == NBRPROBE)
  }//end while(1)

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
