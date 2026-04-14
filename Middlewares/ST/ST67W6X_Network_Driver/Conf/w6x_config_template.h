/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    w6x_config_template.h
  * @author  ST67 Application Team
  * @brief   Header file for the W6X configuration module
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
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef W6X_CONFIG_TEMPLATE_H
#define W6X_CONFIG_TEMPLATE_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Includes ------------------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported constants --------------------------------------------------------*/
/*
 * All available configuration defines can be found in
 * Middlewares\ST\ST67W6X_Network_Driver\Conf\w6x_config_template.h
 */

/** ============================
  * System
  *
  * All available configuration defines in
  * Middlewares\ST\ST67W6X_Network_Driver\Core\w6x_default_config.h
  * ============================
  */

/** NCP power save mode : 0: NCP stays always active / 1: NCP goes in low power mode when idle */
#define W6X_POWER_SAVE_AUTO                     1

/** NCP clock mode : 1: Internal RC oscillator, 2: External passive crystal, 3: External active crystal */
#define W6X_CLOCK_MODE                          1U

/** Enable/Disable NULL pointer check in the API functions.
  * 0: Disabled, 1: Enabled */
#define W6X_ASSERT_ENABLE                       0

/** ============================
  * Wi-Fi
  *
  * All available configuration defines in
  * Middlewares\ST\ST67W6X_Network_Driver\Core\w6x_default_config.h
  * ============================
  */

/** Boolean to enable/disable autoconnect functionality */
#define W6X_WIFI_AUTOCONNECT                    0

/** Define the max number of stations that can connect to the Soft-AP */
#define W6X_WIFI_SAP_MAX_CONNECTED_STATIONS     4

/** Define the region code, supported values : [CN, JP, US, EU, 00] */
#define W6X_WIFI_COUNTRY_CODE                   "00"

/** Define if the country code will match AP's one.
  * 0: match AP's country code,
  * 1: static country code */
#define W6X_WIFI_ADAPTIVE_COUNTRY_CODE          0

/** ============================
  * BLE
  *
  * All available configuration defines in
  * Middlewares\ST\ST67W6X_Network_Driver\Core\w6x_default_config.h
  * ============================
  */

/** String defining BLE hostname */
#define W6X_BLE_HOSTNAME                        "ST67W61_BLE"

/** ============================
  * Net
  *
  * All available configuration defines in
  * Middlewares\ST\ST67W6X_Network_Driver\Core\w6x_default_config.h
  * ============================
  */

/** Define the DHCP configuration : 0: NO DHCP, 1: DHCP CLIENT STA, 2:DHCP SERVER AP, 3: DHCP STA+AP */
#define W6X_NET_DHCP                            1U

/** String defining Soft-AP subnet to use.
  *  Last digit of IP address automatically set to 1 */
#define W6X_NET_SAP_IP_SUBNET                   {10, 19, 96}

/** String defining Wi-Fi hostname */
#define W6X_NET_HOSTNAME                        "ST67W61_WiFi"

/** Timeout in ticks when calling W6X_Net_Recv() */
#define W6X_NET_RECV_TIMEOUT                    5000U

/** Timeout in ticks when calling W6X_Net_Send() */
#define W6X_NET_SEND_TIMEOUT                    5000U

/** Default Net socket receive buffer size
  * @note In the NCP, the LWIP recv function is used with a static buffer with
  * a fixed length of 4608 (3 * 1536). The data is read in chunks of 4608 bytes
  * So in order to get optimal performances, the buffer on NCP side should be twice as big */
#define W6X_NET_RECV_BUFFER_SIZE                9216U

/** ============================
  * HTTP
  *
  * All available configuration defines in
  * Middlewares\ST\ST67W6X_Network_Driver\Core\w6x_default_config.h
  * ============================
  */
/** HTTP Client thread stack size */
#define W6X_HTTP_CLIENT_THREAD_STACK_SIZE       1536U

/** HTTP Client thread priority */
#define W6X_HTTP_CLIENT_THREAD_PRIO             30

/** HTTP data receive buffer size */
#define W6X_HTTP_CLIENT_DATA_RECV_SIZE          1024U

/** Timeout value in millisecond for receiving data via TCP socket used by the HTTP client.
  * This value is set to compensate for when the NCP get stuck for a long time (1 second or more)
  * when retrieving data from an HTTP server for example */
#define W6X_HTTP_CLIENT_TCP_SOCK_RECV_TIMEOUT   1000

/** Size of the TCP socket used by the HTTP client, recommended to be at least 0x2000 when fetching lots of data.
  * 0x2000 is the value used in the SPI host project for Firmware updates,
  * which retrieves around 1 mega bytes of data. */
#define W6X_HTTP_CLIENT_TCP_SOCKET_SIZE         0x3000

/** ============================
  * Netif
  *
  * All available configuration defines in
  * Middlewares\ST\ST67W6X_Network_Driver\Core\w6x_default_config.h
  * ============================
  */
/** RX queue depth (number of pending frames) for Network STA traffic */
#define W6X_NETIF_STA_RXQ_DEPTH                16

/** RX queue depth (number of pending frames) for Network AP traffic */
#define W6X_NETIF_AP_RXQ_DEPTH                 8

