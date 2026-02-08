
#include "w5500.h"
#include "w5500_config.h"
#include "stm32f4xx_ll_gpio.h"
#include "w5500_spi_driver.h"

#if W5500_USE_FreeRTOS == YES
#include "FreeRTOS.h"
#include "task.h"
#endif

#define _W5500_SPI_VDM_OP_          0x00
#define _W5500_SPI_FDM_OP_LEN1_     0x01
#define _W5500_SPI_FDM_OP_LEN2_     0x02
#define _W5500_SPI_FDM_OP_LEN4_     0x03

#define GPIOx  CONCAT(GPIO, W5500_CS_GPIO)
#define PINx   CONCAT(LL_GPIO_PIN_, W5500_CS_PIN)

#define CS_LOW()   LL_GPIO_ResetOutputPin(GPIOx, PINx)
#define CS_HIGH()  LL_GPIO_SetOutputPin(GPIOx, PINx)



#if   (_WIZCHIP_ == 5500)
////////////////////////////////////////////////////

uint8_t WIZCHIP_READ (uint32_t AddrSel) {
  uint8_t ret;
  uint8_t spi_data[3];
  uint8_t* restrict puc = spi_data;
//   WIZCHIP_CRITICAL_ENTER();
  CS_LOW();
  AddrSel |= (_W5500_SPI_READ_ | _W5500_SPI_VDM_OP_);
	*puc++ = (AddrSel & 0x00FF0000) >> 16;
	*puc++ = (AddrSel & 0x0000FF00) >> 8;
	*puc = AddrSel & 0x000000FF;
  #if W5500_SPI_USE_DMA == YES
  vW5500SpiTransmitBurstDMA(spi_data, 3);
  #else 
  uint8_t ucLen = sizeof(spi_data);
  puc = spi_data;
  while (ucLen-- > 0) {
    vW5500SpiTransmit1Byte(*puc++);
  }
  #endif
  ret  = ucW5500SpiReceive1Byte();
  CS_HIGH();
//   WIZCHIP_CRITICAL_EXIT();
  return ret;
}

void WIZCHIP_WRITE (uint32_t AddrSel, uint8_t wb) {
  uint8_t spi_data[4];
  uint8_t* restrict puc = spi_data; 
//  WIZCHIP_CRITICAL_ENTER();
  CS_LOW();
  AddrSel |= (_W5500_SPI_WRITE_ | _W5500_SPI_VDM_OP_);
	*puc++ = (AddrSel & 0x00FF0000) >> 16;
	*puc++ = (AddrSel & 0x0000FF00) >> 8;
	*puc++ = AddrSel & 0x000000FF;
	*puc = wb;  
  #if W5500_SPI_USE_DMA == YES
  vW5500SpiTransmitBurstDMA(spi_data, 4);
  #else 
  uint8_t ucLen = sizeof(spi_data);
  puc = spi_data;
  while (ucLen-- > 0) {
    vW5500SpiTransmit1Byte(*puc++);
  }
  #endif
  CS_HIGH();
//   WIZCHIP_CRITICAL_EXIT();
}
        
void WIZCHIP_READ_BUF (uint32_t AddrSel, uint8_t* pBuf, uint16_t len) {
  uint8_t spi_data[3];
  uint8_t* restrict puc = spi_data;
  uint16_t i;
//   WIZCHIP_CRITICAL_ENTER();
  CS_LOW();
  AddrSel |= (_W5500_SPI_READ_ | _W5500_SPI_VDM_OP_);
  *puc++ = (AddrSel & 0x00FF0000) >> 16;
  *puc++ = (AddrSel & 0x0000FF00) >> 8;
  *puc = AddrSel & 0x000000FF;
  #if W5500_SPI_USE_DMA == YES
  vW5500SpiTransmitBurstDMA(spi_data, 3);
  vW5500SpiReceiveBurstDMA(pBuf, len);
  #else 
  uint8_t ucLen = sizeof(spi_data);
  puc = spi_data;
  while (ucLen-- > 0) {
    vW5500SpiTransmit1Byte(*puc++);
  }
  while (len--> 0) {
    *pBuf++ = ucW5500SpiReceive1Byte();
  }
  #endif
  CS_HIGH();
//  WIZCHIP_CRITICAL_EXIT();
}

