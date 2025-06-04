/**
  ******************************************************************************
  * @file    w6x_default_config.h
  * @author  GPM Application Team
  * @brief   Header file for the W61 configuration module
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
#ifndef W6X_DEFAULT_CONFIG_H
#define W6X_DEFAULT_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Includes ------------------------------------------------------------------*/
#include "w6x_config.h"

/* Exported constants --------------------------------------------------------*/
/** @defgroup ST67W6X_API_System_Public_Constants ST67W6X System Constants
  * @ingroup  ST67W6X_API_System
  * @{
  */

#ifndef W6X_POWER_SAVE_AUTO
/** NCP will go by default in low power mode when NCP is in idle mode */
#define W6X_POWER_SAVE_AUTO                     1
#endif /* W6X_POWER_SAVE_AUTO */

#ifndef W6X_CLOCK_MODE
/** NCP clock mode : 1: Internal RC oscillator, 2: External passive crystal, 3: External active crystal */
#define W6X_CLOCK_MODE                          1
#endif /* W6X_CLOCK_MODE */

/** @} */

/** @addtogroup ST67W6X_API_WiFi_Public_Constants
  * @{
  */

#ifndef W6X_WIFI_AUTOCONNECT
/** Boolean to enable/disable autoconnect functionality. */
#define W6X_WIFI_AUTOCONNECT                    0
#endif /* W61_AUTOCONNECT */

#ifndef W6X_WIFI_DHCP
/** Define the DHCP configuration : 0: NO DHCP, 1: DHCP CLIENT STA, 2:DHCP SERVER AP, 3: DHCP STA+AP */
#define W6X_WIFI_DHCP                           1
#endif /* W6X_WIFI_DHCP */

#ifndef W6X_WIFI_SAP_MAX_CONNECTED_STATIONS
/** Define the max number of stations that can connect to the Soft-AP */
#define W6X_WIFI_SAP_MAX_CONNECTED_STATIONS     4
#endif /* W6X_WIFI_SAP_MAX_CONNECTED_STATIONS */

#ifndef W6X_WIFI_SAP_IP_SUBNET
/** String defining Soft-AP subnet to use.
  *  Last digit of IP address automatically set to 1 */
#define W6X_WIFI_SAP_IP_SUBNET                  {192, 168, 8}
#endif /* W6X_WIFI_SAP_IP_SUBNET */

#ifndef W6X_WIFI_SAP_IP_SUBNET_BACKUP
/** String defining Soft-AP subnet to use in case of conflict with the AP the STA is connected to.
  *  Last digit of IP address automatically set to 1 */
#define W6X_WIFI_SAP_IP_SUBNET_BACKUP           {192, 168, 9}
#endif /* W6X_WIFI_SAP_IP_SUBNET_BACKUP */

#ifndef W6X_WIFI_DNS_MANUAL
/** Define if the DNS addresses are set manually or automatically */
#define W6X_WIFI_DNS_MANUAL                     0
#endif /* W6X_WIFI_DNS_MANUAL */

#ifndef W6X_WIFI_DNS_IP_1
/** String defining DNS IP 1 address to use
  * @note: This address will be used only if W6X_WIFI_DNS_MANUAL equals 1 */
#define W6X_WIFI_DNS_IP_1                       {208, 67, 222, 222}
#endif /* W6X_WIFI_DNS_IP_1 */

#ifndef W6X_WIFI_DNS_IP_2
/** String defining DNS IP 2 address to use
  * @note: This address will be used only if W6X_WIFI_DNS_MANUAL equals 1 */
#define W6X_WIFI_DNS_IP_2                       {8, 8, 8, 8}
#endif /* W6X_WIFI_DNS_IP_2 */

#ifndef W6X_WIFI_DNS_IP_3
/** String defining DNS IP 3 address to use
  * @note: This address will be used only if W6X_WIFI_DNS_MANUAL equals 1 */
#define W6X_WIFI_DNS_IP_3                       {0, 0, 0, 0}
#endif /* W6X_WIFI_DNS_IP_3 */

#ifndef W6X_WIFI_COUNTRY_CODE
/** Define the region code, supported values : [CN, JP, US, EU, 00] */
#define W6X_WIFI_COUNTRY_CODE                   "00"
#endif /* W6X_WIFI_COUNTRY_CODE */

#ifndef W6X_WIFI_ADAPTIVE_COUNTRY_CODE
/** Define if the country code will match AP's one.
  * 0: match AP's country code,
  * 1: static country code */
#define W6X_WIFI_ADAPTIVE_COUNTRY_CODE          0
#endif /* W6X_WIFI_ADAPTIVE_COUNTRY_CODE */

#ifndef W6X_WIFI_HOSTNAME
/** String defining Wi-Fi hostname */
#define W6X_WIFI_HOSTNAME                       "ST67W61_WiFi"
#endif /* W6X_WIFI_HOSTNAME */

/** @} */

/** @addtogroup ST67W6X_API_BLE_Public_Constants
  * @{
  */

#ifndef W6X_BLE_HOSTNAME
/** String defining BLE hostname */
#define W6X_BLE_HOSTNAME                        "ST67W61_BLE"
#endif /* W6X_BLE_HOSTNAME */

/** @} */

/** @addtogroup ST67W6X_API_Net_Public_Constants
  * @{
  */

#ifndef W6X_NET_RECV_TIMEOUT
/** Default net socket receive timeout in ms */
#define W6X_NET_RECV_TIMEOUT                    5000
#endif /* W6X_NET_RECV_TIMEOUT */

#ifndef W6X_NET_SEND_TIMEOUT
/** Default net socket send timeout in ms */
#define W6X_NET_SEND_TIMEOUT                    5000
#endif /* W6X_NET_SEND_TIMEOUT */

#ifndef W6X_NET_RECV_BUFFER_SIZE
/** Default Net socket receive buffer size
  * @note In the NCP, the LWIP recv function is used with a static buffer with
  * a fixed length of 4608 (3 * 1536). The data is read in chunks of 4608 bytes
  * So in order to get optimal performances, the buffer on NCP side should be twice as big */
#define W6X_NET_RECV_BUFFER_SIZE                (2 * 3 * 1536)
#endif /* W6X_NET_RECV_BUFFER_SIZE */

/** @} */

/** @addtogroup ST67W6X_API_HTTP_Public_Constants
  * @{
  */

#ifndef W6X_HTTP_CLIENT_TCP_SOCK_RECV_TIMEOUT
/** Timeout value in millisecond for receiving data via TCP socket used by the HTTP client.
  * This value is set to compensate for when the NCP get stuck for a long time (1 second or more)
  * when retrieving data from an HTTP server for example */
#define W6X_HTTP_CLIENT_TCP_SOCK_RECV_TIMEOUT   1000
#endif /* W6X_HTTP_CLIENT_TCP_SOCK_RECV_TIMEOUT */

#ifndef W6X_HTTP_CLIENT_TCP_SOCKET_SIZE
/** Size of the TCP socket used by the HTTP client, recommended to be at least 0x2000 when fetching lots of data.
  * 0x2000 is the value used in the SPI host project for OTA update, which retrieves around 1 mega bytes of data.*/
#define W6X_HTTP_CLIENT_TCP_SOCKET_SIZE         0x3000
#endif /* W6X_HTTP_CLIENT_TCP_SOCKET_SIZE */

/** @} */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* W6X_DEFAULT_CONFIG_H */
