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

#define UDP_PORT 1234
#define SERVICE_ID 190

//#define SEND_INTERVAL		(60 * CLOCK_SECOND)
#define SEND_INTERVAL		(300 * CLOCK_SECOND)
#define SEND_TIME		(random_rand() % (SEND_INTERVAL))

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
	NBRPROBE
};

struct unicast_message {
	uint8_t type;
	uint8_t value;
	uint8_t holdV;

	uip_ipaddr_t address;
	uip_ipaddr_t *addrPtr; 

	char paddingBuf[50];
};

/*---------------------------------------------------------------------------*/
PROCESS(unicast_sender_process, "Unicast sender example process");
PROCESS(test1, "test");
AUTOSTART_PROCESSES(&unicast_sender_process, &test1);
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
    printf("RECEIVED NBR_CH_CHANGE %d\n", msg->value);
    printf("START PROBING?\n\n");

    msg2.type = NBRPROBE;
    //msg2.type = OKCH;
    msg2.value = msg->value;
    msg2.addrPtr = sender_addr;

    //process_post_synch(&test1, event_data_ready, &msg2);
    process_post(&test1, event_data_ready, &msg2);
  }//end if(msg->type == NBR_CH_CHANGE)

  else if(msg->type == NBRPROBE) {
    printf("%d GET NBRPROBE FROM ", msg->value);
    uip_debug_ipaddr_print(sender_addr);
    printf("\n");
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
      printf("Sending unicast to ");
      uip_debug_ipaddr_print(&sendTo1);
      printf("\n");
      sprintf(buf, "Message %d", message_number);
      message_number++;
      simple_udp_sendto(&unicast_connection, buf, strlen(buf) + 1, &sendTo1);
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

//uint8_t calculateProbe = 0;

  PROCESS_BEGIN();

  while(1) {
    PROCESS_WAIT_EVENT_UNTIL(ev == event_data_ready);

    if(msg->type == NBR_CH_CHANGE) {
      //changeTo = msg->value;
      //currentVal = cc2420_get_channel();

      //uip_ds6_if.addr_list[1].currentCh = msg->value;
      //uip_ds6_if.addr_list[1].currentCh = cc2420_get_channel();
      printf("PREVCH %d CURRENTCH %d\n", uip_ds6_if.addr_list[1].prevCh, uip_ds6_if.addr_list[1].currentCh);

      msg2.type = NBR_CH_CHANGE;
      msg2.value = msg->value;
	//msg2.address = uip_ds6_route_nexthop(r);

      for(r = uip_ds6_route_head(); r != NULL; 
	r = uip_ds6_route_next(r)) {

	/*printf("CLIENT ROUTE: ");
	uip_debug_ipaddr_print(&r->ipaddr);
	printf(" via ");
	uip_debug_ipaddr_print(uip_ds6_route_nexthop(r));
	printf(" nbrCh %d", r->nbrCh);
	printf("\n");*/

	if(!uip_ipaddr_cmp(&nextHopAddr, uip_ds6_route_nexthop(r))) {

	  printf("Sending channel change %d to tree neighbour ", msg2.value);
	  uip_debug_ipaddr_print(uip_ds6_route_nexthop(r));
	  printf("\n");	

	  msg2.addrPtr = uip_ds6_route_nexthop(r);

	  //! keep nextHop address so that it won't send to the same address again
	  uip_ipaddr_copy(&nextHopAddr, uip_ds6_route_nexthop(r));

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

      uip_ds6_if.addr_list[1].prevCh = 18;
      printf("END SENDING CHANNEL CHANGE\n\n");
    }//end if(msg->type == NBR_CH_CHANGE)

    if(msg->type == NBRPROBE) {
	msg2.type = NBRPROBE;
	msg2.addrPtr = msg->addrPtr;
	msg2.value = msg->value;

	changeTo = msg2.value;
	uip_ipaddr_copy(&holdAddr, msg2.addrPtr);
      //printf("NBRPROBE\n\n");


//! updates the routing table
//! keeps the value in changeTo so that it doesn't need to go through the table again
      for(r = uip_ds6_route_head(); r != NULL; 
	r = uip_ds6_route_next(r)) {

	printf("CHECK ROUTE: ");
	uip_debug_ipaddr_print(&r->ipaddr);
	printf(" via ");
	uip_debug_ipaddr_print(uip_ds6_route_nexthop(r));
	printf(" nbrCh %d", r->nbrCh);
	printf("\n");

      }
//for(calculateProbe = 0; calculateProbe <= 5; calculateProbe++) {
	y = 1;
	  //msg2.paddingBuf[50] = "hello world! hello world! hello world! hello world! hello world!";

	//! for padding as shortest packet size is 43 bytes (defined in contikimac.c)
	msg2.paddingBuf[50] = " ";
	for(x = 1; x <= 5; x++) {
	  //msg2.value = changeTo;
	  msg2.value = y;
	  msg2.addrPtr = &holdAddr;

	  
	printf("%d Sending NBRPROBE %d to sender ", sizeof(msg2), msg2.value);
	uip_debug_ipaddr_print(msg2.addrPtr);
	printf("\n");

	y++;
	simple_udp_sendto(&unicast_connection, &msg2, sizeof(msg2), msg2.addrPtr);
	}
//}

    }//end if(msg->type == NBRPROBE)
  }//end while(1)

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
