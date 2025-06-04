/**
  ******************************************************************************
  * @file    w6x_api.h
  * @author  GPM Application Team
  * @brief   This file provides the different W6x core resources definitions
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef W6X_API_H
#define W6X_API_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Includes ------------------------------------------------------------------*/
#include <stdio.h>
#include <stdint.h>
#include "w6x_types.h"

/** @defgroup ST67W6X_API ST67W6X API
  */

/** @defgroup ST67W6X_API_System ST67W6X System
  * @ingroup  ST67W6X_API
  */

/** @defgroup ST67W6X_API_WiFi ST67W6X Wi-Fi
  * @ingroup  ST67W6X_API
  */

/** @defgroup ST67W6X_API_Net ST67W6X Net
  * @ingroup  ST67W6X_API
  */

/** @defgroup ST67W6X_API_OTA ST67W6X OTA
  * @ingroup  ST67W6X_API
  */

/** @defgroup ST67W6X_API_HTTP ST67W6X HTTP
  * @ingroup  ST67W6X_API
  */

/** @defgroup ST67W6X_API_MQTT ST67W6X MQTT
  * @ingroup  ST67W6X_API
  */

/** @defgroup ST67W6X_API_BLE ST67W6X BLE
  * @ingroup  ST67W6X_API
  */

/** @defgroup ST67W6X_Utilities ST67W6X Utilities
  */

/** @defgroup ST67W6X_Utilities_Common ST67W6X Utility Common
  * @ingroup  ST67W6X_Utilities
  */

/** @defgroup ST67W6X_Utilities_Performance ST67W6X Utility Performance
  * @ingroup  ST67W6X_Utilities
  */

/** @defgroup ST67W6X_Utilities_Logging ST67W6X Utility Logging
  * @ingroup  ST67W6X_Utilities
  */

/** @defgroup ST67W6X_Utilities_Shell ST67W6X Utility Shell
  * @ingroup  ST67W6X_Utilities
  */

/** @defgroup ST67W6X_Utilities_Performance_Mem_Perf ST67W6X Utility Performance Mem Perf
  * @ingroup  ST67W6X_Utilities_Performance
  */

/** @defgroup ST67W6X_Utilities_Performance_Task_Perf ST67W6X Utility Performance Task Perf
  * @ingroup  ST67W6X_Utilities_Performance
  */

/** @defgroup ST67W6X_Utilities_Performance_Iperf ST67W6X Utility Performance Iperf
  * @ingroup  ST67W6X_Utilities_Performance
  */

/* Exported constants --------------------------------------------------------*/
/* Exported types ------------------------------------------------------------*/
/* Exported macros -----------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */
/* ===================================================================== */
/** @defgroup ST67W6X_API_System_Public_Functions ST67W6X System Functions
  * @ingroup  ST67W6X_API_System
  * @{
  */
/* ===================================================================== */
/**
  * @brief  Initialize the LL part of the W6X core
  * @return Operation status
  */
W6X_Status_t W6X_Init(void);

/**
  * @brief  De-Initialize the LL part of the W6X core
  * @return Operation status
  */
W6X_Status_t W6X_DeInit(void);

/**
  * @brief  Register Upper Layer callbacks.
  * @param  App_cb: Callback for Applicative events
  * @return Operation status.
  */
W6X_Status_t W6X_RegisterAppCb(W6X_App_Cb_t *App_cb);

/**
  * @brief  Get the W6X Callback handler
  * @return W6X_App_Cb_t
  */
W6X_App_Cb_t *W6X_GetCbHandler(void);

/**
  * @brief  Get the W6X module info
  * @return W6X_ModuleInfo_t
  */
W6X_ModuleInfo_t *W6X_GetModuleInfo(void);

/**
  * @brief  Display the module information
  * @return Operation status
  */
W6X_Status_t W6X_ModuleInfoDisplay(void);

/**
  * @brief  Configure Power mode
  * @param  ps_mode: power save mode to set (0: normal mode, 1: standby mode)
  * @return Operation Status.
  */
W6X_Status_t W6X_SetPowerMode(uint32_t ps_mode);

/**
  * @brief  Get Power mode
  * @param  ps_mode: power save mode to set (0: normal mode, 1: standby mode)
  * @return Operation Status.
  */
W6X_Status_t W6X_GetPowerMode(uint32_t *ps_mode);

/**
  * @brief  Write a file to the NCP file system
  * @param  filename: File name
  * @return Operation status
  */
W6X_Status_t W6X_FS_WriteFile(char *filename);

/**
  * @brief  Read a file from the NCP file system
  * @param  filename: File name
  * @param  offset: Offset in the file
  * @param  data: Data to read
  * @param  len: Length of the data buffer
  * @return Operation status
  */
W6X_Status_t W6X_FS_ReadFile(char *filename, uint32_t offset, uint8_t *data, uint32_t len);

/**
  * @brief  Delete a file from the NCP file system
  * @param  filename: File name
  * @return Operation status
  */
W6X_Status_t W6X_FS_DeleteFile(char *filename);

/**
  * @brief  Get the size of a file in the NCP file system
  * @param  filename: File name
  * @param  size: Size of the file
  * @return Operation status
  */
W6X_Status_t W6X_FS_GetSizeFile(char *filename, uint32_t *size);

/**
  * @brief  List files in the file system (NCP and Host if LFS is enabled)
  * @param  files_list: List of files
  * @return Operation status
  */
W6X_Status_t W6X_FS_ListFiles(W6X_FS_FilesListFull_t **files_list);

/**
  * @brief  Restore module default configuration
  * @note   If the ::W6X_WIFI_AUTOCONNECT is enabled, the Wi-Fi station credentials must be reconfigured
  *         through the ::W6X_WiFi_Connect function
  * @return Operation status
  */
W6X_Status_t W6X_RestoreDefaultConfig(void);

/**
  * @brief  Execute AT command
  * @param  at_cmd: AT command string
  * @return Operation status
  */
W6X_Status_t W6X_ExeATCommand(char *at_cmd);

/**
  * @brief  Convert the W6X status to a string
  * @param  status: W6X status
  * @return Status string
  */
const char *W6X_StatusToStr(W6X_Status_t status);

/** @} */

/* ===================================================================== */
/** @defgroup ST67W6X_API_WiFi_Public_Functions ST67W6X Wi-Fi Functions
  * @ingroup  ST67W6X_API_WiFi
  * @{
  */
