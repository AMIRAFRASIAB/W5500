
#ifndef _W5500_CMD_DECODER_H_
#define _W5500_CMD_DECODER_H_

#include "stdint.h"
#include "stdbool.h"

#define __ADD_CMD(identifier, string)  __attribute__((used)) const char identifier[] = string; static void run_ ## identifier (char** ppcArgv, uint8_t ucArgc);
#define ADD_CMD(...)                   __ADD_CMD(__VA_ARGS__)
#define EXPAND_CMD(cmd)                (W55DispatchItem_s){.pcCMD = cmd,     .vFn = run_ ## cmd }


typedef void (*vFn_t) (char**, uint8_t);
typedef struct {
  const char*   pcCMD;
  vFn_t         vFn;
} W55DispatchItem_s;



void vW55CmdDecode (void* pvStream, uint16_t usStreamLen, const W55DispatchItem_s* pxList, uint16_t usListLen);

extern const W55DispatchItem_s xCmdListPort0[];
extern const W55DispatchItem_s xCmdListPort1[];
extern const W55DispatchItem_s xCmdListPort2[];
extern const W55DispatchItem_s xCmdListPort3[];
extern const W55DispatchItem_s xCmdListPort4[];
extern const W55DispatchItem_s xCmdListPort5[];
extern const W55DispatchItem_s xCmdListPort6[];
extern const W55DispatchItem_s xCmdListPort7[];
extern const uint16_t usCmdListPort0Len;
extern const uint16_t usCmdListPort1Len;
extern const uint16_t usCmdListPort2Len;
extern const uint16_t usCmdListPort3Len;
extern const uint16_t usCmdListPort4Len;
extern const uint16_t usCmdListPort5Len;
extern const uint16_t usCmdListPort6Len;
extern const uint16_t usCmdListPort7Len;



#endif //_W5500_CMD_DECODER_H_