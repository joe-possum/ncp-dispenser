/***************************************************************************//**
 * @file
 * @brief Event handling and application code for Empty NCP Host application example
 *******************************************************************************
 * # License
 * <b>Copyright 2018 Silicon Laboratories Inc. www.silabs.com</b>
 *******************************************************************************
 *
 * The licensor of this software is Silicon Laboratories Inc. Your use of this
 * software is governed by the terms of Silicon Labs Master Software License
 * Agreement (MSLA) available at
 * www.silabs.com/about-us/legal/master-software-license-agreement. This
 * software is distributed to you in Source Code format and is governed by the
 * sections of the MSLA applicable to Source Code.
 *
 ******************************************************************************/

/* standard library headers */
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdlib.h>

/* BG stack headers */
#include "bg_types.h"
#include "gecko_bglib.h"

/* Own header */
#include "app.h"
#include "dump.h"

// App booted flag
static bool appBooted = false;
static struct {
  bd_addr remote;
  uint16 rate0, rate1;
  uint8 connection;
} config = { .remote = { .addr = {0,0,0,0,0,0}}, .rate0 = 0, .rate1 = 0, }; 
  
void parse_address(const char *fmt,bd_addr *address) {
  char buf[3];
  int octet;
  for(uint8 i = 0; i < 6; i++) {
    memcpy(buf,&fmt[3*i],2);
    buf[2] = 0;
    sscanf(buf,"%02x",&octet);
    address->addr[5-i] = octet;
  }
}

const char *getAppOptions(void) {
  return "a<remote-address>0<rate0-handle>1<rate1-handle>";
}

void appOption(int option, const char *arg) {
  switch(option) {
  case '0':
    config.rate0 = atoi(arg);
    break;
  case '1':
    config.rate1 = atoi(arg);
    break;
  case 'a':
    parse_address(arg,&config.remote);
    break;
  default:
    fprintf(stderr,"Unhandled option '-%c'\n",option);
    exit(1);
  }
}

void appInit(void) {
  for(int i = 0; i < 6; i++) {
    if(config.remote.addr[i]) return;
  }
  printf("Usage: master -a <address>\n");
  exit(1);
}

/***********************************************************************************************//**
 *  \brief  Event handler function.
 *  \param[in] evt Event pointer.
 **************************************************************************************************/
void appHandleEvents(struct gecko_cmd_packet *evt)
{
  if (NULL == evt) {
    return;
  }

  // Do not handle any events until system is booted up properly.
  if ((BGLIB_MSG_ID(evt->header) != gecko_evt_system_boot_id)
      && !appBooted) {
#if defined(DEBUG)
    printf("Event: 0x%04x\n", BGLIB_MSG_ID(evt->header));
#endif
    usleep(50000);
    return;
  }

  /* Handle events */
#ifdef DUMP
  dump_event(evt);
#endif
  switch (BGLIB_MSG_ID(evt->header)) {
  case gecko_evt_system_boot_id:

    appBooted = true;
    gecko_cmd_le_gap_connect(config.remote,le_gap_address_type_public,le_gap_phy_1m);
    break;

  case gecko_evt_le_connection_opened_id: /***************************************************************** le_connection_opened **/
#define ED evt->data.evt_le_connection_opened
    config.connection = ED.connection;
    break;
#undef ED

  case gecko_evt_gatt_mtu_exchanged_id: /********************************************************************* gatt_mtu_exchanged **/
#define ED evt->data.evt_gatt_mtu_exchanged
    if(config.rate0) {
      gecko_cmd_gatt_set_characteristic_notification(ED.connection,config.rate0,gatt_notification);
    } else if(config.rate1) {
	gecko_cmd_gatt_set_characteristic_notification(ED.connection,config.rate1,gatt_notification);
    }
    break;
#undef ED

  case gecko_evt_le_connection_closed_id:
    exit(1);
    break;

  default:
    break;
  }
}
