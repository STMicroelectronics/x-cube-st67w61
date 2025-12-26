/**
  ******************************************************************************
  * @file    w6x_legacy.h
  * @author  GPM Application Team
  * @brief   This file provides the legacy W6x core resources definitions
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
#ifndef W6X_LEGACY_H
#define W6X_LEGACY_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Includes ------------------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */

/**
  * @brief  Write a file from the Host file system to the NCP file system
  * @param  filename: File name
  * @return Operation status
  */
static inline W6X_Status_t W6X_FS_WriteFile(char *filename)
{
  return W6X_FS_WriteFileByName(filename);
}

/**
  * @brief  Restore module default configuration
  * @note   If the ::W6X_WIFI_AUTOCONNECT is enabled, the Wi-Fi station credentials must be reconfigured
  *         through the ::W6X_WiFi_Connect function
  * @return Operation status
  */
static inline W6X_Status_t W6X_RestoreDefaultConfig(void)
{
  return W6X_Reset(1);
}

/**
  * @brief  Starts the NCP FWU process
  * @param  enable: 0 Terminate the FWU transmission. 1 Start the FWU transmission
  * @return Operation status
  */
static inline W6X_Status_t W6X_OTA_Starts(uint32_t enable)
{
  return W6X_FWU_Starts(enable);
}

/**
  * @brief  Finish the NCP FWU process which reboot the module to apply the new firmware
  * @return Operation status
  */
static inline W6X_Status_t W6X_OTA_Finish(void)
{
  return W6X_FWU_Finish();
}

/**
  * @brief  Send the firmware binary to the module
  * @param  buff: Buffer containing the firmware binary chunk
  * @param  len: Length of the firmware binary chunk
  * @return Operation status
  */
static inline W6X_Status_t W6X_OTA_Send(uint8_t *buff, uint32_t len)
{
  return W6X_FWU_Send(buff, len);
}

/**
  * @brief  Set the module in station mode
  * @return Operation status
  */
static inline W6X_Status_t W6X_WiFi_StartSta(void)
{
  return W6X_WiFi_Station_Start();
}

/**
  * @brief  This function set the Wi-Fi STA host name
  * @param  Hostname: Hostname to set
  * @return Operation status
  */
static inline W6X_Status_t W6X_WiFi_SetHostname(uint8_t Hostname[33])
{
  return W6X_Net_SetHostname(Hostname);
}

/**
  * @brief  This function retrieves the Wi-Fi STA host name
  * @param  Hostname: Hostname to get
  * @return Operation status
  */
static inline W6X_Status_t W6X_WiFi_GetHostname(uint8_t Hostname[33])
{
  return W6X_Net_GetHostname(Hostname);
}

/**
  * @brief  This function retrieves the Wi-Fi interface's IP address
  * @param  Ip_addr: IP address of the interface
  * @param  Gateway_addr: Gateway address of the interface
  * @param  Netmask_addr: Netmask address of the interface
  * @return Operation status
  */
static inline W6X_Status_t W6X_WiFi_GetStaIpAddress(uint8_t Ip_addr[4], uint8_t Gateway_addr[4],
                                                    uint8_t Netmask_addr[4])
{
  return W6X_Net_Station_GetIPAddress(Ip_addr, Gateway_addr, Netmask_addr);
}

/**
  * @brief  This function retrieves the Wi-Fi station MAC address
  * @param  Mac: MAC address of the interface
  * @return Operation status
  */
static inline W6X_Status_t W6X_WiFi_GetStaMacAddress(uint8_t Mac[6])
{
  return W6X_WiFi_Station_GetMACAddress(Mac);
}

/**
  * @brief  This function set the Wi-Fi interface's IP address
  * @param  Ipaddr: IP address to set
  * @param  Gateway_addr: Gateway address to set
  * @param  Netmask_addr: Netmask address to set
  * @return Operation status
  */
static inline W6X_Status_t W6X_WiFi_SetStaIpAddress(uint8_t Ipaddr[4], uint8_t Gateway_addr[4],
                                                    uint8_t Netmask_addr[4])
{
  return W6X_Net_Station_SetIPAddress(Ipaddr, Gateway_addr, Netmask_addr);
}

