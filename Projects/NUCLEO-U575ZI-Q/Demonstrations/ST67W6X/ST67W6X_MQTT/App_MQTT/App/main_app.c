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
#include <time.h>

/* Application */
#include "main.h"
#include "main_app.h"
#include "app_config.h"

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
#include "cJSON.h"

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
#include "sys_sensors.h"
#include "stm32u5xx_nucleo_errno.h"

/* USER CODE END Includes */

/* Global variables ----------------------------------------------------------*/
extern RTC_HandleTypeDef hrtc;

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
#define EVENT_FLAG_SCAN_DONE            (1<<1)    /*!< Scan done event bitmask */

#define WIFI_SCAN_TIMEOUT               10000     /*!< Delay before to declare the scan in failure */

/** Priority of the subscription process task */
#define SUBSCRIPTION_THREAD_PRIO        24

/** Stack size of the subscription process task */
#define SUBSCRIPTION_TASK_STACK_SIZE    1024

static const char *WeekDayString[] = { "Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun" };
static const char *MonthString[] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul",
                                     "Aug", "Sep", "Oct", "Nov", "Dec"
                                   };

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
/* MQTT buffer to receive subscribed message from the ST67W6X Driver */
static uint8_t mqtt_buffer[MQTT_TOPIC_BUFFER_SIZE + MQTT_MSG_BUFFER_SIZE];

/* MQTT structure to receive subscribed message from the ST67W6X Driver */
static W6X_MQTT_Data_t mqtt_recv_data;

/* Event bitmask flag used for asynchronous execution */
static EventGroupHandle_t scan_event_flags = NULL; /*!< Wi-Fi scan event flags */

/* MQTT buffer used to define the topic string of the subscription or publish */
static uint8_t mqtt_topic[MQTT_TOPIC_BUFFER_SIZE];

/* MQTT published message buffer */
static uint8_t mqtt_pubmsg[MQTT_MSG_BUFFER_SIZE];

/* MQTT Broker connection */
static W6X_MQTT_Connect_t mqtt_connect =
{
  .HostName = MQTT_HOST_NAME,           /*!< Host name of remote MQTT Broker */
  .HostPort = MQTT_HOST_PORT,           /*!< Port of remote MQTT Broker */
  .Scheme = MQTT_SECURITY_LEVEL,        /*!< Security level. only non-secure is currently available */
  .MQClientId = MQTT_CLIENT_ID,         /*!< MQTT Client ID to be identified on MQTT Broker */
  .MQUserName = MQTT_USERNAME,          /*!< MQTT Username to be identified on MQTT Broker. Not used in non-secure */
  .MQUserPwd = MQTT_USER_PASSWORD,      /*!< MQTT Password to be identified on MQTT Broker. Not used in non-secure */
  .Certificate = MQTT_CERTIFICATE,      /*!< Client Certificate. Required when the scheme is greater or equal to 3 */
  .PrivateKey = MQTT_KEY,               /*!< Client Private key. Required when the scheme is greater or equal to 3 */
  .CACertificate = MQTT_CA_CERTIFICATE, /*!< CA certificate. Required when the scheme is greater or equal to 2 */
  .SNI_enabled = MQTT_SNI_ENABLED       /*!< Server Name Indication (SNI) enabled */
};

/* Subscribed message Queue Handle */
static QueueHandle_t sub_msg_queue;

/* Subscribed message process Task Handle */
static TaskHandle_t  sub_task_handle;

bool green_led_status = false;

#if (TEST_AUTOMATION_ENABLE == 1)
static bool subscription_received = false;
#endif /* TEST_AUTOMATION_ENABLE */

/** Application information */
static const APP_Info_t app_info =
{
  .name = "ST67W6X MQTT",
  .version = HOST_APP_VERSION_STR
};

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/**
  * @brief  MQTT subscription process task
  * @param  arg: Task argument
  */
static void Subscription_process_task(void *arg);

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

/**
  * @brief  Initialize the low power manager
  */
static void LowPowerManagerInit(void);

