
#include "stm32f4xx.h"
#include "stm32f4xx_ll_spi.h"
#include "stm32f4xx_ll_dma.h"
#include "stm32f4xx_ll_gpio.h"
#include "stm32f4xx_hal_rcc.h"
#include "w5500_config.h"
#include "w5500_spi_driver.h"
#include "swo.h"
#include "main.h"

#if W5500_USE_FreeRTOS == YES
  #include "FreeRTOS.h"
  #include "task.h"
  #include "semphr.h"
  static SemaphoreHandle_t hSemaphore = NULL;
#endif

#if W5500_TRACE_ENABLE == YES 
  #include "serial_debugger.h"
#else 
  #define LOG_TRACE(...)        
  #define LOG_INFO(...)
  #define LOG_WARNING(...)
  #define LOG_ERROR(...)
  #define LOG_FATAL(...)
#endif

static uint8_t flag = 0;
static uint8_t rxByte;
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
#define __HAL_RCC_SPI_CLK_ENABLE()   CONCAT(__HAL_RCC_SPI, W5500_SPI, _CLK_ENABLE)()
#define __HAL_RCC_DMATx_CLK_ENABLE() CONCAT(__HAL_RCC_DMA, W5500_DMA_TX_NUM, _CLK_ENABLE)()
#define __HAL_RCC_DMARx_CLK_ENABLE() CONCAT(__HAL_RCC_DMA, W5500_DMA_RX_NUM, _CLK_ENABLE)()

#define GPIO_CS                      CONCAT(GPIO, W5500_CS_GPIO)
#define GPIO_RST                     CONCAT(GPIO, W5500_RST_GPIO)
#define GPIO_MOSI                    CONCAT(GPIO, W5500_MOSI_GPIO)
#define GPIO_MISO                    CONCAT(GPIO, W5500_MISO_GPIO)
#define GPIO_SCLK                    CONCAT(GPIO, W5500_SCLK_GPIO)

#define LL_GPIO_PIN_CS               CONCAT(LL_GPIO_PIN_, W5500_CS_PIN)
#define LL_GPIO_PIN_RST              CONCAT(LL_GPIO_PIN_, W5500_RST_PIN)
#define LL_GPIO_PIN_MOSI             CONCAT(LL_GPIO_PIN_, W5500_MOSI_PIN)
#define LL_GPIO_PIN_MISO             CONCAT(LL_GPIO_PIN_, W5500_MISO_PIN)
#define LL_GPIO_PIN_SCLK             CONCAT(LL_GPIO_PIN_, W5500_SCLK_PIN)

#define LL_GPIO_AF_MOSI              CONCAT(LL_GPIO_AF_, W5500_MOSI_AF)
#define LL_GPIO_AF_MISO              CONCAT(LL_GPIO_AF_, W5500_MISO_AF)
#define LL_GPIO_AF_SCLK              CONCAT(LL_GPIO_AF_, W5500_SCLK_AF)

#define LL_DMA_ClearFlag(f, n)       CONCAT(LL_DMA_ClearFlag_, f, n)

