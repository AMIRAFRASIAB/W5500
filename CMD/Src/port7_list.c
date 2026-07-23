
#include "w55_cmd_decoder.h"
#include "stdlib.h"


ADD_CMD( PORT7_CMD1 ,"CMD1"   );  
//------------------------------------------------------------------------
const W55DispatchItem_s xCmdListPort7[] = {
  EXPAND_CMD( PORT7_CMD1 ),
};

void run_PORT7_CMD1 (char** ppcArgv, uint8_t ucArgc) {
  
}


/*************************************************************************/
/*****************************< Don't Touch >*****************************/
const uint16_t usCmdListPort7Len = sizeof(xCmdListPort7) / sizeof(*xCmdListPort7);

