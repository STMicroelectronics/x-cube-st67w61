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

/** Timeout value to set the FOTA timer to when the FOTA application encountered an error for the first time.
  * This allows to tune the timeout value before doing a retry attempt. (not applicable if FOTA timer is not used)*/
#define FOTA_TIMEOUT                20000

/** Delay to wait before rebooting the host device, waiting for NCP device to finish update */
#define FOTA_DELAY_BEFORE_REBOOT    16000

/** Stack size of the FOTA application, this value needs to take into account the HTTP client
  * and NCP OTA static data allocation */
#define FOTA_TASK_STACK_SIZE        2560

/** The max size of the URI supported, this because the buffer
  * that will receive this info is allocated at compile time (static) */
#define FOTA_URI_MAX_SIZE           128

/** Default URI for the ST67 binary, should be smaller in bytes size than the value defined by FOTA_URI_MAX_SIZE */
#define FOTA_HTTP_URI               "/download/st67w611m_mission_t01_v2.0.75.bin.ota"

/** Default HTTP server address */
#define FOTA_HTTP_SERVER_ADDR       "192.168.8.105"

/** Default HTTP port */
#define FOTA_HTTP_SERVER_PORT       8000

/** As specified in RFC 1035 Domain Implementation and Specification
  * from November 1987, domain names are 255 octets or less */
#define FOTA_MAX_DOMAIN_NAME_SIZE   255U

/** Subscribed topic max buffer size */
#define MQTT_TOPIC_BUFFER_SIZE      100

/** Subscribed message max buffer size */
#define MQTT_MSG_BUFFER_SIZE        600

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
#define LOW_POWER_MODE              LOW_POWER_DISABLE

/**
  * Enable/Disable MCU Debugger pins (dbg serial wires)
  * @note  by HW serial wires are ON by default, need to put them OFF to save power
  */
#define DEBUGGER_ENABLED            1

/* USER CODE BEGIN EC */
#undef FOTA_HTTP_URI

/** The folder containing the FOTA header, ST67 binary and the STM32 binary */
#define FOTA_HTTP_URI_TARGET        "/STM32U575ZI_NUCLEO"

/** Root folder of interest on the HTTP server */
#define FOTA_HTTP_COMMON_URI        "/download" FOTA_HTTP_URI_TARGET

/** Default URI for the ST67 binary, should be smaller in bytes size than the value defined by FOTA_URI_MAX_SIZE */
#define FOTA_HTTP_URI               FOTA_HTTP_COMMON_URI "/st67w611m_mission_t01_v2.0.75.bin.ota"

/** Default URI for the STM32 binary, should be smaller in bytes size than the value defined by FOTA_URI_MAX_SIZE */
#define FOTA_HTTP_URI_STM32         FOTA_HTTP_COMMON_URI "/fota_ST67W6X_CLI.bin"

/** Default URI for the FOTA header */
#define FOTA_HTTP_URI_HEADER        FOTA_HTTP_COMMON_URI "/ST67W611_STM32U575ZI_NUCLEO.json"

/* USER CODE END EC */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* APP_CONFIG_H */
