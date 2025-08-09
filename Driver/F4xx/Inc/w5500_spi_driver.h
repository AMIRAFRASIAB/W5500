

#ifndef __W5500_SPI_H_
#define __W5500_SPI_H_

#ifdef __cplusplus
  extern "C" {
#endif   


#include <stdint.h>
#include <stdbool.h>



bool    w5500_spi_init (void);
void    w5500_spi_ReceiveBurstDMA (uint8_t* buf, uint16_t len);
void    w5500_spi_TransmitBurstDMA (uint8_t* buf, uint16_t len);
uint8_t w5500_spi_Receive1Byte (void);
void    w5500_spi_Transmit1Byte (uint8_t data);
void    w5500_cs_high (void);
void    w5500_cs_low (void);



#ifdef __cplusplus
  }
#endif   
#endif //__W5500_SPI_H_