/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Functions Definition ------------------------------------------------------*/
void main_app(void)
{
  int32_t ret = 0;

  /* USER CODE BEGIN main_app_1 */

  /* USER CODE END main_app_1 */

  /* Wi-Fi variables */
  W6X_WiFi_Scan_Opts_t Opts = {0};
  W6X_WiFi_Connect_Opts_t ConnectOpts = {0};
  uint8_t Mac[6] = {0};

  /* USER CODE BEGIN main_app_2 */
  /* Sensor variables */
  Sys_Sensors_Data_t sensors_data;
  bool sensor_initialized = false;

  /* USER CODE END main_app_2 */

  /* Time variables */
  uint8_t Time[32] = {0};
  uint8_t SNTP_Enable = 0;
  int16_t SNTP_Timezone = 0;
  RTC_TimeTypeDef time = {0};
  RTC_DateTypeDef date = {0};

  LowPowerManagerInit();

  /* Initialize the logging utilities */
  LoggingInit();
  /* Initialize the shell utilities on UART instance */
  ShellInit();

  LogInfo("#### Welcome to %s Application #####\n", app_info.name);
  LogInfo("# build: %s %s\n", __TIME__, __DATE__);
  LogInfo("--------------- Host info ---------------\n");
  LogInfo("Host FW Version:          %s\n", app_info.version);

  /* USER CODE BEGIN main_app_3 */

  /* USER CODE END main_app_3 */

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
  ret = W6X_WiFi_SetDTIM(0);
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

  /* USER CODE BEGIN main_app_4 */

  /* USER CODE END main_app_4 */
  /* Run a Wi-Fi scan to retrieve the list of all nearby Access Points */
  scan_event_flags = xEventGroupCreate();
  W6X_WiFi_Scan(&Opts, &APP_wifi_scan_cb);

  /* Wait to receive the EVENT_FLAG_SCAN_DONE event. The scan is declared as failed after 'ScanTimeout' delay */
  if ((int32_t)xEventGroupWaitBits(scan_event_flags, EVENT_FLAG_SCAN_DONE, pdTRUE, pdFALSE,
                                   pdMS_TO_TICKS(WIFI_SCAN_TIMEOUT)) != EVENT_FLAG_SCAN_DONE)
  {
    LogError("Scan Failed\n");
    goto _err;
  }

  /* Connect the device to the pre-defined Access Point */
  LogInfo("\nConnecting to Local Access Point\n");
  strncpy((char *)ConnectOpts.SSID, WIFI_SSID, W6X_WIFI_MAX_SSID_SIZE);
  strncpy((char *)ConnectOpts.Password, WIFI_PASSWORD, W6X_WIFI_MAX_PASSWORD_SIZE);
  ret = W6X_WiFi_Connect(&ConnectOpts);
  if (ret)
  {
    LogError("failed to connect, %" PRIi32 "\n", ret);
    goto _err;
  }
  LogInfo("App connected\n");

  ret = W6X_WiFi_GetStaMacAddress(Mac);
  if (ret)
  {
    LogError("failed to get the MAC Address, %" PRIi32 "\n", ret);
    goto _err;
  }

  /* Configure the SNTP client to receive local time */
  ret = W6X_Net_GetSNTPConfiguration(&SNTP_Enable, &SNTP_Timezone, NULL, NULL, NULL);

  if (ret == W6X_STATUS_OK)
  {
    if ((SNTP_Enable == 0) || (SNTP_Timezone != WIFI_SNTP_TIMEZONE))
    {
      /* Configure the SNTP client with default SNTP servers */
      ret = W6X_Net_SetSNTPConfiguration(1, WIFI_SNTP_TIMEZONE, (uint8_t *)"0.pool.ntp.org",
                                         (uint8_t *)"time.google.com", NULL);
      vTaskDelay(5000); /* Wait few seconds to execute the first request */
    }
  }

  if (ret != W6X_STATUS_OK)
  {
    LogError("SNTP Time Failure\n");
    goto _err;
  }

  /* Get the SNTP Time. expected format: Thu Oct  3 12:07:28 2024 */
  if (W6X_STATUS_OK == W6X_Net_GetTime(Time))
  {
    int32_t year;
    int32_t month;
    int32_t weekday;
    int32_t day;
    int32_t hour;
    int32_t minute;
    int32_t second;
    char mday[4] = {0};
    char mon[4] = {0};

    LogInfo("SNTP Time: %s\n", Time);
    /* Parse the time string into multiple variables */
    if (sscanf((char *)Time, "%s %s  %" SCNd32 " %2" SCNd32 ":%2" SCNd32 ":%2" SCNd32 " %" SCNd32,
               mday, mon, &day, &hour, &minute, &second, &year) != 7)
    {
      LogError("Process Time failed\n");
      goto _err;
    }

    /* Convert the week day string into value */
    for (weekday = 0; weekday < 7; weekday++)
    {
      if (strcmp(mday, WeekDayString[weekday]) == 0)
      {
        break;
      }
    }

    /* Convert the month string into value */
    for (month = 0; month < 12; month++)
    {
      if (strcmp(mon, MonthString[month]) == 0)
      {
        break;
      }
    }

    /* Failed if the week day or month conversion was not correct */
    if ((weekday == 7) || (month == 12))
    {
      LogError("Process Time failed\n");
      goto _err;
    }

    /* Set the Time in RTC IP */
    time.Hours = hour;
    time.Minutes = minute;
    time.Seconds = second;
    time.TimeFormat = RTC_HOURFORMAT12_AM;
    time.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
    time.StoreOperation = RTC_STOREOPERATION_RESET;

    if (HAL_RTC_SetTime(&hrtc, &time, RTC_FORMAT_BIN) != HAL_OK)
    {
      LogError("Process Time failed\n");
      goto _err;
    }

    /* Set the Date in RTC IP */
    date.Year = year - 2000;    /* Convert the year to 2-digits format */
    date.Month = month + 1;     /* Add month index due to RTC month range [1-12] */
    date.Date = day;
    date.WeekDay = weekday + 1; /* Add week day index due to RTC month range [1-7] */

    if (HAL_RTC_SetDate(&hrtc, &date, RTC_FORMAT_BIN) != HAL_OK)
    {
      LogError("Process Time failed\n");
      goto _err;
    }
  }
  else
  {
    LogError("SNTP Time Failure\n");
    goto _err;
  }

  /* USER CODE BEGIN main_app_5 */
  /* Initialize the Sensors */
  if (Sys_Sensors_Init() != BSP_ERROR_NONE)
  {
    LogWarn("MEMS Sensors init failed. the MQTT publish will not use the sensors values\n");
  }
  else
  {
    LogInfo("MEMS Sensors init successful\n");
    sensor_initialized = true;
  }

  /* USER CODE END main_app_5 */

  /* Configure the MQTT broker connection with user parameters */
  if (W6X_STATUS_OK == W6X_MQTT_Configure(&mqtt_connect))
  {
    LogInfo("MQTT Configure successful\n");
  }
  else
  {
    LogError("MQTT Configure Failure\n");
    goto _err;
  }

  /* Connect the device to the pre-defined MQTT broker */
  if (W6X_STATUS_OK == W6X_MQTT_Connect(&mqtt_connect))
  {
    LogInfo("MQTT Connect successful\n");
  }
  else
  {
    LogError("MQTT Connect Failure\n");
    goto _err;
  }

  /* Add a new task to process the received message of subscribed topics */
  sub_msg_queue = xQueueCreate(10, sizeof(APP_MQTT_Data_t));
  xTaskCreate(Subscription_process_task, (char *)"mqtt_sub",
              SUBSCRIPTION_TASK_STACK_SIZE >> 2, NULL, SUBSCRIPTION_THREAD_PRIO, &sub_task_handle);

  /* Subscribe to a control topic with topic_level based on ClientID */
  snprintf((char *)mqtt_topic, MQTT_TOPIC_BUFFER_SIZE, "/devices/%s/control", mqtt_connect.MQClientId);
  LogInfo("Subscribing to topic %s.\n", mqtt_topic);
  ret = W6X_MQTT_Subscribe(mqtt_topic);
  memset(mqtt_topic, 0, sizeof(mqtt_topic));

  /* Subscribe to a sensor topic with topic_level based on ClientID */
  snprintf((char *)mqtt_topic, MQTT_TOPIC_BUFFER_SIZE, "/sensors/%s", mqtt_connect.MQClientId);
  LogInfo("Subscribing to topic %s.\n", mqtt_topic);
  ret = W6X_MQTT_Subscribe(mqtt_topic);

  /* Reuse the same topic to publish message */
  do
  {
    uint32_t len = 0;
    W6X_WiFi_StaStateType_e State = W6X_WIFI_STATE_STA_NO_STARTED_CONNECTION;
    W6X_WiFi_Connect_t ConnectData = {0};

    /* Get the current date and time */
    HAL_RTC_GetTime(&hrtc, &time, RTC_FORMAT_BIN);
    HAL_RTC_GetDate(&hrtc, &date, RTC_FORMAT_BIN);

    /* Get the Wi-Fi station state to retrieve the RSSI value */
    ret = W6X_WiFi_GetStaState(&State, &ConnectData);

    /* Create the json string message with:
      * - current date and time
      * - mac address of device
      * - current RSSI level of the Access Point
      *
      * Note: The state.reported object hierarchy is used to help the interoperability
      *       with 1st tier cloud providers.
      */
    len = snprintf((char *)mqtt_pubmsg, MQTT_MSG_BUFFER_SIZE, "{\n \"state\": {\n \"reported\": {\n "
                   "   \"time\": \"%02" PRIu16 "-%02" PRIu16 "-%02" PRIu16 " %02" PRIu16 ":%02" PRIu16 ":%02" PRIu16
                   "\", \"mac\": \"" MACSTR "\", \"rssi\": %" PRIi32 "\n",
                   date.Year, date.Month, date.Date, time.Hours, time.Minutes, time.Seconds,
                   MAC2STR(Mac), ConnectData.Rssi);

    /* USER CODE BEGIN main_app_6 */
    /* Append the JSON message with time and sensors values if the sensor board is correctly started */
    if (sensor_initialized)
    {
      /* Read the sensor values of the components on the MEMS expansion board */
      Sys_Sensors_Read(&sensors_data);

      /* Append the json string message with:
       * - environmental sensor values
       *
       * Note: The state.reported object hierarchy is used to help the interoperability
       *       with 1st tier cloud providers.
       */
      len += snprintf((char *)&mqtt_pubmsg[len], MQTT_MSG_BUFFER_SIZE - len,
                      ", \"temperature\": %.2f, \"humidity\": %.2f, \"pressure\": %.2f",
                      (double)sensors_data.temperature, (double)sensors_data.humidity, (double)sensors_data.pressure);
    }

    /* USER CODE END main_app_6 */

    snprintf((char *)&mqtt_pubmsg[len], MQTT_MSG_BUFFER_SIZE - len, "\n  }\n }\n}");

    /* Publish the message on a topic with topic_level based on ClientID */
    ret = W6X_MQTT_Publish(mqtt_topic, mqtt_pubmsg, strlen((char *)mqtt_pubmsg));
    if (ret == W6X_STATUS_OK)
    {
      LogInfo("MQTT Publish OK\n");
    }
    vTaskDelay(2000);
  }
#if (TEST_AUTOMATION_ENABLE == 1)
  while ((ret == W6X_STATUS_OK) && (subscription_received == false));
#else
  while (ret == W6X_STATUS_OK);
#endif /* TEST_AUTOMATION_ENABLE */

  if (ret != W6X_STATUS_OK)
  {
    LogError("MQTT Failure\n");
  }

  /* Close the MQTT connection */
  ret = W6X_MQTT_Disconnect();
  if (ret == W6X_STATUS_OK)
  {
    LogInfo("MQTT Disconnect success\n");
  }
  else
  {
    LogError("MQTT Disconnect failed\n");
  }

  /* Disconnect the device from the Access Point */
  ret = W6X_WiFi_Disconnect(1);
  if (ret == W6X_STATUS_OK)
  {
    LogInfo("Wi-Fi Disconnect success\n");
  }
  else
  {
    LogError("Wi-Fi Disconnect failed\n");
  }

  LogInfo("##### Quitting the application\n");

  /* USER CODE BEGIN main_app_Last */

  /* USER CODE END main_app_Last */

_err:
  /* USER CODE BEGIN main_app_Err_1 */

  /* USER CODE END main_app_Err_1 */
  /* De-initialize the ST67W6X MQTT module */
  W6X_MQTT_DeInit();

  /* De-initialize the ST67W6X Network module */
  W6X_Net_DeInit();

  /* De-initialize the ST67W6X Wi-Fi module */
  W6X_WiFi_DeInit();

  /* De-initialize the ST67W6X Driver */
  W6X_DeInit();

  /* USER CODE BEGIN main_app_Err_2 */

  /* USER CODE END main_app_Err_2 */
  LogInfo("##### Application end\n");
}

