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
#include "lwip.h"
#include <lwip/errno.h>
#include <netdb.h>

#include "mqtt.h"

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
#include "stm32h5xx_nucleo_errno.h"

/* USER CODE END Includes */

/* Global variables ----------------------------------------------------------*/
/** RTC handle */
extern RTC_HandleTypeDef hrtc;

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
/** Scan done event bitmask */
#define EVENT_FLAG_SCAN_DONE            (1<<1)

/** Delay before to declare the scan in failure */
#define WIFI_SCAN_TIMEOUT               10000

/** Priority of the subscription process task */
#define SUBSCRIPTION_THREAD_PRIO        24

/** Stack size of the subscription process task */
#define SUBSCRIPTION_TASK_STACK_SIZE    1024

#ifndef SNTP_TIMEZONE
/** SNTP timezone configuration */
#define SNTP_TIMEZONE                   1
#endif /* SNTP_TIMEZONE */

#ifndef MQTT_TOPIC_BUFFER_SIZE
/** Subscribed topic max buffer size */
#define MQTT_TOPIC_BUFFER_SIZE          100
#endif /* MQTT_TOPIC_BUFFER_SIZE */

#ifndef MQTT_MSG_BUFFER_SIZE
/** Subscribed message max buffer size */
#define MQTT_MSG_BUFFER_SIZE            600
#endif /* MQTT_MSG_BUFFER_SIZE */

#ifndef MQTT_HOST_NAME
/** Host name of remote MQTT Broker
  * Multiple options are possibles:
  *   - broker.hivemq.com
  *   - broker.emqx.io
  *   - test.mosquitto.org
  */
#define MQTT_HOST_NAME                  "broker.emqx.io"
#endif /* MQTT_HOST_NAME */

#ifndef MQTT_HOST_PORT
/** Port of remote MQTT Broker */
#define MQTT_HOST_PORT                  1883
#endif /* MQTT_HOST_PORT */

#ifndef MQTT_SECURITY_LEVEL
/** Security level
  * 0: No security (TCP connection)
  * only non-secure is currently available */
#define MQTT_SECURITY_LEVEL             0
#endif /* MQTT_SECURITY_LEVEL */

#ifndef MQTT_CLIENT_ID
/** MQTT Client ID to be identified on MQTT Broker */
#define MQTT_CLIENT_ID                  "mySTM32_772"
#endif /* MQTT_CLIENT_ID */

#ifndef MQTT_USERNAME
/** MQTT Username to be identified on MQTT Broker. Required when the scheme is greater or equal to 1 */
#define MQTT_USERNAME                   "user"
#endif /* MQTT_USERNAME */

#ifndef MQTT_USER_PASSWORD
/** MQTT Password to be identified on MQTT Broker. Required when the scheme is greater or equal to 1 */
#define MQTT_USER_PASSWORD              "password"
#endif /* MQTT_USER_PASSWORD */

#ifndef MQTT_CERTIFICATE
/** MQTT Client Certificate. Required when the scheme is greater or equal to 3 */
#define MQTT_CERTIFICATE                "client_1.crt"
#endif /* MQTT_CERTIFICATE */

#ifndef MQTT_KEY
/** MQTTClient Private key. Required when the scheme is greater or equal to 3 */
#define MQTT_KEY                        "client_1.key"
#endif /* MQTT_KEY */

#ifndef MQTT_CA_CERTIFICATE
/** MQTT Client CA certificate. Required when the scheme is greater or equal to 2 */
#define MQTT_CA_CERTIFICATE             "ca_1.crt"
#endif /* MQTT_CA_CERTIFICATE */

#ifndef MQTT_SNI_ENABLED
/** MQTT Server Name Indication (SNI) enabled */
#define MQTT_SNI_ENABLED                1
#endif /* MQTT_SNI_ENABLED */

#ifndef MQTT_KEEP_ALIVE
/** Keep Alive interval using MQTT ping. Range [0, 7200]. 0 is forced to 120 */
#define MQTT_KEEP_ALIVE                 120
#endif /* MQTT_KEEP_ALIVE */

#ifndef MQTT_DIS_CLEAN_SESSION
/** Skip cleaning the MQTT session */
#define MQTT_DIS_CLEAN_SESSION          0
#endif /* MQTT_DIS_CLEAN_SESSION */

#ifndef MQTT_PUBLISH_QOS
/** Publish QoS. 0: At most once, 1: At least once, 2: Exactly once */
#define MQTT_PUBLISH_QOS                0
#endif /* MQTT_PUBLISH_QOS */

#ifndef MQTT_PUBLISH_RETAIN
/** Publish Retain flag */
#define MQTT_PUBLISH_RETAIN             0
#endif /* MQTT_PUBLISH_RETAIN */

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
/** MQTT buffer to publish message to the ST67W6X Driver
  * sendbuf should be large enough to hold multiple whole mqtt messages */
static uint8_t sendbuf[2048];

/** MQTT buffer to receive subscribed message from the ST67W6X Driver
  * recvbuf should be large enough any whole mqtt message expected to be received */
static uint8_t recvbuf[1024];

