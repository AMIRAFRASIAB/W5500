
#ifndef __FREE_RTOS_W5500_H
#define __FREE_RTOS_W5500_H

#ifdef __cplusplus
  extern "C" {
#endif //__cplusplus

#include <stdint.h>
#include <stdbool.h>
#include "w5500_client.h"

bool FreeRTOS_w5500_client_inti (W5500_Cnf_t* cnf);
uint32_t FreeRTOS_w5500_client_transmit (uint8_t* buf, uint16_t len, uint32_t ticksToWait);
uint32_t FreeRTOS_w5500_client_receive (uint8_t* buf, uint8_t len, uint32_t ticksToWait);

#ifdef __cplusplus
  }
#endif //__cplusplus  

#endif //__FREE_RTOS_W5500_H