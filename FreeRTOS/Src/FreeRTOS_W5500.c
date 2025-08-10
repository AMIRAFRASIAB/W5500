
#include "FreeRTOS_W5500.h"
#include "w5500_config.h"
#include "w5500_client.h"

#include "FreeRTOS.h"
#include "task.h"
#include "stream_buffer.h"
#include "semphr.h"


#if W5500_TRACE_ENABLE == YES 
  #include "serial_debugger.h"
#else 
  #define LOG_TRACE(...)        
  #define LOG_INFO(...)
  #define LOG_WARNING(...)
  #define LOG_ERROR(...)
  #define LOG_FATAL(...)
#endif
//-------------------------------------------------------------------------------

static TaskHandle_t hTaskW5500 = NULL;
static StreamBufferHandle_t hStreamTx = NULL;
static StreamBufferHandle_t hStreamRx = NULL;
static SemaphoreHandle_t hMutexTx = NULL;
static SemaphoreHandle_t hMutexRx = NULL;
static uint8_t __initialized = 0;
static W5500_Cnf_t* info = NULL;

//-------------------------------------------------------------------------------
static void serviceW5500 (void* const pvParameters) {
  static uint8_t rxBuf[W5500_STREAM_BUF_RX_SIZE];
  static uint8_t txBuf[W5500_STREAM_BUF_TX_SIZE];
  static uint16_t rxSize;
  static uint16_t txSize;
  static TickType_t xLastWakeTime;
  xLastWakeTime = xTaskGetTickCount();
  while (1) {
    vTaskDelayUntil(&xLastWakeTime, W5500_TASK_FREQUENCY_PERIOD);
    if (w5500_client_reconnect(info)) {
      //Receive
      rxSize = w5500_client_receive(rxBuf, sizeof(rxBuf));
      if (rxSize > 0) {
        xStreamBufferSend(hStreamRx, rxBuf, rxSize, 0);
      }
      //Transmit
      txSize = xStreamBufferReceive(hStreamTx, txBuf, sizeof(txBuf), 0);
      w5500_client_transmit(txBuf, txSize);
    }
  }
}
//-------------------------------------------------------------------------------
bool FreeRTOS_w5500_client_inti (W5500_Cnf_t* cnf) {
  LOG_TRACE("W5500 :: Initializing the RTOS driver...");
  bool status = true;
  info = cnf;
  status = status && (hMutexTx = xSemaphoreCreateMutex()) != NULL;
  status = status && (hMutexRx = xSemaphoreCreateMutex()) != NULL;
  status = status && (hStreamTx = xStreamBufferCreate(W5500_STREAM_BUF_TX_SIZE, 1)) != NULL;
  status = status && (hStreamRx = xStreamBufferCreate(W5500_STREAM_BUF_RX_SIZE, 1)) != NULL;
  status = status && w5500_client_init(info);
  status = status && xTaskCreate(&serviceW5500, "W5500", (W5500_TASK_STACK_SIZE_BYTES / 4), NULL, W5500_TASK_PRIORITY, &hTaskW5500) == pdTRUE;
  __initialized = status;
  if (!status) {
    LOG_ERROR("W5500 :: Failed to initial the RTOS driver");
  }
  return status;
}
//-------------------------------------------------------------------------------
uint32_t FreeRTOS_w5500_client_transmit (uint8_t* buf, uint16_t len, uint32_t ticksToWait) {
  TimeOut_t xTimeOut;
  uint32_t byteSent = 0;
  uint32_t sent;
  if (buf == NULL || len == 0)  {
    return 0;
  }
  vTaskSetTimeOutState(&xTimeOut);
  if (xSemaphoreTake(hMutexTx, ticksToWait) == pdTRUE) {
    while (byteSent < len) {
      if (xTaskCheckForTimeOut(&xTimeOut, &ticksToWait) == pdFALSE) {
        sent = xStreamBufferSend(hStreamTx, buf + byteSent, len - byteSent, ticksToWait);
        if (sent > 0) {
          byteSent += sent;
        }
      }
      else {
        break;
      }
    }
    xSemaphoreGive(hMutexTx);
  }
  return byteSent;
}
//-------------------------------------------------------------------------------
uint32_t FreeRTOS_w5500_client_receive (uint8_t* buf, uint8_t len, uint32_t ticksToWait) {
  TimeOut_t xTimeOut;
  uint16_t ret = 0;
  if (buf == NULL || len == 0)  {
    return 0;
  }
  vTaskSetTimeOutState(&xTimeOut);
  if (xSemaphoreTake(hMutexRx, ticksToWait) == pdTRUE) {
    if (xTaskCheckForTimeOut(&xTimeOut, &ticksToWait) == pdFALSE) {
      ret = xStreamBufferReceive(hStreamRx, buf, len, ticksToWait);
    }
    xSemaphoreGive(hMutexRx);
  }
  return ret;
}
//-------------------------------------------------------------------------------