/* ===================================================================== */
/**
  * @brief  Init the Wi-Fi module
  * @return Operation status
  */
W6X_Status_t W6X_WiFi_Init(void);

/**
  * @brief  De-Init the WiFi module
  */
void W6X_WiFi_DeInit(void);

/**
  * @brief  Set the module in station mode
  * @return Operation status
  */
W6X_Status_t W6X_WiFi_StartSta(void);

/**
  * @brief  List a defined number of available access points
  * @param  Opts: Scan options
  * @param  cb: Callback to handle scan results
  * @return Operation status
  */
W6X_Status_t W6X_WiFi_Scan(W6X_WiFi_Scan_Opts_t *Opts, W6X_WiFi_Scan_Result_cb_t cb);

/**
  * @brief  Print the scan results
  * @param  Results: Scan results
  */
void W6X_WiFi_PrintScan(W6X_WiFi_Scan_Result_t *Results);

/**
  * @brief  Join an Access Point
  * @param  ConnectOpts: Connection options
  * @note   If the connection is successful, the Wi-Fi station credentials are stored in the NCP
  * @note   If the ::W6X_WIFI_AUTOCONNECT is enabled, the Wi-Fi station will be automatically reconnected
  *         at the next power on
  * @return Operation status
  */
W6X_Status_t W6X_WiFi_Connect(W6X_WiFi_Connect_Opts_t *ConnectOpts);

/**
  * @brief  Get the Wi-Fi station mode
  * @param  Mode: Wi-Fi mode
  * @return Operation status
  */
W6X_Status_t W6X_WiFi_GetStaMode(W6X_WiFi_Mode_t *Mode);

/**
  * @brief  Set the Wi-Fi station mode
  * @param  Mode: Wi-Fi mode
  * @return Operation status
  */
W6X_Status_t W6X_WiFi_SetStaMode(W6X_WiFi_Mode_t *Mode);

/**
  * @brief  Retrieve auto connect state
  * @param  OnOff : return the module state (enable = 1 / disable = 0) auto connect
  * @return Operation status
  */
W6X_Status_t W6X_WiFi_GetAutoConnect(uint32_t *OnOff);
/**
  * @brief  This function set the Wi-Fi STA host name.
  * @param  Hostname: Hostname to set
  * @return Operation status.
  */
W6X_Status_t W6X_WiFi_SetHostname(uint8_t Hostname[33]);

/**
  * @brief  This function retrieves the Wi-Fi STA host name.
  * @param  Hostname: Hostname to get
  * @return Operation status.
  */
W6X_Status_t W6X_WiFi_GetHostname(uint8_t Hostname[33]);

/**
  * @brief  This function retrieves the Wi-Fi interface's IP address.
  * @param  Ip_addr: IP address of the interface
  * @param  Gateway_addr: Gateway address of the interface
  * @param  Netmask_addr: Netmask address of the interface
  * @return Operation status.
  */
W6X_Status_t W6X_WiFi_GetStaIpAddress(uint8_t Ip_addr[4], uint8_t Gateway_addr[4], uint8_t Netmask_addr[4]);

/**
  * @brief  This function retrieves the Wi-Fi station MAC address.
  * @param  Mac: MAC address of the interface
  * @return Operation status.
  */
W6X_Status_t W6X_WiFi_GetStaMacAddress(uint8_t Mac[6]);

/**
  * @brief  This function set the Wi-Fi interface's IP address.
  * @param  Ipaddr: IP address to set
  * @param  Gateway_addr: Gateway address to set
  * @param  Netmask_addr: Netmask address to set
  * @return Operation status.
  */
W6X_Status_t W6X_WiFi_SetStaIpAddress(uint8_t Ipaddr[4], uint8_t Gateway_addr[4], uint8_t Netmask_addr[4]);

/**
  * @brief  This function retrieves the Wi-Fi DNS addresses.
  * @param  Dns_enable: DNS enable status
  * @param  Dns1_addr: DNS1 address
  * @param  Dns2_addr: DNS2 address
  * @param  Dns3_addr: DNS3 address
  * @return Operation status.
  */
W6X_Status_t W6X_WiFi_GetDnsAddress(uint32_t *Dns_enable, uint8_t Dns1_addr[4], uint8_t Dns2_addr[4],
                                    uint8_t Dns3_addr[4]);

/**
  * @brief  This function set the Wi-Fi DNS addresses.
  * @param  Dns_enable: DNS enable manually
  * @param  Dns1_addr: DNS1 address
  * @param  Dns2_addr: DNS2 address
  * @param  Dns3_addr: DNS3 address
  * @return Operation status.
  */
W6X_Status_t W6X_WiFi_SetDnsAddress(uint32_t *Dns_enable, uint8_t Dns1_addr[4], uint8_t Dns2_addr[4],
                                    uint8_t Dns3_addr[4]);

/**
  * @brief  This function retrieves the Wi-Fi station state.
  * @param  State: Station state
  * @param  ConnectData: Connection data
  * @return Operation status.
  */
W6X_Status_t W6X_WiFi_GetStaState(W6X_WiFi_StaStateType_e *State, W6X_WiFi_Connect_t *ConnectData);

/**
  * @brief  This function retrieves the country code configuration.
  * @param  Policy: value to specify if the country code align on AP's one
  * @param  CountryString: pointer to Country code string
  * @return Operation status.
  */
W6X_Status_t W6X_WiFi_GetCountryCode(uint32_t *Policy, char *CountryString);

/**
  * @brief  This function set the country code configuration.
  * @param  Policy: value to specify if the country code align on AP's one
  * @param  CountryString: pointer to Country code string
  * @return Operation status.
  */
W6X_Status_t W6X_WiFi_SetCountryCode(uint32_t *Policy, char *CountryString);

/**
  * @brief  Disconnect from a Wi-Fi Network
  * @param  restore: Remove the stored connection information in the NCP
  * @note   The ::W6X_WIFI_AUTOCONNECT won't be executed if the restore parameter is set to 1.
  *         The Wi-Fi credentials must be reconfigured through the ::W6X_WiFi_Connect function
  * @return Operation status
  */
W6X_Status_t W6X_WiFi_Disconnect(uint32_t restore);

/**
  * @brief  Configure a Soft-AP
  * @param  ap_config: Soft-AP configuration
  * @return Operation status
  */
