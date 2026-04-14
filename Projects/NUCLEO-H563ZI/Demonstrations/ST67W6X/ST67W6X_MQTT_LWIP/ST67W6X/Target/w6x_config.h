/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    w6x_config.h
  * @author  ST67 Application Team
  * @brief   Header file for the W6X configuration module
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025-2026 STMicroelectronics.
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
#ifndef W6X_CONFIG_H
#define W6X_CONFIG_H

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
#define W6X_POWER_SAVE_AUTO                     0

/** NCP clock mode : 1: Internal RC oscillator, 2: External passive crystal, 3: External active crystal */
#define W6X_CLOCK_MODE                          1U

/** ============================
  * Wi-Fi
  *
  * All available configuration defines in
  * Middlewares\ST\ST67W6X_Network_Driver\Core\w6x_default_config.h
  * ============================
  */

/** Boolean to enable/disable autoconnect functionality */
#define W6X_WIFI_AUTOCONNECT                    0

/** Define the region code, supported values : [CN, JP, US, EU, 00] */
#define W6X_WIFI_COUNTRY_CODE                   "00"

/** Define if the country code will match AP's one.
  * 0: match AP's country code,
  * 1: static country code */
#define W6X_WIFI_ADAPTIVE_COUNTRY_CODE          0

/** ============================
  * Network wrapper functions
  *
  * All available configuration defines in
  * Middlewares\ST\ST67W6X_Network_Driver\Api\w6x_types.h
  * ============================
  */

#include "lwip/sockets.h"
#include "lwip/udp.h"

/** Interface of receive data from a socket from a specific address */
#define NET_RECVFROM                            lwip_recvfrom

/** Interface of receive data from a socket */
#define NET_RECV                                lwip_recv

/** Interface of send data to a socket to a specific address */
#define NET_SENDTO                              lwip_sendto

/** Interface of send data to a socket */
#define NET_SEND                                lwip_send

/** Interface of shutdown a socket */
#define NET_SHUTDOWN                            lwip_shutdown

/** Interface of create a socket */
#define NET_SOCKET                              lwip_socket

/** Interface of set socket options */
#define NET_SETSOCKOPT                          lwip_setsockopt

/** Interface of close a socket */
#define NET_CLOSE                               lwip_close

/** Interface of connect a socket */
#define NET_CONNECT                             lwip_connect

/** Interface of accept a connection on a socket */
#define NET_ACCEPT                              lwip_accept

/** Interface of bind a socket to an address */
#define NET_BIND                                lwip_bind

/** Interface of listen on a socket */
#define NET_LISTEN                              lwip_listen

/** Interface of convert an IP address from binary to text form */
#define NET_INET_NTOP                           lwip_inet_ntop

/** Interface of convert an IP address from text to binary form */
#define NET_INET_PTON                           lwip_inet_pton

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
#define IPERF_V6                                1

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

#endif /* W6X_CONFIG_H */