/**
  * @brief  This function retrieves the Wi-Fi DNS addresses
  * @param  Dns_enable: DNS enable status. Unused
  * @param  Dns1_addr: DNS1 address
  * @param  Dns2_addr: DNS2 address
  * @param  Dns3_addr: DNS3 address
  * @return Operation status
  */
static inline W6X_Status_t W6X_WiFi_GetDnsAddress(uint32_t *Dns_enable, uint8_t Dns1_addr[4], uint8_t Dns2_addr[4],
                                                  uint8_t Dns3_addr[4])
{
  (void)Dns_enable; /* unused */
  return W6X_Net_GetDnsAddress(Dns1_addr, Dns2_addr, Dns3_addr);
}

/**
  * @brief  This function set the Wi-Fi DNS addresses
  * @param  Dns_enable: DNS enable manually. Unused
  * @param  Dns1_addr: DNS1 address
  * @param  Dns2_addr: DNS2 address
  * @param  Dns3_addr: DNS3 address
  * @return Operation status
  */
static inline W6X_Status_t W6X_WiFi_SetDnsAddress(uint32_t *Dns_enable, uint8_t Dns1_addr[4], uint8_t Dns2_addr[4],
                                                  uint8_t Dns3_addr[4])
{
  (void)Dns_enable; /* unused */
  return W6X_Net_SetDnsAddress(Dns1_addr, Dns2_addr, Dns3_addr);
}

/**
  * @brief  This function retrieves the Wi-Fi station state
  * @param  State: Station state
  * @param  ConnectData: Connection data
  * @return Operation status
  */
static inline W6X_Status_t W6X_WiFi_GetStaState(W6X_WiFi_StaStateType_e *State, W6X_WiFi_Connect_t *ConnectData)
{
  return W6X_WiFi_Station_GetState(State, ConnectData);
}

/**
  * @brief  Configure a Soft-AP
  * @param  ap_config: Soft-AP configuration
  * @return Operation status
  */
static inline W6X_Status_t W6X_WiFi_StartAp(W6X_WiFi_ApConfig_t *ap_config)
{
  return W6X_WiFi_AP_Start(ap_config);
}

/**
  * @brief  Stop the Soft-AP
  * @return Operation status
  */
static inline W6X_Status_t W6X_WiFi_StopAp(void)
{
  return W6X_WiFi_AP_Stop();
}

/**
  * @brief  Get the Soft-AP configuration
  * @param  ap_config: Soft-AP configuration
  * @return Operation status
  */
static inline W6X_Status_t W6X_WiFi_GetApConfig(W6X_WiFi_ApConfig_t *ap_config)
{
  return W6X_WiFi_AP_GetConfig(ap_config);
}

/**
  * @brief  List the connected stations
  * @param  ConnectedSta: Connected stations structure
  * @return Operation status
  */
static inline W6X_Status_t W6X_WiFi_ListConnectedSta(W6X_WiFi_Connected_Sta_t *ConnectedSta)
{
  return W6X_WiFi_AP_ListConnectedStations(ConnectedSta);
}

/**
  * @brief  Disconnect station from the Soft-AP
  * @param  MAC: MAC address of the station to disconnect
  * @return Operation status
  */
static inline W6X_Status_t W6X_WiFi_DisconnectSta(uint8_t MAC[6])
{
  return W6X_WiFi_AP_DisconnectStation(MAC);
}

/**
  * @brief  This function retrieves the Wi-Fi Soft-AP MAC address
  * @param  Mac: MAC address of the interface
  * @return Operation status
  */
static inline W6X_Status_t W6X_WiFi_GetApMacAddress(uint8_t Mac[6])
{
  return W6X_WiFi_AP_GetMACAddress(Mac);
}

/**
  * @brief  Get the Soft-AP IP addresses
  * @param  Ipaddr: IP address of the Soft-AP
  * @param  Netmask_addr: Netmask address of the Soft-AP
  * @return Operation status
  */
static inline W6X_Status_t W6X_WiFi_GetApIpAddress(uint8_t Ipaddr[4], uint8_t Netmask_addr[4])
{
  return W6X_Net_AP_GetIPAddress(Ipaddr, Netmask_addr);
}

