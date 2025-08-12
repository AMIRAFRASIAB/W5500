/**
 * @file w5500_client.c
 * @brief W5500 Ethernet client driver implementation.
 *
 * This file provides functions for initializing, connecting, sending,
 * receiving, reconnecting, and disconnecting a TCP client using the
 * W5500 Ethernet module.
 *
 * It handles SPI communication setup, network configuration,
 * socket management, and connection state monitoring.
 *
 * The driver supports optional FreeRTOS integration and configurable
 * debug tracing.
 *
 * @note This implementation uses socket number 1 for all client operations.
 * @note Network parameters can be set statically or dynamically based on
 *       compile-time configuration.
 *
 * @author AMIR HOSEIN BAGHERI (Whitewolf97@yahoo.com)
 * @date 2025-08-12
 */

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
/**
 * @brief Check the W5500 LAN cable link status with retries.
 * 
 * This function polls the PHY link status of the W5500 chip multiple times
 * with a delay between each attempt. It returns as soon as the cable link
 * is detected as active.
 * 
 * @param[in] tries Number of attempts to check the link status.
 * @param[in] delay Delay (in milliseconds) between each try.
 * 
 * @return true if the LAN cable link is active within the given tries; false otherwise.
 */
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
/**
 * @brief Initialize the W5500 Ethernet client with the specified network configuration.
 *
 * This function initializes the W5500 chip and its SPI interface, configures the network
 * settings, checks the LAN cable connection, and establishes a TCP connection to the server.
 *
 * @param[in] INFO Pointer to a W5500_Cnf_t structure containing network configuration information.
 *                 If `W5500_USER_NETWORK_CONFIG` is set to `NO`, this parameter is ignored and
 *                 a static configuration is used instead.
 *
 * @return true if the initialization and connection are successful; false otherwise.
 *
 * @note This function performs the following steps:
 *       - Initializes SPI communication with the W5500.
 *       - Registers chip select and SPI callback functions.
 *       - Initializes the Wiznet chip memory size.
 *       - Sets the network information on the W5500.
 *       - Checks the LAN cable status.
 *       - Closes any existing sockets and disconnects.
 *       - Creates a TCP socket and connects to the server specified in INFO.
 *
 * @warning If the INFO pointer is NULL when user configuration is enabled, the function returns false.
 * @warning Ensure that the W5500 hardware is correctly connected before calling this function.
 */
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
  LOG_TRACE("W5500 :: LAN Cable checking...");
  ctlnetwork(CN_SET_NETINFO, (void*)&INFO->info);
  if (!w5500_cable_getStatus(3, 100)) {
    LOG_ERROR("W5500 :: Cable is not connect");
    return false;
  }
  for (uint8_t i = 0; i < 8; i++) {
    disconnect(i);
    W5500_Delay(1);
    close(i);
  }
//  setRTR((W5500_RETRY_CONN_DELAY * 10)); // Retry timeout in 100us units -> = 25 ms
//  setRCR(W5500_RETRY_COUNTS);          // Retry count
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
/**
 * @brief Transmit data through the W5500 client socket.
 * 
 * This function sends `len` bytes of data from the provided buffer through
 * socket number 1. If the buffer is NULL or length is zero, no data is sent.
 * 
 * @param[in] buf Pointer to the buffer containing the data to send.
 * @param[in] len Number of bytes to send from the buffer.
 * 
 * @return The number of bytes actually sent on success.
 * @return -1 if sending failed.
 * @return 0 if buf is NULL or len is zero (no data sent).
 */
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
/**
 * @brief Receive data from the W5500 client socket.
 *
 * This function attempts to read up to `len` bytes of data from the W5500 socket
 * into the provided buffer. It first checks if data is available before attempting
 * to receive. If no data is available or an error occurs, it returns 0.
 *
 * @param[out] buf Pointer to the buffer where received data will be stored.
 * @param[in] len Maximum number of bytes to receive.
 *
 * @return The number of bytes actually received, or 0 if no data is available or an error occurred.
 *
 * @note Socket number 1 is used in this function.
 * @note Ensure the buffer has enough space for `len` bytes.
 */
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
/**
 * @brief Check if the W5500 client socket is connected.
 * 
 * This function checks the status of socket number 1 and returns whether
 * it is currently in the established (connected) state.
 * 
 * @return true if the socket is connected, false otherwise.
 */
bool w5500_client_is_connected (void) {
  // check socket 1 status
  uint8_t status = getSn_SR(1);  
  return (status == SOCK_ESTABLISHED);
}
//--------------------------------------------------------------------------
/**
 * @brief Attempt to reconnect the W5500 client socket to the server.
 *
 * This function checks the LAN cable status, socket status, and if not connected,
 * closes the socket if needed, recreates it, and tries to reconnect to the server
 * specified in the given network configuration.
 *
 * @param[in] INFO Pointer to a W5500_Cnf_t structure with network configuration.
 *                 If `W5500_USER_NETWORK_CONFIG` is set to `NO`, this parameter is ignored.
 *
 * @return true if the socket is already connected or reconnect was successful.
 * @return false if any step fails (cable disconnected, socket creation failure, or connection failure).
 *
 * @note Uses socket number 1 for connection.
 * @warning Ensure valid network configuration is provided if `W5500_USER_NETWORK_CONFIG` is enabled.
 */
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
/**
 * @brief Disconnect and close the W5500 client socket with a timeout.
 *
 * This function attempts to gracefully disconnect the socket number 1 and waits
 * until the socket status changes to closed or the specified timeout expires.
 * If the socket does not close within the timeout, it forces the socket to close.
 *
 * @param[in] timeout_ms Maximum time in milliseconds to wait for the socket to close.
 *
 * @return true if the socket is closed successfully; false otherwise.
 *
 */
bool w5500_client_disconnect (uint32_t timeout_ms) {
  uint8_t status = getSn_SR(1);
  // Already closed
  if (status == SOCK_CLOSED) {
      return true;
  }
  disconnect(1);
  uint32_t elapsed = 0;
  while (getSn_SR(1) != SOCK_CLOSED && elapsed < timeout_ms) {
    W5500_Delay(10);
    elapsed += 10;
  }
  // If still not closed, force close
  if (getSn_SR(1) != SOCK_CLOSED) {
    close(1);
  }
  return (getSn_SR(1) == SOCK_CLOSED);
}
//--------------------------------------------------------------------------