/* USER CODE BEGIN MX_App_Init */
void MX_App_MQTT_Init(void);
void MX_App_MQTT_Init(void)
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
static void Subscription_process_task(void *arg)
{
  /* USER CODE BEGIN Subscription_process_task_1 */

  /* USER CODE END Subscription_process_task_1 */
  int32_t ret;
  APP_MQTT_Data_t mqtt_data = {0};
  cJSON *json = NULL;
  cJSON *root = NULL;
  cJSON *child = NULL;
  cJSON_Hooks hooks =
  {
    .malloc_fn = pvPortMalloc,
    .free_fn = vPortFree,
  };

  cJSON_InitHooks(&hooks);

  for (;;)
  {
    /* Wait a new message from the subscribed topics */
    ret = xQueueReceive(sub_msg_queue, &mqtt_data, 2000);
    if (ret)
    {
      LogInfo("MQTT Subscription Received on topic %s\n", (char *)mqtt_data.topic);
#if (TEST_AUTOMATION_ENABLE == 1)
      subscription_received = true;
#endif /* TEST_AUTOMATION_ENABLE */

      /* Parse the string message into a JSON element */
      root = cJSON_Parse((const char *)mqtt_data.message);
      if (root == NULL)
      {
        LogError("Processing error of JSON message\n");
        goto _err;
      }

      /* Get the data content into state.reported object hierarchy if defined */
      child = cJSON_GetObjectItemCaseSensitive(root, "state");
      if (child != NULL)
      {
        child = cJSON_GetObjectItemCaseSensitive(child, "reported");
      }
      else
      {
        /* Set the child from the root if the 'state' level does not exists */
        child = root;
      }

      if (child == NULL)
      {
        LogError("Processing error of JSON message\n");
        goto _err;
      }

      /* Process the field 'time'. Value type: String. Format: "%y-%m-%d %H:%M:%S" */
      json = cJSON_GetObjectItemCaseSensitive(child, "time");
      if (json != NULL)
      {
        if (cJSON_IsString(json) == true)
        {
          LogInfo("  %s: %s\n", json->string, json->valuestring);
        }
        else
        {
          LogError("JSON parsing error of %s value.\n", json->string);
        }
      }

      /* Process the field 'rssi'. Value type: Int. Format: -50 (in db) */
      json = cJSON_GetObjectItemCaseSensitive(child, "rssi");
      if (json != NULL)
      {
        if (cJSON_IsNumber(json) == true)
        {
          LogInfo("  %s: %" PRIi32 "\n", json->string, (int32_t)json->valueint);
        }
        else
        {
          LogError("JSON parsing error of %s value.\n", json->string);
        }
      }

      /* Process the field 'mac'. Value type: String. Format: "00:00:00:00:00:00" */
      json = cJSON_GetObjectItemCaseSensitive(child, "mac");
      if (json != NULL)
      {
        if (cJSON_IsString(json) == true)
        {
          LogInfo("  %s: %s\n", json->string, json->valuestring);
        }
        else
        {
          LogError("JSON parsing error of %s value.\n", json->string);
        }
      }

      /* Process the field 'LedOn'. Value type: Bool */
      json = cJSON_GetObjectItemCaseSensitive(child, "LedOn");
      if (json != NULL)
      {
        if (cJSON_IsBool(json) == true)
        {
          green_led_status = (cJSON_IsTrue(json) == true);
          HAL_GPIO_WritePin(LED_GREEN_GPIO_Port, LED_GREEN_Pin, green_led_status ? GPIO_PIN_SET : GPIO_PIN_RESET);
          LogInfo("  %s: %" PRIu16 "\n", json->string, green_led_status);
        }
        else
        {
          LogError("JSON parsing error of %s value.\n", json->string);
        }
      }

      /* Process the field 'Reboot'. Value type: Bool */
      json = cJSON_GetObjectItemCaseSensitive(child, "Reboot");
      if (json != NULL)
      {
        if (cJSON_IsBool(json) == true)
        {
          if (cJSON_IsTrue(json) == true)
          {
            LogInfo("  %s requested in 1s ...\n", json->string);
            vTaskDelay(1000);
            HAL_NVIC_SystemReset();
          }
        }
        else
        {
          LogError("JSON parsing error of Reboot value.\n");
        }
      }

      /* USER CODE BEGIN Subscription_process_task_2 */
      /* Process the field 'temperature'. Value type: Float. Format: 20.00 (in degC) */
      json = cJSON_GetObjectItemCaseSensitive(child, "temperature");
      if (json != NULL)
      {
        if (cJSON_IsNumber(json) == true)
        {
          LogInfo("  %s: %.2f\n", json->string, json->valuedouble);
        }
        else
        {
          LogError("JSON parsing error of %s value.\n", json->string);
        }
      }

      /* Process the field 'pressure'. Value type: Float. Format: 1000.00 (in mbar) */
      json = cJSON_GetObjectItemCaseSensitive(child, "pressure");
      if (json != NULL)
      {
        if (cJSON_IsNumber(json) == true)
        {
          LogInfo("  %s: %.2f\n", json->string, json->valuedouble);
        }
        else
        {
          LogError("JSON parsing error of %s value.\n", json->string);
        }
      }

      /* Process the field 'humidity'. Value type: Float. Format: 50.00 (in percent) */
      json = cJSON_GetObjectItemCaseSensitive(child, "humidity");
      if (json != NULL)
      {
        if (cJSON_IsNumber(json) == true)
        {
          LogInfo("  %s: %.2f\n", json->string, json->valuedouble);
        }
        else
        {
          LogError("JSON parsing error of %s value.\n", json->string);
        }
      }

      /* USER CODE END Subscription_process_task_2 */

_err:
      /* Clean the JSON element */
      cJSON_Delete(root);

      /* Free the topic and message allocated */
      vPortFree(mqtt_data.topic);
      vPortFree(mqtt_data.message);
    }
  }
  /* USER CODE BEGIN Subscription_process_task_End */

  /* USER CODE END Subscription_process_task_End */
}

