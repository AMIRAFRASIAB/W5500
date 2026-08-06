
#ifndef __FREE_RTOS_W5500_H
#define __FREE_RTOS_W5500_H

#ifdef __cplusplus
  extern "C" {
#endif //__cplusplus

#include <stdint.h>
#include <stdbool.h>
#include "w5500_client.h"

typedef struct {
  const uint8_t* pucAddr;
  uint32_t       ulLen;
} W5500TxItem_t;

typedef enum {
  eW5500StateDisconnect,
  eW5500StateTryForConnection,
  eW5500StateTransiver,
} W5500State_e;

bool bFreeRTOSW5500ClientInit (void);
void vFreeRTOSW5500Transmit (const W5500TxItem_t* pxItem, uint8_t ucSocketNumber);
void vFreeRTOSW5500IrqHook (void);
W5500State_e xFreeRTOSW5500GetTaskState (uint8_t ucSocketNumber);
uint32_t ulFreeRTOSW5500ClientReceive (uint8_t* buf, uint8_t len, uint32_t ticksToWait, uint8_t ucSocketNumber);
void vFreeRTOSW5500TaskDisable (uint8_t ucSocketNumber);


#ifdef __cplusplus
  }
#endif //__cplusplus  

#endif //__FREE_RTOS_W5500_H