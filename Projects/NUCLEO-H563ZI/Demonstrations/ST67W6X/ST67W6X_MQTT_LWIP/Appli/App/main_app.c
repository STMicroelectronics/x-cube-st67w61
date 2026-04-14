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
#include <time.h>

/* Application */
#include "main.h"
#include "main_app.h"
#include "app_config.h"
#include "lwip.h"
#include <lwip/errno.h>
#include <netdb.h>
#include "dns.h"
#include "altls_mbedtls.h"
#include "mqtt.h"
#include "sntp.h"

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

#if (TEST_AUTOMATION_ENABLE == 1)
#include "util_mem_perf.h"
#include "util_task_perf.h"
#endif /* TEST_AUTOMATION_ENABLE */

/* USER CODE BEGIN Includes */
#include "sys_sensors.h"
#include "stm32h5xx_nucleo_errno.h"

/* USER CODE END Includes */

/* Global variables ----------------------------------------------------------*/
/** RTC handle */
/* coverity[misra_c_2012_rule_8_5_violation : FALSE] */
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

/**
  * @brief  DNS resolution structure
  */
typedef struct
{
  int32_t res;                    /*!< Result of the resolution */
  ip_addr_t ipaddr;               /*!< IP Address */
} APP_DNS_res_t;

/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private defines -----------------------------------------------------------*/
/** Scan done event bitmask */
#define EVENT_FLAG_SCAN_DONE            (1UL << 1U)

/** DNS done event bitmask */
#define EVENT_FLAG_DNS_DONE             (1UL << 1U)

/** DNS resolve timeout in milliseconds */
#define DNS_RESOLVE_TIMEOUT_MS          5000

/** Delay before to declare the scan in failure */
#define WIFI_SCAN_TIMEOUT               10000

/** Priority of the subscription process task */
#define SUBSCRIPTION_THREAD_PRIO        24

/** Stack size of the subscription process task */
#define SUBSCRIPTION_TASK_STACK_SIZE    1024U

/** Priority of the refresher process task */
#define REFRESHER_THREAD_PRIO           10

/** Stack size of the refresher process task */
#define REFRESHER_TASK_STACK_SIZE       4096U

#ifndef SNTP_TIMEZONE
/** SNTP timezone configuration */
#define SNTP_TIMEZONE                   1
#endif /* SNTP_TIMEZONE */

#ifndef MQTT_TOPIC_BUFFER_SIZE
/** Subscribed topic max buffer size */
#define MQTT_TOPIC_BUFFER_SIZE          100U
#endif /* MQTT_TOPIC_BUFFER_SIZE */

#ifndef MQTT_MSG_BUFFER_SIZE
/** Subscribed message max buffer size */
#define MQTT_MSG_BUFFER_SIZE            600U
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
  * 1: SSL with Username/Password authentication
  * 2: SSL with Server certificate (CACertificate)
  * 3: SSL with Client certificate (Certificate and PrivateKey)
  * 4: SSL with both certificates (CACertificate, Certificate, PrivateKey) */
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
#define MQTT_CERTIFICATE                "client_mqtt.crt"
#endif /* MQTT_CERTIFICATE */

#ifndef MQTT_KEY
/** MQTTClient Private key. Required when the scheme is greater or equal to 3 */
#define MQTT_KEY                        "client_mqtt.key"
#endif /* MQTT_KEY */

#ifndef MQTT_CA_CERTIFICATE
/** MQTT Client CA certificate. Required when the scheme is greater or equal to 2 */
#define MQTT_CA_CERTIFICATE             "ca_mqtt.crt"
#endif /* MQTT_CA_CERTIFICATE */

#ifndef MQTT_SNI
/** MQTT Server Name Indication (SNI) */
#define MQTT_SNI                        "server.local"
#endif /* MQTT_SNI */

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

