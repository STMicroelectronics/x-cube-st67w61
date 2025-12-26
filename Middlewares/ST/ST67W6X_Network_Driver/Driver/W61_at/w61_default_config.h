/**
  ******************************************************************************
  * @file    w61_default_config.h
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
#ifndef W61_DEFAULT_CONFIG_H
#define W61_DEFAULT_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Includes ------------------------------------------------------------------*/
#include "w61_driver_config.h"

/* Exported constants --------------------------------------------------------*/
/** @addtogroup ST67W61_AT_WiFi_Constants
  * @{
  */

#ifndef W61_WIFI_MAX_DETECTED_AP
/** Maximum number of detected AP during the scan. Cannot be greater than 50 */
#define W61_WIFI_MAX_DETECTED_AP                20
#endif /* W61_WIFI_MAX_DETECTED_AP */

#ifndef WIFI_LOG_ENABLE
/** Enable/Disable Wi-Fi module logging */
#define WIFI_LOG_ENABLE                         0
#endif /* WIFI_LOG_ENABLE */

/** @} */

/** @addtogroup ST67W61_AT_BLE_Constants
  * @{
  */

#ifndef W61_BLE_MAX_CONN_NBR
/** Maximum number of BLE connections. Must not be greater than 10.
  * The maximum connections depend on the mode as follows:
  * - Server mode (10 connection max)
  * - Client mode (9 connection max)
  * - Dual Mode (10 connections max with up to 9 as Client)
  */
#define W61_BLE_MAX_CONN_NBR                    2
#endif /* W61_BLE_MAX_CONN_NBR */

/** Maximum number of BLE application services that can be created */
#define W61_BLE_MAX_CREATED_SERVICE_NBR         3

/** Maximum number of BLE services supported including Generic access and Generic attributes predefined services */
#define W61_BLE_MAX_SERVICE_NBR                 W61_BLE_MAX_CREATED_SERVICE_NBR + 2

/** Maximum number of BLE characteristics per service */
#define W61_BLE_MAX_CHAR_NBR                    5

#ifndef W61_BLE_MAX_DETECTED_PERIPHERAL
/** Maximum number of detected peripheral during the scan. Cannot be greater than 50 */
#define W61_BLE_MAX_DETECTED_PERIPHERAL         10
#endif /* W61_BLE_MAX_DETECTED_PERIPHERAL */

/** BLE Service/Characteristic UUID maximum size size */
#define W61_BLE_MAX_UUID_SIZE                   17

/** Maximum number of bonded devices */
#define W61_BLE_MAX_BONDED_DEVICES              2

#ifndef BLE_LOG_ENABLE
/** Enable/Disable BLE module logging */
#define BLE_LOG_ENABLE                          0
#endif /* BLE_LOG_ENABLE */

/** @} */

/** @addtogroup ST67W61_AT_Net_Constants
  * @{
  */

#ifndef W61_NET_IPV6_ENABLE
/** Enable IPv6 support : 0: Disabled, 1: Enabled */
#define W61_NET_IPV6_ENABLE                     0
#endif /* W61_NET_IPV6_ENABLE */

#ifndef NET_LOG_ENABLE
/** Enable/Disable Network module logging */
#define NET_LOG_ENABLE                          0
#endif /* NET_LOG_ENABLE */

/** @} */

/** @addtogroup ST67W61_AT_MQTT_Constants
  * @{
  */

#ifndef MQTT_LOG_ENABLE
/** Enable/Disable MQTT module logging */
#define MQTT_LOG_ENABLE                         0
#endif /* MQTT_LOG_ENABLE */

/** @} */

/** @addtogroup ST67W61_AT_Common_Constants
  * @{
  */

#ifndef W61_MAX_SPI_XFER
/** Maximum SPI buffer size */
#define W61_MAX_SPI_XFER                        1520
#endif /* W61_MAX_SPI_XFER */

#if ((W61_MAX_SPI_XFER < 1520) || (W61_MAX_SPI_XFER > (6 * 1024)))
#error "W6X_MAX_SPI_XFER must be between 1520 and (6*1024)"
#endif /* W61_MAX_SPI_XFER */

#ifndef SYS_LOG_ENABLE
/** Enable/Disable System module logging */
#define SYS_LOG_ENABLE                          0
#endif /* SYS_LOG_ENABLE */

#ifndef W61_AT_LOG_ENABLE
/** Debugging only: Enable/Disable AT log, i.e. logs the AT commands incoming/outcoming from/to the NCP */
#define W61_AT_LOG_ENABLE                       0
#endif /* W61_AT_LOG_ENABLE */

#ifndef MDM_CMD_LOG_ENABLE
/** Enable/Disable Modem command log */
#define MDM_CMD_LOG_ENABLE                      W61_AT_LOG_ENABLE
#endif /* MDM_CMD_LOG_ENABLE */

#ifndef W61_ASSERT_ENABLE
/** Enable/Disable NULL pointer check in the AT functions.
  * 0: Disabled, 1: Enabled */
#define W61_ASSERT_ENABLE                       0
#endif /* W61_ASSERT_ENABLE */

/** @} */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* W61_DEFAULT_CONFIG_H */
