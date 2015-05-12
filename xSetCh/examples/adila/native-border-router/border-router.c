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
/**
 * \file
 *         border-router
 * \author
 *         Niclas Finne <nfi@sics.se>
 *         Joakim Eriksson <joakime@sics.se>
 *         Nicolas Tsiftes <nvt@sics.se>
 */

#include "contiki.h"
#include "contiki-lib.h"
#include "contiki-net.h"
#include "net/uip.h"
#include "net/uip-ds6.h"
#include "net/rpl/rpl.h"

#include "net/netstack.h"
#include "dev/slip.h"
#include "cmd.h"
#include "border-router.h"
#include "border-router-cmds.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define DEBUG DEBUG_FULL
#include "net/uip-debug.h"

//#define MAX_SENSORS 4
//#define MAX_SENSORS 20
#define MAX_SENSORS 50

//#include "random.h"

#include "lib/list.h"
#include "lib/memb.h"

#include "simple-udp.h"
#define UDP_PORT 1234
#define SERVICE_ID 190
static struct simple_udp_connection unicast_connection;

//the application specific event value
static process_event_t event_data_ready;

uint8_t a = 0;
uip_ipaddr_t nextHopAddr;
uip_ipaddr_t nH;
uint8_t channelOK = 1;

uint8_t noOfRoutes;
uint8_t sendingTo = 0;
static uip_ipaddr_t holdAddr;

uint8_t noOfRetransmit;

uint8_t i;
uint8_t x;

struct lpbrList {
  struct lpbrList *next;
  uip_ipaddr_t routeAddr;
  uip_ipaddr_t nbrAddr;
  uint8_t chNum;
  uint8_t rxValue;

  //uint8_t batteryLevel;
  //uint8_t pktSent;
  //uint8_t pktRecv;
};

LIST(lpbrList_table);
MEMB(lpbrList_mem, struct lpbrList, 50);

struct sentRecv {
  struct sentRecv *next;
  uip_ipaddr_t sendToAddr;
  uint8_t noSent;
  uint8_t noRecv;
};

LIST(sentRecv_table);
//MEMB(sentRecv_mem, struct sentRecv, 20); //for now, only sent to LPBR
MEMB(sentRecv_mem, struct sentRecv, 50); //for now, only sent to LPBR

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

uint8_t value3;
};

uint16_t dag_id[] = {0x1111, 0x1100, 0, 0, 0, 0, 0, 0x0011};

extern long slip_sent;
extern long slip_received;

static uip_ipaddr_t prefix;
static uint8_t prefix_set;
static uint8_t mac_set;

static uint8_t sensor_count = 0;

/* allocate MAX_SENSORS char[32]'s */
static char sensors[MAX_SENSORS][32];

extern int contiki_argc;
extern char **contiki_argv;
extern const char *slip_config_ipaddr;

CMD_HANDLERS(border_router_cmd_handler);

PROCESS(border_router_process, "Border router process");
PROCESS(chChange_process, "Channel change process");

PROCESS(test, "TEST");

//#if WEBSERVER==0
/* No webserver */
AUTOSTART_PROCESSES(&border_router_process,&border_router_cmd_process, &chChange_process, &test);
//#elif WEBSERVER>1
/* Use an external webserver application */
/*#include "webserver-nogui.h"
AUTOSTART_PROCESSES(&border_router_process,&border_router_cmd_process,
		    &webserver_nogui_process, &chChange_process);
#else*/
/* Use simple webserver with only one page */
/*#include "httpd-simple.h"
PROCESS(webserver_nogui_process, "Web server");
PROCESS_THREAD(webserver_nogui_process, ev, data)
{
  PROCESS_BEGIN();

  httpd_init();

  while(1) {
    PROCESS_WAIT_EVENT_UNTIL(ev == tcpip_event);
    httpd_appcall(data);
  }

  PROCESS_END();
}
AUTOSTART_PROCESSES(&border_router_process,&border_router_cmd_process,
		    &webserver_nogui_process, &chChange_process);
*/
//static const char *TOP = "<html><head><title>ContikiRPL</title></head><body>\n";
//static const char *BOTTOM = "</body></html>\n";
static char buf[128];
static int blen;
#define ADD(...) do {                                                   \
    blen += snprintf(&buf[blen], sizeof(buf) - blen, __VA_ARGS__);      \
  } while(0)
