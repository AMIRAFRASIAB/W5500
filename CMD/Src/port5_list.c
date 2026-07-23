
#include "w55_cmd_decoder.h"
#include "stdlib.h"


ADD_CMD( PORT5_CMD1 ,"CMD1"   );  
//------------------------------------------------------------------------
const W55DispatchItem_s xCmdListPort5[] = {
  EXPAND_CMD( PORT5_CMD1 ),
};

void run_PORT5_CMD1 (char** ppcArgv, uint8_t ucArgc) {
  
}


/*************************************************************************/
/*****************************< Don't Touch >*****************************/
const uint16_t usCmdListPort5Len = sizeof(xCmdListPort5) / sizeof(*xCmdListPort5);