W6X_Status_t W6X_WiFi_StartAp(W6X_WiFi_ApConfig_t *ap_config);

/**
  * @brief  Stop the Soft-AP
  * @return Operation status
  */
W6X_Status_t W6X_WiFi_StopAp(void);

/**
  * @brief  Get the Soft-AP configuration
  * @param  ap_config: Soft-AP configuration
  * @return Operation status
  */
W6X_Status_t W6X_WiFi_GetApConfig(W6X_WiFi_ApConfig_t *ap_config);

/**
  * @brief  List the connected stations
  * @param  ConnectedSta: Connected stations structure
  * @return Operation status
  */
W6X_Status_t W6X_WiFi_ListConnectedSta(W6X_WiFi_Connected_Sta_t *ConnectedSta);

/**
  * @brief  Disconnect station from the Soft-AP
  * @param  MAC: MAC address of the station to disconnect
  * @return Operation status
  */
W6X_Status_t W6X_WiFi_DisconnectSta(uint8_t MAC[6]);

/**
  * @brief  Get the Wi-Fi Soft-AP mode
  * @param  Mode: Wi-Fi mode
  * @return Operation status
  */
W6X_Status_t W6X_WiFi_GetApMode(W6X_WiFi_Mode_t *Mode);

/**
  * @brief  Set the Wi-Fi Soft-AP mode
  * @param  Mode: Wi-Fi mode
  * @return Operation status
  */
W6X_Status_t W6X_WiFi_SetApMode(W6X_WiFi_Mode_t *Mode);

/**
  * @brief  Get the Soft-AP IP addresses
  * @param  Ipaddr: IP address of the Soft-AP
  * @param  Netmask_addr: Netmask address of the Soft-AP
  * @return Operation status
  */
W6X_Status_t W6X_WiFi_GetApIpAddress(uint8_t Ipaddr[4], uint8_t Netmask_addr[4]);

/**
  * @brief  Get the DHCP configuration.
  * @param  State: pointer to the DHCP state
  * @param  lease_time: lease time
  * @param  start_ip: pointer to the start IP address
  * @param  end_ip: pointer to the end IP address
  * @return Operation status
  */
W6X_Status_t W6X_WiFi_GetDhcp(W6X_WiFi_DhcpType_e *State, uint32_t *lease_time, uint8_t start_ip[4], uint8_t end_ip[4]);

/**
  * @brief  Set the DHCP configuration.
  * @param  State: DHCP state
  * @param  Operate: pointer to enable / disable DHCP
  * @param  lease_time: lease time
  * @return Operation status
  */
W6X_Status_t W6X_WiFi_SetDhcp(W6X_WiFi_DhcpType_e *State, uint32_t *Operate, uint32_t lease_time);

/**
  * @brief  This function retrieves the Wi-Fi Soft-AP MAC address.
  * @param  Mac: MAC address of the interface
  * @return Operation status
  */
W6X_Status_t W6X_WiFi_GetApMacAddress(uint8_t Mac[6]);

/**
  * @brief  Convert the Wi-Fi state to a string
  * @param  state: Wi-Fi state
  * @return State string
  */
const char *W6X_WiFi_StateToStr(uint32_t state);

/**
  * @brief  Convert the Wi-Fi security type to a string
  * @param  security: Wi-Fi security type
  * @return Security type string
  */
const char *W6X_WiFi_SecurityToStr(uint32_t security);

/**
  * @brief  Convert the Wi-Fi error reason to a string
  * @param  reason: Wi-Fi error reason
  * @return Error reason string
  */
const char *W6X_WiFi_ReasonToStr(void *reason);

/**
  * @brief  Set Low Power Wi-Fi DTIM (Delivery Traffic Indication Message)
  * @param  dtim DTIM interval
  * @note   DTIM is negotiated with AP. Station wakes up every beacon interval (typ. 100ms) x dtim.
  * @return Operation status
  */
W6X_Status_t W6X_WiFi_SetDTIM(uint32_t dtim);

/**
  * @brief  Get Low Power Wi-Fi DTIM (Delivery Traffic Indication Message)
  * @param  dtim DTIM interval
  * @return Operation status
  */
W6X_Status_t W6X_WiFi_GetDTIM(uint32_t *dtim);

/**
  * @brief  Setup Target Wake Time (TWT) for the Wi-Fi station
  * @param  twt_params: TWT parameters
  * @return Operation status
  */
W6X_Status_t W6X_WiFi_SetupTWT(W6X_WiFi_TWT_Setup_Params_t *twt_params);

/**
  * @brief  Set Target Wake Time (TWT) for the Wi-Fi station
  * @return Operation status
  */
W6X_Status_t W6X_WiFi_SetTWT(void);

/**
  * @brief  Teardown Target Wake Time (TWT) for the Wi-Fi station
  * @param  twt_params: TWT parameters
  * @return Operation status
  */
W6X_Status_t W6X_WiFi_TeardownTWT(W6X_WiFi_TWT_Teardown_Params_t *twt_params);

/** @} */

/* ===================================================================== */
/** @defgroup ST67W6X_API_Net_Public_Functions ST67W6X Net Functions
  * @ingroup  ST67W6X_API_Net
  * @{
  */
/* ===================================================================== */
/**
  * @brief  Init the Net module
  * @return Operation status
  */
W6X_Status_t W6X_Net_Init(void);

/**
  * @brief  De-Init the Net module
  */
void W6X_Net_DeInit(void);

/**
  * @brief  Ping an IP address in the network
  * @param  location : URL or IP address to ping
  * @param  length : Length of the URL or IP address
  * @param  count : Number of pings to send
  * @param  interval_ms : Interval between pings
  * @param  average_time : Average time of the pings
  * @param  received_response : Number of received responses
  * @return Operation status
  */
W6X_Status_t W6X_Net_Ping(uint8_t *location, uint16_t length, uint16_t count, uint16_t interval_ms,
                          uint32_t *average_time, uint16_t *received_response);

/**
  * @brief  Get IP address from URL using DNS
  * @param  location : Host URL
  * @param  ipaddr : array of the IP address
  * @return Operation status
  */
W6X_Status_t W6X_Net_GetHostAddress(const char *location, uint8_t *ipaddr);

/**
  * @brief  Query date string from SNTP, the used format is asctime style time
  * @param  Time:  Buffer to store time
  * @return Operation status.
  */