/*---------------------------------------------------------------------------*/
uint8_t lpbrCheck2ndHop(const uip_ipaddr_t *toSendAddr, uint8_t chCheck) {
  static uip_ds6_nbr_t *nbr;

  //check with all LPBR neighbours
  for(nbr = nbr_table_head(ds6_neighbors); nbr != NULL;
    nbr = nbr_table_next(ds6_neighbors,nbr)) {
    if(toSendAddr->u8[13] != nbr->ipaddr.u8[13]) {
      if((nbr->nbrCh) != chCheck) {
	channelOK = 1;
	//printf("LPBR2hopsLPBR ch %d lchNum %d l %d to %d\n\n", chCheck, nbr->nbrCh, nbr->ipaddr.u8[13], toSendAddr->u8[13]);
        //!chCheck = random_rand();
      }//END ((nbr->nbrCh) != chCheck)
      else {
	channelOK = 0;
	//printf("CHECK CH: 2HOPSLPBR 2 HOP FAILED\n\n");
        //printf("2hopsLPBR %d\n\n", channelOK);
        break;
       //return NOT OK
      }
    }//END IF 
  }//END FOR
  return channelOK;
}
/*---------------------------------------------------------------------------*/
//static void secondCheck(const uip_ipaddr_t *toSendAddr, uint8_t chCheck) {
uint8_t secondCheck(const uip_ipaddr_t *toSendAddr, uint8_t chCheck) {
  struct lpbrList *l;

  for(l = list_head(lpbrList_table); l != NULL; l = l->next) {
    if(l->nbrAddr.u8[13] == toSendAddr->u8[13]) {
      if(l->chNum != chCheck) {
	//printf("2hopsothernodes %d ch %d lchNum %d l %d to %d\n\n", l->routeAddr.u8[13], chCheck, l->chNum, l->nbrAddr.u8[13], toSendAddr->u8[13]);
	//printf("l->chNum %d != chCheck %d\n\n", l->chNum, chCheck);
	channelOK = 1;
      }
      else {
	channelOK = 0;
	//printf("CHECK CH: SECONDCHECK FAILED!\n\n");
	break;
      }
    }
  }
  return channelOK;  
  //return chCheck;
}
/*---------------------------------------------------------------------------*/
uint8_t twoHopsOtherNodes(const uip_ipaddr_t *toSendAddr, uint8_t chCheck) {
  struct lpbrList *l;
  uint8_t tempVal;

  static uip_ds6_nbr_t *nbr;
  static uip_ds6_route_t *r;

  //for(r = uip_ds6_route_head(); r != NULL; r = uip_ds6_route_next(r)) {
  for(l = list_head(lpbrList_table); l != NULL; l = l->next) {
    if(l->nbrAddr.u8[13] == toSendAddr->u8[13]) {
      //printf("1hopsothernodes %d ch %d lchNum %d l %d to %d\n\n", l->routeAddr.u8[13], chCheck, l->chNum, l->nbrAddr.u8[13], toSendAddr->u8[13]);
      if(l->chNum != chCheck) {
	//printf("l->chNum %d != chCheck %d\n\n", l->chNum, chCheck);
	//tempVal = l->routeAddr.u8[13];

	for(nbr = nbr_table_head(ds6_neighbors); nbr != NULL;
	  nbr = nbr_table_next(ds6_neighbors,nbr)) {
	    if(l->routeAddr.u8[13] == nbr->ipaddr.u8[13]) {
	      if(uip_ds6_if.addr_list[1].currentCh != chCheck) {
	        //printf("2nd hop is LPBR %d chCheck %d\n\n", uip_ds6_if.addr_list[1].currentCh, chCheck);
	        channelOK = 1;
	      }
	      else {
		channelOK = 0;
		//printf("CHECK CH: 2HOPSOTHERNODES 2 HOP LPBR FAILED for %d\n\n", l->routeAddr.u8[13]);
		break;
	      }
	    }
  	}
	channelOK = secondCheck(&l->routeAddr, chCheck);
	if(channelOK == 0) {
	  break;
	}
      }
      else {
        channelOK = 0;
	//printf("CHECK CH: 2HOPSOTHERNODES 1 HOP FAILED\n\n");
	break;
      }
    }//END IF 1ST HOP
  }//END FOR 1ST FOR

  return channelOK;
  //return chCheck;
}
/*---------------------------------------------------------------------------*/
uint8_t twoHopsLPBR(const uip_ipaddr_t *toSendAddr, uint8_t chCheck) {
  static uip_ds6_nbr_t *nbr;

  for(nbr = nbr_table_head(ds6_neighbors); nbr != NULL;
    nbr = nbr_table_next(ds6_neighbors,nbr)) {
    if(toSendAddr->u8[13] == nbr->ipaddr.u8[13]) {
      //check LPBR != chCheck (1 hop)
      if(uip_ds6_if.addr_list[1].currentCh != chCheck) {
	//printf("LPBR1hopsLPBR ch %d lchNum %d l %d to %d\n\n", chCheck, uip_ds6_if.addr_list[1].currentCh, nbr->ipaddr.u8[13], toSendAddr->u8[13]);

        channelOK = lpbrCheck2ndHop(toSendAddr, chCheck);
        if(channelOK == 0) {
	  break;
	}
      }//END IF
      else {
	channelOK = 0;
	//printf("CHECK CH: 2HOPSOTHERNODES 1 HOP FAILED\n\n");
	break;
        //return NOT OK (same CH as LPBR)
      }
    }//END IF toSendAddr == nbr->ipaddr
  }//END FOR

  return channelOK;
  //return chCheck;
}
/*---------------------------------------------------------------------------*/
void doSending(struct unicast_message *msg) {
  //struct unicast_message *msg;
  struct unicast_message msg2;

  uint8_t randomNewCh;
  static uip_ds6_route_t *r;

  static uip_ds6_nbr_t *nbr;
  static uip_ds6_route_t *re;

  //printf("GOT FROM STARTCHCHANGE WHEN 0 ");
  //uip_debug_ipaddr_print(&msg->address);
  //printf("\n\n");
  uip_ipaddr_copy(&holdAddr, &msg->address);

  randomNewCh = (random_rand() % 16) + 11;
  //! check if randomNewCh is blacklisted (if it's on the list, low success rate)

  msg2.type = CH_CHANGE;
  msg2.value = randomNewCh;
  //msg2.address = r->ipaddr;
  msg2.address = msg->address;
  msg2.paddingBuf[30] = " ";

  channelOK = twoHopsLPBR(&msg2.address, msg2.value);
  channelOK = twoHopsOtherNodes(&msg2.address, msg2.value);

  //channel will be selected and checked with 2 hops for 4 times
  //if failed, it will use the default channel, 26
  while(channelOK == 0 && i < 4) {
    randomNewCh = (random_rand() % 16) + 11;
    while(msg2.value == randomNewCh) {
      randomNewCh = (random_rand() % 16) + 11;
      //printf("RANDOMNEWCH %d i %d\n\n", randomNewCh, i);
    }
    channelOK = twoHopsLPBR(&msg2.address, randomNewCh);
    channelOK = twoHopsOtherNodes(&msg2.address, randomNewCh);

    i++;
  }

  if(i == 4) {
    //couldn't find a 2 hops channel value, use the default 26
    //printf("%d RANDOMNEWCH USE DEFAULT\n\n", i);
    randomNewCh = 26;
    i = 0;
  }

/*  while(channelOK == 0) {
    randomNewCh = (random_rand() % 16) + 11;
    while(msg2.value == randomNewCh) {
      randomNewCh = (random_rand() % 16) + 11;
    }
    channelOK = twoHopsLPBR(&msg2.address, randomNewCh);
    channelOK = twoHopsOtherNodes(&msg2.address, randomNewCh);
  }

  //printf("LPBR2 CHANNEL OK? %d\n\n\n", channelOK);
  msg2.value = randomNewCh;
*/
msg2.value = 22;

  printf("%d: %d BR Sending channel to change for ", sizeof(msg2), msg2.value);
  uip_debug_ipaddr_print(&msg2.address);
  //printf(" via ");
  //uip_debug_ipaddr_print(uip_ds6_route_nexthop(r));
  printf("\n");

  simple_udp_sendto(&unicast_connection, &msg2, sizeof(msg2) + 1, &msg2.address);
  process_post(&chChange_process, event_data_ready, NULL);
}
/*---------------------------------------------------------------------------*/
static void startChChange(uint8_t currentNode) {
  struct unicast_message msg2;
  static uip_ds6_route_t *r;

  uint8_t i = 0;

  for(r = uip_ds6_route_head(); r != NULL; 
    r = uip_ds6_route_next(r)) {
    i++;
    if(i == currentNode) {
      msg2.address = r->ipaddr;
      //printf("i %d currentNode %d ", i, currentNode);
      //uip_debug_ipaddr_print(&r->ipaddr);
      //printf("\n\n");
      doSending(&msg2);
      break;
    }
  }
}
/*---------------------------------------------------------------------------*/
static void howManyRoutes() {
  static uip_ds6_route_t *r;

  for(r = uip_ds6_route_head(); r != NULL; 
    r = uip_ds6_route_next(r)) {

    noOfRoutes = noOfRoutes + 1;

//printf("1:%x 2:%x 3:%x 4:%x 5:%x 6:%x 7:%x 8:%x 9:%x 10:%x 11:%x 12:%x 13:%x 14:%x 15:%x\n", r->ipaddr.u8[1], r->ipaddr.u8[2], r->ipaddr.u8[3], r->ipaddr.u8[4], r->ipaddr.u8[5], r->ipaddr.u8[6], r->ipaddr.u8[7], r->ipaddr.u8[8], r->ipaddr.u8[9], r->ipaddr.u8[10], r->ipaddr.u8[11], r->ipaddr.u8[12], r->ipaddr.u8[13], r->ipaddr.u8[14], r->ipaddr.u8[15]);
    //printf("%d NO OF ROUTE ", noOfRoutes);
    //uip_debug_ipaddr_print(&r->ipaddr);
    //printf("\n");
  }
}
/*---------------------------------------------------------------------------*/
static void recheck2() {
  struct lpbrList *l;
  static uip_ds6_route_t *r;
  uint8_t checkOK = 0;
  uip_ipaddr_t theMissedAddr;

  struct unicast_message msg2;

  for(r = uip_ds6_route_head(); r != NULL; r = uip_ds6_route_next(r)) {
    for(l = list_head(lpbrList_table); l != NULL; l = l->next) {
      if(uip_ipaddr_cmp(&r->ipaddr, &l->routeAddr)) {
        /*printf("SAME ");
	uip_debug_ipaddr_print(&r->ipaddr);
	printf(" ");
	uip_debug_ipaddr_print(&l->routeAddr);
	printf("\n");*/
	checkOK = 1;
	break;
      }
      else {
	checkOK = 0;
      }
    }
    if(checkOK == 0) {
      uip_ipaddr_copy(&theMissedAddr, &r->ipaddr);
      printf("themissedaddr ");
      uip_debug_ipaddr_print(&theMissedAddr);
      printf("\n\n");
      //break;
      msg2.address = theMissedAddr;
      //process_post_synch(&test, event_data_ready, &msg2);
      //doSending(&msg2);
    }
  }
 
/*  if(checkOK == 0) {
    printf("recheck2 checkOK %d ", checkOK);
    uip_debug_ipaddr_print(&theMissedAddr);
    printf("\n");
  }
*/
}

