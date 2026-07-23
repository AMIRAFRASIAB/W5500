
#include "w55_cmd_decoder.h"
#include "stdlib.h"


ADD_CMD( PORT4_CMD1 ,"CMD1"   );  
//------------------------------------------------------------------------
const W55DispatchItem_s xCmdListPort4[] = {
  EXPAND_CMD( PORT4_CMD1 ),
};

void run_PORT4_CMD1 (char** ppcArgv, uint8_t ucArgc) {
  
}


/*************************************************************************/
/*****************************< Don't Touch >*****************************/
const uint16_t usCmdListPort4Len = sizeof(xCmdListPort4) / sizeof(*xCmdListPort4);

