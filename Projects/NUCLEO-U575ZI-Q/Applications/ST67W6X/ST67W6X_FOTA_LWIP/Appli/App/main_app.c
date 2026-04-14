/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    main_app.c
  * @author  ST67 Application Team
  * @brief   main_app program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025-2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include <inttypes.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

/* Application */
#include "main.h"
#include "main_app.h"
#include "app_config.h"
#include "fota.h"
#include "lwip.h"

#if (LOW_POWER_MODE > LOW_POWER_DISABLE)
#include "utilities_conf.h"
#include "stm32_lpm.h"
#endif /* LOW_POWER_MODE */

#include "w6x_api.h"
#include "common_parser.h" /* Common Parser functions */
#include "spi_iface.h" /* SPI falling/rising_callback */
#include "logging.h"
#include "shell.h"
#include "logshell_ctrl.h"

#ifndef REDEFINE_FREERTOS_INTERFACE
/* Depending on the version of FreeRTOS the inclusion might need to be redefined in app_config.h */
#include "app_freertos.h"
#include "queue.h"
#include "event_groups.h"
#endif /* REDEFINE_FREERTOS_INTERFACE */

#if (LOW_POWER_MODE == LOW_POWER_STDBY_ENABLE)
#error "low power standby mode not supported"
#endif /* LOW_POWER_MODE */

/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Global variables ----------------------------------------------------------*/
/* USER CODE BEGIN GV */

/* USER CODE END GV */

/* Private typedef -----------------------------------------------------------*/
/**
  * @brief  Application information structure
  */
typedef struct
{
  char *version;                  /*!< Version of the application */
  char *name;                     /*!< Name of the application */
} APP_Info_t;

/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private defines -----------------------------------------------------------*/
#define EVENT_FLAG_SCAN_DONE   (1UL << 1U)        /*!< Scan done event bitmask */

#define WIFI_SCAN_TIMEOUT      10000              /*!< Delay before to declare the scan in failure */

/** Value in milliseconds of the timer configured in the echo FOTA application,
  * it will trigger the FOTA update event after the amount of time specified once configured. */
#define FOTA_TRIGGER_TIMER     10000

/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macros ------------------------------------------------------------*/
/** Stringify version */
#define XSTR(x) #x

/** Macro to stringify version */
#define MSTR(x) XSTR(x)

/** Application version */
#define HOST_APP_VERSION_STR      \
  MSTR(HOST_APP_VERSION_MAIN) "." \
  MSTR(HOST_APP_VERSION_SUB1) "." \
  MSTR(HOST_APP_VERSION_SUB2)

/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* Event bitmask flag used for asynchronous execution */
/** Wi-Fi scan event flags */
static EventGroupHandle_t scan_event_flags = NULL;

/** State to indicate if the application should quit */
static uint8_t quit_msg = 0;

/** Application information */
static const APP_Info_t app_info =
{
  .name = "ST67W6X Wi-Fi FOTA",
  .version = HOST_APP_VERSION_STR
};

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/**
  * @brief  Callback function used by FOTA when the process is completed and is about to reboot
  * @return FOTA_SUCCESS if the callback is successful, otherwise an error is caught by the FOTA task.
  * @note   Here it disconnects the device from the Access Point
  */
int32_t Fota_CompletionCb(void);

/**
  * @brief  Wi-Fi event callback
  * @param  event_id: Event ID
  * @param  event_args: Event arguments
  */
static void APP_wifi_cb(W6X_event_id_t event_id, void *event_args);

/**
  * @brief  Network event callback
  * @param  event_id: Event ID
  * @param  event_args: Event arguments
  */
static void APP_net_cb(W6X_event_id_t event_id, void *event_args);

/**
  * @brief  MQTT event callback
  * @param  event_id: Event ID
  * @param  event_args: Event arguments
  */
static void APP_mqtt_cb(W6X_event_id_t event_id, void *event_args);

/**
  * @brief  BLE event callback
  * @param  event_id: Event ID
  * @param  event_args: Event arguments
  */
static void APP_ble_cb(W6X_event_id_t event_id, void *event_args);

/**
  * @brief  W6X error callback
  * @param  ret_w6x: W6X status
  * @param  func_name: function name
  */
static void APP_error_cb(W6X_Status_t ret_w6x, char const *func_name);

/**
  * @brief  Wi-Fi scan callback
  * @param  status: Scan status
  * @param  Scan_results: Scan results
  */
static void APP_wifi_scan_cb(int32_t status, W6X_WiFi_Scan_Result_t *Scan_results);

#if (SHELL_ENABLE == 1)
/**
  * @brief  Shell command to quit the application
  * @param  argc: number of arguments
  * @param  argv: pointer to the arguments
  * @retval ::SHELL_STATUS_OK on success
  */
