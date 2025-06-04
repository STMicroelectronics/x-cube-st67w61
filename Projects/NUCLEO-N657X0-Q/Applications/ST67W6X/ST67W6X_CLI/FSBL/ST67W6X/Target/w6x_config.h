/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    w6x_config.h
  * @author  GPM Application Team
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

/** NCP will go by default in low power mode when NCP is in idle mode */
#define W6X_POWER_SAVE_AUTO                     1

/** NCP clock mode : 1: Internal RC oscillator, 2: External passive crystal, 3: External active crystal */
#define W6X_CLOCK_MODE                          1

/** ============================
  * Wi-Fi
  *
  * All available configuration defines in
  * Middlewares\ST\ST67W6X_Network_Driver\Core\w6x_default_config.h
  * ============================
  */

/** Boolean to enable/disable autoconnect functionality */
#define W6X_WIFI_AUTOCONNECT                    1

/** Define the DHCP configuration : 0: NO DHCP, 1: DHCP CLIENT STA, 2:DHCP SERVER AP, 3: DHCP STA+AP */
#define W6X_WIFI_DHCP                           3

/** Define the max number of stations that can connect to the Soft-AP */
#define W6X_WIFI_SAP_MAX_CONNECTED_STATIONS     4

/** String defining Soft-AP subnet to use.
  *  Last digit of IP address automatically set to 1 */
#define W6X_WIFI_SAP_IP_SUBNET                  {192, 168, 8}

/** String defining Soft-AP subnet to use in case of conflict with the AP the STA is connected to.
  *  Last digit of IP address automatically set to 1 */
#define W6X_WIFI_SAP_IP_SUBNET_BACKUP           {192, 168, 9}

/** Define if the DNS addresses are set manually or automatically */
#define W6X_WIFI_DNS_MANUAL                     0

/** String defining DNS IP 1 address to use
  * @note: This address will be used only if W6X_WIFI_DNS_MANUAL equals 1 */
#define W6X_WIFI_DNS_IP_1                       {208, 67, 222, 222}

/** String defining DNS IP 2 address to use
  * @note: This address will be used only if W6X_WIFI_DNS_MANUAL equals 1 */
#define W6X_WIFI_DNS_IP_2                       {8, 8, 8, 8}

/** String defining DNS IP 3 address to use
  * @note: This address will be used only if W6X_WIFI_DNS_MANUAL equals 1 */
#define W6X_WIFI_DNS_IP_3                       {0, 0, 0, 0}

/** Define the region code, supported values : [CN, JP, US, EU, 00] */
#define W6X_WIFI_COUNTRY_CODE                   "00"

/** Define if the country code will match AP's one.
  * 0: match AP's country code,
  * 1: static country code */
#define W6X_WIFI_ADAPTIVE_COUNTRY_CODE          0

/** String defining Wi-Fi hostname */
#define W6X_WIFI_HOSTNAME                       "ST67W61_WiFi"

/** ============================
  * Net
  *
  * All available configuration defines in
  * Middlewares\ST\ST67W6X_Network_Driver\Core\w6x_default_config.h
  * ============================
  */

/** Timeout in ticks when calling W6X_Net_Recv() */
#define W6X_NET_RECV_TIMEOUT                    10000

/** Timeout in ticks when calling W6X_Net_Send() */
#define W6X_NET_SEND_TIMEOUT                    10000

/** ============================
  * Utility Performance Iperf
  *
  * All available configuration defines in
  * Middlewares\ST\ST67W6X_Network_Driver\Utils\Performance\iperf.h
  * ============================
  */
/** Enable Iperf feature */
#define IPERF_ENABLE                            1

/** ============================
  * Utility Performance Memory usage
  *
  * All available configuration defines in
  * Middlewares\ST\ST67W6X_Network_Driver\Utils\Performance\util_mem_perf.h
  *
  * @note: This feature requires to call the hook functions in the FreeRTOS.
  *        Add the following lines in the FreeRTOSConfig.h file:
  *
  *        #if defined(__ICCARM__) || defined(__ARMCC_VERSION) || defined(__GNUC__)
  *        void mem_perf_malloc_hook(void *pvAddress, size_t uiSize);
  *        void mem_perf_free_hook(void *pvAddress, size_t uiSize);
  *        #endif
  *        #define traceMALLOC mem_perf_malloc_hook
  *        #define traceFREE mem_perf_free_hook
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
  *        #if defined(__ICCARM__) || defined(__ARMCC_VERSION) || defined(__GNUC__)
  *        void task_perf_in_hook(void);
  *        void task_perf_out_hook(void);
  *        #endif
  *        #define traceTASK_SWITCHED_IN task_perf_in_hook
  *        #define traceTASK_SWITCHED_OUT task_perf_out_hook
  *
  * ============================
  */
/** Enable task performance measurement */
#define TASK_PERF_ENABLE                        1

/** ============================
  * Utility Performance WFA
  *
  * All available configuration defines in
  * Middlewares\ST\ST67W6X_Network_Driver\Utils\Performance\wfa_tg.h
  * ============================
  */
/** Enable Wi-Fi Alliance Traffic Generator */
#define WFA_TG_ENABLE                           1

/** ============================
  * External service littlefs usage
  *
  * All available configuration defines in
  * ============================
  */
/** Enable LittleFS */
#define LFS_ENABLE                              1

#if (LFS_ENABLE == 1)
#include "easyflash.h"
#endif /* LFS_ENABLE */

/* USER CODE BEGIN EC */

/* USER CODE END EC */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* W6X_CONFIG_H */
