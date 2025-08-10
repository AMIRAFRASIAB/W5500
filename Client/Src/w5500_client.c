
#include "w5500_spi_driver.h"
#include "w5500_client.h"
#include "w5500_config.h"
#include "socket.h"
#include "main.h"

#if W5500_TRACE_ENABLE == YES 
  #include "serial_debugger.h"
#else 
  #define LOG_TRACE(...)        
  #define LOG_INFO(...)
  #define LOG_WARNING(...)
  #define LOG_ERROR(...)
  #define LOG_FATAL(...)
#endif

#if W5500_USE_FreeRTOS == YES
#include "FreeRTOS.h"
#include "task.h"
#endif

#if (W5500_USER_NETWORK_CONFIG==NO)
const W5500_Cnf_t STATIC_INFO = {
  .info = {
    .mac   = {W5500_MAC_ADDRESS},
    .ip    = {W5500_OWN_IP},
    .sn    = {W5500_SUBNET},
    .gw    = {W5500_GATEWAY},
    .dns   = {W5500_DNS},
    .dhcp  =  W5500_DHCP,
  },
  .dest_ip = W5500_DESTINATION_IP,
  .port    = W5500_PORT,
};
#endif

//--------------------------------------------------------------------------
bool w5500_cable_getStatus (uint8_t tries, uint16_t delay) {
  uint8_t tmp;
  while (tries-- > 0) {
    ctlwizchip(CW_GET_PHYLINK, (void*)&tmp); 
    if (tmp != PHY_LINK_OFF) {
      return true;
    }
    W5500_Delay(delay);
  }
  return false;
}
//--------------------------------------------------------------------------
bool w5500_client_init (const W5500_Cnf_t* INFO) {
  #if (W5500_USER_NETWORK_CONFIG==NO)
  INFO = &STATIC_INFO;
  #else 
  if (INFO == NULL) {
    LOG_ERROR("W5500 :: NULL config");
    return false;
  }
  #endif
  LOG_TRACE("W5500 :: Client initializing...");
  if (!w5500_spi_init()) {
    LOG_ERROR("W5500 :: Failed to initial the SPI");
    return false;
  }
  reg_wizchip_cs_cbfunc(w5500_cs_low, w5500_cs_high);
  reg_wizchip_spi_cbfunc(w5500_spi_Receive1Byte, w5500_spi_Transmit1Byte);  
  reg_wizchip_spiburst_cbfunc(w5500_spi_ReceiveBurstDMA, w5500_spi_TransmitBurstDMA);  
  uint8_t tmp;
	uint8_t memsize[2][8] = {
    { 2, 2, 2, 2, 2, 2, 2, 2 },
    { 2, 2, 2, 2, 2, 2, 2, 2 },
  };
  if (ctlwizchip(CW_INIT_WIZCHIP, (void*)memsize) == -1) {
		LOG_ERROR("W5500 :: Failed to initial the LAN module");
		return false;
	}
  ctlnetwork(CN_SET_NETINFO, (void*)&INFO->info);
  if (!w5500_cable_getStatus(10, 500)) {
    LOG_ERROR("W5500 :: Cable is not connected");
    return false;
  }
  if (socket(1, Sn_MR_TCP, 0, 0) != 1) {
	  LOG_ERROR("W5500 :: Failed to create the socket");
    return false;
  }
  if (connect(1, (uint8_t*)INFO->dest_ip, INFO->port) != SOCK_OK) {
    LOG_ERROR("W5500 :: Can't connect to the server");
    return false;
  }
  LOG_TRACE("W5500 :: Initial success");
  return true;
}
//--------------------------------------------------------------------------
int32_t w5500_client_transmit (uint8_t* buf, uint16_t len) {
  if (buf == NULL || len == 0) {
    return 0;
  }
  int32_t ret = send(1, buf, len);
  if (ret == SOCK_ERROR) {
    LOG_ERROR("W5500 :: Send failed");
    return -1;
  }
  // Number of bytes actually sent
  return ret;  
}
//--------------------------------------------------------------------------
uint16_t w5500_client_receive (uint8_t* buf, uint16_t len) {
  if (buf == NULL || len == 0) {
    return 0;
  }
  int32_t rx_size = getSn_RX_RSR(1);
  if (rx_size <= 0) {
    // No data or error
    return 0;
  }
  uint16_t to_read = (len < rx_size) ? len : (uint16_t)rx_size;
  int32_t ret = recv(1, buf, to_read);
  if (ret == SOCK_ERROR || ret <= 0) {
    LOG_ERROR("W5500 :: Receive failed");
    return 0;
  }
  return (uint16_t)ret;
}
//--------------------------------------------------------------------------
bool w5500_client_is_connected (void) {
  // check socket 1 status
  uint8_t status = getSn_SR(1);  
  return (status == SOCK_ESTABLISHED);
}
//--------------------------------------------------------------------------
bool w5500_client_reconnect (const W5500_Cnf_t* INFO) {
  #if (W5500_USER_NETWORK_CONFIG==NO)
  INFO = &STATIC_INFO;
  #else 
  if (INFO == NULL) {
    LOG_ERROR("W5500 :: NULL config");
    return false;
  }
  #endif
  if (!w5500_cable_getStatus(1, 0)) {
    LOG_ERROR("W5500 :: Cable disconnect");
    return false;
  }
  uint8_t status = getSn_SR(1);
  if (status == SOCK_ESTABLISHED) {
    // Already connected
    return true;
  }
  // If socket is not closed, close it first
  if (status != SOCK_CLOSED) {
    close(1);
  }
  // Create socket again
  if (socket(1, Sn_MR_TCP, 0, 0) != 1) {
    LOG_ERROR("W5500 :: Failed to create socket");
    return false;
  }
  // Attempt to connect to the server
  if (connect(1, (uint8_t*)INFO->dest_ip, INFO->port) != SOCK_OK) {
    LOG_ERROR("W5500 :: Connect attempt failed");
    return false;
  }
  LOG_TRACE("W5500 :: Connected successfully");
  return true;
}
//--------------------------------------------------------------------------

