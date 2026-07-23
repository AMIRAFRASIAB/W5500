
#include "w55_cmd_decoder.h"
#include "stdlib.h"


ADD_CMD( PORT6_CMD1 ,"CMD1"   );  
//------------------------------------------------------------------------
const W55DispatchItem_s xCmdListPort6[] = {
  EXPAND_CMD( PORT6_CMD1 ),
};

void run_PORT6_CMD1 (char** ppcArgv, uint8_t ucArgc) {
  
}


/*************************************************************************/
/*****************************< Don't Touch >*****************************/
const uint16_t usCmdListPort6Len = sizeof(xCmdListPort6) / sizeof(*xCmdListPort6);

