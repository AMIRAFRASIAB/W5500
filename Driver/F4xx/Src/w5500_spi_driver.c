
#include "stm32f4xx.h"
#include "stm32f4xx_ll_spi.h"
#include "stm32f4xx_ll_dma.h"
#include "stm32f4xx_ll_gpio.h"
#include "stm32f4xx_hal_rcc.h"
#include "stm32f4xx_ll_bus.h"
#include "stm32f4xx_ll_system.h"
#include "stm32f4xx_ll_exti.h"
#include "w5500_config.h"
#include "w5500_spi_driver.h"
#include "w5500_log.h"
#include "swo.h"


#if W5500_USE_FreeRTOS == YES
  #include "FreeRTOS.h"
  #include "task.h"
  #include "semphr.h"
  static SemaphoreHandle_t hSemaphore = NULL;
#endif
static uint8_t flag = 0;
static uint8_t rxByte;
bool bW5500IrqFlag = false;

/* Private Macros */
#define CS                           BB_GPIO_ODR(W5500_CS_GPIO, W5500_CS_PIN)
#define RST                          BB_GPIO_ODR(W5500_RST_GPIO, W5500_RST_PIN)
#define SPI                          CONCAT(SPI, W5500_SPI)
#define DMATx                        CONCAT(DMA, W5500_DMA_TX_NUM)
#define DMARx                        CONCAT(DMA, W5500_DMA_RX_NUM)
                                     
#define LL_DMA_STREAM_Tx             CONCAT(LL_DMA_STREAM_, W5500_DMA_TX_STREAM)
#define LL_DMA_STREAM_Rx             CONCAT(LL_DMA_STREAM_, W5500_DMA_RX_STREAM)
#define LL_DMA_CHANNEL_Tx            CONCAT(LL_DMA_CHANNEL_, W5500_DMA_TX_CHANNEL)
#define LL_DMA_CHANNEL_Rx            CONCAT(LL_DMA_CHANNEL_, W5500_DMA_RX_CHANNEL)

#define W5500_DMA_RX_IRQHandler      CONCAT(DMA, CONCAT(W5500_DMA_RX_NUM, _Stream, W5500_DMA_RX_STREAM, _IRQHandler))
#define W5500_DMA_RX_IRQn            CONCAT(DMA, CONCAT(W5500_DMA_RX_NUM, _Stream, W5500_DMA_RX_STREAM, _IRQn))


#define __HAL_RCC_CS_CLK_ENABLE()    CONCAT(__HAL_RCC_GPIO, W5500_CS_GPIO, _CLK_ENABLE)()
#define __HAL_RCC_MOSI_CLK_ENABLE()  CONCAT(__HAL_RCC_GPIO, W5500_MOSI_GPIO, _CLK_ENABLE)()
#define __HAL_RCC_MISO_CLK_ENABLE()  CONCAT(__HAL_RCC_GPIO, W5500_MISO_GPIO, _CLK_ENABLE)()
#define __HAL_RCC_SCLK_CLK_ENABLE()  CONCAT(__HAL_RCC_GPIO, W5500_SCLK_GPIO, _CLK_ENABLE)()
#define __HAL_RCC_RST_CLK_ENABLE()   CONCAT(__HAL_RCC_GPIO, W5500_RST_GPIO, _CLK_ENABLE)()
#define __HAL_RCC_IRQ_CLK_ENABLE()   CONCAT(__HAL_RCC_GPIO, W5500_IRQ_GPIO, _CLK_ENABLE)()
#define __HAL_RCC_SPI_CLK_ENABLE()   CONCAT(__HAL_RCC_SPI, W5500_SPI, _CLK_ENABLE)()
#define __HAL_RCC_DMATx_CLK_ENABLE() CONCAT(__HAL_RCC_DMA, W5500_DMA_TX_NUM, _CLK_ENABLE)()
#define __HAL_RCC_DMARx_CLK_ENABLE() CONCAT(__HAL_RCC_DMA, W5500_DMA_RX_NUM, _CLK_ENABLE)()

#define GPIO_CS                      CONCAT(GPIO, W5500_CS_GPIO)
#define GPIO_RST                     CONCAT(GPIO, W5500_RST_GPIO)
#define GPIO_MOSI                    CONCAT(GPIO, W5500_MOSI_GPIO)
#define GPIO_MISO                    CONCAT(GPIO, W5500_MISO_GPIO)
#define GPIO_SCLK                    CONCAT(GPIO, W5500_SCLK_GPIO)
#define GPIO_IRQ                     CONCAT(GPIO, W5500_IRQ_GPIO)