/*uint8_t waitingTime(const uip_ipaddr_t *ipAddress) {
  struct lpbrList *l;
  uint8_t checkOK = 0;

  for(l = list_head(lpbrList_table); l != NULL; l = l->next) {
    if(uip_ipaddr_cmp(&l->routeAddr, ipAddress)) {
      printf("CALL NEXT ROUTE FOR CH_CHANGE\n\n");
      checkOK = 1;
      break;
    }
    else {
      checkOK = 0;
    }
  }

  if(checkOK == 0) {
    printf("NEED MORE TIME\n\n");
    return checkOK;
  }
  else {
    printf("CAN PROCEES TO NEXT CH_CHANGE\n\n");
    return checkOK;
  }
}*/

static void recheck(const uip_ipaddr_t *ipAddress) {
//static void recheck() {
  struct lpbrList *l;
  //static uip_ds6_route_t *r;
  uint8_t checkOK = 0;
  //uip_ipaddr_t *theMissedAddr;

  for(l = list_head(lpbrList_table); l != NULL; l = l->next) {
    if(uip_ipaddr_cmp(&l->routeAddr, ipAddress)) {
      //printf("SAME ");
      //uip_debug_ipaddr_print(ipAddress);
      //printf(" ");
      //uip_debug_ipaddr_print(&l->routeAddr);
      //printf("\n");
      checkOK = 1;
      break;
    }
    else {
      checkOK = 0;
    }
  }

  //if(checkOK == 1) {
  if(checkOK == 1 || noOfRetransmit == 4) {
    //printf("NOOFRETRANSMIT %d\n\n", noOfRetransmit);
    sendingTo = sendingTo + 1;
    noOfRetransmit = 0;
    //printf("CHANNELOK 1 SENDING TO %d\n\n", sendingTo);
    startChChange(sendingTo);

    //!!!!! if sending done recheck the whole table again?
/*    if(sendingTo == (noOfRoutes + 1)) {
      //recheck again
      //printf("SENDINGTO-1 %d NOOFROUTES %d\n\n", sendingTo, noOfRoutes + 1);
      recheck2();
    }
*/
  }
  else {
    //repeat sending to the same node
    //startChChange(sendingTo);
    //printf("NOOFRETRANSMIT %d\n\n", noOfRetransmit);
    //printf("CHANNELOK 0 SENDING TO %d\n\n", sendingTo);
    noOfRetransmit = noOfRetransmit + 1;
    startChChange(sendingTo);
  }
}
/*---------------------------------------------------------------------------*/
static void readProbe(uint8_t checkValue) {
  struct lpbrList *l;

  if(checkValue == 1) {
    printf("-----------------------------------------------------------\n");
    printf("ROUTE\t\t\tROUTE NBR\t\tCH\tRX\n");
    checkValue = 0;
  }

  for(l = list_head(lpbrList_table); l != NULL; l = l->next) {
    if(checkValue == 0) {
    uip_debug_ipaddr_print(&l->routeAddr);
    printf("\t");
    uip_debug_ipaddr_print(&l->nbrAddr);
    printf("\t");
    printf("%d\t%d\n", l->chNum, l->rxValue);
    }

    else {
      //? 2 hops?
      //? compare checkValue == l->chNum for LPBR to decide what channel the node should probe on
      //? return;
    }
  }
}
/*---------------------------------------------------------------------------*/
static void keepLpbrList(const uip_ipaddr_t *senderAddr, uip_ipaddr_t nbrAddr, uint8_t chValue, uint8_t pktRecv) {
  struct lpbrList *l; 

  for(l = list_head(lpbrList_table); l != NULL; l = l->next) {
    if(uip_ipaddr_cmp(senderAddr, &l->routeAddr)) {
      if(uip_ipaddr_cmp(&nbrAddr, &l->nbrAddr)) {
        if(chValue == l->chNum) {
	  l->rxValue = pktRecv;
	  return;
        }
      }
    }
  }

  l = memb_alloc(&lpbrList_mem);
  if(l != NULL) {
    uip_ipaddr_copy(&l->routeAddr, senderAddr);
    uip_ipaddr_copy(&l->nbrAddr, &nbrAddr);
    l->chNum = chValue;
    l->rxValue = pktRecv;
    list_add(lpbrList_table, l);
  }
}
/*---------------------------------------------------------------------------*/
static void keepSentRecv(const uip_ipaddr_t *sendToAddr, uint8_t pktSent, uint8_t pktRecv) {
  struct sentRecv *sr; 

  for(sr = list_head(sentRecv_table); sr != NULL; sr = sr->next) {
    printf("keepSentRecv s: %d r: %d ", sr->noSent, sr->noRecv);
    uip_debug_ipaddr_print(&sr->sendToAddr);
    printf("\n");
  }

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
/*static void recheckChChange(struct unicast_message *msg) {
  struct sentRecv *sr; 
  struct unicast_message msg2;

  printf("RECHECK VALUE SENT %d ", msg->value);
  uip_debug_ipaddr_print(msg->addrPtr);
  printf("\n\n");
  for(sr = list_head(sentRecv_table); sr != NULL; sr = sr->next) {
    if(uip_ipaddr_cmp(msg->addrPtr, &sr->sendToAddr)) {
      printf("keepSentRecv SAME s: %d r: %d ", msg->value, sr->noRecv);
      uip_debug_ipaddr_print(&sr->sendToAddr);
      printf("\n");

      //if(msg->value == sr->noRecv) {
      if(msg->value != sr->noRecv) {
	sr->noRecv = 0;
	msg2.address = sr->sendToAddr;
	doSending(&msg2);
      }
    }
  }

  //doSending(&msg2);
}
*/
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
  msg = data;

  static uip_ds6_route_t *r;
  static uip_ds6_nbr_t *nbr;

  struct unicast_message msg2;

  struct lpbrList *l;

  if(msg->type == PROBERESULT) {
    printf("LPBR RECEIVED PROBERESULT: from ");
    uip_debug_ipaddr_print(sender_addr);
    printf(" nbr %d ", ((uint8_t *)packetbuf_addr(PACKETBUF_ADDR_SENDER))[5]) ;
    uip_debug_ipaddr_print(&msg->address);
    printf(" chNum %d rxValue %d\n", msg->value, msg->value2);

////!!!!TO DO
//keepLpbrList(&msg2);

    keepLpbrList(sender_addr, msg->address, msg->value, msg->value2);
    readProbe(1);
  }

  else if(msg->type == CONFIRM_CH) {
    printf("CONFIRM CH Received %d from ", msg->value);
    uip_debug_ipaddr_print(sender_addr);
    printf("\n\n");

    for(r = uip_ds6_route_head(); r != NULL; r = uip_ds6_route_next(r)) {
      if(sender_addr->u8[13] == r->ipaddr.u8[13]) {
      //if(uip_ipaddr_cmp(sender_addr, &r->ipaddr)) {
        r->routeCh = msg->value;
        //printf("UPDATE ROUTING TABLE: ");
	//uip_debug_ipaddr_print(&r->ipaddr);
	//printf(" VIA ");
	//uip_debug_ipaddr_print(uip_ds6_route_nexthop(r));
	//printf(" R->ROUTECH %d\n", r->routeCh);
      }
    }

    //! updates LPBR RT
    for(nbr = nbr_table_head(ds6_neighbors); nbr != NULL;
      nbr = nbr_table_next(ds6_neighbors,nbr)) {
      if(sender_addr->u8[13] == nbr->ipaddr.u8[13]) {
        nbr->nbrCh = msg->value;
        //printf("UPDATE NBR TABLE: ");
        //uip_debug_ipaddr_print(&nbr->ipaddr);
        //printf(" nbr->nbrCh %d ", nbr->nbrCh);
        //printf("\n");
      }
    }

    /*for(r = uip_ds6_route_head(); r != NULL; r = uip_ds6_route_next(r)) {
      printf("RT: ");
      uip_debug_ipaddr_print(&r->ipaddr);
      printf(" routeCh %d", r->routeCh);
      printf("\n");
    }*/

    /*for(nbr = nbr_table_head(ds6_neighbors); nbr != NULL;
      nbr = nbr_table_next(ds6_neighbors,nbr)) {
      printf("NBR TABLE ");
      uip_debug_ipaddr_print(&nbr->ipaddr);
      printf(" channel %d\n", nbr->nbrCh);
    }*/
  }

/*  else if(msg->type == SENTRECV) {
    printf("Received SENTRECV %d from ", msg->value);
    uip_debug_ipaddr_print(sender_addr);
    printf("\n");

    msg2.value = msg->value;
    uip_ipaddr_copy(&msg2.addrPtr, &sender_addr);

    /*if(sender_addr->u8[11] == 2) {
      recheckChChange(&msg2);
    }*/
//    recheckChChange(&msg2);

//  }

  else {
    printf("Data received from ");
    uip_debug_ipaddr_print(sender_addr);
    printf(" on port %d from port %d with length %d: '%s'\n",
         receiver_port, sender_port, datalen, data);

    keepSentRecv(sender_addr, 0, 1);

/*    for(nbr = nbr_table_head(ds6_neighbors); nbr != NULL;
      nbr = nbr_table_next(ds6_neighbors,nbr)) {
      printf("NBR: ");
      uip_debug_ipaddr_print(&nbr->ipaddr);
      printf(" nbrCh %d", nbr->nbrCh);
      printf("\n");
    }

    for(r = uip_ds6_route_head(); r != NULL; r = uip_ds6_route_next(r)) {
      printf("RT: ");
      uip_debug_ipaddr_print(&r->ipaddr);
      printf(" routeCh %d", r->routeCh);
      printf("\n");
    }
*/
  }
}
/*---------------------------------------------------------------------------*/
/*static void
ipaddr_add(const uip_ipaddr_t *addr)
{
  uint16_t a;
  int i, f;
  for(i = 0, f = 0; i < sizeof(uip_ipaddr_t); i += 2) {
    a = (addr->u8[i] << 8) + addr->u8[i + 1];
    if(a == 0 && f >= 0) {
      if(f++ == 0 && sizeof(buf) - blen >= 2) {
        buf[blen++] = ':';
        buf[blen++] = ':';
      }
    } else {
      if(f > 0) {
        f = -1;
      } else if(i > 0 && blen < sizeof(buf)) {
        buf[blen++] = ':';
      }
      ADD("%x", a);
    }
  }
}*/
/*---------------------------------------------------------------------------*/
/*static
PT_THREAD(generate_routes(struct httpd_state *s))
{
  static int i;
  static uip_ds6_route_t *r;
  static uip_ds6_nbr_t *nbr;

  PSOCK_BEGIN(&s->sout);

  SEND_STRING(&s->sout, TOP);

  blen = 0;
  ADD("Neighbors<pre>");
  for(nbr = nbr_table_head(ds6_neighbors);
      nbr != NULL;
      nbr = nbr_table_next(ds6_neighbors, nbr)) {
    ipaddr_add(&nbr->ipaddr);
    ADD("\n");
    if(blen > sizeof(buf) - 45) {
      SEND_STRING(&s->sout, buf);
      blen = 0;
    }
  }

  ADD("</pre>Routes<pre>");
  SEND_STRING(&s->sout, buf);
  blen = 0;
  for(r = uip_ds6_route_head();
      r != NULL;
      r = uip_ds6_route_next(r)) {
    ipaddr_add(&r->ipaddr);
    ADD("/%u (via ", r->length);
    ipaddr_add(uip_ds6_route_nexthop(r));
    if(r->state.lifetime < 600) {
      ADD(") %lus\n", (unsigned long)r->state.lifetime);
    } else {
      ADD(")\n");
    }
    SEND_STRING(&s->sout, buf);
    blen = 0;
  }
  ADD("</pre>");
//if(blen > 0) {
  SEND_STRING(&s->sout, buf);
// blen = 0;
//}

  if(sensor_count > 0) {
    ADD("</pre>Sensors<pre>");
    SEND_STRING(&s->sout, buf);
    blen = 0;
    for(i = 0; i < sensor_count; i++) {
      ADD("%s\n", sensors[i]);
      SEND_STRING(&s->sout, buf);
      blen = 0;
    }
    ADD("</pre>");
    SEND_STRING(&s->sout, buf);
  }


  SEND_STRING(&s->sout, BOTTOM);

  PSOCK_END(&s->sout);
}*/
/*---------------------------------------------------------------------------*/
/*httpd_simple_script_t
httpd_simple_get_script(const char *name)
{
  return generate_routes;
}

#endif*/ /* WEBSERVER */