int32_t APP_shell_quit(int32_t argc, char **argv);
#endif /* SHELL_ENABLE */

/**
  * @brief  Initialize the low power manager
  */
void LowPowerManagerInit(void);

/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Functions Definition ------------------------------------------------------*/
void main_app(void)
{
  W6X_Status_t ret;
  EventBits_t eventBits = 0;

  /* USER CODE BEGIN main_app_1 */

  /* USER CODE END main_app_1 */

  /* Wi-Fi variables */
  W6X_WiFi_Scan_Opts_t Opts = {0};
  W6X_WiFi_Connect_Opts_t ConnectOpts = {0};
  W6X_WiFi_Connect_t connectData = {0};
  W6X_WiFi_StaStateType_e state = W6X_WIFI_STATE_STA_OFF;

#if (LOW_POWER_MODE > LOW_POWER_DISABLE)
  LowPowerManagerInit();
#endif /* LOW_POWER_MODE */

  /* Initialize the logging utilities */
  LoggingInit();
#if (SHELL_ENABLE == 1)
  /* Initialize the shell utilities on UART instance */
  ShellInit();
#endif /* SHELL_ENABLE == 1 */

  LogInfo("#### Welcome to %s Application #####\n", app_info.name);
  LogInfo("# build: %s %s\n", __TIME__, __DATE__);
  LogInfo("--------------- Host info ---------------\n");
  LogInfo("Host FW Version:          %s\n", app_info.version);

  /* USER CODE BEGIN main_app_2 */

  /* USER CODE END main_app_2 */

  /* Register the application callback to received events from ST67W6X Driver */
  W6X_App_Cb_t App_cb = {0};
  App_cb.APP_wifi_cb = APP_wifi_cb;
  App_cb.APP_net_cb = APP_net_cb;
  App_cb.APP_ble_cb = APP_ble_cb;
  App_cb.APP_mqtt_cb = APP_mqtt_cb;
  App_cb.APP_error_cb = APP_error_cb;
  (void)W6X_RegisterAppCb(&App_cb);

  /* Initialize the ST67W6X Driver */
  ret = W6X_Init();
  if (ret != W6X_STATUS_OK)
  {
    LogError("Failed to initialize ST67W6X Driver, %" PRIi32 "\n", ret);
    goto _err;
  }

  /* Initialize the ST67W6X Wi-Fi module */
  ret = W6X_WiFi_Init();
  if (ret != W6X_STATUS_OK)
  {
    LogError("Failed to initialize ST67W6X Wi-Fi component, %" PRIi32 "\n", ret);
    goto _err;
  }
  LogInfo("Wi-Fi init is done\n");

  /* Initialize the LWIP stack */
  if (MX_LWIP_Init() != 0)
  {
    LogError("Failed to initialize LWIP stack\n");
    goto _err;
  }

  /* USER CODE BEGIN main_app_3 */

  /* USER CODE END main_app_3 */
  /* Run a Wi-Fi scan to retrieve the list of all nearby Access Points */
  scan_event_flags = xEventGroupCreate();
  (void)W6X_WiFi_Scan(&Opts, &APP_wifi_scan_cb);

  /* Wait to receive the EVENT_FLAG_SCAN_DONE event. The scan is declared as failed after 'ScanTimeout' delay */
  eventBits = xEventGroupWaitBits(scan_event_flags, EVENT_FLAG_SCAN_DONE,
                                  pdTRUE, pdFALSE,
                                  pdMS_TO_TICKS(WIFI_SCAN_TIMEOUT));
  if ((eventBits & EVENT_FLAG_SCAN_DONE) == 0U)
  {
    LogError("Scan Failed\n");
    goto _err;
  }

  /* Connect the device to the pre-defined Access Point */
  LogInfo("\nConnecting to Local Access Point\n");
  (void)strncpy((char *)ConnectOpts.SSID, WIFI_SSID, W6X_WIFI_MAX_SSID_SIZE);
  (void)strncpy((char *)ConnectOpts.Password, WIFI_PASSWORD, W6X_WIFI_MAX_PASSWORD_SIZE);
  ret = W6X_WiFi_Connect(&ConnectOpts);
  if (ret != W6X_STATUS_OK)
  {
    LogError("Failed to connect, %" PRIi32 "\n", ret);
    goto _err;
  }

  LogInfo("App connected\n");
  if (W6X_WiFi_Station_GetState(&state, &connectData) != W6X_STATUS_OK)
  {
    LogError("Failed to get Station state\n");
    goto _err;
  }

  LogInfo("Connected to following Access Point :\n");
  LogInfo("[" MACSTR "] Channel: %" PRIu32 " | RSSI: %" PRIi32 " | SSID: %s\n",
          MAC2STR(connectData.MAC),
          connectData.Channel,
          connectData.Rssi,
          connectData.SSID);

  /* Define the default factor to apply to AP DTIM interval when connected */
  ret = W6X_WiFi_SetDTIM(WIFI_DTIM);
  if (ret != W6X_STATUS_OK)
  {
    LogError("Failed to initialize the DTIM, %" PRIi32 "\n", ret);
    goto _err;
  }

  /* Register the callback to be called when the FOTA process is completed */
  Fota_RegisterCallbacks(Fota_CompletionCb, NULL);

  /* Start the FOTA task that initialize the FOTA and awaits an event in order to run the FOTA process */
  LogInfo("Starting FOTA task\n");
  (void)Fota_StartFotaTask();

  /* Creates or/and configure a timer that triggers an event to start the FOTA process */
  LogInfo("Starting FOTA timer trigger, current value in millisecond is %" PRIu32 "\n",
          (uint32_t)FOTA_TRIGGER_TIMER);
  (void)Fota_SetFotaTimer(FOTA_TRIGGER_TIMER);
  /* Start the FOTA timer */
  (void)Fota_StartFotaTimer();

  LogInfo("Waiting for FOTA trigger\n");
#if (SHELL_ENABLE == 1)
  LogInfo("\nApplication runs in CLI mode. Type help or quit to exit.\n");
#endif /* SHELL_ENABLE */
  while (quit_msg == 0U)
  {
    /* Wait for FOTA task to send a specific event,
       this will notify the main_app task that FOTA task finished successfully with the FOTA W61 transfer */
    (void)Fota_WaitForFOTACompletion();
  }

  quit_msg = 0;
  while (quit_msg == 0U)
  {
    vTaskDelay(pdMS_TO_TICKS(1000));
  }

  LogInfo("##### Quitting the application\n");

  /* USER CODE BEGIN main_app_Last */

  /* USER CODE END main_app_Last */

_err:
  /* USER CODE BEGIN main_app_Err_1 */

  /* USER CODE END main_app_Err_1 */

  /* De-initialize the FOTA task */
  (void)Fota_DeleteFotaTask();

  /* De-initialize the ST67W6X Wi-Fi module */
  W6X_WiFi_DeInit();

  /* De-initialize the ST67W6X Driver */
  W6X_DeInit();

  /* USER CODE BEGIN main_app_Err_2 */

  /* USER CODE END main_app_Err_2 */
  LogInfo("##### Application end\n");
}

