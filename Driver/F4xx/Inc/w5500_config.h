/**
 * @file w5500_config.h
 * @brief Configuration macros for W5500 Ethernet module driver.
 *
 * This header file provides all the configurable macros related to
 * hardware pin assignments, SPI interface settings, DMA configuration,
 * FreeRTOS integration, and default network parameters for the W5500
 * Ethernet controller.
 *
 * Users should modify these macros to match their specific hardware
 * setup and project requirements before compiling the W5500 driver.
 *
 * @note Ensure GPIO pins, SPI settings, and DMA streams correspond to
 *       the target MCU and board design.
 *
 * @author AMIR HOSEIN BAGHERI  (whitewolf97@yahoo.com)
 * @date [2025-8-12]
 */
 
#ifndef __W5500_CONFIG_H_
#define __W5500_CONFIG_H_

#ifdef __cplusplus
  extern "C" {
#endif   

#include "swo.h"
#include "bsp.h"
#include "serial_debugger.h"
#include "task_manager.h"

#define W5500_SPI                          3       
#define W5500_SPI_TIMEOUT                  5      
#define W5500_SPI_PRESCALER                LL_SPI_BAUDRATEPRESCALER_DIV16
#define W5500_TRACE_ENABLE                 YES
#define W5500_DEBUG_LIB                    "serial_debugger.h"                                              
                                              
#define W5500_CS_GPIO                      A
#define W5500_CS_PIN                       15

#define W5500_RST_GPIO                     B
#define W5500_RST_PIN                      5
                                             
#define W5500_MOSI_GPIO                    C
#define W5500_MOSI_PIN                     12
#define W5500_MOSI_AF                      6
                                           
#define W5500_MISO_GPIO                    B
#define W5500_MISO_PIN                     4
#define W5500_MISO_AF                      6
                                           
#define W5500_SCLK_GPIO                    B
#define W5500_SCLK_PIN                     3
#define W5500_SCLK_AF                      6
                                           
#define W5500_SPI_USE_DMA                  YES
                                              
#if (W5500_SPI_USE_DMA==YES)             
#define W5500_DMA_TX_NUM                   1
#define W5500_DMA_TX_STREAM                5
#define W5500_DMA_TX_CHANNEL               0
#define W5500_DMA_TX_STREAM_PRIORITY       LL_DMA_PRIORITY_LOW
                                           
#define W5500_DMA_RX_NUM                   1
#define W5500_DMA_RX_STREAM                0
#define W5500_DMA_RX_CHANNEL               0
#define W5500_DMA_RX_IRQ_PRIORITY          W5500_DMA_RX_IRQ_PRIORITYY
#define W5500_DMA_RX_STREAM_PRIORITY       LL_DMA_PRIORITY_LOW
#endif                                     
                                           
#define W5500_USE_FreeRTOS                 YES
#if (W5500_USE_FreeRTOS==YES)
#define W5500_GetTick                      xTaskGetTickCount
#define W5500_Delay                        vTaskDelay
#define W5500_TaskCreate                   tm_xTaskCreate
#define W5500_STREAM_BUF_RX_SIZE           128
#define W5500_STREAM_BUF_TX_SIZE           128
#define W5500_TASK_STACK_SIZE_BYTES        (W5500_STACK_SIZE * 4) 
#define W5500_TASK_PRIORITY                W5500_TASK_PRIORITYY
#define W5500_TASK_FREQUENCY_PERIOD        100
#define W5500_CHECK_FREQUENCY_PERIOD       1000
#define W5500_HEART_BEAT_TIMEOUT           3000
#else 
#define W5500_GetTick                      HAL_GetTick
#define W5500_Delay                        HAL_Delay
#endif      

#define W5500_SOCKET_NUM_START             7070      
#define W5500_SOCKET_NUM_INCREMENT         NO      
#define W5500_USER_NETWORK_CONFIG          YES
#if (W5500_USER_NETWORK_CONFIG==NO)
#define W5500_MAC_ADDRESS                  0x00, 0x08, 0xDC, 0xAB, 0xCD, 0xEF
#define W5500_PORT                         8234
#define W5500_OWN_IP                       192, 168, 14, 4
#define W5500_DESTINATION_IP               192, 168, 14, 2
#define W5500_SUBNET                       255, 255, 255, 0
#define W5500_GATEWAY                      192, 168, 14, 1
#define W5500_DNS                          8, 8, 8, 8
#define W5500_DHCP                         NETINFO_STATIC /// NETINFO_STATIC or NETINFO_DHCP
#endif  

#define W5500_RETRY_CONN_DELAY             10
#define W5500_RETRY_COUNTS                 2
#define W5500_APIs_TIMEOUT                 50

#ifdef __cplusplus
  }
#endif   
#endif //__W5500_CONFIG_H_  