void WIZCHIP_WRITE_BUF (uint32_t AddrSel, uint8_t* pBuf, uint16_t len) {
  uint8_t spi_data[3];
  uint8_t* restrict puc = spi_data;
  uint16_t i;
//   WIZCHIP_CRITICAL_ENTER();
  CS_LOW();
  AddrSel |= (_W5500_SPI_WRITE_ | _W5500_SPI_VDM_OP_);
	*puc++ = (AddrSel & 0x00FF0000) >> 16;
	*puc++ = (AddrSel & 0x0000FF00) >> 8;
	*puc = AddrSel & 0x000000FF;
  #if W5500_SPI_USE_DMA == YES
  vW5500SpiTransmitBurstDMA(spi_data, 3);
  vW5500SpiTransmitBurstDMA(pBuf, len);
  #else 
  uint8_t ucLen = sizeof(spi_data);
  puc = spi_data;
  while (ucLen-- > 0) {
    vW5500SpiTransmit1Byte(*puc++);
  }
  while (len-- > 0) {
    vW5500SpiTransmit1Byte(*pBuf++);
  }
  #endif
  CS_HIGH();
//   WIZCHIP_CRITICAL_EXIT();
}

uint16_t getSn_TX_FSR (uint8_t sn) {
  uint16_t val=0,val1=0;
  uint32_t startTick = W5500_GetTick();
  do {
    val1 = WIZCHIP_READ(Sn_TX_FSR(sn));
    val1 = (val1 << 8) + WIZCHIP_READ(WIZCHIP_OFFSET_INC(Sn_TX_FSR(sn),1));
    if (val1 != 0) {
      val = WIZCHIP_READ(Sn_TX_FSR(sn));
      val = (val << 8) + WIZCHIP_READ(WIZCHIP_OFFSET_INC(Sn_TX_FSR(sn),1));
    }
    if (W5500_GetTick() - startTick > W5500_APIs_TIMEOUT) {
      break;
    }
    #if W5500_USE_FreeRTOS==YES
    taskYIELD();
    #endif
  } while (val != val1);
  return val;
}

uint16_t getSn_RX_RSR (uint8_t sn) {
  uint16_t val=0,val1=0;
  uint32_t startTick = W5500_GetTick();
  do {
    val1 = WIZCHIP_READ(Sn_RX_RSR(sn));
    val1 = (val1 << 8) + WIZCHIP_READ(WIZCHIP_OFFSET_INC(Sn_RX_RSR(sn),1));
    if (val1 != 0) {
      val = WIZCHIP_READ(Sn_RX_RSR(sn));
      val = (val << 8) + WIZCHIP_READ(WIZCHIP_OFFSET_INC(Sn_RX_RSR(sn),1));
    }
    if (W5500_GetTick() - startTick > W5500_APIs_TIMEOUT) {
      break;
    }
    #if W5500_USE_FreeRTOS==YES
    taskYIELD();
    #endif
  } while (val != val1);
  return val;
}

void wiz_send_data (uint8_t sn, uint8_t *wizdata, uint16_t len) {
  uint16_t ptr = 0;
  uint32_t addrsel = 0;
  if (len == 0) return;
  ptr = getSn_TX_WR(sn);
  addrsel = ((uint32_t)ptr << 8) + (WIZCHIP_TXBUF_BLOCK(sn) << 3);
  WIZCHIP_WRITE_BUF(addrsel,wizdata, len);
  ptr += len;
  setSn_TX_WR(sn,ptr);
}

void wiz_recv_data (uint8_t sn, uint8_t *wizdata, uint16_t len) {
  uint16_t ptr = 0;
  uint32_t addrsel = 0;
  if (len == 0) return;
  ptr = getSn_RX_RD(sn);
  addrsel = ((uint32_t)ptr << 8) + (WIZCHIP_RXBUF_BLOCK(sn) << 3);
  WIZCHIP_READ_BUF(addrsel, wizdata, len);
  ptr += len;
  setSn_RX_RD(sn,ptr);
}

void wiz_recv_ignore (uint8_t sn, uint16_t len) {
  uint16_t ptr = 0;
  ptr = getSn_RX_RD(sn);
  ptr += len;
  setSn_RX_RD(sn,ptr);
}

#endif
