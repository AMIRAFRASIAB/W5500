#include "w55_cmd_decoder.h"
#include "stdlib.h"
#include "FreeRTOS_W5500.h"

extern const W5500TxItem_t xW55EchoItem;

ADD_CMD( PORT0_CMD1 ,"PING"   );

//------------------------------------------------------------------------
const W55DispatchItem_s xCmdListPort0[] = {
  EXPAND_CMD( PORT0_CMD1 ),
};

void run_PORT0_CMD1 (char** ppcArgv, uint8_t ucArgc) {
  vFreeRTOSW5500Transmit(&xW55EchoItem, 0);
  
}

void run_PORT0_CMD2 (char** ppcArgv, uint8_t ucArgc) {
  
}

/*************************************************************************/
/*****************************< Don't Touch >*****************************/
const uint16_t usCmdListPort0Len = sizeof(xCmdListPort0) / sizeof(*xCmdListPort0);