/** Event bitmask flag used when DNS done */
static EventGroupHandle_t dns_event_flags = NULL;

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
    * 1: SSL with Username/Password authentication
    * 2: SSL with Server certificate (CACertificate)
    * 3: SSL with Client certificate (Certificate and PrivateKey)
    * 4: SSL with both certificates (CACertificate, Certificate, PrivateKey) */
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
  /** Server Name Indication (SNI) */
  .SNI = MQTT_SNI,
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
#if (LFS_ENABLE == 0)
/** Certificate Authority (CA) content */
static const char ca_certificate[] =
  "-----BEGIN CERTIFICATE-----\r\n"
  "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\r\n"
  "Qw==\r\n"
  "-----END CERTIFICATE-----\r\n";

/** Client Certificate content */
static const char client_certificate[] =
  "-----BEGIN CERTIFICATE-----\r\n"
  "MIIEkTCCAnmgAwIBAgIUd2CXKip9OHFjOWnIoM7yJuNg2EYwDQYJKoZIhvcNAQEL\r\n"
  "BQAwXTELMAkGA1UEBhMCRlIxDjAMBgNVBAcMBVBhcmlzMRswGQYDVQQKDBJTVE1p\r\n"
  "Y3JvZWxlY3Ryb25pY3MxDzANBgNVBAsMBlJvb3RDQTEQMA4GA1UEAwwHUm9vdCBD\r\n"
  "QTAgFw0yNjAyMjcxMjUyMzNaGA8yMDU2MDQxMDEyNTIzM1owYjELMAkGA1UEBhMC\r\n"
  "RlIxDjAMBgNVBAcMBVBhcmlzMRswGQYDVQQKDBJTVE1pY3JvZWxlY3Ryb25pY3Mx\r\n"
  "DzANBgNVBAsMBkNsaWVudDEVMBMGA1UEAwwMY2xpZW50LmxvY2FsMIIBIjANBgkq\r\n"
  "hkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAlfoN+NpkCNoAmqwn6CmJORyOmSzbEnFK\r\n"
  "5qlLzJzAWKYfgHQvBoW3X5JL4pBVJTPRt0JvIJxnU77I4rtQcsBcN5JK6k/L6z71\r\n"
  "P/Ikc4UDRGRPkLcRwJKf+msUxKRGhIYN3CE55feUOCB3LF/a426pXEeBJELbxEA6\r\n"
  "an5pxf3emNM22Vvz3AJ23bv2qRu8Rc7fQ71lZSgwiwbzax+qshJMJ3gu0YCsxml+\r\n"
  "5sZqFHghRPfV0SXFDBa2jAmg/mRP3dBm2uUhtniTftGdfG9V4gH2N5hPFXWZY4rz\r\n"
  "Y2gjld2OTMNHBKhZ2YIFNam4VXRGzrWqA1sBf2GnHUM+2I+nQLofXQIDAQABo0Iw\r\n"
  "QDAdBgNVHQ4EFgQUG2+/+goza2iQYjK/uFlS5Ehnx/MwHwYDVR0jBBgwFoAUE5/a\r\n"
  "8NV6E1Aurf4eZxtzQbgu56EwDQYJKoZIhvcNAQELBQADggIBAAs7e8ioaDIOvKtF\r\n"
  "uYcBxl+E7kD4tUn1GCEWQSRxavYl97IjtfabRRskDJwKzB07UdCnkpviTyeJ7Ga2\r\n"
  "anJNevhBRbbSMLPIjMw01RJhVcAxa13g8K63PpTZy4WtkBaMNQHzlQzPEpn9Z/Ip\r\n"
  "vB4TgdSrFOoWUdl+a3xoRlGXF14r7c+kzLS7/N1x0X9D6lQcNqw1oTtvfMgZUBX1\r\n"
  "F31hY3nYrP0+etkr8nZtvYBysByXCn/wcIb8ARr4qj39c45zKnOC4hhqLh40QUfA\r\n"
  "rK6BU6TFzJUpG9Br5ku1czpwR+ZRF+y+yoq7Gk4k68z0lZoUQ0dNXZmbWqIoxHfK\r\n"
  "PbJQSDzdn+6eE/pQHl0OLKIRnaqyhcteqj/+ixluvXQOOtlUpFizf3pVEUY6x5LZ\r\n"
  "DRDqd/4jI0Uwk/TCpt0HZUH8JsUD7KW1m46WOqVkR84d5143/kqjgcBLtC/zhpkK\r\n"
  "VzCPS9tj7r6fEzwBZArP+4by+DFX4rYzjBaeDeZ7p0r631vwcfY2NxECk7/UlmU0\r\n"
  "c89N4kHjwwtt6X3d98ZDTp+iI3rMSaOxGUZ8wGF5CJnRVeC4+sGLmTxaVdeV2aZR\r\n"
  "YICEIk+3p2uiGjvfp9i5wA6iBPASsyu3UAiTgKviednCuUgZy3G/36zCCesWbxKh\r\n"
  "ryVA6qIrAmgq/YPIeQ6xZNyVZIMV\r\n"
  "-----END CERTIFICATE-----\r\n";