static void APP_wifi_scan_cb(int32_t status, W6X_WiFi_Scan_Result_t *Scan_results)
{
  /* USER CODE BEGIN APP_wifi_scan_cb_1 */

  /* USER CODE END APP_wifi_scan_cb_1 */
  LogInfo("SCAN DONE\n");
  LogInfo(" Cb informed APP that WIFI SCAN DONE.\n");
  W6X_WiFi_PrintScan(Scan_results);
  xEventGroupSetBits(scan_event_flags, EVENT_FLAG_SCAN_DONE);
  /* USER CODE BEGIN APP_wifi_scan_cb_End */

  /* USER CODE END APP_wifi_scan_cb_End */
}

static void APP_wifi_cb(W6X_event_id_t event_id, void *event_args)
{
  /* USER CODE BEGIN APP_wifi_cb_1 */

  /* USER CODE END APP_wifi_cb_1 */

  W6X_WiFi_Connect_t connectData = {0};
  W6X_WiFi_StaStateType_e state = W6X_WIFI_STATE_STA_OFF;

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

    case W6X_WIFI_EVT_REASON_ID:
      LogInfo("Reason: %s\n", W6X_WiFi_ReasonToStr(event_args));
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

  W6X_Net_CbParamData_t *p_param_app_net_cb;

  switch (event_id)
  {
    case W6X_NET_EVT_SOCK_DATA_ID:
      p_param_app_net_cb = (W6X_Net_CbParamData_t *) event_args;
      LogInfo(" Cb informed app that Wi-Fi %" PRIu32 " bytes available on socket %" PRIu32 ".\n",
              p_param_app_net_cb->available_data_length, p_param_app_net_cb->socket_id);
      break;

    default:
      break;
  }
  /* USER CODE BEGIN APP_net_cb_End */

  /* USER CODE END APP_net_cb_End */
}