#define LL_GPIO_PIN_CS               CONCAT(LL_GPIO_PIN_, W5500_CS_PIN)
#define LL_GPIO_PIN_RST              CONCAT(LL_GPIO_PIN_, W5500_RST_PIN)
#define LL_GPIO_PIN_MOSI             CONCAT(LL_GPIO_PIN_, W5500_MOSI_PIN)
#define LL_GPIO_PIN_MISO             CONCAT(LL_GPIO_PIN_, W5500_MISO_PIN)
#define LL_GPIO_PIN_SCLK             CONCAT(LL_GPIO_PIN_, W5500_SCLK_PIN)
#define LL_GPIO_PIN_IRQ              CONCAT(LL_GPIO_PIN_, W5500_IRQ_PIN)

#define LL_GPIO_AF_MOSI              CONCAT(LL_GPIO_AF_, W5500_MOSI_AF)
#define LL_GPIO_AF_MISO              CONCAT(LL_GPIO_AF_, W5500_MISO_AF)
#define LL_GPIO_AF_SCLK              CONCAT(LL_GPIO_AF_, W5500_SCLK_AF)

#define LL_EXTI_LINE_IRQ             CONCAT(LL_EXTI_LINE_, W5500_IRQ_PIN)
#define LL_SYSCFG_EXTI_LINE_IRQ      CONCAT(LL_SYSCFG_EXTI_LINE, W5500_IRQ_PIN)

#define LL_DMA_ClearFlag(f, n)       CONCAT(LL_DMA_ClearFlag_, f, n)