/**************************************************************/
/* Private APIs */
/**************************************************************/
static void __w5500_gpio_init (void) {
  LOG_TRACE("W5500 :: GPIO initializing");
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
static bool __w5500_spi_init (void) {
  LOG_TRACE("W5500 :: SPI initializing");
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
    LOG_ERROR("W5500 SPI :: Failed to initialize the spi");
    return false;
  }
  #if W5500_USE_FreeRTOS == YES
  hSemaphore = xSemaphoreCreateBinary();
  if (hSemaphore == NULL) {
    LOG_ERROR("W5500 SPI :: Failed to create semaphore");
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
static void __w5500_dma_init (void) {
  #if W5500_SPI_USE_DMA == YES
  LOG_TRACE("W5500 :: DMA initializing");
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
static uint8_t __w5500_spi_TransmitReceive1Byte (uint8_t data) {
  uint32_t timeout = W5500_SPI_TIMEOUT;
  uint32_t start = W5500_GetTick();
  while (!LL_SPI_IsActiveFlag_TXE(SPI)) {
    if (W5500_GetTick() - start > timeout) {
      return 0xFF;
    }
    #if W5500_USE_FreeRTOS == YES
    taskYIELD();
    #endif
  }
  LL_SPI_TransmitData8(SPI, data);
  while (!LL_SPI_IsActiveFlag_RXNE(SPI)) {
    if (W5500_GetTick() - start > timeout) {
      return 0xFF;
    }
    #if W5500_USE_FreeRTOS == YES
    taskYIELD();
    #endif
  }
  uint8_t rxByte = LL_SPI_ReceiveData8(SPI);
  (void)LL_SPI_ReadReg(SPI, SR);
  return rxByte;
}
//-----------------------------------------------------------------------
// Public APIs 
//-----------------------------------------------------------------------
void w5500_cs_low (void) {
  CS = 0;
}
//-----------------------------------------------------------------------
void w5500_cs_high (void) {
  CS = 1;
}
//-----------------------------------------------------------------------
void w5500_spi_Transmit1Byte (uint8_t data) {
  __w5500_spi_TransmitReceive1Byte(data);
}
//----------------------------------------------------------------------- 
uint8_t w5500_spi_Receive1Byte (void) {
  return __w5500_spi_TransmitReceive1Byte(0x00);
}
//----------------------------------------------------------------------- 
void w5500_spi_TransmitBurstDMA (uint8_t* buf, uint16_t len) {
  #if W5500_SPI_USE_DMA == NO
  while (len-- > 0) {
    __w5500_spi_TransmitReceive1Byte(*buf++);
  }
  #else 
  uint32_t start = W5500_GetTick();
  while (LL_SPI_IsActiveFlag_BSY(SPI)) {
    if (W5500_GetTick() - start > W5500_SPI_TIMEOUT) {
      LOG_ERROR("W5500 :: spi tx :: busy flag timeout");
      return;
    }
    #if W5500_USE_FreeRTOS == YES
    taskYIELD();
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
  LL_DMA_SetMemoryAddress(DMATx, LL_DMA_STREAM_Tx, (uint32_t)buf);
  LL_DMA_SetMemoryAddress(DMARx, LL_DMA_STREAM_Rx, (uint32_t)&rxByte);
  LL_DMA_SetDataLength(DMATx, LL_DMA_STREAM_Tx, len);
  LL_DMA_SetDataLength(DMARx, LL_DMA_STREAM_Rx, len);
  LL_SPI_Enable(SPI);
  #if W5500_USE_FreeRTOS == YES
  xSemaphoreTake(hSemaphore, 0);
  LL_DMA_EnableStream(DMARx, LL_DMA_STREAM_Rx);
  LL_DMA_EnableStream(DMATx, LL_DMA_STREAM_Tx);
  if (xSemaphoreTake(hSemaphore, W5500_SPI_TIMEOUT) != pdTRUE) {
    LOG_ERROR("W5500 :: spi tx :: Failed to take the semaphore");
  }
  #else 
  flag = 1;
  LL_DMA_EnableStream(DMARx, LL_DMA_STREAM_Rx);
  LL_DMA_EnableStream(DMATx, LL_DMA_STREAM_Tx);
  while (flag) {
    if (W5500_GetTick() - start > W5500_SPI_TIMEOUT) {
      break;
    }
  }
  #endif
  #endif
}
//-----------------------------------------------------------------------
void w5500_spi_ReceiveBurstDMA (uint8_t* buf, uint16_t len) {
  #if W5500_SPI_USE_DMA == NO
  for (uint16_t i = 0; i < len; i++) {
    *buf++ = __w5500_spi_TransmitReceive1Byte(0x00);
  }
  #else 
  uint32_t start = W5500_GetTick();
  while (LL_SPI_IsActiveFlag_BSY(SPI)) {
    if (W5500_GetTick() - start > W5500_SPI_TIMEOUT) {
      LOG_ERROR("W5500 :: spi rx :: busy flag timeout");
    }
    #if W5500_USE_FreeRTOS == YES
    taskYIELD();
    #endif
  }
  LL_SPI_ClearFlag_OVR(SPI);
  static const uint8_t dummy = 0x00;
  LL_DMA_DisableStream(DMARx, LL_DMA_STREAM_Rx);
  LL_DMA_DisableStream(DMATx, LL_DMA_STREAM_Tx);
  __DSB();
  LL_DMA_ClearFlag(TC, W5500_DMA_RX_STREAM)(DMARx);
  LL_DMA_ClearFlag(TC, W5500_DMA_TX_STREAM)(DMATx);
  LL_DMA_EnableIT_TC(DMARx, LL_DMA_STREAM_Rx);
  LL_DMA_SetMemoryIncMode(DMATx, LL_DMA_STREAM_Tx, LL_DMA_MEMORY_NOINCREMENT);
  LL_DMA_SetMemoryIncMode(DMARx, LL_DMA_STREAM_Rx, LL_DMA_MEMORY_INCREMENT);
  LL_DMA_SetMemoryAddress(DMATx, LL_DMA_STREAM_Tx, (uint32_t)(&dummy));
  LL_DMA_SetMemoryAddress(DMARx, LL_DMA_STREAM_Rx, (uint32_t)buf);
  LL_DMA_SetDataLength(DMATx, LL_DMA_STREAM_Tx, len);
  LL_DMA_SetDataLength(DMARx, LL_DMA_STREAM_Rx, len);
  #if W5500_USE_FreeRTOS == YES
  xSemaphoreTake(hSemaphore, 0);
  LL_DMA_EnableStream(DMARx, LL_DMA_STREAM_Rx);
  LL_DMA_EnableStream(DMATx, LL_DMA_STREAM_Tx);
  if (xSemaphoreTake(hSemaphore, W5500_SPI_TIMEOUT) != pdTRUE) {
    LOG_ERROR("W5500 :: spi rx :: Failed to take the semaphore");
  }
  #else 
  flag = 1;
  LL_DMA_EnableStream(DMARx, LL_DMA_STREAM_Rx);
  LL_DMA_EnableStream(DMATx, LL_DMA_STREAM_Tx);
  while (flag) {
    if (W5500_GetTick() - start > W5500_SPI_TIMEOUT) {
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
  xSemaphoreGiveFromISR(hSemaphore, NULL);
  #else 
  flag = 0;
  #endif
}
#endif 
//-----------------------------------------------------------------------
bool w5500_spi_init (void) {
  bool status;
  __w5500_gpio_init();
  CS = 1;
  RST = 0;
  W5500_Delay(10);
  RST = 1;
  status = __w5500_spi_init();
  __w5500_dma_init();
  return status;
}