/*---------------------------------------------------------------------------*/
static void
print_local_addresses(void)
{
  int i;
  uint8_t state;

  PRINTA("Server IPv6 addresses:\n");
  for(i = 0; i < UIP_DS6_ADDR_NB; i++) {
    state = uip_ds6_if.addr_list[i].state;
    if(uip_ds6_if.addr_list[i].isused &&
       (state == ADDR_TENTATIVE || state == ADDR_PREFERRED)) {
      PRINTA(" %p: =>", &uip_ds6_if.addr_list[i]);
      uip_debug_ipaddr_print(&(uip_ds6_if.addr_list[i]).ipaddr);
      PRINTA("\n");
    }
  }
}
/*---------------------------------------------------------------------------*/
static void
request_mac(void)
{
  write_to_slip((uint8_t *)"?M", 2);
}
/*---------------------------------------------------------------------------*/
void
border_router_set_mac(const uint8_t *data)
{
  memcpy(uip_lladdr.addr, data, sizeof(uip_lladdr.addr));
  rimeaddr_set_node_addr((rimeaddr_t *)uip_lladdr.addr);

  /* is this ok - should instead remove all addresses and
     add them back again - a bit messy... ?*/
  uip_ds6_init();
  rpl_init();

  mac_set = 1;
}
/*---------------------------------------------------------------------------*/
void
border_router_print_stat()
{
  printf("bytes received over SLIP: %ld\n", slip_received);
  printf("bytes sent over SLIP: %ld\n", slip_sent);
}

