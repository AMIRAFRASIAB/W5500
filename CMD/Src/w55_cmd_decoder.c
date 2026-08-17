

#include "w55_cmd_decoder.h"
#include "string.h"


//------------------------------------------------------------------------
void vW55CmdDecoderUpdateTokens (const char* ARGS, uint8_t strLen, char** ppcArgv, uint8_t* pucArgc) {
  uint8_t ucArgc = 0;
  char* args = (void*)ARGS;
//  args[strLen - 1] = '\0';
  char* tok = strtok(args," ");
  while (tok != NULL) {
    ppcArgv[ucArgc++] = tok;
    tok = strtok(NULL, " ");
    ucArgc &= 0x0F;
  }
  *pucArgc = ucArgc;
}
//------------------------------------------------------------------------
bool bW55CmdDecode (void* pvStream, uint16_t usStreamLen, const W55DispatchItem_s* pxList, uint16_t usListLen) {
  const W55DispatchItem_s* xMatchedItems[10] = {NULL};
  char* pcArgv[16] = {0};
  uint8_t ucArgc = 0;
  uint8_t ucMatchCounter = 0;
  char* input = (char*)pvStream;
  uint8_t ucLen;
  for (uint32_t i = 0; i < usListLen; i++) {
		ucLen = strlen(pxList[i].pcCMD);
    if (!strncmp(pxList[i].pcCMD, input, strlen(pxList[i].pcCMD))) {
      xMatchedItems[ucMatchCounter++] = pxList + i;
      if (ucMatchCounter >= sizeof(xMatchedItems) / sizeof(*xMatchedItems)) {
        return false;
      }
    }
  }
  const W55DispatchItem_s* targetItem = xMatchedItems[0];
  for (uint8_t i = 1; i < ucMatchCounter; i++) {
    if (strlen(targetItem->pcCMD) < strlen(xMatchedItems[i]->pcCMD)) {
      targetItem = xMatchedItems[i];
    }
  }
  if (targetItem != NULL) {
    ucLen = strlen(targetItem->pcCMD);
    vW55CmdDecoderUpdateTokens(input + ucLen, usStreamLen - ucLen, pcArgv, &ucArgc);
    targetItem->vFn(pcArgv, ucArgc);
  }
  else {
    return false;
  }
  return true;
}




