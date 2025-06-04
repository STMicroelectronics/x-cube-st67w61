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

/* Soft-AP connection parameters */
#define WIFI_SAP_SSID               "ST67W6X_AP"

#define WIFI_SAP_PASSWORD           "12345678"

#define WIFI_SAP_CHANNEL            1

#define WIFI_SAP_SECURITY           W6X_WIFI_AP_SECURITY_WPA2_PSK

#define WIFI_SAP_MAX_CONNECTIONS    4

/** Low power configuration [0: disable / 1: sleep / 2: stop / 3: standby] */
#define LOW_POWER_MODE              LOW_POWER_DISABLE

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