/**************************************************************/
/* Private APIs */
/**************************************************************/
static void prvvW5500IrqPinInit (void) {  
  W5500_LOG_IRQ_INIT(); //LOG
  __HAL_RCC_SYSCFG_CLK_ENABLE();
  __HAL_RCC_IRQ_CLK_ENABLE();
  __DSB();
  LL_GPIO_SetPinMode(GPIO_IRQ, LL_GPIO_PIN_IRQ, LL_GPIO_MODE_INPUT);
  LL_GPIO_SetPinPull(GPIO_IRQ, LL_GPIO_PIN_IRQ, LL_GPIO_PULL_UP);
  LL_GPIO_LockPin(GPIO_IRQ, LL_GPIO_PIN_IRQ);
  LL_EXTI_EnableIT_0_31(LL_EXTI_LINE_IRQ);
  LL_EXTI_ClearFlag_0_31(LL_EXTI_LINE_IRQ);
  LL_EXTI_EnableFallingTrig_0_31(LL_EXTI_LINE_IRQ);
  LL_EXTI_DisableRisingTrig_0_31(LL_EXTI_LINE_IRQ);
  LL_SYSCFG_SetEXTISource(LL_SYSCFG_EXTI_PORTC, LL_SYSCFG_EXTI_LINE_IRQ);
  NVIC_ClearPendingIRQ(W5500_IRQn);
  NVIC_SetPriority(W5500_IRQn, W5500_IRQ_PIN_PRIORITY);
  NVIC_EnableIRQ(W5500_IRQn);
}
//-----------------------------------------------------------------------
static void prvvW5500GpioInit (void) {
  W5500_LOG_GPIO_INIT(); //LOG
  // CS
  __HAL_RCC_CS_CLK_ENABLE();
  LL_GPIO_SetPinMode(GPIO_CS, LL_GPIO_PIN_CS, LL_GPIO_MODE_OUTPUT);
  LL_GPIO_SetPinSpeed(GPIO_CS, LL_GPIO_PIN_CS, LL_GPIO_SPEED_FREQ_MEDIUM);
  LL_GPIO_LockPin(GPIO_CS, LL_GPIO_PIN_CS);
  // RST
  __HAL_RCC_RST_CLK_ENABLE();
  LL_GPIO_SetPinMode(GPIO_RST, LL_GPIO_PIN_RST, LL_GPIO_MODE_OUTPUT);
  LL_GPIO_LockPin(GPIO_RST, LL_GPIO_PIN_RST);
  // MOSI
  __HAL_RCC_MOSI_CLK_ENABLE();
  LL_GPIO_SetPinMode(GPIO_MOSI, LL_GPIO_PIN_MOSI, LL_GPIO_MODE_ALTERNATE);
  LL_GPIO_SetPinSpeed(GPIO_MOSI, LL_GPIO_PIN_MOSI, LL_GPIO_SPEED_FREQ_VERY_HIGH);
  #if W5500_MOSI_PIN <= 7
  LL_GPIO_SetAFPin_0_7(GPIO_MOSI, LL_GPIO_PIN_MOSI, LL_GPIO_AF_MOSI);
  #else 
  LL_GPIO_SetAFPin_8_15(GPIO_MOSI, LL_GPIO_PIN_MOSI, LL_GPIO_AF_MOSI);
  #endif
  LL_GPIO_LockPin(GPIO_MOSI, LL_GPIO_PIN_MOSI);
  // MISO
  __HAL_RCC_MISO_CLK_ENABLE();
  LL_GPIO_SetPinMode(GPIO_MISO, LL_GPIO_PIN_MISO, LL_GPIO_MODE_ALTERNATE);
  LL_GPIO_SetPinSpeed(GPIO_MISO, LL_GPIO_PIN_MISO, LL_GPIO_SPEED_FREQ_VERY_HIGH);
  #if W5500_MISO_PIN <= 7
  LL_GPIO_SetAFPin_0_7(GPIO_MISO, LL_GPIO_PIN_MISO, LL_GPIO_AF_MISO);
  #else 
  LL_GPIO_SetAFPin_8_15(GPIO_MISO, LL_GPIO_PIN_MISO, LL_GPIO_AF_MISO);
  #endif
  LL_GPIO_LockPin(GPIO_MISO, LL_GPIO_PIN_MISO);
  // SCLK
  __HAL_RCC_SCLK_CLK_ENABLE();
  LL_GPIO_SetPinMode(GPIO_SCLK, LL_GPIO_PIN_SCLK, LL_GPIO_MODE_ALTERNATE);
  LL_GPIO_SetPinSpeed(GPIO_SCLK, LL_GPIO_PIN_SCLK, LL_GPIO_SPEED_FREQ_VERY_HIGH);
  #if W5500_SCLK_PIN <= 7
  LL_GPIO_SetAFPin_0_7(GPIO_SCLK, LL_GPIO_PIN_SCLK, LL_GPIO_AF_SCLK);
  #else 
  LL_GPIO_SetAFPin_8_15(GPIO_SCLK, LL_GPIO_PIN_SCLK, LL_GPIO_AF_SCLK);
  #endif
  LL_GPIO_LockPin(GPIO_SCLK, LL_GPIO_PIN_SCLK);
}
//-----------------------------------------------------------------------
static bool prvbW5500SpiInit (void) {
  W5500_LOG_SPI_INIT(); //LOG
  __HAL_RCC_SPI_CLK_ENABLE();
  __DSB();
  LL_SPI_Disable(SPI);
  LL_SPI_InitTypeDef spix = {
    .BitOrder = LL_SPI_MSB_FIRST,
    .BaudRate = W5500_SPI_PRESCALER, 
    .ClockPhase = LL_SPI_PHASE_1EDGE,
    .ClockPolarity = LL_SPI_POLARITY_LOW,
    .CRCCalculation = LL_SPI_CRCCALCULATION_DISABLE,
    .CRCPoly = 10,
    .DataWidth = LL_SPI_DATAWIDTH_8BIT,
    .Mode = LL_SPI_MODE_MASTER,
    .NSS = LL_SPI_NSS_SOFT,
    .TransferDirection = LL_SPI_FULL_DUPLEX,
  };
  if (LL_SPI_Init(SPI, &spix) != SUCCESS) {
    W5500_LOG_SPI_INIT_FAIL() //LOG
    return false;
  }
  #if W5500_USE_FreeRTOS == YES
  hSemaphore = xSemaphoreCreateBinary();
  if (hSemaphore == NULL) {
    W5500_LOG_SPI_SMPHR_CREATE_FAIL(); //LOG
    return false;
  }
  #endif
  #if W5500_SPI_USE_DMA == YES
  LL_SPI_EnableDMAReq_RX(SPI);
  LL_SPI_EnableDMAReq_TX(SPI);
  #endif
  LL_SPI_Enable(SPI);
  return true;
}
//-----------------------------------------------------------------------
static void prvvW5500DmaInit (void) {
  #if W5500_SPI_USE_DMA == YES
  W5500_LOG_DMA_INIT(); //LOG
  /* Tx */
  __HAL_RCC_DMATx_CLK_ENABLE();
  __DSB();
  LL_DMA_DisableStream(DMATx, LL_DMA_STREAM_Tx);
  __DSB();
  LL_DMA_ClearFlag(DME, W5500_DMA_TX_STREAM)(DMATx);
  LL_DMA_ClearFlag(FE, W5500_DMA_TX_STREAM)(DMATx);
  LL_DMA_ClearFlag(HT, W5500_DMA_TX_STREAM)(DMATx);
  LL_DMA_ClearFlag(TC, W5500_DMA_TX_STREAM)(DMATx);
  LL_DMA_ClearFlag(TE, W5500_DMA_TX_STREAM)(DMATx);
  LL_DMA_SetChannelSelection(DMATx, LL_DMA_STREAM_Tx, LL_DMA_CHANNEL_Tx);
  LL_DMA_SetStreamPriorityLevel(DMATx, LL_DMA_STREAM_Tx, W5500_DMA_TX_STREAM_PRIORITY);
  LL_DMA_SetMemorySize(DMATx, LL_DMA_STREAM_Tx, LL_DMA_MDATAALIGN_BYTE);
  LL_DMA_SetPeriphSize(DMATx, LL_DMA_STREAM_Tx, LL_DMA_PDATAALIGN_BYTE);
  LL_DMA_SetMemoryIncMode(DMATx, LL_DMA_STREAM_Tx, LL_DMA_MEMORY_INCREMENT);
  LL_DMA_SetPeriphIncMode(DMATx, LL_DMA_STREAM_Tx, LL_DMA_PERIPH_NOINCREMENT);
  LL_DMA_SetDataTransferDirection(DMATx, LL_DMA_STREAM_Tx, LL_DMA_DIRECTION_MEMORY_TO_PERIPH);
  LL_DMA_SetPeriphAddress(DMATx, LL_DMA_STREAM_Tx, LL_SPI_DMA_GetRegAddr(SPI));
/*
  This 3 lines are not necessary any more
  LL_DMA_EnableIT_TC(DMATx, LL_DMA_STREAM_Tx);                    
  NVIC_SetPriority(W5500_DMA_TX_IRQn, W5500_DMA_TX_IRQ_PRIORITY); 
  NVIC_EnableIRQ(W5500_DMA_TX_IRQn);                              
*/  
  /* Rx */
  __HAL_RCC_DMARx_CLK_ENABLE();
  __DSB();
  LL_DMA_DisableStream(DMARx, LL_DMA_STREAM_Rx);
  __DSB();
  LL_DMA_ClearFlag(DME, W5500_DMA_RX_STREAM)(DMARx);
  LL_DMA_ClearFlag(FE, W5500_DMA_RX_STREAM)(DMARx);
  LL_DMA_ClearFlag(HT, W5500_DMA_RX_STREAM)(DMARx);
  LL_DMA_ClearFlag(TC, W5500_DMA_RX_STREAM)(DMARx);
  LL_DMA_ClearFlag(TE, W5500_DMA_RX_STREAM)(DMARx);
  LL_DMA_SetChannelSelection(DMARx, LL_DMA_STREAM_Rx, LL_DMA_CHANNEL_Rx);
  LL_DMA_SetStreamPriorityLevel(DMARx, LL_DMA_STREAM_Rx, W5500_DMA_RX_STREAM_PRIORITY);
  LL_DMA_SetMemorySize(DMARx, LL_DMA_STREAM_Rx, LL_DMA_MDATAALIGN_BYTE);
  LL_DMA_SetPeriphSize(DMARx, LL_DMA_STREAM_Rx, LL_DMA_PDATAALIGN_BYTE);
  LL_DMA_SetMemoryIncMode(DMARx, LL_DMA_STREAM_Rx, LL_DMA_MEMORY_INCREMENT);
  LL_DMA_SetPeriphIncMode(DMARx, LL_DMA_STREAM_Rx, LL_DMA_PERIPH_NOINCREMENT);
  LL_DMA_SetDataTransferDirection(DMARx, LL_DMA_STREAM_Rx, LL_DMA_DIRECTION_PERIPH_TO_MEMORY);
  LL_DMA_SetPeriphAddress(DMARx, LL_DMA_STREAM_Rx, LL_SPI_DMA_GetRegAddr(SPI));
  LL_DMA_EnableIT_TC(DMARx, LL_DMA_STREAM_Rx);
  NVIC_SetPriority(W5500_DMA_RX_IRQn, W5500_DMA_RX_IRQ_PRIORITY);
  NVIC_EnableIRQ(W5500_DMA_RX_IRQn);
  #endif
}
//-----------------------------------------------------------------------
static uint8_t prvucW5500SpiTransmitReceive1Byte (uint8_t data) {
  uint32_t ulStart = W5500_GetTick();
  while (!LL_SPI_IsActiveFlag_TXE(SPI)) {
    if (W5500_GetTick() - ulStart > W5500_SPI_TIMEOUT) {
      return 0xFF;
    }
    #if W5500_USE_FreeRTOS == YES
//    taskYIELD();
    #endif
  }
  LL_SPI_TransmitData8(SPI, data);
  while (!LL_SPI_IsActiveFlag_RXNE(SPI)) {
    if (W5500_GetTick() - ulStart > W5500_SPI_TIMEOUT) {
      return 0xFF;
    }
    #if W5500_USE_FreeRTOS == YES
//    taskYIELD();
    #endif
  }
  uint8_t ucByte = LL_SPI_ReceiveData8(SPI);
  (void)LL_SPI_ReadReg(SPI, SR);
  return ucByte;
}
/**************************************************************/
/* Public APIs */
/**************************************************************/
void vW5500RstLow (void) {
  RST = 0;
}
//-----------------------------------------------------------------------
void vW5500RstHigh (void) {
  RST = 1;
}
//-----------------------------------------------------------------------
void vW5500CsLow (void) {
  CS = 0;
}
//-----------------------------------------------------------------------
void vW5500CsHigh (void) {
  CS = 1;
}
//-----------------------------------------------------------------------
void vW5500SpiTransmit1Byte (uint8_t ucData) {
  prvucW5500SpiTransmitReceive1Byte(ucData);
}
//----------------------------------------------------------------------- 
uint8_t ucW5500SpiReceive1Byte (void) {
  return prvucW5500SpiTransmitReceive1Byte(0x00);
}
//----------------------------------------------------------------------- 
void vW5500SpiTransmitBurstDMA (uint8_t* pucBuf, uint16_t usLen) {
  #if W5500_SPI_USE_DMA == NO
  while (usLen-- > 0) {
    prvucW5500SpiTransmitReceive1Byte(*pucBuf++);
  }
  #else 
  uint32_t ulStart = W5500_GetTick();
  while (LL_SPI_IsActiveFlag_BSY(SPI)) {
    if (W5500_GetTick() - ulStart > W5500_SPI_TIMEOUT) {
      W5500_LOG_SPI_TX_BUSY(); //LOG
      return;
    }
    #if W5500_USE_FreeRTOS == YES
//    taskYIELD();
    #endif
  }
  LL_SPI_ClearFlag_OVR(SPI);
  LL_DMA_DisableStream(DMATx, LL_DMA_STREAM_Tx);
  LL_DMA_DisableStream(DMARx, LL_DMA_STREAM_Rx);
  LL_DMA_ClearFlag(TC, W5500_DMA_TX_STREAM)(DMATx);
  LL_DMA_ClearFlag(FE, W5500_DMA_TX_STREAM)(DMATx);
  LL_DMA_ClearFlag(TC, W5500_DMA_RX_STREAM)(DMARx);
  LL_DMA_ClearFlag(FE, W5500_DMA_RX_STREAM)(DMARx);
  LL_DMA_EnableIT_TC(DMARx, LL_DMA_STREAM_Rx);
  LL_DMA_SetMemoryIncMode(DMATx, LL_DMA_STREAM_Tx, LL_DMA_MEMORY_INCREMENT);
  LL_DMA_SetMemoryIncMode(DMARx, LL_DMA_STREAM_Rx, LL_DMA_MEMORY_NOINCREMENT);
  LL_DMA_SetMemoryAddress(DMATx, LL_DMA_STREAM_Tx, (uint32_t)pucBuf);
  LL_DMA_SetMemoryAddress(DMARx, LL_DMA_STREAM_Rx, (uint32_t)&rxByte);
  LL_DMA_SetDataLength(DMATx, LL_DMA_STREAM_Tx, usLen);
  LL_DMA_SetDataLength(DMARx, LL_DMA_STREAM_Rx, usLen);
  LL_SPI_Enable(SPI);
  #if W5500_USE_FreeRTOS == YES
  xSemaphoreTake(hSemaphore, 0);
  LL_DMA_EnableStream(DMARx, LL_DMA_STREAM_Rx);
  LL_DMA_EnableStream(DMATx, LL_DMA_STREAM_Tx);
  if (xSemaphoreTake(hSemaphore, W5500_SPI_TIMEOUT) != pdTRUE) {
    W5500_LOG_SMPHR_TAKE_FAIL(); //LOG
  }
  #else 
  flag = 1;
  LL_DMA_EnableStream(DMARx, LL_DMA_STREAM_Rx);
  LL_DMA_EnableStream(DMATx, LL_DMA_STREAM_Tx);
  while (flag) {
    if (W5500_GetTick() - ulStart > W5500_SPI_TIMEOUT) {
      break;
    }
  }
  #endif
  #endif
}
//-----------------------------------------------------------------------
void vW5500SpiReceiveBurstDMA (uint8_t* pucBuf, uint16_t usLen) {
  #if W5500_SPI_USE_DMA == NO
  while (usLen-- > 0) {
    *pucBuf++ = prvucW5500SpiTransmitReceive1Byte(0x00);
  }
  #else 
  uint32_t ulStart = W5500_GetTick();
  while (LL_SPI_IsActiveFlag_BSY(SPI)) {
    if (W5500_GetTick() - ulStart > W5500_SPI_TIMEOUT) {
      W5500_LOG_SPI_RX_BUSY(); //LOG
    }
    #if W5500_USE_FreeRTOS == YES
//    taskYIELD();
    #endif
  }
  LL_SPI_ClearFlag_OVR(SPI);
  static const uint8_t prvucDummyByte = 0x00;
  LL_DMA_DisableStream(DMARx, LL_DMA_STREAM_Rx);
  LL_DMA_DisableStream(DMATx, LL_DMA_STREAM_Tx);
  __DSB();
  LL_DMA_ClearFlag(TC, W5500_DMA_RX_STREAM)(DMARx);
  LL_DMA_ClearFlag(TC, W5500_DMA_TX_STREAM)(DMATx);
  LL_DMA_EnableIT_TC(DMARx, LL_DMA_STREAM_Rx);
  LL_DMA_SetMemoryIncMode(DMATx, LL_DMA_STREAM_Tx, LL_DMA_MEMORY_NOINCREMENT);
  LL_DMA_SetMemoryIncMode(DMARx, LL_DMA_STREAM_Rx, LL_DMA_MEMORY_INCREMENT);
  LL_DMA_SetMemoryAddress(DMATx, LL_DMA_STREAM_Tx, (uint32_t)(&prvucDummyByte));
  LL_DMA_SetMemoryAddress(DMARx, LL_DMA_STREAM_Rx, (uint32_t)pucBuf);
  LL_DMA_SetDataLength(DMATx, LL_DMA_STREAM_Tx, usLen);
  LL_DMA_SetDataLength(DMARx, LL_DMA_STREAM_Rx, usLen);
  #if W5500_USE_FreeRTOS == YES
  xSemaphoreTake(hSemaphore, 0);
  LL_DMA_EnableStream(DMARx, LL_DMA_STREAM_Rx);
  LL_DMA_EnableStream(DMATx, LL_DMA_STREAM_Tx);
  if (xSemaphoreTake(hSemaphore, W5500_SPI_TIMEOUT) != pdTRUE) {
    W5500_LOG_SMPHR_TAKE_FAIL(); //LOG
  }
  #else 
  flag = 1;
  LL_DMA_EnableStream(DMARx, LL_DMA_STREAM_Rx);
  LL_DMA_EnableStream(DMATx, LL_DMA_STREAM_Tx);
  while (flag) {
    if (W5500_GetTick() - ulStart > W5500_SPI_TIMEOUT) {
      break;
    }
  }
  #endif
  #endif
}
//-----------------------------------------------------------------------
#if W5500_SPI_USE_DMA == YES
void W5500_DMA_RX_IRQHandler (void) {
  LL_DMA_ClearFlag(TC, W5500_DMA_RX_STREAM)(DMARx);
  #if W5500_USE_FreeRTOS == YES
  xSemaphoreGiveFromISR(hSemaphore, &(BaseType_t){pdTRUE});
  #else 
  flag = 0;
  #endif
}
#endif 
//-----------------------------------------------------------------------
bool bW5500HardWareInit (void) {
  bool status;
  prvvW5500GpioInit();
  prvvW5500IrqPinInit();
  CS = 1;
  RST = 0;
  W5500_Delay(10);
  RST = 1;
  status = prvbW5500SpiInit();
  prvvW5500DmaInit();
  return status;
}
//-----------------------------------------------------------------------
void W5500_IRQHandler (void) {
  LL_EXTI_ClearFlag_0_31(LL_EXTI_LINE_IRQ);
  bW5500IrqFlag = true;
}