W6X_Status_t W6X_Net_GetTime(uint8_t *Time);

/**
  * @brief  Get current SNTP status, timezone and servers
  * @param  Enable:  SNTP usage status
  * @param  Timezone:  Configured Timezone
  * @param  SntpServer1:  Configured Primary SNTP Server URL
  * @param  SntpServer2:  Configured Secondary SNTP Server URL
  * @param  SntpServer3:  Configured Third SNTP Server URL
  * @return Operation status.
  */
W6X_Status_t W6X_Net_GetSNTPConfiguration(uint8_t *Enable, int16_t *Timezone, uint8_t *SntpServer1,
                                          uint8_t *SntpServer2, uint8_t *SntpServer3);

/**
  * @brief  Set SNTP status, timezone and servers
  * @param  Enable:  Enable/Disable SNTP usage
  * @param  Timezone:  Timezone to set in one of the 2 following formats:
  *                     - range [-12,14]: marks most of the time zones by offset from UTC in whole hours
  *                     - HHmm with HH in range[-12,+14] and mm in range [00,59]
  * @param  SntpServer1:  Primary SNTP Server URL to use
  * @param  SntpServer2:  Secondary SNTP Server URL to use
  * @param  SntpServer3:  Third SNTP Server URL to use
  * @return Operation status.
  */
W6X_Status_t W6X_Net_SetSNTPConfiguration(uint8_t Enable, int16_t Timezone, uint8_t *SntpServer1,
                                          uint8_t *SntpServer2, uint8_t *SntpServer3);

/**
  * @brief  Get SNTP Synchronization interval
  * @param  Interval:  Configured SNTP time synchronization interval, in seconds
  * @return Operation status.
  */
W6X_Status_t W6X_Net_GetSNTPInterval(uint16_t *Interval);

/**
  * @brief  Set SNTP Synchronization interval
  * @param  Interval:  SNTP time synchronization interval, in seconds (range:[15,4294967])
  * @return Operation status.
  */
W6X_Status_t W6X_Net_SetSNTPInterval(uint16_t Interval);

/**
  * @brief  Get information for an opened socket
  * @param  Socket: Connection ID of the socket to get information for
  * @param  Protocol: Protocol used in this socket ("TCP" "UDP" or "SSL")
  * @param  RemoteIp: IP address the socket i connected to
  * @param  RemotePort: Port the socket i connected to
  * @param  LocalPort: Local port the socket uses
  * @param  Type:  if the device is the server for that socket, 0 if it is client
  * @return Operation status.
  */
W6X_Status_t W6X_Net_GetConnectionStatus(uint8_t Socket, uint8_t *Protocol, uint8_t *RemoteIp,
                                         uint32_t *RemotePort, uint32_t *LocalPort, uint8_t *Type);

/**
  * @brief  Get a socket instance is available
  * @param  family: IP Address family (AF_INET or AF_INET6)
  * @param  type: Socket type (SOCK_STREAM, SOCK_DGRAM, SOCK_RAW)
  * @param  proto: Protocol type (IPPROTO_TCP, IPPROTO_UDP, IPPROTO_RAW)
  * @return the socket instance if a socket is available, -1 otherwise
  */
int32_t W6X_Net_Socket(int32_t family, int32_t type, int32_t proto);

/**
  * @brief  Close a socket instance and release the associated resources
  * @param  sock: Socket ID to close
  * @return Operation status.
  */
int32_t W6X_Net_Close(int32_t sock);

/**
  * @brief  Shutdown a socket instance
  * @param  sock: Socket ID to shutdown
  * @param  how: How to shutdown the socket (SHUT_RD, SHUT_WR, SHUT_RDWR)
  * @return Operation status.
  */
int32_t W6X_Net_Shutdown(int32_t sock, int32_t how);

/**
  * @brief  Bind a socket instance to a specific address
  * @param  sock: Socket ID to bind
  * @param  addr: Address to bind to
  * @param  addrlen: Length of the address
  * @return Operation status.
  */
int32_t W6X_Net_Bind(int32_t sock, const struct sockaddr *addr, socklen_t addrlen);

/**
  * @brief  Connect a socket instance to a specific address
  * @param  sock: Socket ID to connect
  * @param  addr: Address to connect to
  * @param  addrlen: Length of the address
  * @return Operation status.
  */
int32_t W6X_Net_Connect(int32_t sock, const struct sockaddr *addr, socklen_t addrlen);

/**
  * @brief  Listen for incoming connections on a socket
  * @param  sock: Socket ID to listen on
  * @param  backlog: Maximum number of pending connections
  * @return Operation status.
  */
int32_t W6X_Net_Listen(int32_t sock, int32_t backlog);

/**
  * @brief  Accept an incoming connection on a socket
  * @param  sock: Socket ID to accept on
  * @param  addr: Address of the incoming connection
  * @param  addrlen: Length of the address
  * @return Operation status.
  */
int32_t W6X_Net_Accept(int32_t sock, struct sockaddr *addr, socklen_t *addrlen);

/**
  * @brief  Send data on a socket
  * @param  sock: Socket ID to send on
  * @param  buf: Buffer to send
  * @param  len: Length of the buffer
  * @param  flags: Flags to use
  * @return Number of bytes sent, or -1 on error
  */
ssize_t W6X_Net_Send(int32_t sock, const void *buf, size_t len, int32_t flags);

/**
  * @brief  Receive data from a socket
  * @param  sock: Socket ID to receive on
  * @param  buf: Buffer to receive into
  * @param  max_len: Maximum length of the buffer
  * @param  flags: Flags to use
  * @return Number of bytes received, or -1 on error
  */
ssize_t W6X_Net_Recv(int32_t sock, void *buf, size_t max_len, int32_t flags);

/**
  * @brief  Get a socket option
  * @param  sock: Socket ID to get the option from
  * @param  level: Level of the option
  * @param  optname: Name of the option
  * @param  optval: Buffer to store the option value
  * @param  optlen: Length of the option value buffer
  * @return Operation status.
  */
int32_t W6X_Net_Getsockopt(int32_t sock, int32_t level, int32_t optname, void *optval, socklen_t *optlen);

/**
  * @brief  Set a socket option
  * @param  sock: Socket ID to set the option on
  * @param  level: Level of the option
  * @param  optname: Name of the option
  * @param  optval: Value of the option
  * @param  optlen: Length of the option value
  * @return Operation status.
  */
