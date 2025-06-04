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
#define W61_WIFI_MAX_DETECTED_AP                50
#endif /* W61_WIFI_MAX_DETECTED_AP */

/** @} */

/** @addtogroup ST67W61_AT_BLE_Constants
  * @{
  */

#ifndef W61_BLE_MAX_CONN_NBR
/** Maximum number of BLE connections */
#define W61_BLE_MAX_CONN_NBR                    1
#endif /* W61_BLE_MAX_CONN_NBR */

#ifndef W61_BLE_MAX_SERVICE_NBR
/** Maximum number of BLE services */
#define W61_BLE_MAX_SERVICE_NBR                 5
#endif /* W61_BLE_MAX_SERVICE_NBR */

#ifndef W61_BLE_MAX_CHAR_NBR
/** Maximum number of BLE characteristics per service */
#define W61_BLE_MAX_CHAR_NBR                    5
#endif /* W61_BLE_MAX_CHAR_NBR */

#ifndef W61_BLE_MAX_DETECTED_PERIPHERAL
/** Maximum number of detected peripheral during the scan. Cannot be greater than 50 */
#define W61_BLE_MAX_DETECTED_PERIPHERAL         10
#endif /* W61_BLE_MAX_DETECTED_PERIPHERAL */

/** BLE Service/Characteristic UUID maximum size size */
#define W61_BLE_MAX_UUID_SIZE                   17

/** Maximum number of bonded devices */
#define W61_BLE_MAX_BONDED_DEVICES              2

/** @} */

/** @addtogroup ST67W61_AT_Net_Constants
  * @{
  */

#if (!defined(W61_NET_PING_REPETITION) || (W61_NET_PING_REPETITION == 0))
#undef W61_NET_PING_REPETITION
/** Number of ping repetition */
#define W61_NET_PING_REPETITION                   4
#endif /* W61_NET_PING_REPETITION */

#if (!defined(W61_NET_PING_PACKET_SIZE) || (W61_NET_PING_PACKET_SIZE == 0))
#undef W61_NET_PING_PACKET_SIZE
/** Ping packet size */
#define W61_NET_PING_PACKET_SIZE                  64
#endif /* W61_NET_PING_PACKET_SIZE */

#if (!defined(W61_NET_PING_INTERVAL) || (W61_NET_PING_INTERVAL == 0))
#undef W61_NET_PING_INTERVAL
/** Ping interval in ms */
#define W61_NET_PING_INTERVAL                     1000
#endif /* W61_NET_PING_INTERVAL */

/** @} */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* W61_DEFAULT_CONFIG_H */
