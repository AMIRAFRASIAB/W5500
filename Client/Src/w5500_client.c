
#include "w5500_spi_driver.h"
#include "w5500_client.h"
#include "w5500_config.h"
#include "socket.h"
#include "w5500_log.h"

#if W5500_USE_FreeRTOS == YES
#include "FreeRTOS.h"
#include "task.h"
#endif

static uint16_t usSocketId[8] = {
  W5500_SOCKET_ID_0,
  W5500_SOCKET_ID_1,
  W5500_SOCKET_ID_2,
  W5500_SOCKET_ID_3,
  W5500_SOCKET_ID_4,
  W5500_SOCKET_ID_5,
  W5500_SOCKET_ID_6,
  W5500_SOCKET_ID_7};

const uint16_t usPortW5500[8] = {
  W5500_PORT_0,
  W5500_PORT_1,
  W5500_PORT_2,
  W5500_PORT_3,
  W5500_PORT_4,
  W5500_PORT_5,
  W5500_PORT_6, 
  W5500_PORT_7};

static const uint8_t ucDstIpArray[8][4] = {
  {W5500_DESTINATION_IP_0},
  {W5500_DESTINATION_IP_1},
  {W5500_DESTINATION_IP_2},
  {W5500_DESTINATION_IP_3},
  {W5500_DESTINATION_IP_4},
  {W5500_DESTINATION_IP_5},
  {W5500_DESTINATION_IP_6},
  {W5500_DESTINATION_IP_7},
};  

static const W5500_Cnf_t xStaticConf = {
  .info = {
    .mac   = {W5500_MAC_ADDRESS},
    .ip    = {W5500_OWN_IP},
    .sn    = {W5500_SUBNET},
    .gw    = {W5500_GATEWAY},
    .dns   = {W5500_DNS},
    .dhcp  =  W5500_DHCP,
  },
};

// Phy Configs
static const wiz_PhyConf xPhyConfArray[5] = {
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
bool w5500_client_init (const W5500_Cnf_t* pxConf) {
  W5500_Cnf_t xConf;
  xConf = (pxConf != NULL)? *pxConf : xStaticConf;
  W5500_LOG_CLIENT_INIT(); //LOG
  if (!bW5500HardWareInit()) {
    goto FAIL;
  }
  reg_wizchip_cs_cbfunc(vW5500CsLow, vW5500CsHigh);
  reg_wizchip_spi_cbfunc(ucW5500SpiReceive1Byte, vW5500SpiTransmit1Byte);  
  reg_wizchip_spiburst_cbfunc(vW5500SpiReceiveBurstDMA, vW5500SpiTransmitBurstDMA);  
	uint8_t memsize[2][8] = {
    {W5500_MEM_SIZE},
    {W5500_MEM_SIZE},
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
  ctlnetwork(CN_SET_NETINFO, (void*)&xConf.info);
  w5500_cable_getStatus(3, 100);
  for (uint8_t i = 0; i < 8; i++) {
    disconnect(i);
    W5500_Delay(1);
    close(i);
  }
  wizphy_setphyconf((wiz_PhyConf*)&xPhyConfArray[W5500_PHY_LINK_INDEX]);
  
  // Disable :confilct:unreach:pppoe:mp
  setIMR(0x00);
  // Clear IMR status register
  setIR(0xFF);
  for (uint8_t i = 0; i < 8; i++) {
    if (usPortW5500[i] == 0) {
      continue;
    }
    // Enable socket number #i interrupt
    uint8_t ucTemp = getSIMR() | (0x01 << i);
    setSIMR(ucTemp);
    // Enable socket #i RECV & TIMEOUT interrupt
    setSn_IMR(i, Sn_IR_RECV | Sn_IR_DISCON);  
    // Clear socket #i all interrupt flags
    setSn_IR(i, 0xFF); 
  }
  return true;
  
  FAIL:
  W5500_LOG_CLIENT_INIT_FAIL(); //LOG
  return false;
}
//--------------------------------------------------------------------------
int32_t w5500_client_transmit (uint8_t* buf, uint16_t len, uint8_t ucSockNum) {
  if (buf == NULL || len == 0 || ucSockNum > 7) {
    return 0;
  }
  int32_t ret = send(ucSockNum, buf, len);
  if (ret != len) {
    W5500_LOG_TRANSMIT_FAIL(); //LOG
    return -1;
  }
  // Number of bytes actually sent
  return ret;
}
//--------------------------------------------------------------------------
uint16_t w5500_client_receive (uint8_t* buf, uint16_t len, uint8_t ucSockNum) {
  if (buf == NULL || len == 0 || ucSockNum > 7) {
    return 0;
  }
  int32_t rx_size = getSn_RX_RSR(ucSockNum);
  if (rx_size <= 0) {
    // No data or error
    return 0;
  }
  uint16_t to_read = (len < rx_size) ? len : (uint16_t)rx_size;
  int32_t ret = recv(ucSockNum, buf, to_read);
  if (ret <= 0) {
    W5500_LOG_RECEIVE_FAIL();
    return 0;
  }
  return (uint16_t)ret;
}
//--------------------------------------------------------------------------
bool w5500_client_reconnect (uint8_t ucSockNum) {
  if (ucSockNum > 7) {
    return false;
  }
  if (!w5500_cable_getStatus(1, 0)) {
    return false;
  }
  uint8_t status = getSn_SR(ucSockNum);
  if (status == SOCK_ESTABLISHED) {
    // Already connected
    return true;
  }
  // If socket is not closed, close it first
  if (status != SOCK_CLOSED) {
    close(ucSockNum);
  }
  // Create socket again
  if (socket(ucSockNum, Sn_MR_TCP, usSocketId[ucSockNum], 0) != ucSockNum) {
    W5500_LOG_SOCKET_FAIL();
    return false;
  }
  // Attempt to connect to the server
  if (connect(ucSockNum, (uint8_t*)&ucDstIpArray[ucSockNum][0], usPortW5500[ucSockNum]) != SOCK_OK) {
    W5500_LOG_CONNECT_ATTEMP_FAIL();
    return false;
  }
  W5500_LOG_CONNECTED(); //LOG
  return true;
}
//--------------------------------------------------------------------------
bool w5500_client_disconnect (uint8_t ucSockNum) {
  if (ucSockNum > 7) {
    return false;
  }
  uint8_t status = getSn_SR(ucSockNum);
  // Already closed
  if (status == SOCK_CLOSED) {
      return true;
  }
  close(ucSockNum);
  return (getSn_SR(ucSockNum) == SOCK_CLOSED);
}
//--------------------------------------------------------------------------