int32_t W6X_Net_Setsockopt(int32_t sock, int32_t level, int32_t optname, const void *optval, socklen_t optlen);

/**
  * @brief  Send data on a socket to a specific address
  * @param  sock: Socket ID to send on
  * @param  buf: Buffer to send
  * @param  len: Length of the buffer
  * @param  flags: Flags to use
  * @param  dest_addr: Address to send to
  * @param  addrlen: Length of the address
  * @return Number of bytes sent, or -1 on error
  */
ssize_t W6X_Net_Sendto(int32_t sock, const void *buf, size_t len, int32_t flags, const struct sockaddr *dest_addr,
                       socklen_t addrlen);

/**
  * @brief  Receive data from a socket from a specific address
  * @param  sock: Socket ID to receive on
  * @param  buf: Buffer to receive into
  * @param  max_len: Maximum length of the buffer
  * @param  flags: Flags to use
  * @param  src_addr: Address to receive from
  * @param  addrlen: Length of the address
  * @return Number of bytes received, or -1 on error
  */
ssize_t W6X_Net_Recvfrom(int32_t sock, void *buf, size_t max_len, int32_t flags, struct sockaddr *src_addr,
                         socklen_t *addrlen);

/**
  * @brief  Add the credential to the TLS context
  * @param  tag: Tag of the TLS context (must be unique by credential file)
  * @param  type: Type of the credential
  * @param  cred: Credential to add
  * @param  credlen: Length of the credential
  * @return Operation status
  */
int32_t W6X_Net_Tls_Credential_Add(uint32_t tag, W6X_Net_Tls_Credential_e type, const void *cred, size_t credlen);

/**
  * @brief  Delete the credential from the TLS context
  * @param  tag: Tag of the TLS context
  * @param  type: Type of the credential
  * @return Operation status
  */
int32_t W6X_Net_Tls_Credential_Delete(uint32_t tag, W6X_Net_Tls_Credential_e type);

/**
  * @brief  Convert an IP address from uint32_t format to text
  * @param  af: Address family (AF_INET or AF_INET6)
  * @param  src: IP address
  * @param  dst: Destination buffer for the text IP address
  * @param  size: Size of the destination buffer
  * @return Pointer to converted IP address.
  */
char *W6X_Net_Inet_ntop(int32_t af, const void *src, char *dst, socklen_t size);

/**
  * @brief  Convert an IP address from text format to uint32_t
  * @param  af: Address family (AF_INET or AF_INET6)
  * @param  src: String containing the IP address to convert
  * @param  dst: 32bits integer to store IP address
  * @return Operation status (-1 on failure, 0 on success).
  */
int32_t W6X_Net_Inet_pton(int32_t af, char *src, const void *dst);
/** @} */

/* ===================================================================== */
/** @defgroup ST67W6X_API_OTA_Public_Functions ST67W6X OTA Functions
  * @ingroup  ST67W6X_API_OTA
  * @{
  */
/* ===================================================================== */

/**
  * @brief  Starts the NCP OTA process
  * @param  enable: 0 Terminate the OTA transmission. 1 Start the OTA transmission.
  * @return Operation status
  */
W6X_Status_t W6X_OTA_Starts(uint32_t enable);

/**
  * @brief  Finish the NCP OTA process which reboot the module to apply the new firmware
  * @return Operation status
  */
W6X_Status_t W6X_OTA_Finish(void);

/**
  * @brief  Send the firmware binary to the module
  * @param  buff: Buffer containing the firmware binary chunk
  * @param  len: Length of the firmware binary chunk
  * @return Operation status
  */
W6X_Status_t W6X_OTA_Send(uint8_t *buff, uint32_t len);

/** @} */

/* ===================================================================== */
/** @defgroup ST67W6X_API_HTTP_Public_Functions ST67W6X HTTP Functions
  * @ingroup  ST67W6X_API_HTTP
  * @{
  */
/* ===================================================================== */

/**
  * @brief  HTTP Client request based on BSD socket
  * @param  server_addr: Server address
  * @param  port: Server port
  * @param  uri: URI to request
  * @param  method: HTTP method to use
  * @param  post_data: Data to post
  * @param  post_data_len: Length of the data to post
  * @param  result_fn: Callback function to call when the request is done
  * @param  callback_arg: Argument to pass to the callback function
  * @param  headers_done_fn: Callback function to call when the headers are received
  * @param  data_fn: Callback function to call when data is received
  * @param  settings: Settings to use for the HTTP request
  * @return Operation status.
  */
W6X_Status_t W6X_HTTP_Client_Request(const ip_addr_t *server_addr, uint16_t port, const char *uri, const char *method,
                                     const void *post_data, size_t post_data_len, W6X_HTTP_result_cb_t result_fn,
                                     void *callback_arg, W6X_HTTP_headers_done_cb_t headers_done_fn,
                                     W6X_HTTP_data_cb_t data_fn, const W6X_HTTP_connection_t *settings);

/** @} */

/* ===================================================================== */
/** @defgroup ST67W6X_API_MQTT_Public_Functions ST67W6X MQTT Functions
  * @ingroup  ST67W6X_API_MQTT
  * @{
  */
/* ===================================================================== */
/**
  * @brief  Init the MQTT module
  * @param  p_mqtt_config: MQTT Received data configuration
  * @return Operation status
  */
W6X_Status_t W6X_MQTT_Init(W6X_MQTT_Data_t *p_mqtt_config);

/**
  * @brief  De-Init the MQTT module
  */
void W6X_MQTT_DeInit(void);

/**
  * @brief  Set/change the pointer where to copy the Recv Data
  * @param  p_mqtt_config: MQTT Received data configuration
  * @return Operation status
  * @note   This function shall only be called when executing the callback (never on applicative task)
  */
W6X_Status_t W6X_MQTT_SetRecvDataPtr(W6X_MQTT_Data_t *p_mqtt_config);

/**
  * @brief  MQTT Set user configuration
  * @param  Config: MQTT configuration
  * @return Operation status
  */
W6X_Status_t W6X_MQTT_Configure(W6X_MQTT_Connect_t *Config);

/**
  * @brief  MQTT Connect to broker
  * @param  Config: MQTT configuration
  * @return Operation status
  */
W6X_Status_t W6X_MQTT_Connect(W6X_MQTT_Connect_t *Config);

