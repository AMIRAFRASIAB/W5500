

#ifndef __W5500_SPI_H_
#define __W5500_SPI_H_

#ifdef __cplusplus
  extern "C" {
#endif   


#include <stdint.h>
#include <stdbool.h>



void vW5500RstLow (void);
void vW5500RstHigh (void);
void vW5500CsLow (void);
void vW5500CsHigh (void);
void vW5500SpiTransmit1Byte (uint8_t ucData);
uint8_t ucW5500SpiReceive1Byte (void);
void vW5500SpiTransmitBurstDMA (uint8_t* pucBuf, uint16_t usLen);
void vW5500SpiReceiveBurstDMA (uint8_t* pucBuf, uint16_t usLen);
bool bW5500HardWareInit (void);


extern bool bW5500IrqFlag;

#ifdef __cplusplus
  }
#endif   
#endif //__W5500_SPI_H_