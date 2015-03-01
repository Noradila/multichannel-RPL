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
//#define DEBUG DEBUG_ALL
//#define DEBUG 1
#include "net/uip-debug.h"
#include "cmd.h"
#include "slip-radio.h"
#include "packetutils.h"

//ADILA EDIT 25/02/15
#include "net/mac/contikimac.h"
#include "net/mac/simplified_nbr_table.h"
//-------------------

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

//ADILA EDIT 27/02/15
//nbr_table *xxx;
//xxx->theIPAddr = 23;

//-------------------

  PRINTF("Slip-radio: packet sent! sid: %d, status: %d, tx: %d\n",
  	 sid, status, transmissions);

//ADILA EDIT 10/11/14
//reset channel here?
//simplified_nbr_table_get_channel();
printf("%d RESET TO LISTENING CH\n\n", uip_ds6_if.addr_list[1].currentCh);
//cc2420_set_channel(26);
//@
cc2420_set_channel(uip_ds6_if.addr_list[1].currentCh);
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

uint8_t aa = 0;

  if(data[0] == '!') {
    /* should send out stuff to the radio - ignore it as IP */
    /* --- s e n d --- */
    if(data[1] == 'S') {
      int pos;
      packet_ids[packet_pos] = data[2];

      //ADILA EDIT 1/12/14
      if(data[3] != 0) {

//if(packetbuf_attr(PACKETBUF_ADILA2) == 0 || packetbuf_attr(PACKETBUF_ADILA2) > 26) {
//packetbuf_set_attr(PACKETBUF_ADILA2, data[3]);
//}

//packetbuf_set_attr(PACKETBUF_ADILA, data[3]);
//packetbuf_set_attr(PACKETBUF_ADILA_IP, packetbuf_addr(PACKETBUF_ADDR_RECEIVER)->u8[5]);
//packetbuf_set_attr(PACKETBUF_ADILA_IP, ((uint8_t *)packetbuf_addr(PACKETBUF_ADDR_RECEIVER))[5]);
//printf("IP:%d %d DATA[3] is %d\n", packetbuf_attr(PACKETBUF_ADILA_IP), packetbuf_attr(PACKETBUF_ADILA), data[3]);

/*printf("IN SLIP-RADIO-------------\n");
simplified_nbr_table_get_channel();
printf("--------------------------\n\n");
*/
aa = simplified_check_table(data[4], data[3]);
//printf("value aa %d\n\n", aa);
printf("DATA[3] %d DATA[4] %d\n\n", data[3], data[4]);
        //printf("DATA[3] IS %d\n", data[3]);

if(aa != 1) {
simplified_nbr_table_set_channel(data[4], data[3]);
}
//ADILA EDIT 25/02/15
//extern int theval;
//theval = 15;
//printf("%d\n\n", theval);
//-------------------

/*if(((uint8_t *)packetbuf_addr(PACKETBUF_ADDR_RECEIVER))[5] != 0) {
simplified_nbr_table_set_channel(((uint8_t *)packetbuf_addr(PACKETBUF_ADDR_RECEIVER))[5], data[3]);
}*/

	//@
        cc2420_set_channel(data[3]);
      }
      //else {
	//printf("DATA[3] IS 0!!!\n\n");
      //}

      packetbuf_clear();
      //pos = packetutils_deserialize_atts(&data[4], len - 4);
      pos = packetutils_deserialize_atts(&data[5], len - 5);
      if(pos < 0) {
        PRINTF("slip-radio: illegal packet attributes\n");
        return 1;
      }
      //pos += 4;
      pos += 5;
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
      //------------------

/*      packetbuf_clear();
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
/*      no_framer.parse();
      NETSTACK_MAC.send(packet_sent, &packet_ids[packet_pos]);

      packet_pos++;
      if(packet_pos >= sizeof(packet_ids)) {
	packet_pos = 0;
      }

      return 1;*/
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
