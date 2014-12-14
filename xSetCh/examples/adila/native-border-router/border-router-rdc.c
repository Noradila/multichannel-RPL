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
 *         A null RDC implementation that uses framer for headers and sends
 *         the packets over slip instead of radio.
 * \author
 *         Adam Dunkels <adam@sics.se>
 *         Joakim Eriksson <joakime@sics.se>
 *         Niclas Finne <nfi@sics.se>
 */

#include "net/packetbuf.h"
#include "net/queuebuf.h"
#include "net/netstack.h"
#include "packetutils.h"
#include "border-router.h"
#include <string.h>

#include "net/uip-ds6.h"

#define DEBUG 0
//#define DEBUG 1
#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

#define PRINTADDR(addr) printf(" %02x%02x:%02x%02x:%02x%02x:%02x%02x ", ((uint8_t *)addr)[0], ((uint8_t *)addr)[1], ((uint8_t *)addr)[2], ((uint8_t *)addr)[3], ((uint8_t *)addr)[4], ((uint8_t *)addr)[5], ((uint8_t *)addr)[6], ((uint8_t *)addr)[7])

#define MAX_CALLBACKS 16
static int callback_pos;

/* a structure for calling back when packet data is coming back
   from radio... */
struct tx_callback {
  mac_callback_t cback;
  void *ptr;
  struct packetbuf_attr attrs[PACKETBUF_NUM_ATTRS];
  struct packetbuf_addr addrs[PACKETBUF_NUM_ADDRS];
};

