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
  * @brief  MQTT data structure
  */
typedef struct
{
  uint8_t *topic;                 /*!< Topic of the received message */
  uint32_t topic_length;          /*!< Length of the topic */
  uint8_t *message;               /*!< Message received */
  uint32_t message_length;        /*!< Length of the message */
} APP_MQTT_Data_t;

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
/* Application events */
/** Event when user button is pressed */
#define EVT_APP_BUTTON                              (1<<0)
/** Event when Wi-Fi is connected to an Access Point */
#define EVT_APP_WIFI_CONNECTED                      (1<<1)
/** Event when Wi-Fi got an IP address from the Access Point */
#define EVT_APP_WIFI_GOT_IP                         (1<<2)
/** Event when Wi-Fi is disconnected from the Access Point */
#define EVT_APP_WIFI_DISCONNECTED                   (1<<2)
/** Event to quit the application */
#define EVT_APP_QUIT                                (1<<3)

/** Application events bitmask */
#define EVT_APP_ALL_BIT         (EVT_APP_BUTTON | \
                                 EVT_APP_WIFI_CONNECTED  | \
                                 EVT_APP_WIFI_GOT_IP  | \
                                 EVT_APP_WIFI_DISCONNECTED  | \
                                 EVT_APP_QUIT )

#ifndef MQTT_TOPIC_BUFFER_SIZE
/** Subscribed topic max buffer size */
#define MQTT_TOPIC_BUFFER_SIZE          100
#endif /* MQTT_TOPIC_BUFFER_SIZE */

#ifndef MQTT_MSG_BUFFER_SIZE
/** Subscribed message max buffer size */
#define MQTT_MSG_BUFFER_SIZE            600
#endif /* MQTT_MSG_BUFFER_SIZE */

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

/** MQTT structure to receive subscribed message from the ST67W6X Driver */
static W6X_MQTT_Data_t mqtt_recv_data;

/** BLE data buffer to receive message from the ST67W6X Driver */
static uint8_t a_APP_AvailableData[247] = {0};

/** Application event group */
static EventGroupHandle_t app_evt_current;

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

/**
  * @brief  Set event group to release the waiting task
  * @param  app_event: Event group
  * @param  evt: Event to set
  */