/** Client Private Key content */
static const char client_key[] =
  "-----BEGIN PRIVATE KEY-----\r\n"
  "MIIEvQIBADANBgkqhkiG9w0BAQEFAASCBKcwggSjAgEAAoIBAQCV+g342mQI2gCa\r\n"
  "rCfoKYk5HI6ZLNsScUrmqUvMnMBYph+AdC8GhbdfkkvikFUlM9G3Qm8gnGdTvsji\r\n"
  "u1BywFw3kkrqT8vrPvU/8iRzhQNEZE+QtxHAkp/6axTEpEaEhg3cITnl95Q4IHcs\r\n"
  "X9rjbqlcR4EkQtvEQDpqfmnF/d6Y0zbZW/PcAnbdu/apG7xFzt9DvWVlKDCLBvNr\r\n"
  "H6qyEkwneC7RgKzGaX7mxmoUeCFE99XRJcUMFraMCaD+ZE/d0Gba5SG2eJN+0Z18\r\n"
  "b1XiAfY3mE8VdZljivNjaCOV3Y5Mw0cEqFnZggU1qbhVdEbOtaoDWwF/YacdQz7Y\r\n"
  "j6dAuh9dAgMBAAECggEAPP4LBZvnV9Q0r7J4rkmKFXBgK7oaw8bQQ7sw6N8MuGCi\r\n"
  "6g4V+8yQlSz9cH/rKKyIysMZR4Vj3iJ2NwMfhfNl7XGwxta54wthGObkXRiIih1T\r\n"
  "YFKbRRo8Nk6rDQeT6BxOcaoPjk8f9614WdMHxTuBY9Zuli0cjBTkzN9pK8yBZNvN\r\n"
  "JXHxgDb/GTQ4ZWEZhh0fBNwXMF9oMbEn59K7Hh6XzsM85EV4M60LvvSiD/m94Oys\r\n"
  "3C6dFL7UaDJ2rmThLlItrCRzE+/2FgbjCAmEv2X2NXDzNEenZoCF9X1ie3v6caIT\r\n"
  "KdNyIZs5Vsaa4eWQdZYDthQUZjbNuK73CgxjqdJ0bwKBgQDSop6hjamHxUXpsaRv\r\n"
  "wHlDz7TXbYx/eyeCLtQESnKedYoSSWPw6tArMlLfQkFiuFh5lXIis08CvRYDf4Ns\r\n"
  "G/eglXO/iIF2lMm+bLEn5MGA+EwpuLHw7fvTpP6udLmElFa6hbmUyj/XgsZlIPA9\r\n"
  "Bl3UiLQi0s1/pNcb0ubZCdgPvwKBgQC2Rwdn+7W9gd1N9qPX79WagxuJ4DRUm21C\r\n"
  "vz2Ve2YMb7UR7+p5sktrtUEolXuPvfahJ+9FDIpaZigJRp2ySzcrMGHufLeW4UB/\r\n"
  "ZnPVWqtEBaQrpshWu5JHlycQAhjzFveEc9expHN/Fm5WkSDxma1NU2016hot+lz+\r\n"
  "Ajsm0cYX4wKBgQCwehmIZ8V7gLhDxVdtXgj73MG6oQlPIeMHOq7ebXW89+PX0G+Q\r\n"
  "wVvqZT5z2fIogSV3sNOw6SSwubYA9kwpPwFpJO6WsgsuTBj/l9eSAiJyKRa++gT0\r\n"
  "RKByQdI0Xo203AgSPMoxNIbqzKHmxwMhTf09fc/XQWF1qamkoT5S5+GDxwKBgDsz\r\n"
  "2r31TVQd5+k4oIK0TSaASuN/RL/uM5CoWLJCgCSt65vF1txsAn8bQeySkK1hP8ec\r\n"
  "FuTQa+dsorhQjUupjmOitUwmieKhirdWaWz0pAfV5TqgUxWImrxR5cgXRk8+OGp2\r\n"
  "zanPBgxTFsdbH94Y0eb5n9ERFiu005tU0i2LmNGNAoGARw3oORmknMdOOuDTtOl8\r\n"
  "yiIMzm6lmSIHI0hpJPPrVz1zzJY6CWwf9rvcGy3Rp+RpoYYgpgMFpfUeeH5zB7L7\r\n"
  "2d+Irfu5wuoWp5ImmAveFIHAdR9rqbzjz3M+/EEg/OKRUKSGHnfdExkhhZFKpJ+B\r\n"
  "ItceYk1hkNpYvnACWDpoIAc=\r\n"
  "-----END PRIVATE KEY-----\r\n";
