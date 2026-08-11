  
#include "socket.h"
#include "FreeRTOS_W5500.h"
#include "w5500_log.h"
#include "w5500_config.h"
#include "w5500_client.h"
#include "w5500_spi_driver.h"
#include "FreeRTOS.h"
#include "task.h"
#include "stream_buffer.h"
#include "semphr.h"
#include "timers.h"
#include "w55_cmd_decoder.h"

const W5500TxItem_t xW55EchoItem = {
  .pucAddr = (uint8_t*)":PONG;\n",
  .ulLen   = 7,
};

typedef struct {
  TaskHandle_t          xTask;
  StreamBufferHandle_t  xStreamRx;
  SemaphoreHandle_t     xMutexRx;
  TimerHandle_t         xTimIdle;
  W5500State_e          xState;
  QueueHandle_t         xQTxItem;
  QueueHandle_t         xQEvent;
  uint8_t               ucIndex;
  bool                  bIdleTimerOverflowFlag;
} ClientObj_t;

typedef struct {
  const W55DispatchItem_s* pxList;
  uint16_t                 usListLen;
  uint8_t                  ucHeader;
  uint8_t                  ucFooter;
  uint8_t                  ucIndex;
} W55Decode_t;

W55Decode_t xDecSetting[8] = {0};
  

static ClientObj_t xCliArr[8] = {0};

