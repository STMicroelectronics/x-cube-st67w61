/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    w61_driver_config_template.h
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
#define W61_WIFI_MAX_DETECTED_AP                50

/** ============================
  * AT Net
  * All available configuration defines in
  * Middlewares\ST\ST67W6X_Network_Driver\Driver\W61_at\w61_default_config.h
  * ============================
  */

/** Number of ping repetition */
#define W61_NET_PING_REPETITION                 4

/** Default size of the ping sent */
#define W61_NET_PING_PACKET_SIZE                64

/** Interval between two sent of ping packets in ms */
#define W61_NET_PING_INTERVAL                   1000

/** ============================
  * AT BLE
  * All available configuration defines in
  * Middlewares\ST\ST67W6X_Network_Driver\Driver\W61_at\w61_default_config.h
  * ============================
  */

/** Maximum number of BLE connections */
#define W61_BLE_MAX_CONN_NBR                    1

/** Maximum number of BLE services */
#define W61_BLE_MAX_SERVICE_NBR                 5

/** Maximum number of BLE characteristics per service */
#define W61_BLE_MAX_CHAR_NBR                    5

/** Maximum number of detected peripheral during the scan. Cannot be greater than 50 */
#define W61_BLE_MAX_DETECTED_PERIPHERAL         30

/** ============================
  * AT Common
  * All available configuration defines in
  * Middlewares\ST\ST67W6X_Network_Driver\Driver\W61_at\w61_at_common.h
  * ============================
  */

/** number of retries to get the resource when the mutex is taken */
#define LOCK_RETRY_MAX_NR                       0

/** time to wait between two attempts to get the mutex */
#define LOCK_RETRY_PERIOD                       100

/** it detect miss-usage of the driver by the application */
#define W61_ASSERT_EN                           1

/** Maximum size of AT log */
#define W61_MAX_AT_LOG_LENGTH                   30

/** Debugging only: Enable AT log, i.e. logs the AT commands incoming/outcoming from/to the NCP */
#define W61_AT_LOG_ENABLE                       0
#include "logging.h"

/** Time to wait before returning BUSY status */
#define  W61_AT_LOCK_TIMEOUT                    2000

/** Timeout for reply/execute of NCP */
#define  W61_NCP_TIMEOUT                        100

/** Timeout for special cases like flash write, OTA, etc */
#define  W61_SYS_TIMEOUT                        200

/** Timeout for remote WIFI device operation (e.g. AP) */
#define  W61_WIFI_TIMEOUT                       2000

/** Timeout for remote network operation (e.g. server) */
#define  W61_NET_TIMEOUT                        2000

/** Timeout for remote BLE device operation */
#define  W61_BLE_TIMEOUT                        2000

/** ============================
  * AT Parser
  * All available configuration defines in
  * Middlewares\ST\ST67W6X_Network_Driver\Driver\W61_at\w61_at_rx_parser.h
  * ============================
  */

/** Stack required especially for Log messages */
#define W61_ATD_RX_TASK_STACK_SIZE_BYTES        1280

/**
  * Notice that callbacks are executed on this task, so some margin is taken when sizing the stack.
  * However, if users (when writing their application callbacks) need more space,
  * e.g. application buffers, printf, etc
  * the W61_ATD_EVENTS_TASK_STACK_SIZE_BYTES should be redefined in the "w61_driver_config.h"
  *
  * 600 bytes is the minimum (event callbacks should do nothing)
  */
#define W61_ATD_EVENTS_TASK_STACK_SIZE_BYTES    2048

/** W61 AT Rx parser task priority, recommended to be higher than application tasks */
#define W61_ATD_RX_TASK_PRIO                    54

/** W61 AT Rx parser task priority should be lower than W61_ATD_RX_TASK_PRIO */
#define W61_ATD_EVENTS_TASK_PRIO                (W61_ATD_RX_TASK_PRIO - 4)

/** Size of the x-buffer used to queue the AT commands-responses */
#define W61_ATD_CMDRSP_XBUF_SIZE                512

/** Max size of the string containing AT commands or AT responses
    It shall be =<  W61_ATD_CMDRSP_XBUF_SIZE */
#define W61_ATD_CMDRSP_STRING_SIZE              192

/** Size of the x-buffer used to queue the AT events */
#define W61_ATD_EVENTS_XBUF_SIZE                512

/**  Max size of the string containing AT events
    It shall be =< W61_ATD_EVENTS_XBUF_SIZE */
#define W61_ATD_EVENT_STRING_SIZE               192

/* USER CODE BEGIN EC */

/* USER CODE END EC */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* W61_DRIVER_CONFIG_TEMPLATE_H */