static void APP_setevent(EventGroupHandle_t *app_event, uint32_t evt);

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
  EventBits_t eventBits = 0;
  W6X_WiFi_Connect_t connectData = {0};
  W6X_WiFi_StaStateType_e state = W6X_WIFI_STATE_STA_OFF;
  uint8_t ip_addr[4] = {0};
  uint8_t gateway_addr[4] = {0};
  uint8_t netmask_addr[4] = {0};
  uint8_t ap_ipaddr[4] = {0};
  uint8_t ap_netmask_addr[4] = {0};

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

  app_evt_current = xEventGroupCreate();

  /* Initialize the ST67W6X Driver */
  ret = W6X_Init();
  if (ret)
  {
    LogError("Failed to initialize ST67W6X Driver, %" PRIi32 "\n", ret);
    goto _err;
  }

  /* Initialize the ST67W6X Wi-Fi module */
  ret = W6X_WiFi_Init();
  if (ret)
  {
    LogError("Failed to initialize ST67W6X Wi-Fi component, %" PRIi32 "\n", ret);
    goto _err;
  }
  LogInfo("Wi-Fi init is done\n");

  /* Initialize the ST67W6X Network module */
  ret = W6X_Net_Init();
  if (ret)
  {
    LogError("Failed to initialize ST67W6X Net component, %" PRIi32 "\n", ret);
    goto _err;
  }
  LogInfo("Net init is done\n");

  /* Initialize the ST67W6X MQTT module */
  mqtt_recv_data.p_recv_data = mqtt_buffer;
  mqtt_recv_data.recv_data_buf_size = MQTT_TOPIC_BUFFER_SIZE + MQTT_MSG_BUFFER_SIZE;
  ret = W6X_MQTT_Init(&mqtt_recv_data);
  if (ret)
  {
    LogError("Failed to initialize ST67W6X MQTT component, %" PRIi32 "\n", ret);
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

  while (1)
  {
    /* Wait to receive a BLE event */
    eventBits = xEventGroupWaitBits(app_evt_current, EVT_APP_ALL_BIT, pdTRUE, pdFALSE, portMAX_DELAY);

    /* Process button */
    if (eventBits & EVT_APP_BUTTON)
    {
    }

    if (eventBits & EVT_APP_WIFI_CONNECTED)
    {
      if (W6X_WiFi_Station_GetState(&state, &connectData) != W6X_STATUS_OK)
      {
        LogWarn("Connected to an unknown Access Point\n");
        continue;
      }

      LogInfo("Connected to following Access Point :\n");
      LogInfo("[" MACSTR "] Channel: %" PRIu32 " | RSSI: %" PRIi32 " | SSID: %s\n",
              MAC2STR(connectData.MAC),
              connectData.Channel,
              connectData.Rssi,
              connectData.SSID);
    }

    if (eventBits & EVT_APP_WIFI_GOT_IP)
    {
      if (W6X_Net_Station_GetIPAddress(ip_addr, gateway_addr, netmask_addr) != W6X_STATUS_OK)
      {
        LogError("Failed to get Station IP address\n");
        continue;
      }
      /* Get the Soft-AP IP address */
      if (W6X_Net_AP_GetIPAddress(ap_ipaddr, ap_netmask_addr) != W6X_STATUS_OK)
      {
        LogError("Failed to get Soft-AP IP address\n");
        continue;
      }
      LogInfo("Station got a new IP address : " IPSTR "\n", IP2STR(ip_addr));

      /* Check if the station and Soft-AP have the same subnet IP */
      if (memcmp(gateway_addr, ap_ipaddr, 4) == 0)
      {
        LogWarn("Station and Soft-AP have the same subnet IP, Soft-AP IP need to be changed\n");
      }
    }

    if (eventBits & EVT_APP_QUIT)
    {
      break;
    }
  }

  LogInfo("##### Quitting the application\n");

  /* USER CODE BEGIN main_app_Last */

  /* USER CODE END main_app_Last */

_err:
  /* USER CODE BEGIN main_app_Err_1 */

  /* USER CODE END main_app_Err_1 */

  /* De-initialize the FOTA task */
  Fota_DeleteFotaTask();

  /* De-initialize the ST67W6X BLE module */
  W6X_Ble_DeInit();

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
    APP_setevent(&app_evt_current, EVT_APP_BUTTON);
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

  W6X_WiFi_CbParamData_t *cb_data = {0};

  switch (event_id)
  {
    case W6X_WIFI_EVT_CONNECTED_ID:
      APP_setevent(&app_evt_current, EVT_APP_WIFI_CONNECTED);
      break;

    case W6X_WIFI_EVT_DISCONNECTED_ID:
      LogInfo("Station disconnected from Access Point\n");
      break;

    case W6X_WIFI_EVT_GOT_IP_ID:
      APP_setevent(&app_evt_current, EVT_APP_WIFI_GOT_IP);
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
      mqtt_recv_data.p_recv_data[p_param_mqtt_data->topic_length] = '\0';
      LogInfo("MQTT Subscription Received on topic %*s: %*s\n",
              p_param_mqtt_data->topic_length,
              mqtt_recv_data.p_recv_data,
              p_param_mqtt_data->message_length,
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
  uint8_t service_index = 0;
  uint8_t charac_index = 0;
  uint32_t charac_handle = 0;
  uint32_t charac_value_handle = 0;

  W6X_Ble_Service_t *service = NULL;
  char tmp_UUID[33];
  uint8_t uuid_size = 0;

  W6X_Ble_CbParamData_t *p_param_ble_data = (W6X_Ble_CbParamData_t *) event_args;

  switch (event_id)
  {
    case W6X_BLE_EVT_CONNECTED_ID:
      LogInfo(" -> BLE CONNECTED: Conn_Handle: %" PRIu16 "\n", p_param_ble_data->remote_ble_device.conn_handle);
      W6X_Ble_SetRecvDataPtr(a_APP_AvailableData, sizeof(a_APP_AvailableData));
      break;

    case W6X_BLE_EVT_CONNECTION_PARAM_ID:
      LogInfo(" -> BLE CONNECTION PARAM UPDATE\n");
      break;

    case W6X_BLE_EVT_DISCONNECTED_ID:
      LogInfo(" -> BLE DISCONNECTED.\n");
      break;

    case W6X_BLE_EVT_INDICATION_STATUS_ENABLED_ID:
      LogInfo(" -> BLE INDICATION ENABLED [Service: %" PRIu16 ", Charac: %" PRIu16 "]\n",
              p_param_ble_data->service_idx, p_param_ble_data->charac_idx);
      break;

    case W6X_BLE_EVT_INDICATION_STATUS_DISABLED_ID:
      LogInfo(" -> BLE INDICATION DISABLED [Service: %" PRIu16 ", Charac: %" PRIu16 "]\n",
              p_param_ble_data->service_idx, p_param_ble_data->charac_idx);
      break;

    case W6X_BLE_EVT_NOTIFICATION_STATUS_ENABLED_ID:
      LogInfo(" -> BLE NOTIFICATION ENABLED [Service: %" PRIu16 ", Charac: %" PRIu16 "]\n",
              p_param_ble_data->service_idx, p_param_ble_data->charac_idx);
      break;

    case W6X_BLE_EVT_NOTIFICATION_STATUS_DISABLED_ID:
      LogInfo(" -> BLE NOTIFICATION DISABLED [Service: %" PRIu16 ", Charac: %" PRIu16 "]\n",
              p_param_ble_data->service_idx, p_param_ble_data->charac_idx);
      break;

    case W6X_BLE_EVT_NOTIFICATION_DATA_ID:
      LogInfo(" -> BLE NOTIFICATION [Connection: %" PRIu16 ", Charac value handle: %" PRIu16 "]\n",
              p_param_ble_data->remote_ble_device.conn_handle, p_param_ble_data->charac_value_handle);
      for (uint32_t i = 0; i < p_param_ble_data->available_data_length; i++)
      {
        LogInfo("0x%02" PRIX16 "\n", a_APP_AvailableData[i]);
      }
      memset(a_APP_AvailableData, 0, sizeof(a_APP_AvailableData));
      break;

    case W6X_BLE_EVT_WRITE_ID:
      LogInfo(" -> BLE WRITE [Connection: %" PRIu16 ", Service: %" PRIu16 ", Charac: %" PRIu16 "]\n",
              p_param_ble_data->remote_ble_device.conn_handle, p_param_ble_data->service_idx,
              p_param_ble_data->charac_idx);
      for (uint32_t i = 0; i < p_param_ble_data->available_data_length; i++)
      {
        LogInfo("0x%02" PRIX16 "\n", a_APP_AvailableData[i]);
      }
      memset(a_APP_AvailableData, 0, sizeof(a_APP_AvailableData));
      break;

    case W6X_BLE_EVT_READ_ID:
      LogInfo(" -> BLE READ [Connection: %" PRIu16 ", Service: %" PRIu16 ", Charac: %" PRIu16 "]\n",
              p_param_ble_data->remote_ble_device.conn_handle, p_param_ble_data->service_idx,
              p_param_ble_data->charac_idx);
      for (uint32_t i = 0; i < p_param_ble_data->available_data_length; i++)
      {
        LogInfo("0x%02" PRIX16 "\n", a_APP_AvailableData[i]);
      }
      memset(a_APP_AvailableData, 0, sizeof(a_APP_AvailableData));
      break;

    case W6X_BLE_EVT_SERVICE_FOUND_ID:
      service_index = p_param_ble_data->Service.service_idx;

      service = &p_param_ble_data->Service;
      memset(tmp_UUID, 0x20, 33);

      uuid_size = service->uuid_type == W6X_BLE_UUID_TYPE_16 ? 4 : 16;
      for (int32_t i = 0; i < uuid_size; i++)
      {
        sprintf(&tmp_UUID[i * 2], "%02" PRIx16, service->service_uuid[i]);
      }

      LogInfo(" -> BLE SERVICE DISCOVERED:\nidx = %" PRIu16 ", UUID = %s\n",
              service_index, tmp_UUID);
      break;

    case W6X_BLE_EVT_CHAR_FOUND_ID:
      service_index = p_param_ble_data->Service.service_idx;
      charac_index = p_param_ble_data->Service.charac[0].char_idx;
      charac_handle = p_param_ble_data->Service.charac[0].char_handle;
      charac_value_handle = p_param_ble_data->Service.charac[0].char_value_handle;

      memset(tmp_UUID, 0x20, 33);

      uuid_size = p_param_ble_data->Service.charac[0].uuid_type == W6X_BLE_UUID_TYPE_16 ? 4 : 16;
      for (int32_t i = 0; i < uuid_size; i++)
      {
        sprintf(&tmp_UUID[i * 2], "%02" PRIx16, p_param_ble_data->Service.charac[0].char_uuid[i]);
      }

      LogInfo(" -> BLE CHARACTERISTIC DISCOVERED:\nService idx = %" PRIu16 ", Charac idx = %" PRIu16
              ", UUID = %s, \r\nChar Handle = %" PRIu32 ",Char Value Handle = %" PRIu32 "\n",
              service_index, charac_index, tmp_UUID, charac_handle, charac_value_handle);
      break;

    case W6X_BLE_EVT_PASSKEY_ENTRY_ID:
      LogInfo(" -> BLE PassKey Entry: Conn_Handle: %" PRIu16 "\n", p_param_ble_data->remote_ble_device.conn_handle);
      LogInfo("    BD Addr: %02X:%02X:%02X:%02X:%02X:%02X\n",
              p_param_ble_data->remote_ble_device.BDAddr[0], p_param_ble_data->remote_ble_device.BDAddr[1],
              p_param_ble_data->remote_ble_device.BDAddr[2], p_param_ble_data->remote_ble_device.BDAddr[3],
              p_param_ble_data->remote_ble_device.BDAddr[4], p_param_ble_data->remote_ble_device.BDAddr[5]);
      LogInfo("    BD Addr type: %" PRIu32 "\n", p_param_ble_data->remote_ble_device.bd_addr_type);
      break;

    case W6X_BLE_EVT_PASSKEY_CONFIRM_ID:
      LogInfo(" -> BLE PassKey received = %06" PRIu32 ", Conn_Handle: %" PRIu16 "\n", p_param_ble_data->PassKey,
              p_param_ble_data->remote_ble_device.conn_handle);
      LogInfo("    BD Addr: %02X:%02X:%02X:%02X:%02X:%02X\n",
              p_param_ble_data->remote_ble_device.BDAddr[0], p_param_ble_data->remote_ble_device.BDAddr[1],
              p_param_ble_data->remote_ble_device.BDAddr[2], p_param_ble_data->remote_ble_device.BDAddr[3],
              p_param_ble_data->remote_ble_device.BDAddr[4], p_param_ble_data->remote_ble_device.BDAddr[5]);
      LogInfo("    BD Addr type: %" PRIu32 "\n", p_param_ble_data->remote_ble_device.bd_addr_type);
      break;

    case W6X_BLE_EVT_PAIRING_CONFIRM_ID:
      LogInfo(" -> BLE Pairing Confirm: Conn_Handle: %" PRIu16 "\n", p_param_ble_data->remote_ble_device.conn_handle);
      LogInfo("    BD Addr: %02X:%02X:%02X:%02X:%02X:%02X\n",
              p_param_ble_data->remote_ble_device.BDAddr[0], p_param_ble_data->remote_ble_device.BDAddr[1],
              p_param_ble_data->remote_ble_device.BDAddr[2], p_param_ble_data->remote_ble_device.BDAddr[3],
              p_param_ble_data->remote_ble_device.BDAddr[4], p_param_ble_data->remote_ble_device.BDAddr[5]);
      LogInfo("    BD Addr type: %" PRIu32 "\n", p_param_ble_data->remote_ble_device.bd_addr_type);
      break;

    case W6X_BLE_EVT_PAIRING_COMPLETED_ID:
      LogInfo(" -> BLE Pairing Completed\n\n");
      LogInfo("    BD Addr: %02X:%02X:%02X:%02X:%02X:%02X\n",
              p_param_ble_data->remote_ble_device.BDAddr[0], p_param_ble_data->remote_ble_device.BDAddr[1],
              p_param_ble_data->remote_ble_device.BDAddr[2], p_param_ble_data->remote_ble_device.BDAddr[3],
              p_param_ble_data->remote_ble_device.BDAddr[4], p_param_ble_data->remote_ble_device.BDAddr[5]);
      LogInfo("    BD Addr type: %" PRIu32 "\n", p_param_ble_data->remote_ble_device.bd_addr_type);
      LogInfo("    LTK: %s\n", p_param_ble_data->LongTermKey);
      break;

    case W6X_BLE_EVT_PASSKEY_DISPLAY_ID:
      LogInfo(" -> BLE PASSKEY  = %06" PRIu32 "\n", p_param_ble_data->PassKey);
      break;

    case W6X_BLE_EVT_PAIRING_FAILED_ID:
      LogInfo(" -> BLE Pairing Failed: Conn_Handle: %" PRIu16 "\n", p_param_ble_data->remote_ble_device.conn_handle);
      LogInfo("    BD Addr: %02X:%02X:%02X:%02X:%02X:%02X\n",
              p_param_ble_data->remote_ble_device.BDAddr[0], p_param_ble_data->remote_ble_device.BDAddr[1],
              p_param_ble_data->remote_ble_device.BDAddr[2], p_param_ble_data->remote_ble_device.BDAddr[3],
              p_param_ble_data->remote_ble_device.BDAddr[4], p_param_ble_data->remote_ble_device.BDAddr[5]);
      LogInfo("    BD Addr type: %" PRIu32 "\n", p_param_ble_data->remote_ble_device.bd_addr_type);
      break;

    case W6X_BLE_EVT_PAIRING_CANCELED_ID:
      LogInfo(" -> BLE Pairing Canceled: Conn_Handle: %" PRIu16 "\n", p_param_ble_data->remote_ble_device.conn_handle);
      LogInfo("    BD Addr: %02X:%02X:%02X:%02X:%02X:%02X\n",
              p_param_ble_data->remote_ble_device.BDAddr[0], p_param_ble_data->remote_ble_device.BDAddr[1],
              p_param_ble_data->remote_ble_device.BDAddr[2], p_param_ble_data->remote_ble_device.BDAddr[3],
              p_param_ble_data->remote_ble_device.BDAddr[4], p_param_ble_data->remote_ble_device.BDAddr[5]);
      LogInfo("    BD Addr type: %" PRIu32 "\n", p_param_ble_data->remote_ble_device.bd_addr_type);
      break;

    default:
      break;
  }
  /* USER CODE BEGIN APP_ble_cb_End */

  /* USER CODE END APP_ble_cb_End */
}

static void APP_error_cb(W6X_Status_t ret_w6x, char const *func_name)
{
  /* USER CODE BEGIN APP_error_cb_1 */

  /* USER CODE END APP_error_cb_1 */
  LogError("[%s] in %s API\n", W6X_StatusToStr(ret_w6x), func_name);
  /* USER CODE BEGIN APP_error_cb_2 */

  /* USER CODE END APP_error_cb_2 */
}

static void APP_setevent(EventGroupHandle_t *app_event, uint32_t evt)
{
  /* USER CODE BEGIN APP_setevent_1 */

  /* USER CODE END APP_setevent_1 */
  BaseType_t xHigherPriorityTaskWoken = pdFALSE;
  if (xPortIsInsideInterrupt())
  {
    xEventGroupSetBitsFromISR(*app_event, evt, &xHigherPriorityTaskWoken);
    if (xHigherPriorityTaskWoken)
    {
      portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
  }
  else
  {
    xEventGroupSetBits(*app_event, evt);
  }
  /* USER CODE BEGIN APP_setevent_End */

  /* USER CODE END APP_setevent_End */
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
  APP_setevent(&app_evt_current, EVT_APP_QUIT);
  return SHELL_STATUS_OK;
}

SHELL_CMD_EXPORT_ALIAS(APP_shell_quit, quit, quit. Stop application execution);
#endif /* SHELL_ENABLE */

/* USER CODE BEGIN PFD */

/* USER CODE END PFD */