/**
  * @brief  MQTT Get connection status
  * @param  Config: MQTT configuration to get the current status
  * @return Operation status
  */
W6X_Status_t W6X_MQTT_GetConnectionStatus(W6X_MQTT_Connect_t *Config);

/**
  * @brief  MQTT Disconnect from broker
  * @return Operation status
  */
W6X_Status_t W6X_MQTT_Disconnect(void);

/**
  * @brief  MQTT Subscribe to a topic
  * @param  Topic: Topic to subscribe to
  * @return Operation status
  */
W6X_Status_t W6X_MQTT_Subscribe(uint8_t *Topic);

/**
  * @brief  MQTT Get subscribed topics
  * @return Operation status
  */
W6X_Status_t W6X_MQTT_GetSubscribedTopics(void);

/**
  * @brief  MQTT Unsubscribe from a topic
  * @param  Topic: Topic to unsubscribe from
  * @return Operation status
  */
W6X_Status_t W6X_MQTT_Unsubscribe(uint8_t *Topic);

/**
  * @brief  MQTT Publish a message to a topic
  * @param  Topic: Topic to publish to
  * @param  Message: Message to publish
  * @param  Message_len: Length of the message
  * @return Operation status
  */
W6X_Status_t W6X_MQTT_Publish(uint8_t *Topic, uint8_t *Message, uint32_t Message_len);

/**
  * @brief  Convert the MQTT state to a string
  * @param  state: MQTT state
  * @return State string
  */
const char *W6X_MQTT_StateToStr(uint32_t state);

/** @} */

/* ===================================================================== */
/** @defgroup ST67W6X_API_BLE_Public_Functions ST67W6X BLE Functions
  * @ingroup  ST67W6X_API_BLE
  * @{
  */
/* ===================================================================== */
/**
  * @brief  Get BLE initialization mode
  * @param  Mode: pointer to BLE mode (Server/Client)
  * @return Operation status
  */
W6X_Status_t W6X_Ble_GetInitMode(W6X_Ble_Mode_e *Mode);

/**
  * @brief  Initialize BLE Server/Client
  * @param  mode: BLE mode (Server/Client)
  * @param  p_recv_data: Pointer to the received data
  * @param  max_len: Maximum length of the received data
  * @return Operation status
  */
W6X_Status_t W6X_Ble_Init(W6X_Ble_Mode_e mode, uint8_t *p_recv_data, size_t max_len);

/**
  * @brief  De-Initialize BLE Server/Client
  */
void W6X_Ble_DeInit(void);

/**
  * @brief  Set/change the pointer where to copy the Recv Data.
  * @param  p_recv_data: Pointer to the received data
  * @param  recv_data_buf_size: Maximum length of the received data
  * @return Operation status
  * @note   This function shall only be called when executing the callback (never on applicative task)
  */
W6X_Status_t W6X_Ble_SetRecvDataPtr(uint8_t *p_recv_data, uint32_t recv_data_buf_size);

/**
  * @brief  BLE Set TX Power
  * @param  power: TX Power
  * @return Operation status
  */
W6X_Status_t W6X_Ble_SetTxPower(uint32_t power);

/**
  * @brief  BLE Get TX Power
  * @param  Power: TX Power
  * @return Operation status
  */
W6X_Status_t W6X_Ble_GetTxPower(uint32_t *Power);

/**
  * @brief  BLE Server Start Advertising
  * @return Operation status
  */
W6X_Status_t W6X_Ble_AdvStart(void);

/**
  * @brief  BLE Server Stop Advertising
  * @return Operation status
  */
W6X_Status_t W6X_Ble_AdvStop(void);

/**
  * @brief  Retrieves the BLE BD address
  * @param  BdAddr: BD address of the device
  * @return Operation status.
  */
W6X_Status_t W6X_Ble_GetBDAddress(uint8_t *BdAddr);

/**
  * @brief  Disconnect from a BLE remote device
  * @param  connection_handle: connection handle
  * @return Operation status.
  */
W6X_Status_t W6X_Ble_Disconnect(uint32_t connection_handle);

/**
  * @brief  Exchange BLE MTU length
  * @param  connection_handle: connection handle
  * @return Operation status.
  */
W6X_Status_t W6X_Ble_ExchangeMTU(uint32_t connection_handle);

/**
  * @brief  Set BLE BD Address
  * @param  bdaddr: BD Address
  * @return Operation status
  */
W6X_Status_t W6X_Ble_SetBdAddress(const uint8_t *bdaddr);

/**
  * @brief  Set BLE device Name
  * @param  name: BLE device name
  * @return Operation status
  */
W6X_Status_t W6X_Ble_SetDeviceName(char *name);

/**
  * @brief  This function retrieves the BLE device name.
  * @param  DeviceName: DeviceName to get
  * @return Operation status.
  */
W6X_Status_t W6X_Ble_GetDeviceName(char *DeviceName);

/**
  * @brief  Set BLE Advertising Data
  * @param  advdata: advertising data
  * @return Operation status
  */
W6X_Status_t W6X_Ble_SetAdvData(const char *advdata);

/**
  * @brief  Set BLE scan response Data
  * @param  scanrespdata: scan response data
  * @return Operation status
  */
W6X_Status_t W6X_Ble_SetScanRespData(const char *scanrespdata);

/**
  * @brief  Set BLE Advertising Parameters
  * @param  adv_int_min: minimum advertising interval this parameter is multiplied by 0.625ms
  * @param  adv_int_max: maximum advertising interval this parameter is multiplied by 0.625ms
  * @param  adv_type: Advertising type
  * @param  adv_channel: advertising channel
  * @return Operation status
  */
W6X_Status_t W6X_Ble_SetAdvParam(uint32_t adv_int_min, uint32_t adv_int_max,
                                 W6X_Ble_AdvType_e adv_type, W6X_Ble_AdvChannel_e adv_channel);

/**
  * @brief  Start BLE Device scan
  * @param  cb: Callback to handle scan results
  * @return Operation status
  */
W6X_Status_t W6X_Ble_StartScan(W6X_Ble_Scan_Result_cb_t cb);

/**
  * @brief  Stop BLE Device scan
  * @return Operation status
  */
W6X_Status_t W6X_Ble_StopScan(void);