#endif /* LFS_ENABLE */

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

/**
  * @brief  DNS lookup callback
  * @param  name: Hostname
  * @param  ipaddr: Resolved IP address
  * @param  arg: User argument
  */
static void dns_lookup_callback(const char *name, const ip_addr_t *ipaddr, void *arg);

/**
  * @brief  DNS lookup callback
  * @param  ip: IP address to connect
  * @param  port: Port to connect
  * @return Socket file descriptor on success, -1 on failure
  */
static int32_t tcp_client_connect(ip4_addr_t *ip, int32_t port);

/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Functions Definition ------------------------------------------------------*/
void main_app(void)
{
  W6X_Status_t ret;
  enum MQTTErrors mqtt_ret;
  EventBits_t eventBits = 0;

  /* USER CODE BEGIN main_app_1 */

  /* USER CODE END main_app_1 */

#if (TEST_AUTOMATION_ENABLE == 1)
  /* Start the task performance measurement */
  task_perf_start();
#endif /* TEST_AUTOMATION_ENABLE */

  /* Wi-Fi variables */
  W6X_WiFi_Scan_Opts_t Opts = {0};
  W6X_WiFi_Connect_Opts_t ConnectOpts = {0};
  W6X_WiFi_Connect_t connectData = {0};
  W6X_WiFi_StaStateType_e state = W6X_WIFI_STATE_STA_OFF;
  uint8_t Mac[6] = {0};
  const char *hostname = MQTT_HOST_NAME;
  int32_t port = MQTT_HOST_PORT;
  int32_t sockfd;
  struct custom_socket_handle handle;
  struct mqtt_client client = {0};
  /* Ensure we have a clean session */
  uint8_t connect_flags = MQTT_CONNECT_CLEAN_SESSION;
  ip_addr_t resolved_ip = {0};

  /* USER CODE BEGIN main_app_2 */
  /* Sensor variables */
  Sys_Sensors_Data_t sensors_data;
  bool sensor_initialized = false;

  /* USER CODE END main_app_2 */

  /* Time variables */
  RTC_TimeTypeDef rtc_time = {0};
  RTC_DateTypeDef rtc_date = {0};

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

  /* USER CODE BEGIN main_app_3 */

  /* USER CODE END main_app_3 */

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

  /* USER CODE BEGIN main_app_4 */

  /* USER CODE END main_app_4 */
  /* Run a Wi-Fi scan to retrieve the list of all nearby Access Points */
  scan_event_flags = xEventGroupCreate();

  dns_event_flags = xEventGroupCreate();

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

  /* Wait a moment to receive the IP Address from Access Point */
  vTaskDelay(pdMS_TO_TICKS(5000));

  if (sntp_init(SNTP_TIMEZONE) != 0)
  {
    LogError("SNTP Time Failure\n");
    goto _err;
  }

  ret = W6X_WiFi_Station_GetMACAddress(Mac);
  if (ret != W6X_STATUS_OK)
  {
    LogError("Failed to get the MAC Address, %" PRIi32 "\n", ret);
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

  {
    /* get address information */
    APP_DNS_res_t dns_res;
    dns_res.res = -1;
    int32_t err = dns_gethostbyname(hostname, &resolved_ip, dns_lookup_callback, &dns_res);
    if (err == ERR_OK)
    {
    }
    else if (err == ERR_INPROGRESS)
    {
      eventBits = xEventGroupWaitBits(dns_event_flags, EVENT_FLAG_DNS_DONE, pdTRUE, pdFALSE,
                                      pdMS_TO_TICKS(DNS_RESOLVE_TIMEOUT_MS));
      if ((eventBits & EVENT_FLAG_DNS_DONE) == 0U)
      {
        LogError("DNS Lookup timed out\n");
        goto _err;
      }
      if (dns_res.res != 0)
      {
        LogError("DNS Lookup resolution failed\n");
        goto _err;
      }
      resolved_ip = dns_res.ipaddr;
    }
    else
    {
      LogError("Failed to resolve hostname:%s", hostname);
      goto _err;
    }
  }

  sockfd = tcp_client_connect(&(resolved_ip.u_addr.ip4), port);
  handle.type = MQTTC_PAL_CONNTION_TYPE_TCP;
  handle.ctx.fd = sockfd;

  if (mqtt_config.Scheme != 0U)
  {
    char *sni = NULL;
    ssl_conn_param_t params = {0};
    ssl_config_ctx_t *ssl_param;

    if ((mqtt_config.Scheme == 2U) || (mqtt_config.Scheme == 4U))
    {
      params.ca_cert = (char *)ca_certificate;
      params.ca_cert_len = sizeof(ca_certificate);
    }
    if (mqtt_config.Scheme >= 3U)
    {
      params.own_cert = (char *)client_certificate;
      params.own_cert_len = sizeof(client_certificate);
      params.private_cert = (char *)client_key;
      params.private_cert_len = sizeof(client_key);
    }

    if (mqtt_config.SNI[0] != 0)
    {
      sni = (char *)mqtt_config.SNI;
    }
    ssl_param = ssl_configure(&params, 1);
    if (ssl_param == NULL)
    {
      goto _err;
    }
    ssl_conn_t *conn_param = ssl_secure_connection(sockfd, ssl_param, 1, sni);
    if (conn_param == NULL)
    {
      goto _err;
    }
    handle.type = MQTTC_PAL_CONNTION_TYPE_TLS;
    handle.ctx.ssl_ctx = &(conn_param->ssl);
  }

  if (sockfd < 0)
  {
    LogError("Failed to open socket: %d\r", sockfd);
    goto _err;
  }

  /* setup a client */
  (void)mqtt_init(&client, &handle, sendbuf, sizeof(sendbuf), recvbuf, sizeof(recvbuf), publish_callback_1);

  {
    char *username = NULL;
    char *password = NULL;

    if (mqtt_config.Scheme >= 1U)
    {
      username = (char *)mqtt_config.MQUserName;
      password = (char *)mqtt_config.MQUserPwd;
    }

    /* Send connection request to the broker. */
    if (MQTT_OK == mqtt_connect(&client, (char const *)mqtt_config.MQClientId, NULL, NULL, 0U,
                                username, password, connect_flags, 400U))
    {
      /* check that we don't have any errors */
      if (client.error != MQTT_OK)
      {
        LogError("Error: %s\r", mqtt_error_str(client.error));
        goto _err;
      }

      LogInfo("MQTT Connect successful\n");
    }
    else
    {
      LogError("MQTT Connect Failure\n");
      goto _err;
    }
  }

  /* start a thread to refresh the client (handle egress and ingree client traffic) */
  (void)xTaskCreate(client_refresher, (char *)"mqtt_ref",
                    REFRESHER_TASK_STACK_SIZE >> 2U,
                    &client, REFRESHER_THREAD_PRIO, NULL);

  /* Add a new task to process the received message of subscribed topics */
  sub_msg_queue = xQueueCreate(10, sizeof(APP_MQTT_Data_t));
  (void)xTaskCreate(Subscription_process_task, (char *)"mqtt_sub",
                    SUBSCRIPTION_TASK_STACK_SIZE >> 2U,
                    NULL, SUBSCRIPTION_THREAD_PRIO, &sub_task_handle);

  /* Subscribe to a control topic with topic_level based on ClientID */
  (void)snprintf((char *)mqtt_topic, MQTT_TOPIC_BUFFER_SIZE, "/devices/%s/control", mqtt_config.MQClientId);
  LogInfo("Subscribing to topic %s.\n", mqtt_topic);
  (void)mqtt_subscribe(&client, (char *)mqtt_topic, 0);
  (void)memset(mqtt_topic, 0, sizeof(mqtt_topic));

  /* Subscribe to a sensor topic with topic_level based on ClientID */
  (void)snprintf((char *)mqtt_topic, MQTT_TOPIC_BUFFER_SIZE, "/sensors/%s", mqtt_config.MQClientId);
  LogInfo("Subscribing to topic %s.\n", mqtt_topic);
  (void)mqtt_subscribe(&client, (char *)mqtt_topic, 0);

  /* Reuse the same topic to publish message */
  do
  {
    uint32_t len = 0;
    W6X_WiFi_StaStateType_e State = W6X_WIFI_STATE_STA_NO_STARTED_CONNECTION;
    W6X_WiFi_Connect_t ConnectData = {0};

    /* Get the current date and time */
    (void)HAL_RTC_GetTime(&hrtc, &rtc_time, RTC_FORMAT_BIN);
    (void)HAL_RTC_GetDate(&hrtc, &rtc_date, RTC_FORMAT_BIN);

    /* Get the Wi-Fi station state to retrieve the RSSI value */
    (void)W6X_WiFi_Station_GetState(&State, &ConnectData);

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
                   rtc_date.Year, rtc_date.Month, rtc_date.Date, rtc_time.Hours, rtc_time.Minutes, rtc_time.Seconds,
                   MAC2STR(Mac), ConnectData.Rssi);

    /* USER CODE BEGIN main_app_6 */
    /* Append the JSON message with time and sensors values if the sensor board is correctly started */
    if (sensor_initialized)
    {
      /* Read the sensor values of the components on the MEMS expansion board */
      (void)Sys_Sensors_Read(&sensors_data);

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

    len += snprintf((char *)&mqtt_pubmsg[len], MQTT_MSG_BUFFER_SIZE - len, "\n  }\n }\n}");

    /* Prevent missing null-terminated character */
    mqtt_pubmsg[len] = 0U;

    /* Publish the message on a topic with topic_level based on ClientID */
    mqtt_ret = mqtt_publish(&client, (char *)mqtt_topic, (char *)mqtt_pubmsg,
                            len, MQTT_PUBLISH_QOS_0);
    if (mqtt_ret == MQTT_OK)
    {
      LogInfo("MQTT Publish OK\n");
    }
    vTaskDelay(pdMS_TO_TICKS(2000));
  }
#if (TEST_AUTOMATION_ENABLE == 1)
  while ((mqtt_ret == W6X_STATUS_OK) && (subscription_received == false));
#else
  while (mqtt_ret == MQTT_OK);
  LogError("MQTT Failure\n");
#endif /* TEST_AUTOMATION_ENABLE */

  /* Close the MQTT connection */
  mqtt_ret = mqtt_disconnect(&client);
  if (mqtt_ret == MQTT_OK)
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
  /* De-initialize the ST67W6X Wi-Fi module */
  W6X_WiFi_DeInit();

  /* De-initialize the ST67W6X Driver */
  W6X_DeInit();

  /* USER CODE BEGIN main_app_Err_2 */

  /* USER CODE END main_app_Err_2 */

#if (TEST_AUTOMATION_ENABLE == 1)
  /* Stop the task perf execution */
  task_perf_stop();

  /* Report the task performance measurement */
  task_perf_report();

  /* Report the memory performance measurement */
  mem_perf_report();
#endif /* TEST_AUTOMATION_ENABLE */

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
  BaseType_t ret;
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
    if (ret > 0)
    {
      LogInfo("MQTT Subscription Received on topic %s\n", (char *)mqtt_data.topic);

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
            vTaskDelay(pdMS_TO_TICKS(1000));
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

#if (TEST_AUTOMATION_ENABLE == 1)
      subscription_received = true;
#endif /* TEST_AUTOMATION_ENABLE */
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

static void publish_callback_1(void **unused, struct mqtt_response_publish *published)
{
  APP_MQTT_Data_t mqtt_data;

  /* Get the received topic length */
  mqtt_data.topic_length = published->topic_name_size + 1;

  /* Allocate a memory buffer to store the topic string in the sub_msg_queue */
  mqtt_data.topic = pvPortMalloc(mqtt_data.topic_length);

  /* Copy the received topic in allocated buffer */
  (void)memcpy(mqtt_data.topic, published->topic_name, published->topic_name_size);
  mqtt_data.topic[published->topic_name_size] = '\0';

  /* Get the received message length */
  mqtt_data.message_length = published->application_message_size + 1;

  /* Allocate a memory buffer to store the message string in the sub_msg_queue */
  mqtt_data.message = pvPortMalloc(mqtt_data.message_length);

  /* Copy the received message in allocated buffer */
  (void)memcpy(mqtt_data.message, published->application_message, published->application_message_size);
  mqtt_data.message[published->application_message_size] = '\0';

  /* Push the new mqtt_data into the sub_msg_queue */
  (void)xQueueSendToBack(sub_msg_queue, &mqtt_data, 0);
}

static void client_refresher(void *client)
{
  while (true)
  {
    (void)mqtt_sync((struct mqtt_client *) client);
    vTaskDelay(pdMS_TO_TICKS(100));
  }
}

static void dns_lookup_callback(const char *name, const ip_addr_t *ipaddr, void *arg)
{
  APP_DNS_res_t *dns_res = (APP_DNS_res_t *)arg;
  if (ipaddr != NULL)
  {
    dns_res->res = 0;
    dns_res->ipaddr = *ipaddr;
  }
  else
  {
    dns_res->res = -1;
  }
  (void)xEventGroupSetBits(dns_event_flags, EVENT_FLAG_DNS_DONE);
}

static int32_t tcp_client_connect(ip4_addr_t *ip, int32_t port)
{
  /* USER CODE BEGIN tcp_client_connect_1 */

  /* USER CODE END tcp_client_connect_1 */
  int32_t fd;
  int32_t res;
  struct sockaddr_in addr;

  LogInfo("tcp client connect %s:%d\r\n", ip4addr_ntoa(ip), port);

  fd = lwip_socket(AF_INET, SOCK_STREAM, 0);
  if (fd < 0)
  {
    LogError("socket create failed\r\n");
    return -2;
  }

  (void)memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_len = sizeof(addr);
  addr.sin_port = htons(port);
  addr.sin_addr.s_addr = ip4_addr_get_u32(ip);

  LogInfo("tcp_client_connect fd:%d\r\n", fd);

  int32_t on = 1;
  res = lwip_setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
  if (res != 0)
  {
    LogError("setsockopt failed, res:%d\r\n", res);
  }

  res = lwip_connect(fd, (struct sockaddr *)&addr, sizeof(addr));
  if (res < 0)
  {
    LogError("connect failed, res:%d\r\n", res);
    (void)close(fd);
    fd = -1;
  }

  /* make non-blocking */
  if (fd != -1)
  {
    int32_t iMode = 1;
    (void)ioctlsocket(fd, FIONBIO, &iMode);
  }
  /* USER CODE BEGIN tcp_client_connect_2 */

  /* USER CODE END tcp_client_connect_2 */

  return fd;
  /* USER CODE BEGIN tcp_client_connect_End */

  /* USER CODE END tcp_client_connect_End */
}

/* USER CODE BEGIN PFD */

/* USER CODE END PFD */
