
#include "w55_cmd_decoder.h"
#include "stdlib.h"

ADD_CMD( PORT1_CMD1 ,"CMD1"   );  
//------------------------------------------------------------------------
const W55DispatchItem_s xCmdListPort1[] = {
  EXPAND_CMD( PORT1_CMD1 ),
};

void run_PORT1_CMD1 (char** ppcArgv, uint8_t ucArgc) {

}

/*************************************************************************/
/*****************************< Don't Touch >*****************************/
const uint16_t usCmdListPort1Len = sizeof(xCmdListPort1) / sizeof(*xCmdListPort1);