/**
  * @brief  Set the BLE scan parameters
  * @param  scan_type: Type of scan (0:passive, 1:active)
  * @param  own_addr_type: type of BLE BD address
  * @param  filter_policy: Scan filter policy
  * @param  scan_interval: scan interval
  * @param  scan_window: scan window
  * @return Operation Status.
  */
W6X_Status_t W6X_Ble_SetScanParam(W6X_Ble_ScanType_e scan_type, W6X_Ble_AddrType_e own_addr_type,
                                  W6X_Ble_FilterPolicy_e filter_policy, uint32_t scan_interval, uint32_t scan_window);

/**
  * @brief  Get the Scan parameters
  * @param  ScanType: pointer to get scan type
  * @param  AddrType: pointer to get BLE BD address type
  * @param  ScanFilter: pointer to get scan filter
  * @param  ScanInterval: pointer to get scan interval this parameter is multiplied by 0.625ms
  * @param  ScanWindow: pointer to get scan window this parameter is multiplied by 0.625ms
  * @return Operation Status.
  */
W6X_Status_t W6X_Ble_GetScanParam(W6X_Ble_ScanType_e *ScanType, W6X_Ble_AddrType_e *AddrType,
                                  W6X_Ble_FilterPolicy_e *ScanFilter, uint32_t *ScanInterval, uint32_t *ScanWindow);

/**
  * @brief  Print the scan results
  * @param  Scan_results: pointer to Scan results
  */
void W6X_Ble_Print_Scan(W6X_Ble_Scan_Result_t *Scan_results);

/**
  * @brief  Get BLE Advertising Parameters
  * @param  AdvIntMin: pointer to get minimum advertising interval
  * @param  AdvIntMax: pointer to get maximum advertising interval
  * @param  AdvType: pointer to get advertising type
  * @param  ChannelMap: pointer to get advertising channel
  * @return Operation status
  */
W6X_Status_t W6X_Ble_GetAdvParam(uint32_t *AdvIntMin, uint32_t *AdvIntMax,
                                 W6X_Ble_AdvType_e *AdvType, W6X_Ble_AdvChannel_e *ChannelMap);

/**
  * @brief  Set BLE Connection Parameters
  * @param  conn_handle: BLE connection handle
  * @param  conn_int_min: minimum connecting interval this parameter is multiplied by 1.25ms
  * @param  conn_int_max: maximum connecting interval this parameter is multiplied by 1.25ms
  * @param  latency: latency
  * @param  timeout: connection timeout this parameter is multiplied by 10ms
  * @return Operation Status.
  */
W6X_Status_t W6X_Ble_SetConnParam(uint32_t conn_handle, uint32_t conn_int_min,
                                  uint32_t conn_int_max, uint32_t latency, uint32_t timeout);

/**
  * @brief  Get the connection parameters
  * @param  ConnHandle: pointer to get BLE connection handle
  * @param  ConnIntMin: pointer to get minimum connection interval
  * @param  ConnIntMax: pointer to get maximum connection interval
  * @param  ConnIntCurrent: pointer to get current connection interval
  * @param  Latency: pointer to get latency
  * @param  Timeout: pointer to get connection timeout
  * @return Operation Status.
  */
W6X_Status_t W6X_Ble_GetConnParam(uint32_t *ConnHandle, uint32_t *ConnIntMin,
                                  uint32_t *ConnIntMax, uint32_t *ConnIntCurrent, uint32_t *Latency, uint32_t *Timeout);

/**
  * @brief  Get the connection information
  * @param  ConnHandle: pointer to get BLE connection handle
  * @param  RemoteBDAddr: pointer to get the remote device address
  * @return Operation Status.
  */
W6X_Status_t W6X_Ble_GetConn(uint32_t *ConnHandle, uint8_t *RemoteBDAddr);

/**
  * @brief  Create connection to a remote device
  * @param  conn_handle: index of the BLE connection
  * @param  RemoteBDAddr: pointer to the remote device address
  * @return Operation Status.
  */
W6X_Status_t W6X_Ble_Connect(uint32_t conn_handle, uint8_t *RemoteBDAddr);

/**
  * @brief  Set the BLE Data length
  * @param  conn_handle: BLE connection handle
  * @param  tx_bytes: data packet length. Range [27,251]
  * @param  tx_trans_time: data packet transition time
  * @return Operation Status.
  */
W6X_Status_t W6X_Ble_SetDataLength(uint32_t conn_handle, uint32_t tx_bytes, uint32_t tx_trans_time);

/**
  * @brief  Create BLE Service
  * @param  service_index: index of the service to create
  * @param  service_uuid: UUID of the service to create
  * @param  uuid_type: UUID type of the service to create (0: 16-bit or 2: 128-bit)
  * @return Operation status.
  */
W6X_Status_t W6X_Ble_CreateService(uint8_t service_index, const char *service_uuid, uint8_t uuid_type);

/**
  * @brief  Delete BLE Service
  * @param  service_index: index of the service to create
  * @return Operation status.
  */
W6X_Status_t W6X_Ble_DeleteService(uint8_t service_index);

/**
  * @brief  Create BLE Characteristic
  * @param  service_index: index of the service containing the new characteristic
  * @param  char_index: index of the characteristic to create
  * @param  char_uuid: UUID of the characteristic to create
  * @param  uuid_type: UUID type of the characteristic to create (0: 16-bit or 2: 128-bit)
  * @param  char_property: property of the characteristic to create
  * @param  char_permission: permission of the characteristic to create
  * @return Operation status.
  */
W6X_Status_t W6X_Ble_CreateCharacteristic(uint8_t service_index, uint8_t char_index, const char *char_uuid,
                                          uint8_t uuid_type, uint8_t char_property, uint8_t char_permission);

/**
  * @brief  List BLE Services and their Characteristics
  * @param  ServicesTable: pointer to get the existing services and characteristics
  * @return Operation Status.
  */
W6X_Status_t W6X_Ble_GetServicesAndCharacteristics(W6X_Ble_Service_t ServicesTable[]);

/**
  * @brief  Register BLE characteristics
  * @return Operation status
  */
W6X_Status_t W6X_Ble_RegisterCharacteristics(void);

/**
  * @brief  Discover BLE services of remote device
  * @param  connection_handle: index of the BLE connection
  * @return Operation Status.
  */
W6X_Status_t W6X_Ble_RemoteServiceDiscovery(uint8_t connection_handle);

