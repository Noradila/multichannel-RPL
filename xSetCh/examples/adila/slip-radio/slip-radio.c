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
 */

/**
 * \file
 *         Slip-radio driver
 * \author
 *         Niclas Finne <nfi@sics.se>
 *         Joakim Eriksson <joakime@sics.se>
 */
#include "contiki.h"
#include "net/uip.h"
#include "net/uip-ds6.h"
#include "dev/slip.h"
#include <string.h>
#include "net/netstack.h"
#include "net/packetbuf.h"

#define DEBUG DEBUG_NONE
#define DEBUG 1
#include "net/uip-debug.h"
#include "cmd.h"
#include "slip-radio.h"
#include "packetutils.h"

#ifdef SLIP_RADIO_CONF_SENSORS
extern const struct slip_radio_sensors SLIP_RADIO_CONF_SENSORS;
#endif

void slip_send_packet(const uint8_t *ptr, int len);

 /* max 16 packets at the same time??? */
uint8_t packet_ids[16];
int packet_pos;

static int slip_radio_cmd_handler(const uint8_t *data, int len);
int cmd_handler_cc2420(const uint8_t *data, int len);
/*---------------------------------------------------------------------------*/
#ifdef CMD_CONF_HANDLERS
CMD_HANDLERS(CMD_CONF_HANDLERS);
#else
CMD_HANDLERS(slip_radio_cmd_handler);
#endif
/*---------------------------------------------------------------------------*/
static void
packet_sent(void *ptr, int status, int transmissions)
{
  uint8_t buf[20];
  uint8_t sid;
  int pos;
  sid = *((uint8_t *)ptr);

  PRINTF("Slip-radio: packet sent! sid: %d, status: %d, tx: %d\n",
  	 sid, status, transmissions);

//ADILA EDIT 10/11/14
//reset channel here?
printf("RESET TO LISTENING CH\n\n");
cc2420_set_channel(26);
//-------------------


  /* packet callback from lower layers */
  /*  neighbor_info_packet_sent(status, transmissions); */
  pos = 0;
//  buf[pos++] = '!';
//  buf[pos++] = 'R';
  buf[pos++] = sid;
  buf[pos++] = status; /* one byte ? */
  buf[pos++] = transmissions;
  cmd_send(buf, pos);
}
/*---------------------------------------------------------------------------*/
static int
slip_radio_cmd_handler(const uint8_t *data, int len)
{
  int i;
  if(data[0] == '!') {
    /* should send out stuff to the radio - ignore it as IP */
    /* --- s e n d --- */
    if(data[1] == 'S') {
      int pos;
      packet_ids[packet_pos] = data[2];

      packetbuf_clear();
      pos = packetutils_deserialize_atts(&data[3], len - 3);
      if(pos < 0) {
        PRINTF("slip-radio: illegal packet attributes\n");
        return 1;
      }
      pos += 3;
      len -= pos;
      if(len > PACKETBUF_SIZE) {
        len = PACKETBUF_SIZE;
      }
      memcpy(packetbuf_dataptr(), &data[pos], len);
      packetbuf_set_datalen(len);

      PRINTF("slip-radio: sending %u (%d bytes)\n",
             data[2], packetbuf_datalen());
      printf("RADIO sending packet\n");

      /* parse frame before sending to get addresses, etc. */
      no_framer.parse();
      NETSTACK_MAC.send(packet_sent, &packet_ids[packet_pos]);

      packet_pos++;
      if(packet_pos >= sizeof(packet_ids)) {
	packet_pos = 0;
      }

      return 1;
    }
  } else if(uip_buf[0] == '?') {
    PRINTF("Got request message of type %c\n", uip_buf[1]);
    if(data[1] == 'M') {
      /* this is just a test so far... just to see if it works */
      uip_buf[0] = '!';
      uip_buf[1] = 'M';
      for(i = 0; i < 8; i++) {
        uip_buf[2 + i] = uip_lladdr.addr[i];
      }
      uip_len = 10;
      cmd_send(uip_buf, uip_len);
      return 1;
    }
  }
  return 0;
}
/*---------------------------------------------------------------------------*/
void
slip_radio_cmd_output(const uint8_t *data, int data_len)
{
  slip_send_packet(data, data_len);
}
/*---------------------------------------------------------------------------*/
static void
slip_input_callback(void)
{

//ADILA EDIT 10/11/14
  static uip_ds6_route_t *r;
  static uip_ds6_nbr_t *nbr;

/*printf(" addr ");
uip_debug_ipaddr_print(PACKETBUF_ADDR_SENDER);
printf(" addr ");
uip_debug_ipaddr_print(PACKETBUF_ADDR_RECEIVER);
printf("\n\n");*/
//printf("PRINT? rime[0] %d rime[1] %d ", rimeaddr_node_addr.u8[0], rimeaddr_node_addr.u8[1]);

    printf("contikimac: send unicast to %u.%u\n",
               packetbuf_addr(PACKETBUF_ADDR_RECEIVER)->u8[0],
               packetbuf_addr(PACKETBUF_ADDR_RECEIVER)->u8[1]);

    printf("contikimac: send unicast to %02x%02x:%02x%02x:%02x%02x:%02x%02x\n",
               packetbuf_addr(PACKETBUF_ADDR_SENDER)->u8[0],
               packetbuf_addr(PACKETBUF_ADDR_SENDER)->u8[1],
               packetbuf_addr(PACKETBUF_ADDR_SENDER)->u8[2],
               packetbuf_addr(PACKETBUF_ADDR_SENDER)->u8[3],
               packetbuf_addr(PACKETBUF_ADDR_SENDER)->u8[4],
               packetbuf_addr(PACKETBUF_ADDR_SENDER)->u8[5],
               packetbuf_addr(PACKETBUF_ADDR_SENDER)->u8[6],
               packetbuf_addr(PACKETBUF_ADDR_SENDER)->u8[7]);
/*
printf("PRINT? ");
    printf("contikimac: send unicast to %02x%02x:%02x%02x:%02x%02x:%02x%02x\n",
               packetbuf_addr(PACKETBUF_ADDR_RECEIVER)->u8[0],
               packetbuf_addr(PACKETBUF_ADDR_RECEIVER)->u8[1],
               packetbuf_addr(PACKETBUF_ADDR_RECEIVER)->u8[2],
               packetbuf_addr(PACKETBUF_ADDR_RECEIVER)->u8[3],
               packetbuf_addr(PACKETBUF_ADDR_RECEIVER)->u8[4],
               packetbuf_addr(PACKETBUF_ADDR_RECEIVER)->u8[5],
               packetbuf_addr(PACKETBUF_ADDR_RECEIVER)->u8[6],
               packetbuf_addr(PACKETBUF_ADDR_RECEIVER)->u8[7]);
*/
nbr = nbr_table_head(ds6_neighbors);
uip_debug_ipaddr_print(&nbr->ipaddr);
	printf("nbr->ipaddr.u8[0] %d %d %d %d %d %d\n\n", nbr->ipaddr.u8[0], nbr->ipaddr.u8[1], nbr->ipaddr.u8[2], nbr->ipaddr.u8[3], nbr->ipaddr.u8[4], nbr->ipaddr.u8[5]);
	for(nbr = nbr_table_head(ds6_neighbors);
	  nbr != NULL;
	  nbr = nbr_table_next(ds6_neighbors, nbr)) {
	uip_debug_ipaddr_print(&nbr->ipaddr);
	printf(" via ");
	//uip_debug_ipaddr_print(nbr_table_nexthop(nbr));
	printf("\n");
	}

/*    for(r = uip_ds6_route_head(); r != NULL; 
	r = uip_ds6_route_next(r)) {
	printf("%d ROUTE: ", r->nbrCh);
	uip_debug_lladdr_print(&r->ipaddr);
	printf(" via ");
	uip_debug_lladdr_print(uip_ds6_route_nexthop(r));
	printf("\n");
    }*/
r = uip_ds6_route_head();
//change to tx channel?
printf("CHANGE TO TX CHANNEL ");
uip_debug_ipaddr_print(&uip_ds6_if.addr_list[1].ipaddr);
printf(" ");
uip_debug_ipaddr_print(uip_ds6_route_nexthop(&uip_ds6_if.addr_list[1].ipaddr));
//uip_debug_ipaddr_print(&r->ipaddr);
printf("\n\n");
//printf("%d.%d.%d.%d.%d.%d.%d.%d.%d", uip_lladdr.addr[0], uip_lladdr.addr[1], uip_lladdr.addr[2], uip_lladdr.addr[3], uip_lladdr.addr[4], uip_lladdr.addr[5], uip_lladdr.addr[6], uip_lladdr.addr[7], uip_lladdr.addr[8]);
//printf(" buf %d.%d.%d.%d.%d.%d.%d.%d.%d buf LLADDR?\n\n", uip_buf[2], uip_buf[3], uip_buf[4], uip_buf[5], uip_buf[6], uip_buf[7], uip_buf[8], uip_buf[9], uip_buf[10]);

//-------------------

  PRINTF("SR-SIN: %u '%c%c'\n", uip_len, uip_buf[0], uip_buf[1]);
  cmd_input(uip_buf, uip_len);
  uip_len = 0;
}
/*---------------------------------------------------------------------------*/
static void
init(void)
{
#ifndef BAUD2UBR
#define BAUD2UBR(baud) baud
#endif
  slip_arch_init(BAUD2UBR(115200));
  process_start(&slip_process, NULL);
  slip_set_input_callback(slip_input_callback);
  packet_pos = 0;
}
/*---------------------------------------------------------------------------*/
#if !SLIP_RADIO_CONF_NO_PUTCHAR
#undef putchar
int
putchar(int c)
{
#define SLIP_END     0300
  static char debug_frame = 0;

  if(!debug_frame) {            /* Start of debug output */
    slip_arch_writeb(SLIP_END);
    slip_arch_writeb('\r');     /* Type debug line == '\r' */
    debug_frame = 1;
  }

  /* Need to also print '\n' because for example COOJA will not show
     any output before line end */
  slip_arch_writeb((char)c);

  /*
   * Line buffered output, a newline marks the end of debug output and
   * implicitly flushes debug output.
   */
  if(c == '\n') {
    slip_arch_writeb(SLIP_END);
    debug_frame = 0;
  }
  return c;
}
#endif
/*---------------------------------------------------------------------------*/
PROCESS(slip_radio_process, "Slip radio process");
AUTOSTART_PROCESSES(&slip_radio_process);
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(slip_radio_process, ev, data)
{
  static struct etimer et;
  PROCESS_BEGIN();

  init();
  NETSTACK_RDC.off(1);
#ifdef SLIP_RADIO_CONF_SENSORS
  SLIP_RADIO_CONF_SENSORS.init();
#endif
  printf("Slip Radio started...\n");

  etimer_set(&et, CLOCK_SECOND * 3);

  while(1) {
    PROCESS_YIELD();

    if(etimer_expired(&et)) {
      etimer_reset(&et);
#ifdef SLIP_RADIO_CONF_SENSORS
      SLIP_RADIO_CONF_SENSORS.send();
#endif
    }
  }
  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