/** ============================
  * Utility Performance network wrapper functions
  *
  * All available configuration defines in
  * Middlewares\ST\ST67W6X_Network_Driver\Api\w6x_types.h
  * ============================
  */

/** Interface of receive data from a socket from a specific address */
#define NET_RECVFROM                            W6X_Net_Recvfrom

/** Interface of receive data from a socket */
#define NET_RECV                                W6X_Net_Recv

/** Interface of send data to a socket to a specific address */
#define NET_SENDTO                              W6X_Net_Sendto

/** Interface of send data to a socket */
#define NET_SEND                                W6X_Net_Send

/** Interface of shutdown a socket */
#define NET_SHUTDOWN                            W6X_Net_Shutdown

/** Interface of create a socket */
#define NET_SOCKET                              W6X_Net_Socket

/** Interface of set socket options */
#define NET_SETSOCKOPT                          W6X_Net_Setsockopt

/** Interface of close a socket */
#define NET_CLOSE                               W6X_Net_Close

/** Interface of connect a socket */
#define NET_CONNECT                             W6X_Net_Connect

/** Interface of accept a connection on a socket */
#define NET_ACCEPT                              W6X_Net_Accept

/** Interface of bind a socket to an address */
#define NET_BIND                                W6X_Net_Bind

/** Interface of listen on a socket */
#define NET_LISTEN                              W6X_Net_Listen

/** Interface of convert an IP address from binary to text form */
#define NET_INET_NTOP                           W6X_Net_Inet_ntop

/** Interface of convert an IP address from text to binary form */
#define NET_INET_PTON                           W6X_Net_Inet_pton

/** ============================
  * Utility Performance Iperf
  *
  * All available configuration defines in
  * Middlewares\ST\ST67W6X_Network_Driver\Utils\Performance\iperf.h
  * ============================
  */
/** Enable Iperf feature */
#define IPERF_ENABLE                            0

/** Enable IPv6 for Iperf */
#define IPERF_V6                                0

/** Enable dual mode for Iperf */
#define IPERF_DUAL_MODE                         0

/** Iperf traffic task priority */
#define IPERF_TRAFFIC_TASK_PRIORITY             40U

/** Iperf traffic task stack size */
#define IPERF_TRAFFIC_TASK_STACK                2048U

/** Iperf report task stack size */
#define IPERF_REPORT_TASK_STACK                 1024U

/** Iperf memory allocator */
#define IPERF_MALLOC                            pvPortMalloc

/** Iperf memory deallocator */
#define IPERF_FREE                              vPortFree

/** ============================
  * Utility Performance Memory usage
  *
  * All available configuration defines in
  * Middlewares\ST\ST67W6X_Network_Driver\Utils\Performance\util_mem_perf.h
  *
  * @note: This feature requires to call the hook functions in the FreeRTOS.
  *        Add the following lines in the FreeRTOSConfig.h file:
  *
  *        \code
  *        #if defined(__ICCARM__) || defined(__ARMCC_VERSION) || defined(__GNUC__)
  *        void mem_perf_malloc_hook(void *pvAddress, size_t uiSize);
  *        void mem_perf_free_hook(void *pvAddress, size_t uiSize);
  *        #endif
  *        #define traceMALLOC mem_perf_malloc_hook
  *        #define traceFREE mem_perf_free_hook
  *        \endcode
  *
  * ============================
  */
/** Enable memory performance measurement */
#define MEM_PERF_ENABLE                         0

/** Number of memory allocation to keep track of */
#define LEAKAGE_ARRAY                           500U

/** Allocator maximum iteration before to break */
#define ALLOC_BREAK                             0xFFFFFFFFU

/** ============================
  * Utility Performance Task usage
  *
  * All available configuration defines in
  * Middlewares\ST\ST67W6X_Network_Driver\Utils\Performance\util_task_perf.h
  *
  * @note: This feature requires to call the hook functions in the FreeRTOS.
  *        Add the following lines in the FreeRTOSConfig.h file:
  *
  *        \code
  *        #if defined(__ICCARM__) || defined(__ARMCC_VERSION) || defined(__GNUC__)
  *        void task_perf_in_hook(void);
  *        void task_perf_out_hook(void);
  *        #endif
  *        #define traceTASK_SWITCHED_IN task_perf_in_hook
  *        #define traceTASK_SWITCHED_OUT task_perf_out_hook
  *        \endcode
  *
  * ============================
  */
/** Enable task performance measurement */
#define TASK_PERF_ENABLE                        0

/** Maximum number of thread to monitor */
#define PERF_MAXTHREAD                          8U

/** ============================
  * Utility Performance WFA
  *
  * All available configuration defines in
  * Middlewares\ST\ST67W6X_Network_Driver\Utils\Performance\wfa_tg.h
  * ============================
  */
/** Enable Wi-Fi Alliance Traffic Generator */
#define WFA_TG_ENABLE                           0

/** ============================
  * External service littlefs usage
  *
  * All available configuration defines in
  * ============================
  */
/** Enable LittleFS */
#define LFS_ENABLE                              0

#if (LFS_ENABLE == 1)
#include "easyflash.h"
#endif /* LFS_ENABLE */

/* USER CODE BEGIN EC */

/* USER CODE END EC */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* W6X_CONFIG_TEMPLATE_H */
