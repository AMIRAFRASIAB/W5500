#include "w55_cmd_decoder.h"
#include "stdlib.h"
#include "FreeRTOS_W5500.h"

extern const W5500TxItem_t xW55EchoItem;

ADD_CMD( PORT4_CMD1 ,"PING"   );

//------------------------------------------------------------------------
const W55DispatchItem_s xCmdListPort4[] = {
  EXPAND_CMD( PORT4_CMD1 ),
};

void run_PORT4_CMD1 (char** ppcArgv, uint8_t ucArgc) {
  vFreeRTOSW5500Transmit(&xW55EchoItem, 4);
  
}

void run_PORT4_CMD2 (char** ppcArgv, uint8_t ucArgc) {
  
}

/*************************************************************************/
/*****************************< Don't Touch >*****************************/
const uint16_t usCmdListPort4Len = sizeof(xCmdListPort4) / sizeof(*xCmdListPort4);