
#include "w55_cmd_decoder.h"
#include "stdlib.h"


ADD_CMD( PORT3_CMD1 ,"CMD1"   );  
//------------------------------------------------------------------------
const W55DispatchItem_s xCmdListPort3[] = {
  EXPAND_CMD( PORT3_CMD1 ),
};

void run_PORT3_CMD1 (char** ppcArgv, uint8_t ucArgc) {
  
}


/*************************************************************************/
/*****************************< Don't Touch >*****************************/
const uint16_t usCmdListPort3Len = sizeof(xCmdListPort3) / sizeof(*xCmdListPort3);