/**
  * @brief  DHCP Client type enumeration
  */
typedef W6X_Net_DhcpType_e W6X_WiFi_DhcpType_e;

/**
  * @brief  Get the DHCP configuration
  * @param  State: pointer to the DHCP state
  * @param  lease_time: lease time
  * @param  start_ip: pointer to the start IP address
  * @param  end_ip: pointer to the end IP address
  * @return Operation status
  */
static inline W6X_Status_t W6X_WiFi_GetDhcp(W6X_WiFi_DhcpType_e *State, uint32_t *lease_time,
                                            uint8_t start_ip[4], uint8_t end_ip[4])
{
  return W6X_Net_GetDhcp(State, lease_time, start_ip, end_ip);
}

/**
  * @brief  Set the DHCP configuration
  * @param  State: DHCP state
  * @param  Operate: pointer to enable / disable DHCP
  * @param  lease_time: lease time
  * @return Operation status
  */
static inline W6X_Status_t W6X_WiFi_SetDhcp(W6X_WiFi_DhcpType_e *State, uint32_t *Operate, uint32_t lease_time)
{
  return W6X_Net_SetDhcp(State, Operate, lease_time);
}

/**
  * @brief  Setup Target Wake Time (TWT) for the Wi-Fi station
  * @param  twt_params: TWT parameters
  * @return Operation status
  */
static inline W6X_Status_t W6X_WiFi_SetupTWT(W6X_WiFi_TWT_Setup_Params_t *twt_params)
{
  return W6X_WiFi_TWT_Setup(twt_params);
}

/**
  * @brief  Get Target Wake Time (TWT) status for the Wi-Fi station
  * @param  twt_status: Pointer to TWT flow status
  * @return Operation status
  */
static inline W6X_Status_t W6X_WiFi_GetTWTStatus(W6X_WiFi_TWT_Status_t *twt_status)
{
  return W6X_WiFi_TWT_GetStatus(twt_status);
}

/**
  * @brief  Teardown Target Wake Time (TWT) for the Wi-Fi station
  * @param  twt_params: TWT parameters
  * @return Operation status
  */
static inline W6X_Status_t W6X_WiFi_TeardownTWT(W6X_WiFi_TWT_Teardown_Params_t *twt_params)
{
  return W6X_WiFi_TWT_Teardown(twt_params);
}

/**
  * @brief  Get IP address from URL using DNS
  * @param  location: Host URL
  * @param  ipaddr: array of the IP address
  * @return Operation status
  */
static inline W6X_Status_t W6X_Net_GetHostAddress(const char *location, uint8_t *ipaddr)
{
  return W6X_Net_ResolveHostAddress(location, ipaddr);
}

/**
  * @brief  Query date string from SNTP, the used format is asctime style time
  * @param  Time: Buffer to store time
  * @return Operation status
  */
static inline W6X_Status_t W6X_Net_GetTime(uint8_t *Time)
{
  W6X_Status_t ret;
  W6X_Net_Time_t time_s;
  ret = W6X_Net_SNTP_GetTime(&time_s);
  if (ret != W6X_STATUS_OK)
  {
    return ret;
  }
  for (uint32_t i = 0; i < sizeof(time_s.raw); i++)
  {
    Time[i] = time_s.raw[i];
  }
  return W6X_STATUS_OK;
}

/**
  * @brief  Get current SNTP status, timezone and servers
  * @param  Enable:  SNTP usage status
  * @param  Timezone:  Configured Timezone
  * @param  SntpServer1:  Configured Primary SNTP Server URL
  * @param  SntpServer2:  Configured Secondary SNTP Server URL
  * @param  SntpServer3:  Configured Third SNTP Server URL
  * @return Operation status
  */
static inline W6X_Status_t W6X_Net_GetSNTPConfiguration(uint8_t *Enable, int16_t *Timezone, uint8_t *SntpServer1,
                                                        uint8_t *SntpServer2, uint8_t *SntpServer3)
{
  return W6X_Net_SNTP_GetConfiguration(Enable, Timezone, SntpServer1, SntpServer2, SntpServer3);
}