/** Event bitmask flag used when Wi-Fi scan done */
static EventGroupHandle_t scan_event_flags = NULL;

/** MQTT buffer used to define the topic string of the subscription or publish */
static uint8_t mqtt_topic[MQTT_TOPIC_BUFFER_SIZE];

/** MQTT published message buffer */
static uint8_t mqtt_pubmsg[MQTT_MSG_BUFFER_SIZE];

/** MQTT Broker connection configuration */
static W6X_MQTT_Connect_t mqtt_config =
{
  .HostName = MQTT_HOST_NAME,         /*!< Host name of remote MQTT Broker */
  .HostPort = MQTT_HOST_PORT,         /*!< Port of remote MQTT Broker */
  .MQClientId = MQTT_CLIENT_ID,       /*!< MQTT Client ID to be identified on MQTT Broker */
  /** Security level
    * 0: No security (TCP connection)
    * only non-secure is currently available */
  .Scheme = MQTT_SECURITY_LEVEL,
  /** MQTT Username to be identified on MQTT Broker
    * Required when the scheme is greater or equal to 1 */
  .MQUserName = MQTT_USERNAME,
  /** MQTT Password to be identified on MQTT Broker
    * Required when the scheme is greater or equal to 1 */
  .MQUserPwd = MQTT_USER_PASSWORD,
  /** CA certificate
    * Required when the scheme is greater or equal to 2 */
  .CACertificateName = MQTT_CA_CERTIFICATE,
  /** Client Certificate
    * Required when the scheme is greater or equal to 3 */
  .CertificateName = MQTT_CERTIFICATE,
  /** Client Private key
    * Required when the scheme is greater or equal to 3 */
  .PrivateKeyName = MQTT_KEY,
  /** Server Name Indication (SNI) enabled */
  .SNI_enabled = MQTT_SNI_ENABLED,
  /** Keep Alive interval using MQTT ping. Range [0, 7200]. 0 is forced to 120 */
  .KeepAlive = MQTT_KEEP_ALIVE,
  /** Skip cleaning the MQTT session */
  .DisableCleanSession = MQTT_DIS_CLEAN_SESSION,
  /** Last Will and Testament (LWT) topic */
  .WillTopic = "",
  /** LWT message */
  .WillMessage = "",
  /** LWT QoS. Range [0, 2] */
  .WillQos = 0,
  /** LWT Retain flag */
  .WillRetain = 0
};

/** Subscribed message Queue Handle */
static QueueHandle_t sub_msg_queue;

/** Subscribed message process Task Handle */
static TaskHandle_t  sub_task_handle;

/** Green led status */
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
  * @brief  MQTT publish callback
  * @param  unused: Not used
  * @param  published: Published message structure
  */
static void publish_callback_1(void **unused, struct mqtt_response_publish *published);

/**
  * @brief  MQTT client refresher
  * @param  client: MQTT client structure
  */
