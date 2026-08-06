#include "w55_cmd_decoder.h"
#include "stdlib.h"
#include "FreeRTOS_W5500.h"

extern const W5500TxItem_t xW55EchoItem;

ADD_CMD( PORT6_CMD1 ,"PING"   );

//------------------------------------------------------------------------
const W55DispatchItem_s xCmdListPort6[] = {
  EXPAND_CMD( PORT6_CMD1 ),
};

void run_PORT6_CMD1 (char** ppcArgv, uint8_t ucArgc) {
  vFreeRTOSW5500Transmit(&xW55EchoItem, 6);
  
}

void run_PORT6_CMD2 (char** ppcArgv, uint8_t ucArgc) {
  
}

/*************************************************************************/
/*****************************< Don't Touch >*****************************/
const uint16_t usCmdListPort6Len = sizeof(xCmdListPort6) / sizeof(*xCmdListPort6);