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

#define MAX_SENSORS 4

#include "lib/list.h"
#include "lib/memb.h"

#include "simple-udp.h"
#define UDP_PORT 1234
#define SERVICE_ID 190
static struct simple_udp_connection unicast_connection;

//the application specific event value
static process_event_t event_data_ready;

uint8_t a = 0;

struct probeResult {
  struct probeResult *next;
  uip_ipaddr_t routeAddr;
  uip_ipaddr_t nbrAddr;
  uint8_t chNum;
  uint8_t rxValue;
};

LIST(probeResult_table);
MEMB(probeResult_mem, struct probeResult, 20);

enum {
	CH_CHANGE,
	NBR_CH_CHANGE,
	NBRPROBE,
	PROBERESULT,
	CONFIRM_CH,
	GET_ACK
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

//#if WEBSERVER==0
/* No webserver */
AUTOSTART_PROCESSES(&border_router_process,&border_router_cmd_process, &chChange_process);
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
static void readProbe() {
  struct probeResult *pr;

  for(pr = list_head(probeResult_table); pr != NULL; pr = pr->next) {
    printf("LPBR ");
    uip_debug_ipaddr_print(&pr->routeAddr);
    printf(" nbrRoute ");
    uip_debug_ipaddr_print(&pr->nbrAddr);
    printf(" ch %d pktRecv %d\n", pr->chNum, pr->rxValue);
  }
}
/*---------------------------------------------------------------------------*/
static void keepProbeResult(const uip_ipaddr_t *routeAddr, const uip_ipaddr_t *nbrAddr, uint8_t chN, uint8_t pktRecv) {
  struct probeResult *pr;

  for(pr = list_head(probeResult_table); pr != NULL; pr = pr->next) {
    if(uip_ipaddr_cmp(routeAddr, &pr->routeAddr)) {
      //if(uip_ipaddr_cmp(nbrAddr, &pr->nbrAddr)) {
       // if(pr->chNum == chN) {
          //pr->rxValue = pktRecv;
	  return;
	//}
      //}
    }
  }

  pr = memb_alloc(&probeResult_mem);
  if(pr != NULL) {
    pr->rxValue = pktRecv;
    pr->chNum = chN;
    uip_ipaddr_copy(&pr->routeAddr, routeAddr);
    uip_ipaddr_copy(&pr->nbrAddr, nbrAddr);
    list_add(probeResult_table, pr);
  }

  readProbe();
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
  msg = data;

  if(msg->type == PROBERESULT) {
    printf("LPBR RECEIVED PROBERESULT: from ");
    uip_debug_ipaddr_print(sender_addr);
    printf(" nbr ");
    uip_debug_ipaddr_print(&msg->address);
    printf(" chNum %d rxValue %d\n", msg->value, msg->value2);

    //keepProbeResult(sender_addr, msg->addrPtr, msg->value, msg->value2);
  }

  else {
  printf("Data received from ");
  uip_debug_ipaddr_print(sender_addr);
  printf(" on port %d from port %d with length %d: '%s'\n",
         receiver_port, sender_port, datalen, data);

  readProbe();
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

  while(1) {
    a++;
    etimer_set(&et, CLOCK_SECOND * 2);
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));

    if(a == 5) {
      process_post_synch(&chChange_process, event_data_ready, NULL);
    }
    /* do anything here??? */
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(chChange_process, ev, data)
{
  static struct etimer time;
  struct unicast_message msg2;
  uint8_t randomNewCh;
  static uip_ds6_route_t *r;

  PROCESS_BEGIN();

  while(1) {
    PROCESS_WAIT_EVENT_UNTIL(ev == event_data_ready);

    /*for(r = uip_ds6_route_head(); r != NULL; 
	r = uip_ds6_route_next(r)) {
	printf("ROUTE: ");
	uip_debug_ipaddr_print(&r->ipaddr);
	printf(" via ");
	uip_debug_ipaddr_print(uip_ds6_route_nexthop(r));
	//printf(" newCh %d probeRecv %d checkCh %d", r->newCh, r->probeRecv, r->checkCh);
	printf(" nbrCh %d", r->nbrCh);
	printf("\n");
    }*/

    for(r = uip_ds6_route_head(); r != NULL; 
	r = uip_ds6_route_next(r)) {

	randomNewCh = random_rand() % 16 + 11;
	//! check if randomNewCh is blacklisted (if it's on the list, low success rate)

	msg2.type = CH_CHANGE;
	msg2.value = randomNewCh;
	msg2.address = r->ipaddr;

	printf("%d: %d BR Sending channel to change for ", sizeof(msg2), msg2.value);
	uip_debug_ipaddr_print(&r->ipaddr);
	printf(" via ");
	uip_debug_ipaddr_print(uip_ds6_route_nexthop(r));
	printf("\n");

	simple_udp_sendto(&unicast_connection, &msg2, sizeof(msg2) + 1, &r->ipaddr);

	//equals to 30 secs? (even though it's supposed to be 3 secs
	//etimer_set(&time, 6 * CLOCK_SECOND);
	etimer_set(&time, 4 * CLOCK_SECOND);
	PROCESS_YIELD_UNTIL(etimer_expired(&time));
    }
  }
  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
