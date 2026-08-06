/*
 * ============================================================================
 * W5500 Library Notes
 * ============================================================================
 *
 * 1. Interrupt Priority
 *    - The W5500 interrupt (EXTI) should have a priority one level lower
 *      (more important) than the DMA interrupt.
 *
 * 2. Task Priority
 *    - The main Engine task should have a priority one level higher
 *      (more important) than the packet decoder task.
 *
 * 3. PHY Link Status
 *    - W5500_PHY_LINK_INDEX has five possible states:
 *        0 ~ 4
 *
 * 4. Port Enable
 *    - If W5500_PORT_x is 0, the corresponding port is disabled.
 *
 * 5. Socket ID Assignment
 *    - If W5500_SOCKET_ID_x is 0, each new connection uses a different
 *      socket ID automatically.
 *    - If W5500_SOCKET_ID_x is non-zero, every connection uses the
 *      specified socket ID.
 *
 * ============================================================================
 */
 
#ifndef __W5500_CONFIG_H_
#define __W5500_CONFIG_H_

#ifdef __cplusplus
  extern "C" {
#endif   

#include "swo.h"
#include "FreeRTOS.h"
#include "task.h"



#define W5500_SPI                          1       
#define W5500_SPI_TIMEOUT                  20      
#define W5500_SPI_PRESCALER                LL_SPI_BAUDRATEPRESCALER_DIV2 //42 MHz

#define W5500_CS_GPIO                      A
#define W5500_CS_PIN                       4

#define W5500_RST_GPIO                     B
#define W5500_RST_PIN                      0

#define W5500_MOSI_GPIO                    A
#define W5500_MOSI_PIN                     7
#define W5500_MOSI_AF                      5

#define W5500_MISO_GPIO                    A
#define W5500_MISO_PIN                     6
#define W5500_MISO_AF                      5

#define W5500_SCLK_GPIO                    A
#define W5500_SCLK_PIN                     5
#define W5500_SCLK_AF                      5

#define W5500_IRQ_GPIO                     A
#define W5500_IRQ_PIN                      3
#define W5500_IRQn                         EXTI3_IRQn
#define W5500_IRQHandler                   EXTI3_IRQHandler
#define W5500_IRQ_PIN_PRIORITY             6

#define W5500_SPI_USE_DMA                  YES
#if (W5500_SPI_USE_DMA==YES)             
#define W5500_DMA_TX_NUM                   2
#define W5500_DMA_TX_STREAM                5
#define W5500_DMA_TX_CHANNEL               3
#define W5500_DMA_TX_STREAM_PRIORITY       LL_DMA_PRIORITY_MEDIUM
#define W5500_DMA_RX_NUM                   2
#define W5500_DMA_RX_STREAM                2
#define W5500_DMA_RX_CHANNEL               3
#define W5500_DMA_RX_IRQ_PRIORITY          7
#define W5500_DMA_RX_STREAM_PRIORITY       LL_DMA_PRIORITY_MEDIUM
#endif                                     
                                           
#define W5500_USE_FreeRTOS                 YES
#if (W5500_USE_FreeRTOS==YES)
#define W5500_GetTick                      xTaskGetTickCount
#define W5500_Delay                        vTaskDelay
#define W5500_TaskCreate                   xTaskCreate
#define W5500_STREAM_BUF_RX_SIZE           128
#define W5500_QUEUE_TX_LEN                 8
#define W5500_TASK_STACK_SIZE_BYTES        2048
#define W5500_RxDEC_TASK_STACK_SIZE_BYTES  1024
#define W5500_TASK_PRIORITY                3
#define W5500_RxDEC_TASK_PRIORITY          2
#define W5500_TASK_RECONNECTION_DELAY      1000
#else 
#define W5500_GetTick                      HAL_GetTick
#define W5500_Delay                        HAL_Delay
#endif      
      
#define W5500_MAC_ADDRESS                  100, 101, 102, 103, 104, 105
#define W5500_OWN_IP                       192, 168, 14,  50
#define W5500_SUBNET                       255, 255, 255, 0
#define W5500_GATEWAY                      192, 168, 14,  1
#define W5500_DNS                          8,   8,   8,   8
#define W5500_DHCP                         NETINFO_STATIC /// NETINFO_STATIC or NETINFO_DHCP
#define W5500_PHY_LINK_INDEX               4

#define W5500_PORT_0                       8250
#define W5500_PORT_1                       8251
#define W5500_PORT_2                       0
#define W5500_PORT_3                       0
#define W5500_PORT_4                       0
#define W5500_PORT_5                       0
#define W5500_PORT_6                       0
#define W5500_PORT_7                       0

#define W5500_DESTINATION_IP_0             192, 168, 14,  2
#define W5500_DESTINATION_IP_1             192, 168, 14,  2
#define W5500_DESTINATION_IP_2             192, 168, 14,  2
#define W5500_DESTINATION_IP_3             192, 168, 14,  2
#define W5500_DESTINATION_IP_4             192, 168, 14,  2
#define W5500_DESTINATION_IP_5             192, 168, 14,  2
#define W5500_DESTINATION_IP_6             192, 168, 14,  2
#define W5500_DESTINATION_IP_7             192, 168, 14,  2

#define W5500_SOCKET_ID_0                  8250   // (0 = Disable This feature)
#define W5500_SOCKET_ID_1                  8251   // (0 = Disable This feature)
#define W5500_SOCKET_ID_2                  0      // (0 = Disable This feature)
#define W5500_SOCKET_ID_3                  0      // (0 = Disable This feature)
#define W5500_SOCKET_ID_4                  0      // (0 = Disable This feature)
#define W5500_SOCKET_ID_5                  0      // (0 = Disable This feature)
#define W5500_SOCKET_ID_6                  0      // (0 = Disable This feature)
#define W5500_SOCKET_ID_7                  0      // (0 = Disable This feature)
      
#define W5500_IDLE_TIMER_PERIOD_0          5000   // (0 = Disable This feature)
#define W5500_IDLE_TIMER_PERIOD_1          5000   // (0 = Disable This feature)
#define W5500_IDLE_TIMER_PERIOD_2          0      // (0 = Disable This feature)
#define W5500_IDLE_TIMER_PERIOD_3          0      // (0 = Disable This feature)
#define W5500_IDLE_TIMER_PERIOD_4          0      // (0 = Disable This feature)
#define W5500_IDLE_TIMER_PERIOD_5          0      // (0 = Disable This feature)
#define W5500_IDLE_TIMER_PERIOD_6          0      // (0 = Disable This feature)
#define W5500_IDLE_TIMER_PERIOD_7          0      // (0 = Disable This feature)

#define W5500_MEM_SIZE_0                   2
#define W5500_MEM_SIZE_1                   2
#define W5500_MEM_SIZE_2                   2
#define W5500_MEM_SIZE_3                   2
#define W5500_MEM_SIZE_4                   2
#define W5500_MEM_SIZE_5                   2
#define W5500_MEM_SIZE_6                   2
#define W5500_MEM_SIZE_7                   2

#define W5500_RETRY_CONN_DELAY             5
#define W5500_RETRY_COUNTS                 2
#define W5500_APIs_TIMEOUT                 10

#define W5500_RX_ENGINE_HEADER_0           ':'
#define W5500_RX_ENGINE_HEADER_1           ':'
#define W5500_RX_ENGINE_HEADER_2           ':'
#define W5500_RX_ENGINE_HEADER_3           ':'
#define W5500_RX_ENGINE_HEADER_4           ':'
#define W5500_RX_ENGINE_HEADER_5           ':'
#define W5500_RX_ENGINE_HEADER_6           ':'
#define W5500_RX_ENGINE_HEADER_7           ':'

#define W5500_RX_ENGINE_FOOTER_0           ';'
#define W5500_RX_ENGINE_FOOTER_1           ';'
#define W5500_RX_ENGINE_FOOTER_2           ';'
#define W5500_RX_ENGINE_FOOTER_3           ';'
#define W5500_RX_ENGINE_FOOTER_4           ';'
#define W5500_RX_ENGINE_FOOTER_5           ';'
#define W5500_RX_ENGINE_FOOTER_6           ';'
#define W5500_RX_ENGINE_FOOTER_7           ';'


#ifdef __cplusplus
  }
