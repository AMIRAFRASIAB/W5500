
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


//-------------------------------------------------------------------------------
static TaskHandle_t xW5500TaskHandle = NULL;
static StreamBufferHandle_t xStreamHandleRx = NULL;
static SemaphoreHandle_t xMutexRxHandle = NULL;
static TimerHandle_t xIdleTimerHandle = NULL;
static W5500_Cnf_t* pxInfo = NULL;
QueueHandle_t xQueueHandleTx = NULL;
static W5500State_e xState = eW5500StateTryForConnection;
static uint8_t ucPhyIndex = 0;
//-------------------------------------------------------------------------------
static void prvvIdleTimerCallbackFunction (TimerHandle_t xTimer) {
  // Timeout expired -> reconnect the socket
  xState = eW5500StateDisconnect;
}
//-------------------------------------------------------------------------------
static void prvvServiceW5500 (void* const pvParameters) {
  W5500_RTOS_TASK_START(); //LOG
  bool status = true;
  status = status && (xMutexRxHandle = xSemaphoreCreateMutex()) != NULL;
  status = status && (xStreamHandleRx = xStreamBufferCreate(W5500_STREAM_BUF_RX_SIZE, 1)) != NULL;
  status = status && (xQueueHandleTx = xQueueCreate(W5500_QUEUE_TX_LEN, sizeof(W5500TxItem_t))) != NULL;
  #if (W5500_INACTIVITY_TIMER_PERIOD > 0)
  status = status && (xIdleTimerHandle = xTimerCreate(NULL, pdMS_TO_TICKS(W5500_INACTIVITY_TIMER_PERIOD), pdFALSE, NULL, &prvvIdleTimerCallbackFunction)) != NULL;
  #endif
  status = status && w5500_client_init(pxInfo, ucPhyIndex);
  if (!status) {
    W5500_RTOS_TASK_STOP();
    vTaskSuspend(NULL);
  }
  uint8_t ucRxBuf[W5500_STREAM_BUF_RX_SIZE];
  uint16_t usRxSize;
  W5500TxItem_t xTxObj;
  
  while (1) {
    switch (xState) {
      case eW5500StateTryForConnection: {
        W5500_RTOS_TRY_FOR_CONNECTION(); //LOG
        if (w5500_client_reconnect(pxInfo)) {
          xState = eW5500StateTransiver;
          if (xIdleTimerHandle) xTimerStart(xIdleTimerHandle, 0); // Timer start
        }
        else {
          vTaskDelay(pdMS_TO_TICKS(W5500_TASK_RECONNECTION_DELAY));
        }
      }
      break;
      case eW5500StateTransiver: {
        if (xQueueReceive(xQueueHandleTx, &xTxObj, pdMS_TO_TICKS(W5500_TASK_RECONNECTION_DELAY)) == pdTRUE) {
          if (xTxObj.pucAddr != NULL) { 
            if (send(1, (uint8_t*)xTxObj.pucAddr, xTxObj.ulLen) != xTxObj.ulLen) {
              W5500_RTOS_TRANSMIT_FAIL(); //LOG
            }
            else {
              // Check PHY
              uint8_t tmp;
              ctlwizchip(CW_GET_PHYLINK, (void*)&tmp); 
              if (tmp == PHY_LINK_OFF) {
                W5500_LOG_CABLE_DISCONNECT(); //LOG
                xState = eW5500StateDisconnect;
              }
            }
          }
        }
        if (xTxObj.pucAddr == NULL) {
          bW5500IrqFlag = false;
          uint8_t uc = getSn_IR(1);
          if (uc & Sn_IR_RECV) {
            if (xIdleTimerHandle) xTimerReset(xIdleTimerHandle, 0); // Timer reset
            uint16_t us = w5500_client_receive(ucRxBuf, sizeof(ucRxBuf));
            if (us > 0) {
              W5500_RTOS_PACKET_RECEIVED(); //LOG
              if (xStreamBufferSend(xStreamHandleRx, ucRxBuf, us, 0) != us) {
                W5500_RTOS_RX_FIFO_FULL(); //LOG
              }
            }
          }
          if (uc & Sn_IR_DISCON) {
            if (xIdleTimerHandle) xTimerStop(xIdleTimerHandle, 0); // Timer stop
            xState = eW5500StateTryForConnection;
            W5500_RTOS_SOCKET_DISCONNECTED(); //LOG
          }
          // Clear socket #1 all interrupt flags
          setSn_IR(1, 0xFF);
        }
      }
      break;
      case eW5500StateDisconnect: {
        if (xIdleTimerHandle) xTimerStop(xIdleTimerHandle, 0); // Timer stop
        W5500_RTOS_SOCKET_DISCONNECTED(); //LOG
        xState = eW5500StateTryForConnection;
        w5500_client_disconnect();
        vTaskDelay(pdMS_TO_TICKS(50));
      }
      break;
      default: {
        xState = eW5500StateDisconnect;
      }
      break;
    };
  }
  W5500_RTOS_TASK_STOP();
  vTaskSuspend(NULL);
}
//-------------------------------------------------------------------------------
void vFreeRTOSW5500Transmit (W5500TxItem_t* pxItem) {
  if (!(pxItem && pxItem->pucAddr && pxItem->ulLen)) {
    return;
  }
  xQueueSend(xQueueHandleTx, pxItem, 0);
}
//-------------------------------------------------------------------------------
W5500State_e xFreeRTOSW5500GetTaskState (void) {
  return xState;
}
//-------------------------------------------------------------------------------
bool bFreeRTOSW5500ClientInit (W5500_Cnf_t* pxConfig, uint8_t ucPhyConfigIndex) {
  W5500_LOG_RTOS_INITIAL(); //LOG
  if (xW5500TaskHandle) {
    return true;
  }
  pxInfo = pxConfig;
  ucPhyIndex = ucPhyConfigIndex;
  bool status = W5500_TaskCreate(prvvServiceW5500, "W5500", (W5500_TASK_STACK_SIZE_BYTES / 4), NULL, W5500_TASK_PRIORITY, &xW5500TaskHandle) == pdTRUE;
  if (!status) {
    W5500_LOG_RTOS_INITIAL_FAIL(); //LOG
  }
  return status;
}
//-------------------------------------------------------------------------------
uint32_t ulFreeRTOSW5500ClientReceive (uint8_t* buf, uint8_t len, uint32_t ticksToWait) {
  TimeOut_t xTimeOut;
  uint16_t ret = 0;
  if (!xW5500TaskHandle || buf == NULL || len == 0)  {
    return 0;
  }
  vTaskSetTimeOutState(&xTimeOut);
  if (xSemaphoreTake(xMutexRxHandle, ticksToWait) == pdTRUE) {
    if (xTaskCheckForTimeOut(&xTimeOut, &ticksToWait) == pdFALSE) {
      ret = xStreamBufferReceive(xStreamHandleRx, buf, len, ticksToWait);
    }
    xSemaphoreGive(xMutexRxHandle);
  }
  return ret;
}
//-------------------------------------------------------------------------------
void vFreeRTOSW5500TaskDisable (void) {
  if (xW5500TaskHandle) {
    vTaskSuspend(xW5500TaskHandle);
    w5500_client_disconnect();
  }
}
//-------------------------------------------------------------------------------
