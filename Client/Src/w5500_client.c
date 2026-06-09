
#include "w5500_spi_driver.h"
#include "w5500_client.h"
#include "w5500_config.h"
#include "socket.h"
#include "w5500_log.h"

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

// Phy Configs
wiz_PhyConf xPhyConfArray[5] = {
  [0] = {
  .by     = PHY_CONFBY_SW,
  .duplex = PHY_DUPLEX_HALF,
  .speed  = PHY_SPEED_10,
  .mode   = PHY_MODE_MANUAL
  },
  [1] = {
  .by     = PHY_CONFBY_SW,
  .duplex = PHY_DUPLEX_FULL,
  .speed  = PHY_SPEED_10,
  .mode   = PHY_MODE_MANUAL
  },
  [2] = {
  .by     = PHY_CONFBY_SW,
  .duplex = PHY_DUPLEX_HALF,
  .speed  = PHY_SPEED_100,
  .mode   = PHY_MODE_MANUAL
  },
  [3] = {
  .by     = PHY_CONFBY_SW,
  .duplex = PHY_DUPLEX_FULL,
  .speed  = PHY_SPEED_100,
  .mode   = PHY_MODE_MANUAL
  },
  [4] = {
  .by     = PHY_CONFBY_SW,
  .duplex = PHY_DUPLEX_HALF,
  .speed  = PHY_SPEED_100,
  .mode   = PHY_MODE_AUTONEGO
  },
};

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
  W5500_LOG_CABLE_DISCONNECT();
  return false;
}
//--------------------------------------------------------------------------
bool w5500_check_presence (void) {
  uint8_t version = getVERSIONR(); 
   if (version != 0x04) {
    W5500_LOG_VERSION_UNKNOWN(); //LOG
    return false;
  }
  return true;
}
//--------------------------------------------------------------------------
bool w5500_client_init (const W5500_Cnf_t* INFO, uint8_t ucPhyConfigInbdex) {
  #if (W5500_USER_NETWORK_CONFIG==NO)
  INFO = &STATIC_INFO;
  #else 
  if (INFO == NULL) {
    W5500_LOG_CONFIG_NULL(); //LOG
    return false;
  }
  #endif
  W5500_LOG_CLIENT_INIT(); //LOG
  if (!bW5500HardWareInit()) {
    goto FAIL;
  }
  reg_wizchip_cs_cbfunc(vW5500CsLow, vW5500CsHigh);
  reg_wizchip_spi_cbfunc(ucW5500SpiReceive1Byte, vW5500SpiTransmit1Byte);  
  reg_wizchip_spiburst_cbfunc(vW5500SpiReceiveBurstDMA, vW5500SpiTransmitBurstDMA);  
  uint8_t tmp;
	uint8_t memsize[2][8] = {
    { 2, 2, 2, 2, 2, 2, 2, 2 },
    { 2, 2, 2, 2, 2, 2, 2, 2 },
  };
  W5500_Delay(1);
  wizphy_reset();
  if (ctlwizchip(CW_INIT_WIZCHIP, (void*)memsize) == -1) {
		goto FAIL;
	}
  W5500_Delay(1);
  // Check W5500 ic presence
  if (!w5500_check_presence()) {
    goto FAIL;
  }  
  ctlnetwork(CN_SET_NETINFO, (void*)&INFO->info);
  w5500_cable_getStatus(3, 100);
  for (uint8_t i = 0; i < 8; i++) {
    disconnect(i);
    W5500_Delay(1);
    close(i);
  }
  wizphy_setphyconf(&xPhyConfArray[ucPhyConfigInbdex]);
  // Socket number is #1
  uint8_t sn = 1;              
  // Disable :confilct:unreach:pppoe:mp
  setIMR(0x00); 
  // Clear IMR status register
  setIR(0xFF);
  // Enable socket number #1 interrupt
  setSIMR(0x01 << sn);
  // Enable socket #1 RECV & TIMEOUT interrupt
  setSn_IMR(sn, Sn_IR_RECV | Sn_IR_DISCON);  
  // Clear socket #1 all interrupt flags
  setSn_IR(sn, 0xFF);                         
  // 16Kbyte internal stack
  setSn_TXBUF_SIZE(sn, 16);              
  // KeepAlive packet timer = 5s
//  setSn_KPALVTR(sn, 1);                    
  return true;
  
  FAIL:
  W5500_LOG_CLIENT_INIT_FAIL(); //LOG
  return false;
}
//--------------------------------------------------------------------------
int32_t w5500_client_transmit (uint8_t* buf, uint16_t len) {
  int32_t ret = send(1, buf, len);
  if (ret != len) {
    W5500_LOG_TRANSMIT_FAIL(); //LOG
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
  if (ret <= 0) {
    W5500_LOG_RECEIVE_FAIL();
    return 0;
  }
  return (uint16_t)ret;
}
//--------------------------------------------------------------------------
bool w5500_client_reconnect (const W5500_Cnf_t* INFO) {
  #if (W5500_USER_NETWORK_CONFIG==NO)
  INFO = &STATIC_INFO;
  #else 
  if (INFO == NULL) {
    W5500_LOG_CONFIG_NULL(); //LOG
    return false;
  }
  #endif
  if (!w5500_cable_getStatus(1, 0)) {
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
    W5500_LOG_SOCKET_FAIL();
    return false;
  }
  // Attempt to connect to the server
  if (connect(1, (uint8_t*)INFO->dest_ip, INFO->port) != SOCK_OK) {
    W5500_LOG_CONNECT_ATTEMP_FAIL();
    return false;
  }
  W5500_LOG_CONNECTED(); //LOG
  return true;
}
//--------------------------------------------------------------------------
bool w5500_client_disconnect (void) {
  uint8_t status = getSn_SR(1);
  // Already closed
  if (status == SOCK_CLOSED) {
      return true;
  }
  close(1);
  return (getSn_SR(1) == SOCK_CLOSED);
}
//--------------------------------------------------------------------------