/**
  * @brief  Discover BLE Characteristics of a service remote device
  * @param  connection_handle: index of the BLE connection
  * @param  service_index: index of the BLE service to discover
  * @return Operation Status.
  */
W6X_Status_t W6X_Ble_RemoteCharDiscovery(uint8_t connection_handle, uint8_t service_index);

/**
  * @brief  Notify the Characteristic Value from the Server to a Client
  * @param  service_index: index of the service containing characteristic to notify
  * @param  char_index: index of the characteristic to notify
  * @param  pdata: pointer to the data to notify
  * @param  req_len: length of the data to notify
  * @param  sent_data_len: length of the data sent
  * @param  timeout: timeout in ms
  * @return Operation status.
  */
W6X_Status_t W6X_Ble_ServerSendNotification(uint8_t service_index, uint8_t char_index, void *pdata, uint32_t req_len,
                                            uint32_t *sent_data_len, uint32_t timeout);

/**
  * @brief  Indicate the Characteristic Value from the Server to a Client
  * @param  service_index: index of the service containing characteristic to indicate
  * @param  char_index: index of the characteristic to indicate
  * @param  pdata: pointer to the data to indicate
  * @param  req_len: length of the data to indicate
  * @param  sent_data_len: length of the data sent
  * @param  timeout: timeout in ms
  * @return Operation status.
  */
W6X_Status_t W6X_Ble_ServerSendIndication(uint8_t service_index, uint8_t char_index, void *pdata, uint32_t req_len,
                                          uint32_t *sent_data_len, uint32_t timeout);

/**
  * @brief  Set the data when Client read characteristic from the Server
  * @param  service_index: index of the service containing characteristic to read
  * @param  char_index: index of the characteristic to read
  * @param  pdata: pointer to the data to read
  * @param  req_len: length of the data to read
  * @param  sent_data_len: length of the data sent
  * @param  timeout: timeout in ms
  * @return Operation status.
  */
W6X_Status_t W6X_Ble_ServerSetReadData(uint8_t service_index, uint8_t char_index, void *pdata, uint32_t req_len,
                                       uint32_t *sent_data_len, uint32_t timeout);

/**
  * @brief  Write data to a Server characteristic
  * @param  conn_handle: BLE connection handle
  * @param  service_index: index of the service containing characteristic to write in
  * @param  char_index: index of the server characteristic to write in
  * @param  pdata: pointer to the data to write
  * @param  req_len: length of the data to write
  * @param  sent_data_len: length of the data sent
  * @param  timeout: timeout in ms
  * @return Operation status.
  */
W6X_Status_t W6X_Ble_ClientWriteData(uint8_t conn_handle, uint8_t service_index, uint8_t char_index,
                                     void *pdata, uint32_t req_len, uint32_t *sent_data_len, uint32_t timeout);

/**
  * @brief  Read data from a Server characteristic
  * @param  conn_handle: BLE connection handle
  * @param  service_index: index of the service to read data from
  * @param  char_index: index of the characteristic to read data from
  * @return Operation Status.
  */
W6X_Status_t W6X_Ble_ClientReadData(uint8_t conn_handle, uint8_t service_index, uint8_t char_index);

/**
  * @brief  Subscribe to notifications or indications from a Server characteristic
  * @param  conn_handle: BLE connection handle
  * @param  char_value_handle: Characteristic value handle
  * @param  char_prop: property of the characteristic to subscribe (1 = notification, 2 = indication)
  * @return Operation Status.
  */
W6X_Status_t W6X_Ble_ClientSubscribeChar(uint8_t conn_handle, uint8_t char_value_handle, uint8_t char_prop);

/**
  * @brief  Unsubscribe to notifications or indications from a Server characteristic
  * @param  conn_handle: BLE connection handle
  * @param  char_value_handle: Characteristic value handle
  * @return Operation Status.
  */
W6X_Status_t W6X_Ble_ClientUnsubscribeChar(uint8_t conn_handle, uint8_t char_value_handle);

/**
  * @brief  Set BLE security parameters
  * @param  security_parameter: BLE security parameter
  * @return Operation Status.
  */
W6X_Status_t W6X_Ble_SetSecurityParam(W6X_Ble_SecurityParameter_e security_parameter);

/**
  * @brief  Get BLE security parameters
  * @param  SecurityParameter: pointer to get BLE security parameter
  * @return Operation Status.
  */
W6X_Status_t W6X_Ble_GetSecurityParam(W6X_Ble_SecurityParameter_e *SecurityParameter);

/**
  * @brief  Start BLE security
  * @param  conn_handle: BLE connection handle
  * @param  security_level: security level. Range: [0,4]
  * @return Operation Status.
  */
W6X_Status_t W6X_Ble_SecurityStart(uint8_t conn_handle, uint8_t security_level);

/**
  * @brief  BLE security pass key confirm
  * @param  conn_handle: BLE connection handle
  * @return Operation Status.
  */
W6X_Status_t W6X_Ble_SecurityPassKeyConfirm(uint8_t conn_handle);

/**
  * @brief  BLE pairing confirm
  * @param  conn_handle: BLE connection handle
  * @return Operation Status.
  */
W6X_Status_t W6X_Ble_SecurityPairingConfirm(uint8_t conn_handle);

/**
  * @brief  BLE set passkey
  * @param  conn_handle: BLE connection handle
  * @param  passkey: BLE security passkey
  * @return Operation Status.
  */
W6X_Status_t W6X_Ble_SecuritySetPassKey(uint8_t conn_handle, uint32_t passkey);

/**
  * @brief  BLE pairing cancel
  * @param  conn_handle: BLE connection handle
  * @return Operation Status.
  */
W6X_Status_t W6X_Ble_SecurityPairingCancel(uint8_t conn_handle);

/**
  * @brief  BLE unpair
  * @param  RemoteBDAddr: remote device address
  * @param  remote_addr_type: remote address type
  * @return Operation Status.
  */
W6X_Status_t W6X_Ble_SecurityUnpair(uint8_t *RemoteBDAddr, W6X_Ble_AddrType_e remote_addr_type);

/**
  * @brief  BLE get paired device list
  * @param RemoteBondedDevices: pointer to bonded devices list
  * @return Operation Status.
  */
W6X_Status_t W6X_Ble_SecurityGetBondedDeviceList(W6X_Ble_Bonded_Devices_Result_t *RemoteBondedDevices);
/** @} */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* W6X_API_H */