static struct tx_callback callbacks[MAX_CALLBACKS];
/*---------------------------------------------------------------------------*/
void packet_sent(uint8_t sessionid, uint8_t status, uint8_t tx)
{
  if(sessionid < MAX_CALLBACKS) {
    struct tx_callback *callback;
    callback = &callbacks[sessionid];
    packetbuf_clear();
    packetbuf_attr_copyfrom(callback->attrs, callback->addrs);
    mac_call_sent_callback(callback->cback, callback->ptr, status, tx);
  } else {
    PRINTF("*** ERROR: too high session id %d\n", sessionid);
  }
}
/*---------------------------------------------------------------------------*/
static int
setup_callback(mac_callback_t sent, void *ptr)
{
  struct tx_callback *callback;
  int tmp = callback_pos;
  callback = &callbacks[callback_pos];
  callback->cback = sent;
  callback->ptr = ptr;
  packetbuf_attr_copyto(callback->attrs, callback->addrs);

  callback_pos++;
  if(callback_pos >= MAX_CALLBACKS) {
    callback_pos = 0;
  }

  return tmp;
}
/*---------------------------------------------------------------------------*/
static void
send_packet(mac_callback_t sent, void *ptr)
{
  int size;
  /* 3 bytes per packet attribute is required for serialization */
  //uint8_t buf[PACKETBUF_NUM_ATTRS * 3 + PACKETBUF_SIZE + 3];

  //ADILA EDIT 1/12/14
  static uip_ds6_route_t *r;
  uip_ipaddr_t nH;

  static uip_ds6_nbr_t *nbr;

  uint8_t buf[PACKETBUF_NUM_ATTRS * 4 + PACKETBUF_SIZE + 4];
  //------------------

  uint8_t sid;

  packetbuf_set_addr(PACKETBUF_ADDR_SENDER, &rimeaddr_node_addr);

  /* ack or not ? */
  packetbuf_set_attr(PACKETBUF_ATTR_MAC_ACK, 1);

  if(NETSTACK_FRAMER.create() < 0) {
    /* Failed to allocate space for headers */
    PRINTF("br-rdc: send failed, too large header\n");
    mac_call_sent_callback(sent, ptr, MAC_TX_ERR_FATAL, 1);

  } else {
    /* here we send the data over SLIP to the radio-chip */
    size = 0;

    //reconstruct the local IP from MAC address
    uip_ip6addr_u8(&nH, 0xfe, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, ((uint8_t *)packetbuf_addr(PACKETBUF_ADDR_RECEIVER))[1], ((uint8_t *)packetbuf_addr(PACKETBUF_ADDR_RECEIVER))[2], ((uint8_t *)packetbuf_addr(PACKETBUF_ADDR_RECEIVER))[3], ((uint8_t *)packetbuf_addr(PACKETBUF_ADDR_RECEIVER))[4], ((uint8_t *)packetbuf_addr(PACKETBUF_ADDR_RECEIVER))[5], ((uint8_t *)packetbuf_addr(PACKETBUF_ADDR_RECEIVER))[6], ((uint8_t *)packetbuf_addr(PACKETBUF_ADDR_RECEIVER))[7]);

#if SERIALIZE_ATTRIBUTES
    //size = packetutils_serialize_atts(&buf[3], sizeof(buf) - 3);

    //ADILA EDIT 1/12/14
    size = packetutils_serialize_atts(&buf[4], sizeof(buf) - 4);
    //------------------

#endif
    //if(size < 0 || size + packetbuf_totlen() + 3 > sizeof(buf)) {

    //ADILA EDIT 1/12/14
    if(size < 0 || size + packetbuf_totlen() + 4 > sizeof(buf)) {
    //------------------
      PRINTF("br-rdc: send failed, too large header\n");
      mac_call_sent_callback(sent, ptr, MAC_TX_ERR_FATAL, 1);
    } else {
      sid = setup_callback(sent, ptr);

      buf[0] = '!';
      buf[1] = 'S';
      buf[2] = sid; /* sequence or session number for this packet */

      //ADILA EDIT 1/12/14
      //! if receiver MAC address is not 0000.0000.0000.0000
      //! it's supposed to be 0021.7402.0002.0202
      if(((uint8_t *)packetbuf_addr(PACKETBUF_ADDR_RECEIVER))[5] != 0) {

	for(nbr = nbr_table_head(ds6_neighbors); nbr != NULL;
	  nbr = nbr_table_next(ds6_neighbors,nbr)) {

	  if(uip_ipaddr_cmp(&nbr->ipaddr, &nH)) {
		  printf("NBR->NEWCH %d\n\n", nbr->newCh);
		  buf[3] = nbr->newCh;
		  break;
	    }
	}


        /*for(r = uip_ds6_route_head(); r != NULL; 
	  r = uip_ds6_route_next(r)) {

	  if(uip_ipaddr_cmp(&nH, uip_ds6_route_nexthop(r))) {
	    //printf("\n\nSAME ");
	    //uip_debug_ipaddr_print(uip_ds6_route_nexthop(r));
	    //printf(" nH ");
	    //uip_debug_ipaddr_print(&nH);
	    //printf("\n\n");

	    //! get the transmitting channel from routing table
	    buf[3] = r->nbrCh;
	    break;
	  }
        }*/
      }
      else {
	buf[3] = 0;
      }

      //printf("\n\n%d SEND DATA OVER SLIP TO RADIO-CHIP %x addr is ",buf[3], ((uint8_t *)packetbuf_addr(PACKETBUF_ADDR_RECEIVER))[5]);

      memcpy(&buf[4 + size], packetbuf_hdrptr(), packetbuf_totlen());

      write_to_slip(buf, packetbuf_totlen() + size + 4);

//------------------

      /* Copy packet data */
      //memcpy(&buf[3 + size], packetbuf_hdrptr(), packetbuf_totlen());

      //write_to_slip(buf, packetbuf_totlen() + size + 3);
    }
  }
}
/*---------------------------------------------------------------------------*/
static void
send_list(mac_callback_t sent, void *ptr, struct rdc_buf_list *buf_list)
{
  if(buf_list != NULL) {
    queuebuf_to_packetbuf(buf_list->buf);
    send_packet(sent, ptr);
  }
}
/*---------------------------------------------------------------------------*/
static void
packet_input(void)
{
  if(NETSTACK_FRAMER.parse() < 0) {
    PRINTF("br-rdc: failed to parse %u\n", packetbuf_datalen());
  } else {
    NETSTACK_MAC.input();
  }
}
/*---------------------------------------------------------------------------*/
static int
on(void)
{
  return 1;
}
/*---------------------------------------------------------------------------*/
static int
off(int keep_radio_on)
{
  return 1;
}
/*---------------------------------------------------------------------------*/
static unsigned short
channel_check_interval(void)
{
  return 0;
}
/*---------------------------------------------------------------------------*/
static void
init(void)
{
  callback_pos = 0;
}
/*---------------------------------------------------------------------------*/
const struct rdc_driver border_router_rdc_driver = {
  "br-rdc",
  init,
  send_packet,
  send_list,
  packet_input,
  on,
  off,
  channel_check_interval,
};
/*---------------------------------------------------------------------------*/