/* coverity[misra_c_2012_rule_5_8_violation : FALSE] */
/* coverity[misra_c_2012_rule_8_6_violation : FALSE] */
void HAL_GPIO_EXTI_Rising_Callback(uint16_t GPIO_Pin)
{
  /* USER CODE BEGIN EXTI_Rising_Callback_1 */

  /* USER CODE END EXTI_Rising_Callback_1 */
  /* Callback when data is available in Network CoProcessor to enable SPI Clock */
  if (GPIO_Pin == SPI_RDY_Pin)
  {
    (void)spi_on_txn_data_ready();
  }
  /* USER CODE BEGIN EXTI_Rising_Callback_End */

  /* USER CODE END EXTI_Rising_Callback_End */
}

/* coverity[misra_c_2012_rule_5_8_violation : FALSE] */
/* coverity[misra_c_2012_rule_8_6_violation : FALSE] */
void HAL_GPIO_EXTI_Falling_Callback(uint16_t GPIO_Pin)
{
  /* USER CODE BEGIN EXTI_Falling_Callback_1 */

  /* USER CODE END EXTI_Falling_Callback_1 */
  /* Callback when data is available in Network CoProcessor to enable SPI Clock */
  if (GPIO_Pin == SPI_RDY_Pin)
  {
    (void)spi_on_header_ack();
  }

  /* Callback when user button is pressed */
  if (GPIO_Pin == USER_BUTTON_Pin)
  {
    Fota_TriggerFotaUpdate();
  }
  /* USER CODE BEGIN EXTI_Falling_Callback_End */

  /* USER CODE END EXTI_Falling_Callback_End */
}

/* USER CODE BEGIN FD */

/* USER CODE END FD */

/* Private Functions Definition ----------------------------------------------*/
int32_t Fota_CompletionCb(void)
{
  /* Disconnect the device from the Access Point */
  if (W6X_WiFi_Disconnect(1) == W6X_STATUS_OK)
  {
    LogInfo("Wi-Fi Disconnect success\n");
  }
  else
  {
    LogError("Wi-Fi Disconnect failed\n");
  }
  quit_msg = 1;

  return FOTA_SUCCESS;
}

