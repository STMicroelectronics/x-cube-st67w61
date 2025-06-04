/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    main_app.c
  * @author  GPM Application Team
  * @brief   main_app program body
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
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
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
typedef struct
{
  uint8_t *topic;                 /*!< Topic of the received message */
  uint32_t topic_length;          /*!< Length of the topic */
  uint8_t *message;               /*!< Message received */
  uint32_t message_length;        /*!< Length of the message */
} APP_MQTT_Data_t;

typedef struct
{
  char *version;                  /*!< Version of the application */
  char *name;                     /*!< Name of the application */
} APP_Info_t;

/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private defines -----------------------------------------------------------*/
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
/** MQTT buffer to receive subscribed message from the ST67W6X Driver */
static uint8_t mqtt_buffer[MQTT_TOPIC_BUFFER_SIZE + MQTT_MSG_BUFFER_SIZE];

/** MQTT data structure to store the received data */
static W6X_MQTT_Data_t mqtt_recv_data;

static uint8_t quit_msg = 0;

/** Application information */
static const APP_Info_t app_info =
{
  .name = "ST67W6X CLI",
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

#if (SHELL_ENABLE == 1)
/**
  * @brief  Shell command to display the application information
  * @param  argc: number of arguments
  * @param  argv: pointer to the arguments
  * @retval ::SHELL_STATUS_OK on success
  */
int32_t APP_shell_info(int32_t argc, char **argv);

/**
  * @brief  Shell command to quit the application
  * @param  argc: number of arguments
  * @param  argv: pointer to the arguments
  * @retval ::SHELL_STATUS_OK on success
  */
int32_t APP_shell_quit(int32_t argc, char **argv);
#endif /* SHELL_ENABLE */

/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Functions Definition ------------------------------------------------------*/
void main_app(void)
{
  int32_t ret = 0;

  /* USER CODE BEGIN main_app_1 */

  /* USER CODE END main_app_1 */

  /* Initialize the logging utilities */
  LoggingInit();
  /* Initialize the shell utilities on UART instance */
  ShellInit();

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
  W6X_RegisterAppCb(&App_cb);

  /* Initialize the ST67W6X Driver */
  ret = W6X_Init();
  if (ret)
  {
    LogError("failed to initialize ST67W6X Driver, %" PRIi32 "\n", ret);
    goto _err;
  }

  /* Initialize the ST67W6X Wi-Fi module */
  ret = W6X_WiFi_Init();
  if (ret)
  {
    LogError("failed to initialize ST67W6X Wi-Fi component, %" PRIi32 "\n", ret);
    goto _err;
  }
  LogInfo("Wi-Fi init is done\n");

  /* Set DTIM value (dtim * 100ms). 0: Disabled, 1: 100ms, 10: 1s */
  ret = W6X_WiFi_SetDTIM(10);
  if (ret)
  {
    LogError("failed to initialize the DTIM, %" PRIi32 "\n", ret);
    goto _err;
  }

  /* Initialize the ST67W6X Network module */
  ret = W6X_Net_Init();
  if (ret)
  {
    LogError("failed to initialize ST67W6X Net component, %" PRIi32 "\n", ret);
    goto _err;
  }
  LogInfo("Net init is done\n");

  /* Initialize the ST67W6X MQTT module */
  mqtt_recv_data.p_recv_data = mqtt_buffer;
  mqtt_recv_data.recv_data_buf_size = MQTT_TOPIC_BUFFER_SIZE + MQTT_MSG_BUFFER_SIZE;
  ret = W6X_MQTT_Init(&mqtt_recv_data);
  if (ret)
  {
    LogError("failed to initialize ST67W6X MQTT component, %" PRIi32 "\n", ret);
    goto _err;
  }
  LogInfo("MQTT init is done\n");

  /* USER CODE BEGIN main_app_3 */

  /* USER CODE END main_app_3 */

  /* Register the callback to be called when the FOTA process is completed */
  Fota_RegisterCallbacks(Fota_CompletionCb, NULL);

  /* Create and runs the FOTA task, it awaits a trigger to launch the FOTA procedure */
  LogInfo("Starting FOTA task\n");
  Fota_StartFotaTask();

  LogInfo("ready\n");

  while (quit_msg == 0)
  {
    vTaskDelay(1000);
  }

  LogInfo("##### Quitting the application\n");

  /* USER CODE BEGIN main_app_Last */

  /* USER CODE END main_app_Last */

_err:
  /* USER CODE BEGIN main_app_Err_1 */

  /* USER CODE END main_app_Err_1 */

  /* De-initialize the FOTA task */
  Fota_DeleteFotaTask();

  /* De-initialize the ST67W6X MQTT module */
  W6X_MQTT_DeInit();

  /* De-initialize the ST67W6X Network module */
  W6X_Net_DeInit();

  /* De-initialize the ST67W6X Wi-Fi module */
  W6X_WiFi_DeInit();

  /* De-initialize the ST67W6X Driver */
  W6X_DeInit();

  shell_freertos_deinit();

  /* USER CODE BEGIN main_app_Err_2 */

  /* USER CODE END main_app_Err_2 */
  LogInfo("##### Application end\n");
}

/* USER CODE BEGIN MX_App_Init */
void MX_App_CLI_Init(void);
void MX_App_CLI_Init(void)
{
  /* This function is not supposed to be filled, created just for compilation purpose
     in case user forgets to uncheck the STM32CubeMX GUI box to avoid its call in main()
     The application initialization is done by the main_app() function on FreeRTOS task. */
  return;
}
/* USER CODE END MX_App_Init */

void HAL_GPIO_EXTI_Callback(uint16_t pin);
void HAL_GPIO_EXTI_Rising_Callback(uint16_t pin);
void HAL_GPIO_EXTI_Falling_Callback(uint16_t pin);

void HAL_GPIO_EXTI_Callback(uint16_t pin)
{
  /* USER CODE BEGIN HAL_GPIO_EXTI_Callback_1 */

  /* USER CODE END HAL_GPIO_EXTI_Callback_1 */
  /* Callback when data is available in Network CoProcessor to enable SPI Clock */
  if (pin == SPI_RDY_Pin)
  {
    if (HAL_GPIO_ReadPin(SPI_RDY_GPIO_Port, SPI_RDY_Pin) == GPIO_PIN_SET)
    {
      HAL_GPIO_EXTI_Rising_Callback(pin);
    }
    else
    {
      HAL_GPIO_EXTI_Falling_Callback(pin);
    }
  }
  /* USER CODE BEGIN HAL_GPIO_EXTI_Callback_End */

  /* USER CODE END HAL_GPIO_EXTI_Callback_End */
}

void HAL_GPIO_EXTI_Rising_Callback(uint16_t pin)
{
  /* USER CODE BEGIN EXTI_Rising_Callback_1 */

  /* USER CODE END EXTI_Rising_Callback_1 */
  /* Callback when data is available in Network CoProcessor to enable SPI Clock */
  if (pin == SPI_RDY_Pin)
  {
    spi_on_txn_data_ready();
  }
  /* USER CODE BEGIN EXTI_Rising_Callback_End */

  /* USER CODE END EXTI_Rising_Callback_End */
}

void HAL_GPIO_EXTI_Falling_Callback(uint16_t pin)
{
  /* USER CODE BEGIN EXTI_Falling_Callback_1 */

  /* USER CODE END EXTI_Falling_Callback_1 */
  /* Callback when data is available in Network CoProcessor to enable SPI Clock */
  if (pin == SPI_RDY_Pin)
  {
    spi_on_header_ack();
  }

  /* Callback when user button is pressed */
  if (pin == USER_BUTTON_Pin)
  {
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

  return FOTA_SUCCESS;
}

static void APP_wifi_cb(W6X_event_id_t event_id, void *event_args)
{
  /* USER CODE BEGIN APP_wifi_cb_1 */

  /* USER CODE END APP_wifi_cb_1 */

  W6X_WiFi_Connect_t connectData = {0};
  W6X_WiFi_CbParamData_t *cb_data = {0};
  W6X_WiFi_StaStateType_e state = W6X_WIFI_STATE_STA_OFF;
  uint8_t ip_addr[4] = {0};
  uint8_t gateway_addr[4] = {0};
  uint8_t netmask_addr[4] = {0};

  switch (event_id)
  {
    case W6X_WIFI_EVT_CONNECTED_ID:
      if (W6X_WiFi_GetStaState(&state, &connectData) != W6X_STATUS_OK)
      {
        LogInfo("Connected to an Access Point\n");
        return;
      }

      LogInfo("Connected to following Access Point :\n");
      LogInfo("[" MACSTR "] Channel: %" PRIu32 " | RSSI: %" PRIi32 " | SSID: %s\n",
              MAC2STR(connectData.MAC),
              connectData.Channel,
              connectData.Rssi,
              connectData.SSID);
      break;
    case W6X_WIFI_EVT_DISCONNECTED_ID:
      LogInfo("Station disconnected from Access Point\n");
      break;

    case W6X_WIFI_EVT_GOT_IP_ID:
      if (W6X_WiFi_GetStaIpAddress(ip_addr, gateway_addr, netmask_addr) != W6X_STATUS_OK)
      {
        LogInfo("Station got an IP from Access Point\n");
        return;
      }
      LogInfo("Station got an IP from Access Point : " IPSTR "\n", IP2STR(ip_addr));
      break;

    case W6X_WIFI_EVT_REASON_ID:
      LogInfo("Reason: %s\n", W6X_WiFi_ReasonToStr(event_args));
      break;

    case W6X_WIFI_EVT_DIST_STA_IP_ID:
      break;

    case W6X_WIFI_EVT_STA_CONNECTED_ID:
      cb_data = (W6X_WiFi_CbParamData_t *)event_args;
      LogInfo("Station connected to soft-AP : [" MACSTR "]\n", MAC2STR(cb_data->MAC));
      break;

    case W6X_WIFI_EVT_STA_DISCONNECTED_ID:
      cb_data = (W6X_WiFi_CbParamData_t *)event_args;
      LogInfo("Station disconnected from soft-AP : [" MACSTR "]\n", MAC2STR(cb_data->MAC));
      break;

    default:
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
  W6X_MQTT_CbParamData_t *p_param_mqtt_data = (W6X_MQTT_CbParamData_t *) event_args;
  switch (event_id)
  {
    case W6X_MQTT_EVT_CONNECTED_ID:
      LogInfo("MQTT Connected\n");
      break;

    case W6X_MQTT_EVT_DISCONNECTED_ID:
      LogInfo("MQTT Disconnected\n");
      break;

    case W6X_MQTT_EVT_SUBSCRIPTION_RECEIVED_ID:
      LogInfo("MQTT Subscription Received on topic: %s : %s\n", mqtt_recv_data.p_recv_data,
              &mqtt_recv_data.p_recv_data[p_param_mqtt_data->topic_length + 1]);
      break;

    default:
      break;
  }
  /* USER CODE BEGIN APP_mqtt_cb_End */

  /* USER CODE END APP_mqtt_cb_End */
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
int32_t APP_shell_info(int32_t argc, char **argv)
{
  /* USER CODE BEGIN APP_shell_info_1 */

  /* USER CODE END APP_shell_info_1 */
  SHELL_PRINTF("--------------- Host info ---------------\n");
  SHELL_PRINTF("Host FW Version:          %s\n", app_info.version);
  SHELL_PRINTF("Host FW Name:             %s\n", app_info.name);

  return SHELL_STATUS_OK;
  /* USER CODE BEGIN APP_shell_info_End */

  /* USER CODE END APP_shell_info_End */
}

SHELL_CMD_EXPORT_ALIAS(APP_shell_info, info_app, info_app. Display application info);

int32_t APP_shell_quit(int32_t argc, char **argv)
{
  quit_msg = 1;
  return SHELL_STATUS_OK;
}

SHELL_CMD_EXPORT_ALIAS(APP_shell_quit, quit, quit. Stop application execution);
#endif /* SHELL_ENABLE */

/* USER CODE BEGIN PFD */

/* USER CODE END PFD */