#endif   

/*************************************************************************/
/*****************************< Don't Touch >*****************************/
#define __CNT_SUM(_0, _1, _2, _3, _4, _5, _6, _7)\
                                           (_0 + _1 + _2 + _3 + _4 + _5 + _6 + _7)
#define W5500_MEM_SIZE                     W5500_MEM_SIZE_0,\
                                           W5500_MEM_SIZE_1,\
                                           W5500_MEM_SIZE_2,\
                                           W5500_MEM_SIZE_3,\
                                           W5500_MEM_SIZE_4,\
                                           W5500_MEM_SIZE_5,\
                                           W5500_MEM_SIZE_6,\
                                           W5500_MEM_SIZE_7
#define _CNT_SUM(...)                      __CNT_SUM(__VA_ARGS__)
#define _MEM_ARGS                          W5500_MEM_SIZE
#if _CNT_SUM(_MEM_ARGS) > 16 || _CNT_SUM(_MEM_ARGS) < 16
  #error "Total W5500 memory must not exceed 16 KB or Less (W5500_MEM_SIZE_x)"
#endif
#if W5500_PHY_LINK_INDEX > 4 || W5500_PHY_LINK_INDEX < 0
  #error "Invalid (W5500_PHY_LINK_INDEX)"
#endif 
#if W5500_PORT_0 == 0
  #error "W5500_PORT_0 can't be zeor"
#endif

#if (W5500_PORT_0 != 0 && W5500_MEM_SIZE_0 == 0) ||\
    (W5500_PORT_1 != 0 && W5500_MEM_SIZE_1 == 0) ||\
    (W5500_PORT_2 != 0 && W5500_MEM_SIZE_2 == 0) ||\
    (W5500_PORT_3 != 0 && W5500_MEM_SIZE_3 == 0) ||\
    (W5500_PORT_4 != 0 && W5500_MEM_SIZE_4 == 0) ||\
    (W5500_PORT_5 != 0 && W5500_MEM_SIZE_5 == 0) ||\
    (W5500_PORT_6 != 0 && W5500_MEM_SIZE_6 == 0) ||\
    (W5500_PORT_7 != 0 && W5500_MEM_SIZE_7 == 0)

  #error "W5500_MEM_SIZE can't be zeor"
#endif
#endif //__W5500_CONFIG_H_  
