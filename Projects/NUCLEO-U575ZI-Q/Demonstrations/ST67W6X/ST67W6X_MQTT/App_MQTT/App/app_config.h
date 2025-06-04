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

/** SNTP timezone configuration */
#define WIFI_SNTP_TIMEZONE          1

/** Subscribed topic max buffer size */
#define MQTT_TOPIC_BUFFER_SIZE      100

/** Subscribed message max buffer size */
#define MQTT_MSG_BUFFER_SIZE        600

/** Host name of remote MQTT Broker
  * Multiple options are possibles:
  *   - broker.hivemq.com
  *   - broker.emqx.io
  *   - test.mosquitto.org
  */
#define MQTT_HOST_NAME              "broker.emqx.io"

/** Port of remote MQTT Broker */
#define MQTT_HOST_PORT              1883

/** Security level. only non-secure is currently available */
#define MQTT_SECURITY_LEVEL         0

/** MQTT Client ID to be identified on MQTT Broker */
#define MQTT_CLIENT_ID              "mySTM32_772"

/** MQTT Username to be identified on MQTT Broker. Not used in non-secure */
#define MQTT_USERNAME               "MyName"

/** MQTT Password to be identified on MQTT Broker. Not used in non-secure */
#define MQTT_USER_PASSWORD          "MyPsw"

/** MQTT Client Certificate. Required when the scheme is greater or equal to 3 */
#define MQTT_CERTIFICATE            "client_1.crt"

/** MQTTClient Private key. Required when the scheme is greater or equal to 3 */
#define MQTT_KEY                    "client_1.key"

/** MQTT Client CA certificate. Required when the scheme is greater or equal to 2 */
#define MQTT_CA_CERTIFICATE         "ca_1.crt"

/** MQTT Server Name Indication (SNI) enabled */
#define MQTT_SNI_ENABLED            1

/** Low power configuration [0: disable / 1: sleep / 2: stop / 3: standby] */
#define LOW_POWER_MODE              LOW_POWER_SLEEP_ENABLE

/**
  * Enable/Disable MCU Debugger pins (dbg serial wires)
  * @note  by HW serial wires are ON by default, need to put them OFF to save power
  */
#define DEBUGGER_ENABLED            1

/* Automation usage */
/** Enable/Disable the test automation */
#define TEST_AUTOMATION_ENABLE      0

/* USER CODE BEGIN EC */

/* USER CODE END EC */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* APP_CONFIG_H */