/*---------------------------------------------------------------------------*/
/* Format: <name=value>;<name=value>;...;<name=value>*/
/* this function just cut at ; and store in the sensor array */
void
border_router_set_sensors(const char *data, int len)
{
  int i;
  int last_pos = 0;
  int sc = 0;
  for(i = 0;i < len; i++) {
    if(data[i] == ';') {
      sensors[sc][i - last_pos] = 0;
      memcpy(sensors[sc++], &data[last_pos], i - last_pos);
      last_pos = i + 1; /* skip the ';' */
    }
    if(sc == MAX_SENSORS) {
      sensor_count = sc;
      return;
    }
  }
  sensors[sc][len - last_pos] = 0;
  memcpy(sensors[sc++], &data[last_pos], len - last_pos);
  sensor_count = sc;
}
/*---------------------------------------------------------------------------*/
static void
set_prefix_64(const uip_ipaddr_t *prefix_64)
{
  uip_ipaddr_t ipaddr;
  memcpy(&prefix, prefix_64, 16);
  memcpy(&ipaddr, prefix_64, 16);

  prefix_set = 1;
  uip_ds6_set_addr_iid(&ipaddr, &uip_lladdr);
  uip_ds6_addr_add(&ipaddr, 0, ADDR_AUTOCONF);
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(border_router_process, ev, data)
{
  static struct etimer et;
  rpl_dag_t *dag;

  static struct  etimer changeChTimer;
  static uip_ds6_route_t *r;
  uint8_t number = 1;

  PROCESS_BEGIN();
  prefix_set = 0;

  PROCESS_PAUSE();

  PRINTF("RPL-Border router started\n");

  slip_config_handle_arguments(contiki_argc, contiki_argv);

  /* tun init is also responsible for setting up the SLIP connection */
  tun_init();

  while(!mac_set) {
    etimer_set(&et, CLOCK_SECOND);
    request_mac();
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
  }

  if(slip_config_ipaddr != NULL) {
    uip_ipaddr_t prefix;

    if(uiplib_ipaddrconv((const char *)slip_config_ipaddr, &prefix)) {
      PRINTF("Setting prefix ");
      PRINT6ADDR(&prefix);
      PRINTF("\n");
      set_prefix_64(&prefix);
    } else {
      PRINTF("Parse error: %s\n", slip_config_ipaddr);
      exit(0);
    }
  }

  dag = rpl_set_root(RPL_DEFAULT_INSTANCE,(uip_ip6addr_t *)dag_id);
  if(dag != NULL) {
    rpl_set_prefix(dag, &prefix, 64);
    PRINTF("created a new RPL dag\n");
  }

#if DEBUG
  print_local_addresses();
#endif

  /* The border router runs with a 100% duty cycle in order to ensure high
     packet reception rates. */
  NETSTACK_MAC.off(1);

  simple_udp_register(&unicast_connection, UDP_PORT,
                      NULL, UDP_PORT, receiver);

  //etimer_set(&changeChTimer, 20 * CLOCK_SECOND);
  //60 is 3.25 min. 40 is 2 min (15 nodes) 20 is 2 min (9 nodes)?????
  etimer_set(&changeChTimer, 40 * CLOCK_SECOND);
//  etimer_set(&changeChTimer, 60 * CLOCK_SECOND);
//  etimer_set(&changeChTimer, 100 * CLOCK_SECOND);
  while(1) {
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&changeChTimer));


  //check with all LPBR neighbours
  printf("LPBR ROUTES\n");
  for(r = uip_ds6_route_head(); r != NULL; r = uip_ds6_route_next(r)) {
    printf("%d: ", number);
    uip_debug_ipaddr_print(&r->ipaddr);
    printf("\n");
    number++;
  }

    howManyRoutes();
    sendingTo = sendingTo + 1;
    startChChange(sendingTo);
    //process_post_synch(&chChange_process, event_data_ready, NULL);

    /* do anything here??? */
  }
  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(chChange_process, ev, data)
{
  static struct etimer time;
  struct unicast_message *msg;
  struct unicast_message msg2;
  msg = data;

  uint8_t randomNewCh;
  static uip_ds6_route_t *r;

  static uip_ds6_nbr_t *nbr;
  static uip_ds6_route_t *re;

uint8_t i = 0;
uint8_t prevRand;

uint8_t wTime;
//uint8_t x = 0;

  PROCESS_BEGIN();

  while(1) {
    PROCESS_WAIT_EVENT_UNTIL(ev == event_data_ready);

    //3 sec per nbr
    //equals to 30 secs? (even though it's supposed to be 3 secs) - depends on how many nbr
    //10 = 60; 5 = 6*5 = 10-30?
    //etimer_set(&time, 10 * CLOCK_SECOND); //1 min
//    etimer_set(&time, 3 * CLOCK_SECOND);

    etimer_set(&time, 5 * CLOCK_SECOND);
    //etimer_set(&time, 7 * CLOCK_SECOND);
    PROCESS_YIELD_UNTIL(etimer_expired(&time));

/*etimer_set(&time, 1 * CLOCK_SECOND);
while(wTime == 0 || x <= 5) {
x++;
wTime = waitingTime(&holdAddr);
printf("IN WHILE WTIME %d X %d\n\n", wTime, x);
//etimer_set(&time, 1 * CLOCK_SECOND);
PROCESS_YIELD_UNTIL(etimer_expired(&time));
etimer_reset(&time);
}
x = 0;*/
    //printf("RECHECK for ");
    //uip_debug_ipaddr_print(&holdAddr);
    //printf("\n\n");
    recheck(&holdAddr);
  }
  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(test, ev, data)
{
  static struct etimer time;
  struct unicast_message *msg;
  struct unicast_message msg2;
  msg = data;

  uint8_t randomNewCh;
  static uip_ds6_route_t *r;

  static uip_ds6_nbr_t *nbr;
  static uip_ds6_route_t *re;

uint8_t i = 0;
uint8_t prevRand;

uint8_t wTime;
//uint8_t x = 0;

  PROCESS_BEGIN();

  while(1) {
    PROCESS_WAIT_EVENT_UNTIL(ev == event_data_ready);

doSending(msg);

  }
  PROCESS_END();
}