static void client_refresher(void *client);

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
  W6X_WiFi_Connect_t connectData = {0};
  W6X_WiFi_StaStateType_e state = W6X_WIFI_STATE_STA_OFF;
  uint8_t Mac[6] = {0};
  const char *hostname = MQTT_HOST_NAME;
  const char *port = MSTR(MQTT_HOST_PORT);
  struct addrinfo hints = {0};
  int32_t sockfd = -1;
  int32_t rv;
  struct addrinfo *p;
  struct addrinfo *servinfo;
  struct custom_socket_handle handle;
  struct mqtt_client client;
  /* Create an identified session (not mandatory) */
  const char *client_id = MQTT_CLIENT_ID;
  /* Ensure we have a clean session */
  uint8_t connect_flags = MQTT_CONNECT_CLEAN_SESSION;

  /* USER CODE BEGIN main_app_2 */
  /* Sensor variables */
  Sys_Sensors_Data_t sensors_data;
  bool sensor_initialized = false;

  /* USER CODE END main_app_2 */

  /* Time variables */
  RTC_TimeTypeDef time = {0};
  RTC_DateTypeDef date = {0};

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

  /* Initialize the LWIP stack */
  ret = MX_LWIP_Init();
  if (ret)
  {
    LogError("failed to initialize LWIP stack %" PRIi32 "\n", ret);
    goto _err;
  }

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
  if (W6X_WiFi_Station_GetState(&state, &connectData) != W6X_STATUS_OK)
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

  /* Wait a moment to receive the IP Address from Access Point */
  vTaskDelay(5000);

  ret = W6X_WiFi_Station_GetMACAddress(Mac);
  if (ret)
  {
    LogError("failed to get the MAC Address, %" PRIi32 "\n", ret);
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

  hints.ai_family = AF_UNSPEC; /* IPv4 or IPv6 */
  hints.ai_socktype = SOCK_STREAM; /* Must be TCP */

  /* get address information */
  rv = getaddrinfo(hostname, port, &hints, &servinfo);
  if (rv != 0)
  {
    LogError("Failed to open socket (getaddrinfo): %d\n", rv);
    goto _err;
  }

  /* open the first possible socket */
  for (p = servinfo; p != NULL; p = p->ai_next)
  {
    sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
    if (sockfd == -1) { continue; }

    /* connect to server */
    rv = connect(sockfd, p->ai_addr, p->ai_addrlen);
    if (rv == -1)
    {
      close(sockfd);
      sockfd = -1;
      continue;
    }
    break;
  }

  /* free servinfo */
  freeaddrinfo(servinfo);

  /* make non-blocking */
  if (sockfd != -1)
  {
    int32_t iMode = 1;
    ioctlsocket(sockfd, FIONBIO, &iMode);
  }

  if (sockfd < 0)
  {
    LogError("Failed to open socket: %d\r", sockfd);
    goto _err;
  }

  handle.type = MQTTC_PAL_CONNTION_TYPE_TCP;
  handle.ctx.fd = sockfd;

  /* setup a client */
  mqtt_init(&client, &handle, sendbuf, sizeof(sendbuf), recvbuf, sizeof(recvbuf), publish_callback_1);

  /* Send connection request to the broker. */
  if (MQTT_OK == mqtt_connect(&client, client_id, NULL, NULL, 0, NULL, NULL, connect_flags, 400))
  {
    /* check that we don't have any errors */
    if (client.error != MQTT_OK)
    {
      LogError("error: %s\r", mqtt_error_str(client.error));
      goto _err;
    }

    LogInfo("MQTT Connect successful\n");
  }
  else
  {
    LogError("MQTT Connect Failure\n");
    goto _err;
  }

  /* start a thread to refresh the client (handle egress and ingree client traffic) */
  xTaskCreate(client_refresher, (char *)"client_ref", 1024,  &client, 10, NULL);

  /* Add a new task to process the received message of subscribed topics */
  sub_msg_queue = xQueueCreate(10, sizeof(APP_MQTT_Data_t));
  xTaskCreate(Subscription_process_task, (char *)"mqtt_sub",
              SUBSCRIPTION_TASK_STACK_SIZE >> 2, NULL, SUBSCRIPTION_THREAD_PRIO, &sub_task_handle);

  /* Subscribe to a control topic with topic_level based on ClientID */
  snprintf((char *)mqtt_topic, MQTT_TOPIC_BUFFER_SIZE, "/devices/%s/control", mqtt_config.MQClientId);
  LogInfo("Subscribing to topic %s.\n", mqtt_topic);
  mqtt_subscribe(&client, (char *)mqtt_topic, 0);
  memset(mqtt_topic, 0, sizeof(mqtt_topic));

  /* Subscribe to a sensor topic with topic_level based on ClientID */
  snprintf((char *)mqtt_topic, MQTT_TOPIC_BUFFER_SIZE, "/sensors/%s", mqtt_config.MQClientId);
  LogInfo("Subscribing to topic %s.\n", mqtt_topic);
  mqtt_subscribe(&client, (char *)mqtt_topic, 0);

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
    W6X_WiFi_Station_GetState(&State, &ConnectData);

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
    ret = mqtt_publish(&client, (char *)mqtt_topic, (char *)mqtt_pubmsg,
                       strlen((char *)mqtt_pubmsg) + 1, MQTT_PUBLISH_QOS_0);
    if (ret == MQTT_OK)
    {
      LogInfo("MQTT Publish OK\n");
    }
    vTaskDelay(2000);
  }
#if (TEST_AUTOMATION_ENABLE == 1)
  while ((ret == W6X_STATUS_OK) && (subscription_received == false));
#else
  while (ret == MQTT_OK);
#endif /* TEST_AUTOMATION_ENABLE */

  if (ret != MQTT_OK)
  {
    LogError("MQTT Failure\n");
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
  /* De-initialize the ST67W6X Wi-Fi module */
  W6X_WiFi_DeInit();

  /* De-initialize the ST67W6X Driver */
  W6X_DeInit();

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

static void publish_callback_1(void **unused, struct mqtt_response_publish *published)
{
  APP_MQTT_Data_t mqtt_data;

  /* Get the received topic length */
  mqtt_data.topic_length = published->topic_name_size + 1;

  /* Allocate a memory buffer to store the topic string in the sub_msg_queue */
  mqtt_data.topic = pvPortMalloc(mqtt_data.topic_length);

  /* Copy the received topic in allocated buffer */
  memcpy(mqtt_data.topic, published->topic_name, published->topic_name_size);
  mqtt_data.topic[published->topic_name_size] = '\0';

  /* Get the received message length */
  mqtt_data.message_length = published->application_message_size + 1;

  /* Allocate a memory buffer to store the message string in the sub_msg_queue */
  mqtt_data.message = pvPortMalloc(mqtt_data.message_length);

  /* Copy the received message in allocated buffer */
  memcpy(mqtt_data.message, published->application_message, published->application_message_size);
  mqtt_data.message[published->application_message_size] = '\0';

  /* Push the new mqtt_data into the sub_msg_queue */
  xQueueSendToBack(sub_msg_queue, &mqtt_data, 0);
}

static void client_refresher(void *client)
{
  while (1)
  {
    mqtt_sync((struct mqtt_client *) client);
    vTaskDelay(100);
  }

}

/* USER CODE BEGIN PFD */

/* USER CODE END PFD */