/**
  * @brief  Set SNTP status, timezone and servers
  * @param  Enable:  Enable/Disable SNTP usage
  * @param  Timezone:  Timezone to set in one of the 2 following formats:
  *                     - range [-12,14]: marks most of the time zones by offset from UTC in whole hours
  *                     - HHmm with HH in range[-12,+14] and mm in range [00,59]
  * @param  SntpServer1:  Primary SNTP Server URL to use
  * @param  SntpServer2:  Secondary SNTP Server URL to use
  * @param  SntpServer3:  Third SNTP Server URL to use
  * @return Operation status
  */
static inline W6X_Status_t W6X_Net_SetSNTPConfiguration(uint8_t Enable, int16_t Timezone, uint8_t *SntpServer1,
                                                        uint8_t *SntpServer2, uint8_t *SntpServer3)
{
  return W6X_Net_SNTP_SetConfiguration(Enable, Timezone, SntpServer1, SntpServer2, SntpServer3);
}

/**
  * @brief  Get SNTP Synchronization interval
  * @param  Interval:  Configured SNTP time synchronization interval, in seconds
  * @return Operation status
  */
static inline W6X_Status_t W6X_Net_GetSNTPInterval(uint16_t *Interval)
{
  return W6X_Net_SNTP_GetInterval(Interval);
}

/**
  * @brief  Set SNTP Synchronization interval
  * @param  Interval:  SNTP time synchronization interval, in seconds (range:[15,4294967])
  * @return Operation status
  */
static inline W6X_Status_t W6X_Net_SetSNTPInterval(uint16_t Interval)
{
  return W6X_Net_SNTP_SetInterval(Interval);
}

/**
  * @brief  Add the credential to the TLS context
  * @param  tag: Tag of the TLS context (must be unique by credential file)
  * @param  type: Type of the credential
  * @param  cred: Credential to add
  * @param  credlen: Length of the credential
  * @return Operation status
  */
static inline int32_t W6X_Net_Tls_Credential_Add(uint32_t tag, W6X_Net_Tls_Credential_e type,
                                                 const void *cred, size_t credlen)
{
  return W6X_Net_TLS_Credential_AddByName(tag, type, (const char *)cred);
}

/**
  * @brief  Delete the credential from the TLS context
  * @param  tag: Tag of the TLS context
  * @param  type: Type of the credential
  * @return Operation status
  */
static inline int32_t W6X_Net_Tls_Credential_Delete(uint32_t tag, W6X_Net_Tls_Credential_e type)
{
  return W6X_Net_TLS_Credential_Delete(tag, type);
}

/**
  * @brief  Indicate the Characteristic Value from the Server to a Client
  * @param  service_index: index of the service containing characteristic to indicate
  * @param  char_index: index of the characteristic to indicate
  * @param  pdata: pointer to the data to indicate
  * @param  req_len: length of the data to indicate
  * @param  sent_data_len: length of the data sent
  * @param  timeout: timeout in ms
  * @return Operation status
  */
static inline W6X_Status_t W6X_Ble_ServerSendIndication(uint8_t service_index, uint8_t char_index,
                                                        void *pdata, uint32_t req_len, uint32_t *sent_data_len,
                                                        uint32_t timeout)
{
  uint8_t conn_handle = 0;
  return W6X_Ble_ServerIndicate(conn_handle, service_index, char_index,
                                pdata, req_len, sent_data_len, timeout);
}

/**
  * @brief  Notify the Characteristic Value from the Server to a Client
  * @param  service_index: index of the service containing characteristic to notify
  * @param  char_index: index of the characteristic to notify
  * @param  pdata: pointer to the data to notify
  * @param  req_len: length of the data to notify
  * @param  sent_data_len: length of the data sent
  * @param  timeout: timeout in ms
  * @return Operation status
  */
static inline W6X_Status_t W6X_Ble_ServerSendNotification(uint8_t service_index, uint8_t char_index,
                                                          void *pdata, uint32_t req_len, uint32_t *sent_data_len,
                                                          uint32_t timeout)
{
  uint8_t conn_handle = 0;
  return W6X_Ble_ServerNotify(conn_handle, service_index, char_index,
                              pdata, req_len, sent_data_len, timeout);
}

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* W6X_LEGACY_H */