static void APP_mqtt_cb(W6X_event_id_t event_id, void *event_args)
{
  /* Careful: The callback is called by different tasks (ATD_RxPooling_task and ATD_EventsPooling_task)
              depending on the events type. It is suggested to make this callback re-entrant
              or in any case not sharing the same global variables between these events.
              Otherwise to set the two tasks at same priority to avoid pre-emption. */

  /* USER CODE BEGIN APP_mqtt_cb_1 */

  /* USER CODE END APP_mqtt_cb_1 */
  W6X_MQTT_CbParamData_t *p_param_mqtt_data = (W6X_MQTT_CbParamData_t *) event_args;
  APP_MQTT_Data_t mqtt_data;
  switch (event_id)
  {
    case W6X_MQTT_EVT_CONNECTED_ID:
      LogInfo("MQTT Connected\n");
      break;

    case W6X_MQTT_EVT_DISCONNECTED_ID:
      LogInfo("MQTT Disconnected\n");
      break;

    case W6X_MQTT_EVT_SUBSCRIPTION_RECEIVED_ID:
      /* Get the received topic length */
      mqtt_data.topic_length = p_param_mqtt_data->topic_length + 1;

      /* Allocate a memory buffer to store the topic string in the sub_msg_queue */
      mqtt_data.topic = pvPortMalloc(mqtt_data.topic_length);

      /* Copy the received topic in allocated buffer */
      snprintf((char *)mqtt_data.topic, mqtt_data.topic_length, "%s", mqtt_recv_data.p_recv_data);

      /* Get the received message length */
      mqtt_data.message_length = p_param_mqtt_data->message_length + 1;
      /* Allocate a memory buffer to store the message string in the sub_msg_queue */
      mqtt_data.message = pvPortMalloc(mqtt_data.message_length);

      /* Copy the received message in allocated buffer */
      snprintf((char *)mqtt_data.message, mqtt_data.message_length, "%s",
               mqtt_recv_data.p_recv_data + mqtt_data.topic_length);

      /* Push the new mqtt_data into the sub_msg_queue */
      xQueueSendToBack(sub_msg_queue, &mqtt_data, 0);
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

static void LowPowerManagerInit(void)
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
  UTIL_LPM_SetOffMode((1 << CFG_LPM_APPLI_ID), UTIL_LPM_DISABLE);
#endif /* LOW_POWER_MODE */
#if (LOW_POWER_MODE < LOW_POWER_STOP_ENABLE)
  /* Disable Stop Mode */
  UTIL_LPM_SetStopMode((1 << CFG_LPM_APPLI_ID), UTIL_LPM_DISABLE);
#endif /* LOW_POWER_MODE */
#endif /* LOW_POWER_MODE */

  /* USER CODE BEGIN LowPowerManagerInit_End */

  /* USER CODE END LowPowerManagerInit_End */
}

/* USER CODE BEGIN PFD */

/* USER CODE END PFD */
