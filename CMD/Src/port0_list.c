
#include "w55_cmd_decoder.h"
#include "stdlib.h"


ADD_CMD( PORT0_CMD1 ,"CMD1"   );  
//------------------------------------------------------------------------
const W55DispatchItem_s xCmdListPort0[] = {
  EXPAND_CMD( PORT0_CMD1 ),
};

void run_PORT0_CMD1 (char** ppcArgv, uint8_t ucArgc) {
  
}


/*************************************************************************/
/*****************************< Don't Touch >*****************************/
const uint16_t usCmdListPort0Len = sizeof(xCmdListPort0) / sizeof(*xCmdListPort0);