static const uint16_t usIdleTimeArr[8] = {
  W5500_IDLE_TIMER_PERIOD_0,
  W5500_IDLE_TIMER_PERIOD_1,
  W5500_IDLE_TIMER_PERIOD_2,
  W5500_IDLE_TIMER_PERIOD_3,
  W5500_IDLE_TIMER_PERIOD_4,
  W5500_IDLE_TIMER_PERIOD_5,
  W5500_IDLE_TIMER_PERIOD_6,
  W5500_IDLE_TIMER_PERIOD_7,
};
TaskHandle_t xTaskW5500IrqDispatcher = NULL;
extern const uint16_t usPortW5500[];
static void prvvServiceLanDecoder (void* const pvParameters);
static void prvvServiceW5500IrqDispatcher (void* const pvParameters);
//-------------------------------------------------------------------------------
static void prvvIdleTimerCallbackFunction (TimerHandle_t xTimer) {
  // Timeout expired -> reconnect the socket
  const uint8_t ucDisConnectionEvent = Sn_IR_DISCON;
  for (uint8_t i = 0; i < 8; i++) {
    if (xCliArr[i].xTimIdle == xTimer) {
      xQueueSend(xCliArr[i].xQEvent, &ucDisConnectionEvent, 0);
      return;
    }
  }
}
//-------------------------------------------------------------------------------
static void prvvServiceW5500 (void* const pvParameters) {
  ClientObj_t* pxClient = pvParameters;
  W5500_RTOS_TASK_START(); //LOG -> index report? TODO:
  bool status = true;
  status = status && (pxClient->xMutexRx = xSemaphoreCreateMutex()) != NULL;
  status = status && (pxClient->xStreamRx = xStreamBufferCreate(W5500_STREAM_BUF_RX_SIZE, 1)) != NULL;
  status = status && (pxClient->xQTxItem = xQueueCreate(W5500_QUEUE_TX_LEN, sizeof(W5500TxItem_t))) != NULL;
  status = status && (pxClient->xQEvent = xQueueCreate(W5500_QUEUE_TX_LEN, sizeof(uint8_t))) != NULL;
  QueueSetHandle_t xQSet = NULL;
  status = status && (xQSet = xQueueCreateSet(2 * W5500_QUEUE_TX_LEN + 1)) != NULL;
  if (usIdleTimeArr[pxClient->ucIndex] != 0) {
    status = status && (pxClient->xTimIdle = xTimerCreate(NULL, pdMS_TO_TICKS(usIdleTimeArr[pxClient->ucIndex]), pdFALSE, NULL, &prvvIdleTimerCallbackFunction)) != NULL;
  }
  if (!status) {
    W5500_RTOS_TASK_STOP(); //LOG 
    vTaskSuspend(NULL);
  }
  xQueueAddToSet(pxClient->xQTxItem, xQSet);
  xQueueAddToSet(pxClient->xQEvent, xQSet);
  uint8_t ucRxBuf[W5500_STREAM_BUF_RX_SIZE];
  W5500TxItem_t xTxObj;
  QueueSetMemberHandle_t xQActive = NULL;
  goto DISCONNECT;
  
  while (1) {
    xQActive = xQueueSelectFromSet(xQSet, pdMS_TO_TICKS(W5500_TASK_RECONNECTION_DELAY));
    if (xQActive == pxClient->xQTxItem) {
      xQueueReceive(pxClient->xQTxItem, &xTxObj, 0);
      if (xTxObj.pucAddr != NULL && xTxObj.ulLen != 0) {
        send(pxClient->ucIndex, (uint8_t*)xTxObj.pucAddr, xTxObj.ulLen);
      }
    }
    else if (xQActive == pxClient->xQEvent) {
      uint8_t ucRxByteIrq = 0;
      xQueueReceive(pxClient->xQEvent, &ucRxByteIrq, 0);
      if (ucRxByteIrq & Sn_IR_RECV) {
        uint16_t us = w5500_client_receive(ucRxBuf, sizeof(ucRxBuf), pxClient->ucIndex);
        if (us > 0) {
          if (pxClient->xTimIdle) xTimerReset(pxClient->xTimIdle, portMAX_DELAY);
          xStreamBufferSend(pxClient->xStreamRx, ucRxBuf, us, 0);
        }
      }
      if (ucRxByteIrq & Sn_IR_DISCON) {
        goto DISCONNECT;
      }
    }
    else {
      uint8_t tmp;
      ctlwizchip(CW_GET_PHYLINK, (void*)&tmp); 
      if (tmp == PHY_LINK_OFF) {
        goto DISCONNECT;
      }
    }
    continue;
    DISCONNECT:
    pxClient->xState = eW5500StateDisconnect;
    if (pxClient->xTimIdle) xTimerStop(pxClient->xTimIdle, portMAX_DELAY);
    w5500_client_disconnect(pxClient->ucIndex);
    vTaskDelay(pdMS_TO_TICKS(100));
    CONNECT:
    pxClient->xState = eW5500StateTryForConnection;
    while (!w5500_client_reconnect(pxClient->ucIndex)) {
      vTaskDelay(pdMS_TO_TICKS(W5500_TASK_RECONNECTION_DELAY));
    }
    pxClient->xState = eW5500StateTransiver;
    portENTER_CRITICAL();
    xQueueReset(pxClient->xQTxItem);
    xQueueReset(pxClient->xQEvent);
    xQueueReset(xQSet);
    portEXIT_CRITICAL();
    vTaskDelay(pdMS_TO_TICKS(500));
    if (pxClient->xTimIdle) xTimerReset(pxClient->xTimIdle, portMAX_DELAY);
  }
  W5500_RTOS_TASK_STOP();
  vTaskSuspend(NULL);
}
//-------------------------------------------------------------------------------
void vFreeRTOSW5500Transmit (const W5500TxItem_t* pxItem, uint8_t ucSocketNumber) {
  if (pxItem == NULL) {
    return;
  }
  QueueHandle_t xQ = xCliArr[ucSocketNumber & 0x07].xQTxItem;
  W5500State_e xState = xCliArr[ucSocketNumber & 0x07].xState;
  if (xQ && xState == eW5500StateTransiver) {
    xQueueSend(xQ, pxItem, 0);
  }
}
//-------------------------------------------------------------------------------
W5500State_e xFreeRTOSW5500GetTaskState (uint8_t ucSocketNumber) {
  return xCliArr[ucSocketNumber & 0x07].xState;
}
//-------------------------------------------------------------------------------
bool bFreeRTOSW5500ClientInit (void) {
  W5500_LOG_RTOS_INITIAL(); //LOG
  char cStr[16];
  bool bStatus = true;
  bStatus = bStatus && w5500_client_init(NULL);
  for (uint8_t i = 0; i < 8; i++) {
    if (usPortW5500[i] != 0 && xCliArr[i].xTask == NULL) {
      snprintf(cStr, sizeof(cStr), "W5500-%1u", i);
      xCliArr[i].ucIndex = i;
      bStatus = bStatus && W5500_TaskCreate(&prvvServiceW5500, cStr, (W5500_TASK_STACK_SIZE_BYTES / 4), &xCliArr[i], W5500_TASK_PRIORITY, &xCliArr[i].xTask) == pdTRUE;
    }
  }
  // Decode setting
  W55Decode_t xSetting[8] = {
    {.ucIndex = 0, .pxList = xCmdListPort0, .usListLen = usCmdListPort0Len, .ucHeader = W5500_RX_ENGINE_HEADER_0, .ucFooter = W5500_RX_ENGINE_FOOTER_0},
    {.ucIndex = 1, .pxList = xCmdListPort1, .usListLen = usCmdListPort1Len, .ucHeader = W5500_RX_ENGINE_HEADER_1, .ucFooter = W5500_RX_ENGINE_FOOTER_1},
    {.ucIndex = 2, .pxList = xCmdListPort2, .usListLen = usCmdListPort2Len, .ucHeader = W5500_RX_ENGINE_HEADER_2, .ucFooter = W5500_RX_ENGINE_FOOTER_2},
    {.ucIndex = 3, .pxList = xCmdListPort3, .usListLen = usCmdListPort3Len, .ucHeader = W5500_RX_ENGINE_HEADER_3, .ucFooter = W5500_RX_ENGINE_FOOTER_3},
    {.ucIndex = 4, .pxList = xCmdListPort4, .usListLen = usCmdListPort4Len, .ucHeader = W5500_RX_ENGINE_HEADER_4, .ucFooter = W5500_RX_ENGINE_FOOTER_4},
    {.ucIndex = 5, .pxList = xCmdListPort5, .usListLen = usCmdListPort5Len, .ucHeader = W5500_RX_ENGINE_HEADER_5, .ucFooter = W5500_RX_ENGINE_FOOTER_5},
    {.ucIndex = 6, .pxList = xCmdListPort6, .usListLen = usCmdListPort6Len, .ucHeader = W5500_RX_ENGINE_HEADER_6, .ucFooter = W5500_RX_ENGINE_FOOTER_6},
    {.ucIndex = 7, .pxList = xCmdListPort7, .usListLen = usCmdListPort7Len, .ucHeader = W5500_RX_ENGINE_HEADER_7, .ucFooter = W5500_RX_ENGINE_FOOTER_7},
  };
  for (uint8_t i = 0; i < 8; i++) {
    if (usPortW5500[i] == 0) {
      continue;
    }
    xDecSetting[i] = xSetting[i];
    snprintf(cStr, sizeof(cStr), "LAN Rx DEC %1u", i);
    bStatus = bStatus && W5500_TaskCreate(&prvvServiceLanDecoder, cStr, (W5500_RxDEC_TASK_STACK_SIZE_BYTES / 4),  &xDecSetting[i], W5500_RxDEC_TASK_PRIORITY, NULL) == pdTRUE;
  }
  bStatus = bStatus && W5500_TaskCreate(&prvvServiceW5500IrqDispatcher, "W55_IRQ", (W5500_RxDEC_TASK_STACK_SIZE_BYTES / 4),  NULL, W5500_IRQ_DISPACHER_TASK_PRIORITY, &xTaskW5500IrqDispatcher) == pdTRUE;
  if (!bStatus) {
    W5500_LOG_RTOS_INITIAL_FAIL(); //LOG
  }
  return bStatus;
}
//-------------------------------------------------------------------------------
uint32_t ulFreeRTOSW5500ClientReceive (uint8_t* buf, uint8_t len, uint32_t ticksToWait, uint8_t ucSocketNumber) {  TimeOut_t xTimeOut;
  uint16_t ret = 0;
  ucSocketNumber &= 0x07;
  if (!xCliArr[ucSocketNumber].xTask || buf == NULL || len == 0)  {
    return 0;
  }
  vTaskSetTimeOutState(&xTimeOut);
  if (xSemaphoreTake(xCliArr[ucSocketNumber].xMutexRx, ticksToWait) == pdTRUE) {
    if (xTaskCheckForTimeOut(&xTimeOut, &ticksToWait) == pdFALSE) {
      ret = xStreamBufferReceive(xCliArr[ucSocketNumber].xStreamRx, buf, len, ticksToWait);
    }
    xSemaphoreGive(xCliArr[ucSocketNumber].xMutexRx);
  }
  return ret;
}
//-------------------------------------------------------------------------------
void vFreeRTOSW5500TaskDisable (uint8_t ucSocketNumber) {
  ucSocketNumber &= 0x07;
  TaskHandle_t xTask = xCliArr[ucSocketNumber ].xTask;
  if (xTask) {
    vTaskSuspend(xTask);
    w5500_client_disconnect(ucSocketNumber);
    xCliArr[ucSocketNumber].xState = eW5500StateDisconnect;
  }
}
//-------------------------------------------------------------------------------
void prvvServiceLanDecoder (void* const pvParameters) {
  W55Decode_t* pxSetting = (W55Decode_t*)pvParameters;
  uint8_t ucHeader = pxSetting->ucHeader;
  uint8_t ucFooter = pxSetting->ucFooter;
  uint8_t ucPortIndex = pxSetting->ucIndex;
  uint8_t ucRxBuf[W5500_STREAM_BUF_RX_SIZE];
  uint16_t usIndex = 0;
  bool bHeaderFound = false;
  uint8_t ucByte;
  while (1) {
    ulFreeRTOSW5500ClientReceive(&ucByte, 1, portMAX_DELAY, ucPortIndex);
    if (ucByte == ucHeader) {
      usIndex = 0;
      bHeaderFound = true;
    }
    else if (ucByte == ucFooter) { 
      if (bHeaderFound == true && usIndex != 0) {
        ucRxBuf[usIndex] = '\0';
//        LOG_TRACE(eEventLanRxPacket); //"LAN: Decoder: Rx packet= %s\n"
        bHeaderFound = false;
        vW55CmdDecode(ucRxBuf, usIndex, pxSetting->pxList, pxSetting->usListLen);
      }
      else {
        bHeaderFound = false;
      }
    }
    else {
      if (bHeaderFound == true) {
        ucRxBuf[usIndex++] = ucByte;
        if (usIndex >= sizeof(ucRxBuf) - 1) {
          bHeaderFound = false;
        }
      }
    }
  }
  vTaskSuspend(NULL);
}
//-------------------------------------------------------------------------------
void prvvServiceW5500IrqDispatcher (void* const pvParameters) {
  vTaskDelay(pdMS_TO_TICKS(2000));
  uint8_t ucQuantity = 0;
  for (uint8_t i = 0; i < 8; i++) {
    if (usPortW5500[i] != 0) {
      ucQuantity++;
    }
  }
  uint8_t ucBuf[8];
  while (1) {
    memset(ucBuf, 0x00, ucQuantity);
    ulTaskNotifyTake(pdFALSE, portMAX_DELAY);
    for (uint8_t i = 0; i < ucQuantity; i++) {
      ucBuf[i] = getSn_IR(i);
      setSn_IR(i, 0xFF);
      xQueueSend(xCliArr[i].xQEvent, &ucBuf[i], 0);
    }
  }
}

