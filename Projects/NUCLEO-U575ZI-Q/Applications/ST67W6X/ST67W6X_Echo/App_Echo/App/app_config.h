/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    app_config.h
  * @author  GPM Application Team
  * @brief   Configuration for main application
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
#ifndef APP_CONFIG_H
#define APP_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Includes ------------------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/**
  * Supported requester to the MCU Low Power Manager - can be increased up  to 32
  * It lists a bit mapping of all user of the Low Power Manager
  */
typedef enum
{
  CFG_LPM_APPLI_ID,
} CFG_LPM_Id_t;

/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
#define LOG_OUTPUT_PRINTF           0
#define LOG_OUTPUT_UART             1
#define LOG_OUTPUT_ITM              2

#define LOW_POWER_DISABLE           0
#define LOW_POWER_SLEEP_ENABLE      1
#define LOW_POWER_STOP_ENABLE       2
#define LOW_POWER_STDBY_ENABLE      3

/** Select output log mode [0: printf / 1: UART / 2: ITM] */
#define LOG_OUTPUT_MODE             LOG_OUTPUT_UART

/* Local Access Point (e.g. gateway, hotspot, etc) connection parameters */
#define WIFI_SSID                   "MY_SSID"

#define WIFI_PASSWORD               "MY_PASSWORD"

#if (defined(NET_USE_IPV6) && (NET_USE_IPV6 == 1))
/** Use a local ipv6 server 2A05:D018:21F:3800:3164:2A5C:75B3:970B port 7. */
#define REMOTE_IP_ADDR              "2A05:D018:21F:3800:3164:2A5C:75B3:970B"
#define REMOTE_IPV6_ADDR0           NET_HTONL(0x2A05D018)
#define REMOTE_IPV6_ADDR1           NET_HTONL(0x021F3800)
#define REMOTE_IPV6_ADDR2           NET_HTONL(0x31642A5C)
#define REMOTE_IPV6_ADDR3           NET_HTONL(0x75B3970B)
#define REMOTE_PORT                 7
#else
/** URL of Echo TCP remote server */
#define ECHO_SERVER_URL             "tcpbin.com"

/** Port of Echo TCP remote server */
#define ECHO_SERVER_PORT            4242
#endif /* NET_USE_IPV6 */

/** Minimal size of a packet */
#define ECHO_TRANSFER_SIZE_START    1000

/** Maximal size of a packet */
#define ECHO_TRANSFER_SIZE_MAX      2000

/** To increment the size of a group of packets */
#define ECHO_TRANSFER_SIZE_ITER     250

/** Number of packets to be sent */
#define ECHO_ITERATION_COUNT        10

/** Low power configuration [0: disable / 1: sleep / 2: stop / 3: standby] */
#define LOW_POWER_MODE              LOW_POWER_SLEEP_ENABLE

/**
  * Enable/Disable MCU Debugger pins (dbg serial wires)
  * @note  by HW serial wires are ON by default, need to put them OFF to save power
  */
#define DEBUGGER_ENABLED            1

/* USER CODE BEGIN EC */

/* USER CODE END EC */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* APP_CONFIG_H */