static void APP_wifi_scan_cb(int32_t status, W6X_WiFi_Scan_Result_t *Scan_results)
{
  /* USER CODE BEGIN APP_wifi_scan_cb_1 */

  /* USER CODE END APP_wifi_scan_cb_1 */
  LogInfo("SCAN DONE\n");
  LogInfo(" Cb informed APP that WIFI SCAN DONE.\n");
  W6X_WiFi_PrintScan(Scan_results);
  (void)xEventGroupSetBits(scan_event_flags, EVENT_FLAG_SCAN_DONE);
  /* USER CODE BEGIN APP_wifi_scan_cb_End */

  /* USER CODE END APP_wifi_scan_cb_End */
}

static void APP_wifi_cb(W6X_event_id_t event_id, void *event_args)
{
  /* USER CODE BEGIN APP_wifi_cb_1 */

  /* USER CODE END APP_wifi_cb_1 */

  switch (event_id)
  {
    case W6X_WIFI_EVT_CONNECTED_ID:
      break;

    case W6X_WIFI_EVT_DISCONNECTED_ID:
      LogInfo("Station disconnected from Access Point\n");
      break;

    case W6X_WIFI_EVT_REASON_ID:
      LogInfo("Reason: %s\n", W6X_WiFi_ReasonToStr(event_args));
      break;

    default:
      /* Wi-Fi events unmanaged */
      break;
  }
  /* USER CODE BEGIN APP_wifi_cb_End */

  /* USER CODE END APP_wifi_cb_End */
}

static void APP_net_cb(W6X_event_id_t event_id, void *event_args)
{
  /* USER CODE BEGIN APP_net_cb_1 */

  /* USER CODE END APP_net_cb_1 */
}

static void APP_mqtt_cb(W6X_event_id_t event_id, void *event_args)
{
  /* USER CODE BEGIN APP_mqtt_cb_1 */

  /* USER CODE END APP_mqtt_cb_1 */
}

static void APP_ble_cb(W6X_event_id_t event_id, void *event_args)
{
  /* USER CODE BEGIN APP_ble_cb_1 */

  /* USER CODE END APP_ble_cb_1 */
}

static void APP_error_cb(W6X_Status_t ret_w6x, char const *func_name)
{
  /* USER CODE BEGIN APP_error_cb_1 */

  /* USER CODE END APP_error_cb_1 */
  LogError("[%s] in %s API\n", W6X_StatusToStr(ret_w6x), func_name);
  /* USER CODE BEGIN APP_error_cb_2 */

  /* USER CODE END APP_error_cb_2 */
}

#if (SHELL_ENABLE == 1)
int32_t APP_shell_quit(int32_t argc, char **argv)
{
  quit_msg = 1;
  return SHELL_STATUS_OK;
}

SHELL_CMD_EXPORT_ALIAS(APP_shell_quit, quit, quit. Stop application execution);
#endif /* SHELL_ENABLE */

void LowPowerManagerInit(void)
{
  /* USER CODE BEGIN LowPowerManagerInit_1 */

  /* USER CODE END LowPowerManagerInit_1 */

#if (LOW_POWER_MODE > LOW_POWER_DISABLE)
  /* Init low power manager */
  UTIL_LPM_Init();

  /* USER CODE BEGIN LowPowerManagerInit_2 */

  /* USER CODE END LowPowerManagerInit_2 */

#if (DEBUGGER_ENABLED == 1)
  HAL_DBGMCU_EnableDBGStopMode();
  HAL_DBGMCU_EnableDBGStandbyMode();
#else
  HAL_DBGMCU_DisableDBGStopMode();
  HAL_DBGMCU_DisableDBGStandbyMode();
#endif /* DEBUGGER_ENABLED */

  /* USER CODE BEGIN LowPowerManagerInit_3 */

  /* USER CODE END LowPowerManagerInit_3 */

#if (LOW_POWER_MODE < LOW_POWER_STDBY_ENABLE)
  /* Disable Stand-by mode */
  UTIL_LPM_SetOffMode((1UL << CFG_LPM_APPLI_ID), UTIL_LPM_DISABLE);
#endif /* LOW_POWER_MODE */
#if (LOW_POWER_MODE < LOW_POWER_STOP_ENABLE)
  /* Disable Stop Mode */
  UTIL_LPM_SetStopMode((1UL << CFG_LPM_APPLI_ID), UTIL_LPM_DISABLE);
#endif /* LOW_POWER_MODE */
#endif /* LOW_POWER_MODE */

  /* USER CODE BEGIN LowPowerManagerInit_End */

  /* USER CODE END LowPowerManagerInit_End */
}

/* USER CODE BEGIN PFD */

/* USER CODE END PFD */
