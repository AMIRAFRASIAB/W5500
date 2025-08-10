
#ifndef __W5500_CLIENT_H_
#define __W5500_CLIENT_H_

#ifdef __cpluplus
  extern "C" {
#endif //__cpluplus  

#include <stdint.h>
#include <stdbool.h>
#include "wizchip_conf.h"



typedef struct __W5500_Cnf_s {
  wiz_NetInfo   info;
  uint8_t       dest_ip[4];
  uint16_t      port;
} W5500_Cnf_t;

bool w5500_client_init (const W5500_Cnf_t* INFO);
bool w5500_cable_getStatus (uint8_t tries, uint16_t delay);
int32_t w5500_client_transmit (uint8_t* buf, uint16_t len);
uint16_t w5500_client_receive (uint8_t* buf, uint16_t len);
bool w5500_client_is_connected (void);
bool w5500_client_reconnect (const W5500_Cnf_t* INFO);

#ifdef __cpluplus
  }
#endif //__cpluplus  
#endif //__W5500_CLIENT_H_