
/**
 * @file FreeRTOS_W5500.c
 * @brief FreeRTOS-based W5500 Ethernet client driver.
 *
 * This file implements a FreeRTOS driver for the W5500 Ethernet module,
 * managing TCP client communication using tasks, stream buffers, and mutexes.
 * It provides initialization, transmit, receive, and disconnect functions.
 * The driver handles reconnection logic and data flow using RTOS primitives.
 *
 * @author AMIR HOSEIN BAGHERI (Whitewolf97@yahoo.com)
 * @date 2025-08-12
 */
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
/**
 * @brief FreeRTOS task function to service W5500 client communication.
 *
 * Periodically (W5500_TASK_FREQUENCY_PERIOD) runs to handle reconnect attempts, receive data from W5500,
 * push received data into the RX stream buffer, and transmit data from the TX stream buffer.
 *
 * @param[in] pvParameters Pointer to task parameters (unused).
 */
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
/**
 * @brief Initialize the FreeRTOS W5500 client driver.
 *
 * Creates RTOS synchronization primitives (mutexes, stream buffers),
 * initializes the W5500 client network stack, and starts the service task.
 *
 * @param[in] cnf Pointer to the W5500_Cnf_t configuration structure.
 * @return true if initialization succeeded, false otherwise.
 */
bool FreeRTOS_w5500_client_init (W5500_Cnf_t* cnf) {
  LOG_TRACE("W5500 :: Initializing the RTOS driver...");
  bool status = true;
  info = cnf;
  status = status && (hMutexTx = xSemaphoreCreateMutex()) != NULL;
  status = status && (hMutexRx = xSemaphoreCreateMutex()) != NULL;
  status = status && (hStreamTx = xStreamBufferCreate(W5500_STREAM_BUF_TX_SIZE, 1)) != NULL;
  status = status && (hStreamRx = xStreamBufferCreate(W5500_STREAM_BUF_RX_SIZE, 1)) != NULL;
  w5500_client_init(info);
  status = status && xTaskCreate(&serviceW5500, "W5500", (W5500_TASK_STACK_SIZE_BYTES / 4), NULL, W5500_TASK_PRIORITY, &hTaskW5500) == pdTRUE;
  __initialized = status;
  if (!status) {
    LOG_ERROR("W5500 :: Failed to initial the RTOS driver");
  }
  return status;
}
//-------------------------------------------------------------------------------
/**
 * @brief Transmit data using the FreeRTOS W5500 client driver.
 *
 * Adds data to the TX stream buffer protected by a mutex.
 * This function attempts to queue **all** bytes from `buf` into the TX buffer,
 * waiting up to `ticksToWait` ticks in total.
 * It blocks as needed until all bytes are queued or the timeout expires.
 *
 * @param[in] buf Pointer to the data buffer to send.
 * @param[in] len Number of bytes to send.
 * @param[in] ticksToWait Maximum total ticks to wait for buffer space.
 * @return Number of bytes successfully queued for transmission (0 if timeout or error).
 */
uint32_t FreeRTOS_w5500_client_transmit (uint8_t* buf, uint16_t len, uint32_t ticksToWait) {
  TimeOut_t xTimeOut;
  uint32_t byteSent = 0;
  uint32_t sent;
  if (!__initialized || buf == NULL || len == 0)  {
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
/**
 * @brief Receive data using the FreeRTOS W5500 client driver.
 *
 * Attempts to receive up to `len` bytes from the RX stream buffer into `buf`.
 * This function waits up to `ticksToWait` ticks to obtain the mutex and
 * for data to become available in the buffer.
 *
 * @param[out] buf Pointer to the buffer where received data will be stored.
 * @param[in] len Maximum number of bytes to receive.
 * @param[in] ticksToWait Maximum ticks to wait for data and mutex availability.
 *
 * @return Number of bytes actually received from the RX buffer.
 *         Returns 0 if timeout occurs, buffer is NULL, length is zero, or driver is uninitialized.
 */
uint32_t FreeRTOS_w5500_client_receive (uint8_t* buf, uint8_t len, uint32_t ticksToWait) {
  TimeOut_t xTimeOut;
  uint16_t ret = 0;
  if (!__initialized || buf == NULL || len == 0)  {
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
/**
 * @brief Disconnect the FreeRTOS W5500 client and stop its service task.
 *
 * This function suspends the W5500 service task to stop ongoing
 * communication and then gracefully disconnects the W5500 client socket
 * with a timeout of 10 milliseconds.
 *
 * After calling this function, the client will no longer send or receive data
 * until reinitialized or the service task is resumed.
 */
void FreeRTOS_w5500_client_disconnect (void) {
  vTaskSuspend(hTaskW5500);
  w5500_client_disconnect(10);
}
