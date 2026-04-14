/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    w61_driver_config_template.h
  * @author  ST67 Application Team
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
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef W61_DRIVER_CONFIG_TEMPLATE_H
#define W61_DRIVER_CONFIG_TEMPLATE_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Includes ------------------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported constants --------------------------------------------------------*/
/** ============================
  * AT Wi-Fi
  * All available configuration defines in
  * Middlewares\ST\ST67W6X_Network_Driver\Driver\W61_at\w61_default_config.h
  * ============================
  */

/** Maximum number of detected AP during the scan. Cannot be greater than 50 */
#define W61_WIFI_MAX_DETECTED_AP                20U

/** Enable/Disable Wi-Fi module logging */
#define WIFI_LOG_ENABLE                         1

/** ============================
  * AT BLE
  * All available configuration defines in
  * Middlewares\ST\ST67W6X_Network_Driver\Driver\W61_at\w61_default_config.h
  * ============================
  */

/** Maximum number of BLE connections. Must not be greater than 10.
  * The maximum connections depend on the mode as follows:
  * - Server mode (10 connection max)
  * - Client mode (9 connection max)
  * - Dual Mode (10 connections max with up to 9 as Client)
  */
#define W61_BLE_MAX_CONN_NBR                    2U

/** Maximum number of detected peripheral during the scan. Cannot be greater than 50 */
#define W61_BLE_MAX_DETECTED_PERIPHERAL         10U

/** Enable/Disable BLE module logging */
#define BLE_LOG_ENABLE                          1

/** ============================
  * AT Net
  * All available configuration defines in
  * Middlewares\ST\ST67W6X_Network_Driver\Driver\W61_at\w61_default_config.h
  * ============================
  */

/** Enable IPv6 support : 0: Disabled, 1: Enabled */
#define W61_NET_IPV6_ENABLE                     0

/** Enable/Disable Network module logging */
#define NET_LOG_ENABLE                          1

/** ============================
  * AT MQTT
  * All available configuration defines in
  * Middlewares\ST\ST67W6X_Network_Driver\Driver\W61_at\w61_default_config.h
  * ============================
  */

/** Enable/Disable MQTT module logging */
#define MQTT_LOG_ENABLE                         1

/** ============================
  * AT Common
  * All available configuration defines in
  * Middlewares\ST\ST67W6X_Network_Driver\Driver\W61_at\w61_at_common.h
  * ============================
  */
/** IO queue depth for AT CMD */
#define IO_AT_CMDQ_DEPTH                        16

/** Maximum SPI buffer size */
#define W61_MAX_SPI_XFER                        1520U

/** Enable/Disable System module logging */
#define SYS_LOG_ENABLE                          1

/** Debugging only: Enable/Disable AT log, i.e. logs the AT commands incoming/outcoming from/to the NCP */
#define W61_AT_LOG_ENABLE                       0
#include "logging.h"

/** Enable/Disable Modem command log */
#define MDM_CMD_LOG_ENABLE                      0

/** Maximum size of AT log */
#define W61_MAX_AT_LOG_LENGTH                   300U

/** Timeout for reply/execute of NCP */
#define W61_NCP_TIMEOUT                         2000U

/** Timeout for special cases like flash write, firmware updates, etc */
#define W61_SYS_TIMEOUT                         2000U

/** Timeout for remote Wi-Fi device operation (e.g. AP) */
#define W61_WIFI_TIMEOUT                        6000U

/** Timeout for remote network operation (e.g. server) */
#define W61_NET_TIMEOUT                         6000U

/** Timeout for remote BLE device operation */
#define W61_BLE_TIMEOUT                         2000U

/** Enable/Disable NULL pointer check in the AT functions.
  * 0: Disabled, 1: Enabled */
#define W61_ASSERT_ENABLE                       0

/** Stack required especially for Log messages */
#define W61_MDM_RX_TASK_STACK_SIZE_BYTES        2048U

/** W61_Modem_Process task priority, recommended to be higher than application tasks */
#define W61_MDM_RX_TASK_PRIO                    54

/** ============================
  * SPI Common
  * All available configuration defines in
  * Middlewares\ST\ST67W6X_Network_Driver\Driver\W61_bus\spi_iface.c
  * ============================
  */

/** SPI thread stack size */
#define SPI_THREAD_STACK_SIZE                   768U

/** SPI thread priority */
#define SPI_THREAD_PRIO                         53

/** SPI Tx Queue length */
#define SPI_TXQ_LEN                             8U

/** SPI Rx Queue length */
#define SPI_RXQ_LEN                             8U

/* USER CODE BEGIN EC */

/* USER CODE END EC */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* W61_DRIVER_CONFIG_TEMPLATE_H */
