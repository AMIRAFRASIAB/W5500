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

#define W5500_SPI                          1       
#define W5500_SPI_TIMEOUT                  20      
#define W5500_SPI_PRESCALER                LL_SPI_BAUDRATEPRESCALER_DIV8
#define W5500_TRACE_ENABLE                 YES
                                              
#define W5500_CS_GPIO                      A       
#define W5500_CS_PIN                       4       
                                              
#define W5500_RST_GPIO                     A
#define W5500_RST_PIN                      3
                                             
#define W5500_MOSI_GPIO                    A
#define W5500_MOSI_PIN                     7  
#define W5500_MOSI_AF                      5
                                           
#define W5500_MISO_GPIO                    A
#define W5500_MISO_PIN                     6  
#define W5500_MISO_AF                      5
                                           
#define W5500_SCLK_GPIO                    A
#define W5500_SCLK_PIN                     5  
#define W5500_SCLK_AF                      5
                                           
#define W5500_SPI_USE_DMA                  YES
                                              
#if (W5500_SPI_USE_DMA==YES)             
#define W5500_DMA_TX_NUM                   2
#define W5500_DMA_TX_STREAM                3
#define W5500_DMA_TX_CHANNEL               3
#define W5500_DMA_TX_STREAM_PRIORITY       LL_DMA_PRIORITY_MEDIUM
                                           
#define W5500_DMA_RX_NUM                   2
#define W5500_DMA_RX_STREAM                2
#define W5500_DMA_RX_CHANNEL               3
#define W5500_DMA_RX_IRQ_PRIORITY          6
#define W5500_DMA_RX_STREAM_PRIORITY       LL_DMA_PRIORITY_MEDIUM
#endif                                     
                                           
#define W5500_USE_FreeRTOS                 YES
#if (W5500_USE_FreeRTOS==YES)
#define W5500_GetTick                      xTaskGetTickCount
#define W5500_Delay                        vTaskDelay
#define W5500_STREAM_BUF_RX_SIZE           64
#define W5500_STREAM_BUF_TX_SIZE           64
#define W5500_TASK_STACK_SIZE_BYTES        1024
#define W5500_TASK_PRIORITY                1
#define W5500_TASK_FREQUENCY_PERIOD        100
#else 
#define W5500_GetTick                      HAL_GetTick
#define W5500_Delay                        HAL_Delay
#endif      
      
#define W5500_USER_NETWORK_CONFIG          NO
#if (W5500_USER_NETWORK_CONFIG==NO)
#define W5500_MAC_ADDRESS                  0x00, 0x08, 0xDC, 0xAB, 0xCD, 0xEF
#define W5500_PORT                         5000
#define W5500_OWN_IP                       192,   168,    1,    4
#define W5500_DESTINATION_IP               192,   168,    1,    2
#define W5500_SUBNET                       255,   255,  255,    0
#define W5500_GATEWAY                      192,   168,    1,    1
#define W5500_DNS                            8,     8,    8,    8
#define W5500_DHCP                         NETINFO_STATIC /// NETINFO_STATIC or NETINFO_DHCP
#endif  

#define W5500_RETRY_CONN_DELAY             5
#define W5500_RETRY_COUNTS                 2  

#ifdef __cplusplus
  }
#endif   
#endif //__W5500_CONFIG_H_  