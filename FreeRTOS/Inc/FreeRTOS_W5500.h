
#ifndef __FREE_RTOS_W5500_H
#define __FREE_RTOS_W5500_H

#ifdef __cplusplus
  extern "C" {
#endif //__cplusplus

#include <stdint.h>
#include <stdbool.h>
#include "w5500_client.h"

typedef struct {
  uint8_t*    pucAddr;
  uint32_t    ulLen;
} W5500TxItem_t;

typedef enum {
  eW5500StateTryForConnection,
  eW5500StateTransiver,
  eW5500StateDisconnect,
} W5500State_e;

W5500State_e xFreeRTOSW5500GetTaskState (void);
bool bFreeRTOSW5500ClientInit (W5500_Cnf_t* pxConfig, uint8_t ucPhyConfigIndex);
uint32_t ulFreeRTOSW5500ClientReceive (uint8_t* buf, uint8_t len, uint32_t ticksToWait);
void vFreeRTOSW5500TaskDisable (void);
void vFreeRTOSW5500Transmit (W5500TxItem_t* pxItem);

#ifdef __cplusplus
  }
#endif //__cplusplus  

#endif //__FREE_RTOS_W5500_H