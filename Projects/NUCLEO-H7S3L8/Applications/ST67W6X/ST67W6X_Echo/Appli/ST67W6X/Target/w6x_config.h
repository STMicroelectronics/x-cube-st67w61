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

/** NCP power save mode : 0: NCP stays always active / 1: NCP goes in low power mode when idle */
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

/** Define the region code, supported values : [CN, JP, US, EU, 00] */
#define W6X_WIFI_COUNTRY_CODE                   "00"

/** Define if the country code will match AP's one.
  * 0: match AP's country code,
  * 1: static country code */
#define W6X_WIFI_ADAPTIVE_COUNTRY_CODE          0

/** ============================
  * Net
  *
  * All available configuration defines in
  * Middlewares\ST\ST67W6X_Network_Driver\Core\w6x_default_config.h
  * ============================
  */

/** Define the DHCP configuration : 0: NO DHCP, 1: DHCP CLIENT STA, 2:DHCP SERVER AP, 3: DHCP STA+AP */
#define W6X_NET_DHCP                            1

/** String defining Wi-Fi hostname */
#define W6X_NET_HOSTNAME                        "ST67W61_WiFi"

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
#define IPERF_ENABLE                            0

/** Enable IPv6 for Iperf */
#define IPERF_V6                                0

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

/* USER CODE BEGIN EC */

/* USER CODE END EC */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* W6X_CONFIG_H */
