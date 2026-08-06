#include "w55_cmd_decoder.h"
#include "stdlib.h"
#include "FreeRTOS_W5500.h"

extern const W5500TxItem_t xW55EchoItem;

ADD_CMD( PORT7_CMD1 ,"PING"   );

//------------------------------------------------------------------------
const W55DispatchItem_s xCmdListPort7[] = {
  EXPAND_CMD( PORT7_CMD1 ),
};

void run_PORT7_CMD1 (char** ppcArgv, uint8_t ucArgc) {
  vFreeRTOSW5500Transmit(&xW55EchoItem, 7);
  
}

void run_PORT7_CMD2 (char** ppcArgv, uint8_t ucArgc) {
  
}

/*************************************************************************/
/*****************************< Don't Touch >*****************************/
const uint16_t usCmdListPort7Len = sizeof(xCmdListPort7) / sizeof(*xCmdListPort7);