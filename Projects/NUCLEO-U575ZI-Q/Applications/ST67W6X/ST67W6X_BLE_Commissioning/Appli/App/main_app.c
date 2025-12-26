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

#if (TEST_AUTOMATION_ENABLE == 1)
#include "util_mem_perf.h"
#include "util_task_perf.h"
#endif /* TEST_AUTOMATION_ENABLE */

/* USER CODE BEGIN Includes */
#include "fota_flash.h"

/* USER CODE END Includes */

/* Global variables ----------------------------------------------------------*/
/* USER CODE BEGIN GV */

/* USER CODE END GV */

/* Private typedef -----------------------------------------------------------*/
/**
  * @brief  Structure to store an Access Point information
  */
typedef struct
{
  uint8_t SSID_len;                         /*!< Service Set Identifier length */
  uint8_t Channel;                          /*!< Wi-Fi channel */
  int16_t RSSI;                             /*!< Signal strength of Wi-Fi spot */
  uint32_t Security;                        /*!< Security of Wi-Fi spot */
  uint8_t SSID[W6X_WIFI_MAX_SSID_SIZE + 1]; /*!< Service Set Identifier value. Wi-Fi spot name */
} APP_AP_t;

/**
  * @brief  BLE parameters structure
  */
typedef struct
{
  uint8_t service_idx;                      /*!< Service index */
  uint8_t charac_idx;                       /*!< Characteristic index */
  uint8_t notification_status[2];           /*!< Notification status */
  uint8_t indication_status[2];             /*!< Indication status */
  uint16_t mtu_size;                        /*!< MTU Size */
  uint32_t PassKey;                         /*!< BLE Security passkey */
  uint32_t available_data_length;           /*!< Length of the available data */
  W6X_Ble_Device_t remote_ble_device;       /*!< BLE Remote device */
} APP_Ble_Data_t;

/**
  * @brief  BLE characteristic information structure
  */
typedef struct
{
  uint8_t service_index;                    /*!< Service index */
  uint8_t char_index;                       /*!< Characteristic index */
  const char *char_uuid;                    /*!< Service UUID */
  uint8_t uuid_type;                        /*!< UUID type */
  uint8_t char_property;                    /*!< Characteristic property */
  uint8_t char_permission;                  /*!< Characteristic permission */
  const char *desc;                         /*!< Characteristic description */
} APP_Ble_Char_t;

/**
  * @brief  BLE write event structure
  */
typedef struct
{
  uint8_t conn_handle;                      /*!< Connection handle */
  uint8_t charac_idx;                       /*!< Characteristic index */
  uint32_t data_len;                        /*!< Length of the data */
  uint8_t *data;                            /*!< Data received */
} APP_Ble_WriteEvent_t;

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
/** Event when BLE is connected */
#define EVT_APP_BLE_CONNECTED                       (1<<1)
/** Event when BLE is disconnected */
#define EVT_APP_BLE_DISCONNECTED                    (1<<2)
/** Event when BLE connection parameters are updated */
#define EVT_APP_BLE_CONNECTION_PARAM_UPDATE         (1<<3)
/** Event when BLE characteristic is read */
#define EVT_APP_BLE_READ                            (1<<4)
/** Event when BLE characteristic is written */
#define EVT_APP_BLE_WRITE                           (1<<5)
/** Event when BLE service is found */
#define EVT_APP_BLE_SERVICE_FOUND                   (1<<6)
/** Event when BLE characteristic is found */
#define EVT_APP_BLE_CHAR_FOUND                      (1<<7)
/** Event when BLE indication is enabled */
#define EVT_APP_BLE_INDICATION_STATUS_ENABLED       (1<<8)
/** Event when BLE indication is disabled */
#define EVT_APP_BLE_INDICATION_STATUS_DISABLED      (1<<9)
/** Event when BLE notification is enabled */
#define EVT_APP_BLE_NOTIFICATION_STATUS_ENABLED     (1<<10)
/** Event when BLE notification is disabled */
#define EVT_APP_BLE_NOTIFICATION_STATUS_DISABLED    (1<<11)
/** Event when BLE notification data is received */
#define EVT_APP_BLE_NOTIFICATION_DATA               (1<<12)
/** Event when BLE MTU size is updated */
#define EVT_APP_BLE_MTU_SIZE                        (1<<13)
/** Event when BLE pairing failed */
#define EVT_APP_BLE_PAIRING_FAILED                  (1<<14)
/** Event when BLE pairing is completed */
#define EVT_APP_BLE_PAIRING_COMPLETED               (1<<15)
/** Event when BLE pairing confirmation is requested */
#define EVT_APP_BLE_PAIRING_CONFIRM                 (1<<16)
/** Event when BLE passkey entry is requested */
#define EVT_APP_BLE_PASSKEY_ENTRY                   (1<<17)
/** Event when BLE passkey is displayed */
#define EVT_APP_BLE_PASSKEY_DISPLAYED               (1<<18)
/** Event when BLE passkey confirmation is requested */
#define EVT_APP_BLE_PASSKEY_CONFIRM                 (1<<19)
/** Event when BLE pairing is canceled */
#define EVT_APP_BLE_PAIRING_CANCELED                (1<<20)
/** Event when Wi-Fi is connected to an Access Point */
#define EVT_APP_WIFI_CONNECTED                      (1<<21)
/** Event when Wi-Fi connection timeout */
#define EVT_APP_WIFI_CONNECTION_TIMEOUT             (1<<22)

/** Wi-Fi scan done event flag */
#define EVENT_FLAG_WIFI_SCAN_DONE                   (1<<1)

/** Application events bitmask */
#define EVT_APP_ALL_BIT         (EVT_APP_BUTTON | \
                                 EVT_APP_BLE_CONNECTED  | \
                                 EVT_APP_BLE_DISCONNECTED | \
                                 EVT_APP_BLE_CONNECTION_PARAM_UPDATE | \
                                 EVT_APP_BLE_READ | \
                                 EVT_APP_BLE_WRITE | \
                                 EVT_APP_BLE_SERVICE_FOUND | \
                                 EVT_APP_BLE_CHAR_FOUND | \
                                 EVT_APP_BLE_INDICATION_STATUS_ENABLED | \
                                 EVT_APP_BLE_INDICATION_STATUS_DISABLED | \
                                 EVT_APP_BLE_NOTIFICATION_STATUS_ENABLED | \
                                 EVT_APP_BLE_NOTIFICATION_STATUS_DISABLED | \
                                 EVT_APP_BLE_NOTIFICATION_DATA | \
                                 EVT_APP_BLE_MTU_SIZE | \
                                 EVT_APP_BLE_PAIRING_FAILED | \
                                 EVT_APP_BLE_PAIRING_COMPLETED | \
                                 EVT_APP_BLE_PAIRING_CONFIRM | \
                                 EVT_APP_BLE_PASSKEY_ENTRY | \
                                 EVT_APP_BLE_PASSKEY_DISPLAYED | \
                                 EVT_APP_BLE_PASSKEY_CONFIRM | \
                                 EVT_APP_BLE_PAIRING_CANCELED | \
                                 EVT_APP_WIFI_CONNECTED | \
                                 EVT_APP_WIFI_CONNECTION_TIMEOUT )

/** BLE Wi-Fi Commissioning Service index */
#define WIFI_COMMISSIONING_SERVICE_INDEX         0U

/** BLE Service index */
#define WIFI_CONTROL_CHAR_INDEX                  0U

/** BLE Wi-Fi Control Characteristic index */
#define WIFI_CONFIGURE_CHAR_INDEX                1U

/** BLE Wi-Fi Configure Characteristic index */
#define WIFI_AP_LIST_CHAR_INDEX                  2U

/** BLE Wi-Fi Monitoring Characteristic index */
#define WIFI_MONITORING_CHAR_INDEX               3U

/** BLE FUOTA Characteristic index */
#define FUOTA_SERVICE_INDEX                      1U

/** BLE FUOTA Base  Address Characteristic index */
#define FUOTA_BASE_ADDR_CHAR_INDEX               0U

/** BLE FUOTA Configuration Characteristic index used for indication confirmation */
#define FUOTA_CONF_CHAR_INDEX                    1U

/** BLE FUOTA Raw Data Characteristic index */
#define FUOTA_RAW_DATA_CHAR_INDEX                2U

/*
 The following 128bits UUIDs have been generated from the random UUID
 generator:
 0000ff9acc7a482a984a7f2ed5b3e58f: Service 128bits UUID
 0000fe9b8e2245419d4c21edae82ed19: Characteristic 128bits UUID
 0000fe9c8e2245419d4c21edae82ed19: Characteristic 128bits UUID
 0000fe9d8e2245419d4c21edae82ed19: Characteristic 128bits UUID
 0000fe9e8e2245419d4c21edae82ed19: Characteristic 128bits UUID
 */
/** BLE Wi-Fi Commissioning Service UUID */
#define WIFI_COMMISSIONING_SERVICE_UUID          "0000ff9acc7a482a984a7f2ed5b3e58f"
/** BLE Wi-Fi Control Characteristic UUID */
#define WIFI_CONTROL_CHAR_UUID                   "0000fe9b8e2245419d4c21edae82ed19"
/** BLE Wi-Fi Configure Characteristic UUID */
#define WIFI_CONFIGURE_CHAR_UUID                 "0000fe9c8e2245419d4c21edae82ed19"
/** BLE Wi-Fi AP List Characteristic UUID */
#define WIFI_AP_LIST_CHAR_UUID                   "0000fe9d8e2245419d4c21edae82ed19"
/** BLE Wi-Fi Monitoring Characteristic UUID */
#define WIFI_MONITORING_CHAR_UUID                "0000fe9e8e2245419d4c21edae82ed19"

/** BLE FUOTA Service UUID */
#define FUOTA_SERVICE_UUID                       "0000fe20cc7a482a984a7f2ed5b3e58f"
/** BLE FUOTA Base Address Characteristic UUID */
#define FUOTA_BASE_ADDR_CHAR_UUID                "0000fe228e2245419d4c21edae82ed19"
/** BLE FUOTA Configuration Characteristic UUID */
#define FUOTA_CONF_CHAR_UUID                     "0000fe238e2245419d4c21edae82ed19"
/** BLE FUOTA Raw Data Characteristic UUID */
#define FUOTA_RAW_DATA_CHAR_UUID                 "0000fe248e2245419d4c21edae82ed19"

/* Wi-Fi Control Characteristic first byte possible values */
#define CONTROL_ACTION_START_SCAN                0x1  /*!< Start Wi-Fi scan */
#define CONTROL_ACTION_CONNECT                   0x3  /*!< Connect to Wi-Fi */
#define CONTROL_ACTION_DISCONNECT                0x4  /*!< Disconnect from Wi-Fi */
#define CONTROL_ACTION_PING                      0x5  /*!< Ping Wi-Fi */
#if (TEST_AUTOMATION_ENABLE == 1)
#define CONTROL_ACTION_QUIT_CONTROL              0xFF /*!< Quit automation */
#endif /* TEST_AUTOMATION_ENABLE */

/* Wi-Fi Configure Characteristic first byte possible values */
#define CONFIGURE_TYPE_SSID                      0x1  /*!< Configure SSID */
#define CONFIGURE_TYPE_PWD                       0x2  /*!< Configure Password */
#define CONFIGURE_TYPE_SECURITY_FLAG             0x5  /*!< Configure Security Flag */

/* Wi-Fi Monitoring Characteristic first byte possible values */
#define MONITORING_TYPE_CONNECTING               0x3  /*!< Connecting to Wi-Fi */
#define MONITORING_TYPE_CONNECTION_DONE          0x4  /*!< Connection done */
#define MONITORING_TYPE_PING_RESPONSE            0x5  /*!< Ping response */
#define MONITORING_TYPE_ERROR                    0x6  /*!< Error */

/* Wi-Fi Monitoring Characteristic second byte possible values */
#define MONITORING_DATA_CONNECTION_TIMEOUT       0x1  /*!< Connection timeout */

/** Delay before to declare the scan in failure */
#define WIFI_SCAN_TIMEOUT                        10000

/** Number of ping requests to send */
#define WIFI_PING_COUNT                          4
/** Size of the ping request */
#define WIFI_PING_SIZE                           64
/** Time interval between two ping requests */
#define WIFI_PING_INTERVAL                       1000
/** Percent convert number */
#define WIFI_PING_PERCENT                        100

/** Generic Passkey */
#define BLE_GENERIC_PASSKEY                      123456

/** FUOTA buffer size required to be at least 512 and multiple of 16 is preferable */
#define FUOTA_BUFFER_SIZE                        2048

/** FUOTA Queue size, it is used to store the BLE FUOTA write events and process them in a different task */
#define FUOTA_QUEUE_SIZE                         16

/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macros ------------------------------------------------------------*/
#ifndef ADDR2STR
/** BD Address buffer to string macros */
#define ADDR2STR(a) (a)[0], (a)[1], (a)[2], (a)[3], (a)[4], (a)[5]
/** BD Address string format */
#define ADDRSTR "%02" PRIx16 ":%02" PRIx16 ":%02" PRIx16 ":%02" PRIx16 ":%02" PRIx16 ":%02" PRIx16
#endif /* ADDR2STR */

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
/** BLE data buffer to receive message from the ST67W6X Driver */
static uint8_t a_APP_AvailableData[247] = {0};

/** Wi-Fi Commissioning update characteristic data */
uint8_t a_APP_WifiCommUpdateCharData[247] = {0};

/** Wi-Fi connected SSID */
uint8_t a_APP_WifiConnectedSSID[W6X_WIFI_MAX_SSID_SIZE + 1] = {0};

/** Wi-Fi scan options */
W6X_WiFi_Scan_Opts_t APP_Opts = {0};

/** Wi-Fi connect options */
W6X_WiFi_Connect_Opts_t APP_ConnectOpts = {0};

/** Size of Wi-Fi Commissioning update characteristic data */
uint8_t size_of_WifiCommUpdateCharData;

/** Wi-Fi scan results */
W6X_WiFi_Scan_Result_t app_scan_results = {0};

/** Wi-Fi scan event flags */
static EventGroupHandle_t scan_event_flags = NULL;

/** Queue to handle BLE write events for FUOTA */
QueueHandle_t bleWriteQueue = NULL;

/** FUOTA buffer to store and sand back via FWU AT send
  * the data received from the BLE FUOTA raw data characteristic */
uint8_t *fuota_buffer = NULL;

/** Offset in the FUOTA buffer where the next data will be written */
uint32_t fuota_buffer_offset = 0;

/** Current address in the flash where the FUOTA data will be written */
uint32_t fuota_current_flash_addr = 0x0;

/** Advertising Data */
char a_AdvData[36] =
{
  '0', 'F', /* Manuf data length */
  'F', 'F', /* Manuf data Flag */
  '3', '0', '0', '0', /*  */
  '0', '2', /* Blue ST SDK v2  */
  '9', 'A', /* Board ID */
  'F', 'E', /* FW ID */
  '0', '0', /* FW data */
  '0', '0', /* FW data */
  '0', '0', /* FW data */
  '0', '0', /* BD Address MSB */
  '0', '0', /*  */
  '0', '0', /*  */
  '0', '0', /*  */
  '0', '0', /*  */
  '0', '0', /* BD Address LSB */
};

/** Application event group */
static EventGroupHandle_t app_evt_current;

/** BLE parameters */
APP_Ble_Data_t app_ble_params =
{
  /* Initialize remote device struct */
  .remote_ble_device.RSSI = 0,
  .remote_ble_device.IsConnected = 0,
  .remote_ble_device.conn_handle = 0,
  .remote_ble_device.DeviceName = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
  .remote_ble_device.BDAddr = {0, 0, 0, 0, 0, 0},
  .service_idx = 0,                /* Initialize service index to 0 */
  .charac_idx = 0,                 /* Initialize characteristic index to 0 */
  .notification_status = {0, 0},   /* Initialize notification status array to {0, 0} */
  .indication_status = {0, 0},     /* Initialize indication status array to {0, 0} */
  .mtu_size = 0,                   /* Initialize MTU size to 0 */
  .available_data_length = 0,      /* Initialize available data length to 0 */
};

/** BLE characteristics */
static const APP_Ble_Char_t app_ble_char[] =
{
  {
    .service_index = WIFI_COMMISSIONING_SERVICE_INDEX,
    .char_index = WIFI_CONTROL_CHAR_INDEX,
    .char_uuid = WIFI_CONTROL_CHAR_UUID,
    .uuid_type = W6X_BLE_UUID_TYPE_128,
    .char_property = W6X_BLE_CHAR_PROP_WRITE_WITH_RESP,
    .char_permission = W6X_BLE_CHAR_PERM_WRITE,
    .desc = "BLE service"
  },
  {
    .service_index = WIFI_COMMISSIONING_SERVICE_INDEX,
    .char_index = WIFI_CONFIGURE_CHAR_INDEX,
    .char_uuid = WIFI_CONFIGURE_CHAR_UUID,
    .uuid_type = W6X_BLE_UUID_TYPE_128,
    .char_property = W6X_BLE_CHAR_PROP_WRITE_WITH_RESP,
    .char_permission = W6X_BLE_CHAR_PERM_WRITE,
    .desc = "BLE WIFI Control charac"
  },
  {
    .service_index = WIFI_COMMISSIONING_SERVICE_INDEX,
    .char_index = WIFI_AP_LIST_CHAR_INDEX,
    .char_uuid = WIFI_AP_LIST_CHAR_UUID,
    .uuid_type = W6X_BLE_UUID_TYPE_128,
    .char_property = W6X_BLE_CHAR_PROP_NOTIFY,
    .char_permission = W6X_BLE_CHAR_PERM_READ,
    .desc = "BLE WIFI Configure charac"
  },
  {
    .service_index = WIFI_COMMISSIONING_SERVICE_INDEX,
    .char_index = WIFI_MONITORING_CHAR_INDEX,
    .char_uuid = WIFI_MONITORING_CHAR_UUID,
    .uuid_type = W6X_BLE_UUID_TYPE_128,
    .char_property = W6X_BLE_CHAR_PROP_READ | W6X_BLE_CHAR_PROP_NOTIFY,
    .char_permission = W6X_BLE_CHAR_PERM_READ | W6X_BLE_CHAR_PERM_WRITE,
    .desc = "BLE WIFI Monitoring charac"
  },
  {
    .service_index = FUOTA_SERVICE_INDEX,
    .char_index = FUOTA_BASE_ADDR_CHAR_INDEX,
    .char_uuid = FUOTA_BASE_ADDR_CHAR_UUID,
    .uuid_type = W6X_BLE_UUID_TYPE_128,
    .char_property = W6X_BLE_CHAR_PROP_WRITE_WITHOUT_RESP,
    .char_permission = W6X_BLE_CHAR_PERM_WRITE,
    .desc = "BLE FUOTA base address charac"
  },
  {
    .service_index = FUOTA_SERVICE_INDEX,
    .char_index = FUOTA_CONF_CHAR_INDEX,
    .char_uuid = FUOTA_CONF_CHAR_UUID,
    .uuid_type = W6X_BLE_UUID_TYPE_128,
    .char_property = W6X_BLE_CHAR_PROP_INDICATE,
    .char_permission = 0,
    .desc = "BLE FUOTA confirmation charac"
  },
  {
    .service_index = FUOTA_SERVICE_INDEX,
    .char_index = FUOTA_RAW_DATA_CHAR_INDEX,
    .char_uuid = FUOTA_RAW_DATA_CHAR_UUID,
    .uuid_type = W6X_BLE_UUID_TYPE_128,
    .char_property = W6X_BLE_CHAR_PROP_WRITE_WITHOUT_RESP,
    .char_permission = W6X_BLE_CHAR_PERM_WRITE,
    .desc = "BLE FUOTA raw data charac"
  }
};

/** List of bonded BLE devices */
W6X_Ble_Bonded_Devices_Result_t app_ble_BondedDeviceList = {0};

/** Application information */
static const APP_Info_t app_info =
{
  .name = "ST67W6X Wi-Fi Commissioning over BLE",
  .version = HOST_APP_VERSION_STR
};

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
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

/**
  * @brief  Wi-Fi scan callback
  * @param  status: Scan status
  * @param  Scan_results: Scan results
  */
static void APP_wifi_scan_cb(int32_t status, W6X_WiFi_Scan_Result_t *Scan_results);

/**
  * @brief Initialize BLE in advertising mode
  * @return Operation Status
  */
W6X_Status_t BleSetServerAdvertising(void);

/**
  * @brief   Task to handle BLE write events for FUOTA
  *
  * @param pvParameters : Pointer to task parameters (not used)
  * @note  This task processes BLE FUOTA write events from the queue and handles operations.
  */
void BleFuotaWriteTask(void *pvParameters);

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
  EventBits_t eventBits = 0;
  W6X_WiFi_Connect_t connectData = {0};
  W6X_WiFi_StaStateType_e state = W6X_WIFI_STATE_STA_OFF;

  /* USER CODE BEGIN main_app_1 */

  /* USER CODE END main_app_1 */

#if (TEST_AUTOMATION_ENABLE == 1)
  /* Start the task performance measurement */
  task_perf_start();
#endif /* TEST_AUTOMATION_ENABLE */

  LowPowerManagerInit();

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

  /* Create the FUOTA write queue */
  /* The queue is used to store the BLE FUOTA write events */
  bleWriteQueue = xQueueCreate(FUOTA_QUEUE_SIZE, sizeof(APP_Ble_WriteEvent_t));

  /* Create the task to handle BLE FUOTA write events */
  xTaskCreate(BleFuotaWriteTask, "BleFuotaWriteTask", 1024 >> 2, NULL, 24, NULL);

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

  /* Initialize the ST67W6X BLE module */
  ret = W6X_Ble_Init(W6X_BLE_MODE_SERVER, a_APP_AvailableData, sizeof(a_APP_AvailableData) - 1);
  if (ret)
  {
    LogError("Failed to initialize ST67W6X BLE component, %" PRIi32 "\n", ret);
    goto _err;
  }
  LogInfo("Ble init is done\n");

  /* USER CODE BEGIN main_app_3 */

  /* USER CODE END main_app_3 */

  ret = BleSetServerAdvertising();
  if (ret != W6X_STATUS_OK)
  {
    goto _err;
  }

  while (1)
  {
    /* Wait to receive a BLE event */
    eventBits = xEventGroupWaitBits(app_evt_current, EVT_APP_ALL_BIT, pdTRUE, pdFALSE, portMAX_DELAY);

    /* Process button */
    if (eventBits & EVT_APP_BUTTON)
    {
      if (app_ble_params.remote_ble_device.IsConnected != 0x00)
      {
        ret = W6X_Ble_Disconnect(app_ble_params.remote_ble_device.conn_handle);
        if (ret != W6X_STATUS_OK)
        {
          LogError("BLE disconnection failed, %" PRIi32 "\n", ret);
        }
      }
      else
      {
        LogInfo("Unpair bonded devices\n");
        W6X_Ble_SecurityGetBondedDeviceList(&app_ble_BondedDeviceList);
        while (app_ble_BondedDeviceList.Count != 0)
        {
          uint32_t count = app_ble_BondedDeviceList.Count - 1;
          LogInfo("Bonded Device: " ADDRSTR "\n",
                  ADDR2STR(app_ble_BondedDeviceList.Bonded_device[count].BDAddr));

          ret = W6X_Ble_SecurityUnpair(app_ble_BondedDeviceList.Bonded_device[count].BDAddr,
                                       W6X_BLE_PUBLIC_ADDR);
          if (ret != W6X_STATUS_OK)
          {
            LogError("Failed to unpair bonded devices, %" PRIi32 "\n", ret);
          }
          LogInfo(ADDRSTR " unpaired\n",
                  ADDR2STR(app_ble_BondedDeviceList.Bonded_device[count].BDAddr));
          W6X_Ble_SecurityGetBondedDeviceList(&app_ble_BondedDeviceList);
        }

        LogInfo("No more bonded device\n");
      }
    }

    /* Process BLE notification */
    if (eventBits & EVT_APP_BLE_NOTIFICATION_STATUS_ENABLED)
    {
      if (a_APP_WifiConnectedSSID[0] != 0)
      {
        /* Retrieve information about Wifi network connected */
        /* Send notification when Wi-Fi connected */
        uint8_t monitoring_data[W6X_WIFI_MAX_SSID_SIZE + 1] = {0};
        /* Payload length = Flag Length + SSID length */
        size_of_WifiCommUpdateCharData = strlen((char *)a_APP_WifiConnectedSSID) + 1;
        /* Add Connection Done Flag at the beginning of the payload */
        monitoring_data[0] = MONITORING_TYPE_CONNECTION_DONE;
        /* Copy SSID of connected device */
        strncpy((char *) &monitoring_data[1], (char *)a_APP_WifiConnectedSSID, size_of_WifiCommUpdateCharData - 1);
        memcpy(a_APP_WifiCommUpdateCharData, (void *) &monitoring_data, size_of_WifiCommUpdateCharData);

        BLE_Send_Wifi_Monitoring_Notification(app_ble_params.remote_ble_device.conn_handle);
      }
    }

    /* Process BLE read */
    if (eventBits & EVT_APP_BLE_READ)
    {
    }

    /* Process BLE write */
    if (eventBits & EVT_APP_BLE_WRITE)
    {
      if ((app_ble_params.service_idx == WIFI_COMMISSIONING_SERVICE_INDEX) &
          (app_ble_params.charac_idx == WIFI_CONTROL_CHAR_INDEX))
      {
        LogInfo("WIFI Control Charac\n");
        if (a_APP_AvailableData[0] == CONTROL_ACTION_START_SCAN) /* Start Scan */
        {
          LogInfo("WIFI Scan Enable\n");
          /* Run a Wi-Fi scan to retrieve the list of all nearby Access Points */
          scan_event_flags = xEventGroupCreate();
          W6X_WiFi_Scan(&APP_Opts, &APP_wifi_scan_cb);

          /* Wait to receive the EVENT_FLAG_SCAN_DONE event. The scan is declared as failed after 'ScanTimeout' delay */
          if ((int32_t)xEventGroupWaitBits(scan_event_flags, EVENT_FLAG_WIFI_SCAN_DONE, pdTRUE, pdFALSE,
                                           pdMS_TO_TICKS(WIFI_SCAN_TIMEOUT)) != EVENT_FLAG_WIFI_SCAN_DONE)
          {
            LogError("Scan Failed\n");
            goto _err;
          }
          else
          {
            /* Retrieve the configuration of nearby Access Point */
            for (uint32_t ap_index = 0; ap_index < app_scan_results.Count; ap_index++)
            {
              if (strlen((char *)app_scan_results.AP[ap_index].SSID) > 0)
              {
                APP_AP_t *scan_item = (APP_AP_t *)a_APP_WifiCommUpdateCharData;
                scan_item->Channel = app_scan_results.AP[ap_index].Channel;
                scan_item->Security = app_scan_results.AP[ap_index].Security;
                scan_item->RSSI = app_scan_results.AP[ap_index].RSSI;
                memcpy(scan_item->SSID, app_scan_results.AP[ap_index].SSID, sizeof(scan_item->SSID));
                scan_item->SSID_len = W6X_WIFI_MAX_SSID_SIZE;
                size_of_WifiCommUpdateCharData = sizeof(APP_AP_t);

                /* Send the configuration of each Access Point as BLE notification */
                BLE_Send_Wifi_Scan_Report_Notification(app_ble_params.remote_ble_device.conn_handle);
              }
            }
          }
        }
        else if (a_APP_AvailableData[0] == CONTROL_ACTION_CONNECT) /* Connect */
        {
          /* Connect the device to the selected Access Point */
          LogInfo("WIFI Connect\n");
          ret = W6X_WiFi_Connect(&APP_ConnectOpts);
          if (ret != W6X_STATUS_OK)
          {
            LogError("Failed to connect, %" PRIi32 "\n", ret);
            APP_setevent(&app_evt_current, EVT_APP_WIFI_CONNECTION_TIMEOUT);
          }
          else
          {
            LogInfo("App connected\n");
            APP_setevent(&app_evt_current, EVT_APP_WIFI_CONNECTED);
          }
        }
        else if (a_APP_AvailableData[0] == CONTROL_ACTION_DISCONNECT) /* Disconnect */
        {
          /* Disconnect the device from the Access Point */
          LogInfo("WIFI Disconnect\n");
          ret = W6X_WiFi_Disconnect(1);
          if (ret == W6X_STATUS_OK)
          {
            LogInfo("Disconnect success\n");
          }
          else
          {
            LogError("Disconnect failed, %" PRIi32 "\n", ret);
          }
        }
        else if (a_APP_AvailableData[0] == CONTROL_ACTION_PING) /* Ping */
        {
          uint16_t ping_count = WIFI_PING_COUNT;
          uint32_t ping_size = WIFI_PING_SIZE;
          uint32_t ping_interval = WIFI_PING_INTERVAL;
          uint32_t ping_timeout = WIFI_PING_INTERVAL;
          uint32_t average_ping = 0;
          uint16_t ping_received_response = 0;
          uint8_t percent_packet_loss = 0;

          char gw_str[17] = {'\0'};
          uint8_t ip_addr[4] = {0};
          uint8_t gateway_addr[4] = {0};
          uint8_t netmask_addr[4] = {0};

          LogInfo("WIFI Ping\n");
          /* Get the IP Address of the Access Point */
          if (W6X_STATUS_OK == W6X_Net_Station_GetIPAddress(ip_addr, gateway_addr, netmask_addr))
          {
            /* Convert the IP Address array into string */
            snprintf(gw_str, 16, IPSTR, IP2STR(gateway_addr));

            /* Run the ping */
            if (W6X_STATUS_OK == W6X_Net_Ping((uint8_t *)gw_str, ping_size, ping_count, ping_interval,
                                              ping_timeout, &average_ping, &ping_received_response))
            {
              if (ping_received_response == 0)
              {
                LogError("No ping received\n");
              }
              else
              {
                /* Success: process the stats returned */
                percent_packet_loss = WIFI_PING_PERCENT * (ping_count - ping_received_response) / ping_count;
                LogInfo("%" PRIu16 " packets transmitted, %" PRIu16 " received, %" PRIu16
                        "%% packet loss, time %" PRIu32 "ms\n",
                        ping_count, ping_received_response,
                        percent_packet_loss, average_ping);

                uint8_t ping_data[8] =
                {
                  MONITORING_TYPE_PING_RESPONSE,
                  (ping_count >> 8) & 0xFF,
                  ping_count & 0xFF,
                  percent_packet_loss,
                  (average_ping >> 24) & 0xFF,
                  (average_ping >> 16) & 0xFF,
                  (average_ping >> 8) & 0xFF,
                  average_ping & 0xFF
                };
                size_of_WifiCommUpdateCharData = 8;
                memcpy(a_APP_WifiCommUpdateCharData, (void *) &ping_data, size_of_WifiCommUpdateCharData);
                BLE_Send_Wifi_Monitoring_Notification(app_ble_params.remote_ble_device.conn_handle);
              }
            }
            else
            {
              LogError("Ping Failure\n");
            }
          }
        }
#if (TEST_AUTOMATION_ENABLE == 1)
        else if ((a_APP_AvailableData[0] == CONTROL_ACTION_QUIT_CONTROL) && (a_APP_AvailableData[1] == 0xFF))
        {
          LogInfo(" -> Quit Automation Write received\n");
          break;
        }
#endif /* TEST_AUTOMATION_ENABLE */
      }
    }

    if (eventBits & EVT_APP_WIFI_CONNECTED)
    {
      if (W6X_WiFi_Station_GetState(&state, &connectData) != W6X_STATUS_OK)
      {
        LogWarn("Connected to an unknown Access Point\n");
        continue;
      }

      memcpy(a_APP_WifiConnectedSSID, connectData.SSID, sizeof(a_APP_WifiConnectedSSID));

      LogInfo("Connected to following Access Point :\n");
      LogInfo("[" MACSTR "] Channel: %" PRIu32 " | RSSI: %" PRIi32 " | SSID: %s\n",
              MAC2STR(connectData.MAC),
              connectData.Channel,
              connectData.Rssi,
              connectData.SSID);

      /* Send notification when Wi-Fi connected */
      uint8_t monitoring_data[W6X_WIFI_MAX_SSID_SIZE + 1] = {0};

      /* Payload length = Flag Length + SSID length */
      size_of_WifiCommUpdateCharData = strlen((char *)APP_ConnectOpts.SSID) + 1;

      /* Add Connection Done Flag at the beginning of the payload */
      monitoring_data[0] = MONITORING_TYPE_CONNECTION_DONE;

      /* Copy SSID of connected device */
      strncpy((char *) &monitoring_data[1], (char *)APP_ConnectOpts.SSID, size_of_WifiCommUpdateCharData - 1);
      memcpy(a_APP_WifiCommUpdateCharData, (void *) &monitoring_data, size_of_WifiCommUpdateCharData);

      BLE_Send_Wifi_Monitoring_Notification(app_ble_params.remote_ble_device.conn_handle);
    }

    if (eventBits & EVT_APP_WIFI_CONNECTION_TIMEOUT)
    {
      /* Send notification if Wi-Fi connection error */
      uint8_t monitoring_data[2] = {MONITORING_TYPE_ERROR, MONITORING_DATA_CONNECTION_TIMEOUT};
      size_of_WifiCommUpdateCharData = 2;
      memcpy(a_APP_WifiCommUpdateCharData, (void *) &monitoring_data, size_of_WifiCommUpdateCharData);

      BLE_Send_Wifi_Monitoring_Notification(app_ble_params.remote_ble_device.conn_handle);
    }

    /* Process BLE passkey entry */
    if (eventBits & EVT_APP_BLE_PASSKEY_ENTRY)
    {
      app_ble_params.PassKey = BLE_GENERIC_PASSKEY;
      if (app_ble_params.remote_ble_device.conn_handle != 0xff)
      {
        ret = W6X_Ble_SecuritySetPassKey(app_ble_params.remote_ble_device.conn_handle, app_ble_params.PassKey);
        if (ret != W6X_STATUS_OK)
        {
          LogError("Failed to set passkey, %" PRIi32 "\n"
                   "Press button while disconnected to clear security Database and relaunch pairing process\n", ret);
        }
        else
        {
          LogInfo("Set passkey\n");
        }
      }
      else
      {
        /* Connection handle not correctly identified */
        LogInfo("Press button while disconnected to clear security Database and relaunch pairing process\n");
      }
    }

    /* Process BLE passkey confirm */
    if (eventBits & EVT_APP_BLE_PASSKEY_CONFIRM)
    {
      if (app_ble_params.remote_ble_device.conn_handle != 0xff)
      {
        ret = W6X_Ble_SecurityPassKeyConfirm(app_ble_params.remote_ble_device.conn_handle);
        if (ret != W6X_STATUS_OK)
        {
          LogError("Failed to send passkey confirm, %" PRIi32 "\n"
                   "Press button while disconnected to clear security Database and relaunch pairing process\n", ret);
        }
        else
        {
          LogInfo("Sent passkey confirm\n");
        }
      }
      else
      {
        /* Connection handle not correctly identified */
        LogInfo("Press button while disconnected to clear security Database and relaunch pairing process\n");
      }
    }

    /* Process BLE pairing confirm */
    if (eventBits & EVT_APP_BLE_PAIRING_CONFIRM)
    {
      if (app_ble_params.remote_ble_device.conn_handle != 0xff)
      {
        ret = W6X_Ble_SecurityPairingConfirm(app_ble_params.remote_ble_device.conn_handle);
        if (ret != W6X_STATUS_OK)
        {
          LogError("Pairing Confirm Failed, %" PRIi32 "\n"
                   "Press button while disconnected to clear security Database and relaunch pairing process\n", ret);
        }
        else
        {
          LogInfo("Pairing Confirm\n");
        }
      }
      else
      {
        /* Connection handle not correctly identified:
         * random address used for connection different from address registered in security Database */
        LogInfo("Press button while disconnected to clear security Database and relaunch pairing process\n");
      }
    }

    /* Process BLE pairing complete */
    if (eventBits & EVT_APP_BLE_PAIRING_COMPLETED)
    {
      LogInfo("Pairing Completed\n");
    }
  }

  /* USER CODE BEGIN main_app_Last */

  /* USER CODE END main_app_Last */

_err:
  /* USER CODE BEGIN main_app_Err_1 */

  /* USER CODE END main_app_Err_1 */
  /* De-initialize the ST67W6X BLE module */
  W6X_Ble_DeInit();

  /* De-initialize the ST67W6X Network module */
  W6X_Net_DeInit();

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

W6X_Status_t BleSetServerAdvertising(void)
{
  W6X_Status_t ret;
  int32_t data_index = 0;
  uint8_t bd_address[W6X_BLE_BD_ADDR_SIZE];
  const char hex_chars[] = "0123456789ABCDEF";
  char ble_device_name[W6X_BLE_DEVICE_NAME_SIZE];

  ret = W6X_Ble_GetBDAddress(bd_address);
  if (ret == W6X_STATUS_OK)
  {
    LogInfo("BD Address: " MACSTR "\n", MAC2STR(bd_address));
  }
  else
  {
    LogError("BD Address identification failed, %" PRIi32 "\n", ret);
    return ret;
  }

  sprintf(ble_device_name, W6X_BLE_HOSTNAME "_%02" PRIX16, bd_address[5]);
  ret = W6X_Ble_SetDeviceName(ble_device_name);
  if (ret != W6X_STATUS_OK)
  {
    LogError("Failed to set device name, %" PRIi32 "\n", ret);
    return ret;
  }

  /* Configure the ST67W6X BLE module
   * - Set the Tx power to 0 (minimal)
   * - Set the Bluetooth Device Address
   * - Set the Advertising data
   */
  LogInfo("Configure BLE\n");

  ret = W6X_Ble_SetTxPower(0);
  if (ret != W6X_STATUS_OK)
  {
    LogError("Failed to set TX power, %" PRIi32 "\n", ret);
    return ret;
  }

  /* Fill adv data with BD Address */
  while (data_index < 6)
  {
    a_AdvData[20 + (2 * data_index)] = hex_chars[(bd_address[data_index] & 0xF0) >> 4];
    a_AdvData[20 + (2 * data_index) + 1] = hex_chars[bd_address[data_index] & 0x0F];
    data_index++;
  }

  ret = W6X_Ble_SetAdvData((const char *) a_AdvData);
  if (ret != W6X_STATUS_OK)
  {
    LogError("Failed to set ADV data, %" PRIi32 "\n", ret);
    return ret;
  }
  LogInfo("BLE configuration is done\n");

  /* Create the BLE Commissioning Service */
  LogInfo("\nBLE Commissioning Service Creation\n");
  ret = W6X_Ble_CreateService(WIFI_COMMISSIONING_SERVICE_INDEX,
                              WIFI_COMMISSIONING_SERVICE_UUID,
                              W6X_BLE_UUID_TYPE_128);
  if (ret != W6X_STATUS_OK)
  {
    LogError("Failed to Create service, %" PRIi32 "\n", ret);
    return ret;
  }

  ret = W6X_Ble_CreateService(FUOTA_SERVICE_INDEX,
                              FUOTA_SERVICE_UUID,
                              W6X_BLE_UUID_TYPE_128);
  if (ret != W6X_STATUS_OK)
  {
    LogError("Failed to Create service, %" PRIi32 "\n", ret);
    return ret;
  }

  /* Create the BLE Characteristics */
  for (uint8_t i = 0; i < sizeof(app_ble_char) / sizeof(APP_Ble_Char_t); i++)
  {
    ret = W6X_Ble_CreateCharacteristic(app_ble_char[i].service_index, app_ble_char[i].char_index,
                                       app_ble_char[i].char_uuid, app_ble_char[i].uuid_type,
                                       app_ble_char[i].char_property, app_ble_char[i].char_permission);
    if (ret != W6X_STATUS_OK)
    {
      LogError("Failed to create %s, %" PRIi32 "\n", app_ble_char[i].desc, ret);
      return ret;
    }

    LogInfo("- %s created\n", app_ble_char[i].desc);
  }

  /* Register the BLE Characteristics */
  ret = W6X_Ble_RegisterCharacteristics();
  if (ret != W6X_STATUS_OK)
  {
    LogError("Failed to Register characteristics, %" PRIi32 "\n", ret);
    return ret;
  }

  LogInfo("- BLE services and charac registered\n");
  LogInfo("BLE service and charac creation is done\n");

  /* Setup security mode */
  ret = W6X_Ble_SetSecurityParam(W6X_BLE_SEC_IO_KEYBOARD_DISPLAY);
  if (ret != W6X_STATUS_OK)
  {
    LogError("BLE Set Security Parameters failed, %" PRIi32 "\n", ret);
  }

  /* Start the BLE Advertising mode: the ST67W6X can be detected as BLE device */
  LogInfo("\nStart BLE advertising\n");
  ret = W6X_Ble_AdvStart();
  if (ret != W6X_STATUS_OK)
  {
    LogError("Failed to start advertising, %" PRIi32 "\n", ret);
    return ret;
  }

  LogInfo("BLE advertising is started\n");

  return ret;
}

/* This task is used to handle BLE FUOTA write events.
 * It processes the events received from the BLE callback in queue dedicated for FUOTA events.
 */
void BleFuotaWriteTask(void *pvParameters)
{
  APP_Ble_WriteEvent_t fuota_evt = {0};
  uint8_t confirm = 0x03;
  uint32_t sent = 0;
  uint32_t total_to_send = 0;
  uint32_t remaining = 0;
  int32_t ret_w6x = W6X_STATUS_ERROR;
  bool fuota_stm32_mode = false;
  for (;;)
  {
    if (xQueueReceive(bleWriteQueue, &fuota_evt, portMAX_DELAY) == pdPASS)
    {
      /* Check if the event is for FUOTA base address characteristic
       * This is used to process FUOTA control commands (START, STOP, CANCEL, etc.).
      */
      if (fuota_evt.charac_idx == FUOTA_BASE_ADDR_CHAR_INDEX)
      {
        uint8_t action = fuota_evt.data[0];
        switch (action)
        {
          case 0x00: /* STOP */
          case 0x08: /* CANCEL */
            /* Free the fuota buffer */
            if (fuota_buffer != NULL)
            {
              vPortFree(fuota_buffer);
              fuota_buffer = NULL;
            }

            LogInfo("FUOTA %s\n", (action == 0x00) ? "STOP" : "CANCEL");
            fuota_buffer_offset = 0;
            /* Stops and reset the ST67 FWU context */
            ret_w6x = W6X_FWU_Starts(0);
            if (ret_w6x != W6X_STATUS_OK)
            {
              LogError("Failed to start the ST67 OTA , %" PRIi32 "\n", ret_w6x);
              confirm = 0x03;
              sent = 0;
              W6X_Ble_ServerIndicate(fuota_evt.conn_handle, FUOTA_SERVICE_INDEX, FUOTA_CONF_CHAR_INDEX,
                                     &confirm, 1, &sent, 1000);
            }
            break;

          case 0x01: /* START STM32 FUOTA */
            /* USER CODE BEGIN fota_ble_task_1 */
            /* Allocate the fuota buffer */
            if (fuota_buffer == NULL)
            {
              fuota_buffer = pvPortMalloc(FUOTA_BUFFER_SIZE);
            }

            fuota_buffer_offset = 0;
            fuota_stm32_mode = true;
            /* Get the start address of the target flash bank */
            fuota_current_flash_addr = fota_flash_GetStartAddrFlashBank();
            /* Bank 2 mass erase */
            if (fota_flash_MassEraseFlashBank() == 0)
            {
              LogInfo("FUOTA STM32 START Application\n");
              confirm = 0x02;
              sent = 0;
              W6X_Ble_ServerIndicate(fuota_evt.conn_handle, FUOTA_SERVICE_INDEX, FUOTA_CONF_CHAR_INDEX,
                                     &confirm, 1, &sent, 1000);
              break;
            }

            /* USER CODE END fota_ble_task_1 */
            LogError("FUOTA not ready or not supported\n");
            confirm = 0x03;
            sent = 0;
            W6X_Ble_ServerIndicate(fuota_evt.conn_handle, FUOTA_SERVICE_INDEX, FUOTA_CONF_CHAR_INDEX,
                                   &confirm, 1, &sent, 1000);

            break;
          case 0x02: /* START ST67 FUOTA */
            /* Allocate the fuota buffer */
            if (fuota_buffer == NULL)
            {
              fuota_buffer = pvPortMalloc(FUOTA_BUFFER_SIZE);
            }

            fuota_buffer_offset = 0;
            /* USER CODE BEGIN fota_ble_task_2 */
            fuota_stm32_mode = false;

            /* USER CODE END fota_ble_task_2 */

            /* Start the ST67 FWU */
            if (W6X_FWU_Starts(1) == W6X_STATUS_OK)
            {
              LogInfo("FUOTA ST67 START Application\n");
              confirm = 0x02;
              sent = 0;
              W6X_Ble_ServerIndicate(fuota_evt.conn_handle, FUOTA_SERVICE_INDEX, FUOTA_CONF_CHAR_INDEX,
                                     &confirm, 1, &sent, 1000);
            }
            else
            {
              LogError("FUOTA not ready\n");
              confirm = 0x03;
              sent = 0;
              W6X_Ble_ServerIndicate(fuota_evt.conn_handle, FUOTA_SERVICE_INDEX, FUOTA_CONF_CHAR_INDEX,
                                     &confirm, 1, &sent, 1000);
            }
            break;
          case 0x06: /* EOF */
            if ((fuota_buffer_offset > 0) && (fuota_buffer != NULL))
            {
              /* USER CODE BEGIN fota_ble_task_3 */
              /* Depending on the mode, write the buffer to flash or send it to the ST67 */
              if (fuota_stm32_mode)
              {
                if (fota_flash_WriteToFlash(fuota_current_flash_addr, fuota_buffer, fuota_buffer_offset) == 0)
                {
                  ret_w6x = W6X_STATUS_OK;
                }
                else
                {
                  ret_w6x = W6X_STATUS_ERROR;
                }
              }

              /* USER CODE END fota_ble_task_3 */
              if (!fuota_stm32_mode)
              {
                ret_w6x = W6X_FWU_Send(fuota_buffer, fuota_buffer_offset);
              }

              /* Free the fuota buffer */
              vPortFree(fuota_buffer);
              fuota_buffer = NULL;
              if (ret_w6x != W6X_STATUS_OK)
              {
                LogError("Failed to transfer last FUOTA buffer, %" PRIi32 "\n", ret_w6x);
                confirm = 0x03;
                sent = 0;
                W6X_Ble_ServerIndicate(fuota_evt.conn_handle, FUOTA_SERVICE_INDEX, FUOTA_CONF_CHAR_INDEX,
                                       &confirm, 1, &sent, 1000);
                break;
              }
              LogInfo("Last FUOTA buffer transferred\n");
              fuota_buffer_offset = 0;
            }
            break;
          case 0x07: /* FINISH */
            if ((fuota_buffer_offset > 0) && (fuota_buffer != NULL))
            {
              /* USER CODE BEGIN fota_ble_task_4 */
              /* Depending on the mode, write the buffer to flash or send it to the ST67 */
              if (fuota_stm32_mode)
              {
                if (fota_flash_WriteToFlash(fuota_current_flash_addr, fuota_buffer, fuota_buffer_offset) == 0)
                {
                  ret_w6x = W6X_STATUS_OK;
                }
                else
                {
                  ret_w6x = W6X_STATUS_ERROR;
                }
              }

              /* USER CODE END fota_ble_task_4 */
              if (!fuota_stm32_mode)
              {
                ret_w6x = W6X_FWU_Send(fuota_buffer, fuota_buffer_offset);
              }

              /* Free the fuota buffer */
              vPortFree(fuota_buffer);
              fuota_buffer = NULL;
              if (ret_w6x != W6X_STATUS_OK)
              {
                LogError("Failed to transfer last FUOTA buffer, %" PRIi32 "\n", ret_w6x);
                confirm = 0x03;
                sent = 0;
                W6X_Ble_ServerIndicate(fuota_evt.conn_handle, FUOTA_SERVICE_INDEX, FUOTA_CONF_CHAR_INDEX,
                                       &confirm, 1, &sent, 1000);
                break;
              }
              LogInfo("Last FUOTA buffer transferred\n");
              fuota_buffer_offset = 0;
            }
            /* Send confirmation that the FUOTA is finished */
            confirm = 0x01;
            sent = 0;
            W6X_Ble_ServerIndicate(fuota_evt.conn_handle, FUOTA_SERVICE_INDEX, FUOTA_CONF_CHAR_INDEX,
                                   &confirm, 1, &sent, 1000);
            vTaskDelay(pdMS_TO_TICKS(1000)); /* Wait for the BLE stack to process indication */

            /* USER CODE BEGIN fota_ble_task_5 */
            /* Depending on the mode, swap STM32 banks to boot on new update or tell the ST67 to reboot on new update */
            if (fuota_stm32_mode)
            {
              if (fota_flash_SwapFlashBanks() != 0)
              {
                LogError("Failed to finish FUOTA, %" PRIi32 "\n", ret_w6x);
                confirm = 0x03;
                sent = 0;
                W6X_Ble_ServerIndicate(fuota_evt.conn_handle, FUOTA_SERVICE_INDEX, FUOTA_CONF_CHAR_INDEX,
                                       &confirm, 1, &sent, 1000);
                break;
              }

              /* Perform a system reset of the STM32 */
              HAL_NVIC_SystemReset();
              /* Because of NVIC reset, any code beyond this point won't be executed */
              break;
            }

            /* USER CODE END fota_ble_task_5 */
            LogInfo("Apply ST67 new binary\n");

            ret_w6x = W6X_FWU_Finish();
            if (ret_w6x != W6X_STATUS_OK)
            {
              LogError("Failed to finish FUOTA, %" PRIi32 "\n", ret_w6x);
              confirm = 0x03;
              sent = 0;
              W6X_Ble_ServerIndicate(fuota_evt.conn_handle, FUOTA_SERVICE_INDEX, FUOTA_CONF_CHAR_INDEX,
                                     &confirm, 1, &sent, 1000);
              break;
            }
            LogInfo("New ST67 binary successfully Applied\n");

            ret_w6x = BleSetServerAdvertising();
            if (ret_w6x != W6X_STATUS_OK)
            {
              LogError("Failed to start BLE advertising, %" PRIi32 "\n", ret_w6x);
              break;
            }

            break;
          default:
            LogError("Unknown FUOTA action: 0x%02X\n", action);
            break;
        }
      }
      /* Check if the event is for FUOTA raw data characteristic
       * This is used to manage the received FUOTA data */
      else if ((fuota_evt.charac_idx == FUOTA_RAW_DATA_CHAR_INDEX) && (fuota_buffer != NULL))
      {
        /* The idea is to store in a buffer data until FUOTA_BUFFER_SIZE capacity is reached
         * and then process the data  */
        total_to_send = fuota_evt.data_len + fuota_buffer_offset;
        remaining = total_to_send - FUOTA_BUFFER_SIZE;

        if (total_to_send >= FUOTA_BUFFER_SIZE)
        {
          /* If additional data is available, copy it to the buffer */
          memcpy(fuota_buffer + fuota_buffer_offset, fuota_evt.data, FUOTA_BUFFER_SIZE - fuota_buffer_offset);
          /* If the total data to send is larger than FUOTA_BUFFER_SIZE,
           * write the buffer to flash or send it to the ST67 */
          /* USER CODE BEGIN fota_ble_task_6 */
          if (fuota_stm32_mode)
          {
            if (fota_flash_WriteToFlash(fuota_current_flash_addr, fuota_buffer, FUOTA_BUFFER_SIZE) == 0)
            {
              fuota_current_flash_addr += FUOTA_BUFFER_SIZE;
              LogDebug("FUOTA buffer transferred\n");
            }
            else
            {
              LogError("Failed to transfer FUOTA buffer, %" PRIi32 "\n", W6X_STATUS_ERROR);
              confirm = 0x03;
              sent = 0;
              W6X_Ble_ServerIndicate(fuota_evt.conn_handle, FUOTA_SERVICE_INDEX, FUOTA_CONF_CHAR_INDEX,
                                     &confirm, 1, &sent, 1000);
            }

            if (total_to_send > FUOTA_BUFFER_SIZE)
            {
              memcpy(fuota_buffer, fuota_evt.data + (FUOTA_BUFFER_SIZE - fuota_buffer_offset), remaining);
              fuota_buffer_offset = remaining;
            }
            else
            {
              fuota_buffer_offset = 0;
            }
            vPortFree(fuota_evt.data);
            continue;
          }

          /* USER CODE END fota_ble_task_6 */
          ret_w6x = W6X_FWU_Send(fuota_buffer, FUOTA_BUFFER_SIZE);
          if (ret_w6x != W6X_STATUS_OK)
          {
            LogError("Failed to transfer FUOTA buffer, %" PRIi32 "\n", ret_w6x);
            confirm = 0x03;
            sent = 0;
            W6X_Ble_ServerIndicate(fuota_evt.conn_handle, FUOTA_SERVICE_INDEX, FUOTA_CONF_CHAR_INDEX,
                                   &confirm, 1, &sent, 1000);
          }
          else
          {
            LogDebug("FUOTA buffer transferred\n");
          }

          if (total_to_send > FUOTA_BUFFER_SIZE)
          {
            memcpy(fuota_buffer, fuota_evt.data + (FUOTA_BUFFER_SIZE - fuota_buffer_offset), remaining);
            fuota_buffer_offset = remaining;
          }
          else
          {
            fuota_buffer_offset = 0;
          }
        }
        else
        {
          memcpy(fuota_buffer + fuota_buffer_offset, fuota_evt.data, fuota_evt.data_len);
          fuota_buffer_offset += fuota_evt.data_len;
        }
      }
      vPortFree(fuota_evt.data);
    }
  }
}

void BLE_Send_Wifi_Scan_Report_Notification(uint8_t conn_handle)
{
  /* USER CODE BEGIN BLE_Send_Wifi_Scan_Report_Notification_1 */

  /* USER CODE END BLE_Send_Wifi_Scan_Report_Notification_1 */
  int32_t ret;
  uint32_t SentDatalen = 0;

  LogInfo("BLE Send Notification\n");
  ret = W6X_Ble_ServerNotify(conn_handle, WIFI_COMMISSIONING_SERVICE_INDEX, WIFI_AP_LIST_CHAR_INDEX,
                             a_APP_WifiCommUpdateCharData, size_of_WifiCommUpdateCharData,
                             &SentDatalen, 6000);
  if (ret != W6X_STATUS_OK)
  {
    LogError("Send Notification FAILED: %" PRIi32 "\n", ret);
  }
  return;
  /* USER CODE BEGIN BLE_Send_Wifi_Scan_Report_Notification_End */

  /* USER CODE END BLE_Send_Wifi_Scan_Report_Notification_End */
}

void BLE_Send_Wifi_Monitoring_Notification(uint8_t conn_handle)
{
  int32_t ret;
  uint32_t SentDatalen = 0;
  /* USER CODE BEGIN BLE_Send_Wifi_Monitoring_Notification_1 */

  /* USER CODE END BLE_Send_Wifi_Monitoring_Notification_1 */

  LogInfo("BLE Send Notification\n");
  ret = W6X_Ble_ServerNotify(conn_handle, WIFI_COMMISSIONING_SERVICE_INDEX, WIFI_MONITORING_CHAR_INDEX,
                             a_APP_WifiCommUpdateCharData, size_of_WifiCommUpdateCharData,
                             &SentDatalen, 6000);
  if (ret != W6X_STATUS_OK)
  {
    LogError("Send Notification FAILED: %" PRIu32 "\n", SentDatalen);
  }
  return;
  /* USER CODE BEGIN BLE_Send_Wifi_Monitoring_Notification_End */

  /* USER CODE END BLE_Send_Wifi_Monitoring_Notification_End */
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
static void APP_wifi_scan_cb(int32_t status, W6X_WiFi_Scan_Result_t *Scan_results)
{
  LogInfo("WiFi Scan done.\n");
  W6X_WiFi_PrintScan(Scan_results);

  if (Scan_results->Count == 0)
  {
    LogInfo("No scan results\n");
  }
  else
  {
    memset(&app_scan_results, 0, sizeof(app_scan_results));
    app_scan_results.Count = Scan_results->Count;
    app_scan_results.AP = Scan_results->AP;
  }
  xEventGroupSetBits(scan_event_flags, EVENT_FLAG_WIFI_SCAN_DONE);
}

static void APP_wifi_cb(W6X_event_id_t event_id, void *event_args)
{
  /* USER CODE BEGIN APP_wifi_cb_1 */

  /* USER CODE END APP_wifi_cb_1 */

  switch (event_id)
  {
    case W6X_WIFI_EVT_CONNECTED_ID:
      APP_setevent(&app_evt_current, EVT_APP_WIFI_CONNECTED);
      break;

    case W6X_WIFI_EVT_DISCONNECTED_ID:
      /* Reinitialize connected device struct */
      memset(APP_ConnectOpts.SSID, 0, W6X_WIFI_MAX_SSID_SIZE + 1);
      memset(APP_ConnectOpts.Password, 0, W6X_WIFI_MAX_PASSWORD_SIZE + 1);
      LogInfo("Station disconnected from Access Point\n");
      memset(a_APP_WifiConnectedSSID, 0, sizeof(a_APP_WifiConnectedSSID));
      break;

    case W6X_WIFI_EVT_REASON_ID:
      /* Reinitialize connected device struct */
      memset(APP_ConnectOpts.SSID, 0, W6X_WIFI_MAX_SSID_SIZE + 1);
      memset(APP_ConnectOpts.Password, 0, W6X_WIFI_MAX_PASSWORD_SIZE + 1);
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
  W6X_Ble_CbParamData_t *p_param_ble_data = (W6X_Ble_CbParamData_t *) event_args;

  switch (event_id)
  {
    case W6X_BLE_EVT_CONNECTED_ID:
      LogInfo(" -> BLE CONNECTED [Conn_Handle: %" PRIu16 "]\n", p_param_ble_data->remote_ble_device.conn_handle);

      /* Fill remote device structure */
      app_ble_params.remote_ble_device.conn_handle = p_param_ble_data->remote_ble_device.conn_handle;
      app_ble_params.remote_ble_device.IsConnected = p_param_ble_data->remote_ble_device.IsConnected;
      memcpy(app_ble_params.remote_ble_device.BDAddr, p_param_ble_data->remote_ble_device.BDAddr,
             sizeof(app_ble_params.remote_ble_device.BDAddr));
      memcpy(app_ble_params.remote_ble_device.DeviceName, p_param_ble_data->remote_ble_device.DeviceName,
             sizeof(app_ble_params.remote_ble_device.DeviceName));
      memcpy(app_ble_params.remote_ble_device.ManufacturerData, p_param_ble_data->remote_ble_device.ManufacturerData,
             sizeof(app_ble_params.remote_ble_device.ManufacturerData));

      APP_setevent(&app_evt_current, EVT_APP_BLE_CONNECTED);
      break;

    case W6X_BLE_EVT_CONNECTION_PARAM_ID:
      LogInfo(" -> BLE CONNECTION PARAM UPDATE [Conn_Handle: %" PRIu16 "]\n",
              p_param_ble_data->remote_ble_device.conn_handle);
      APP_setevent(&app_evt_current, EVT_APP_BLE_CONNECTION_PARAM_UPDATE);
      break;

    case W6X_BLE_EVT_DISCONNECTED_ID:
      LogInfo(" -> BLE DISCONNECTED [Conn_Handle: %" PRIu16 "]\n", p_param_ble_data->remote_ble_device.conn_handle);

      /* Reinitialize remote device struct */
      app_ble_params.remote_ble_device.RSSI = 0;
      app_ble_params.remote_ble_device.IsConnected = 0;
      app_ble_params.remote_ble_device.conn_handle = 0xff;
      memset(app_ble_params.remote_ble_device.DeviceName, 0, sizeof(app_ble_params.remote_ble_device.DeviceName));
      memset(app_ble_params.remote_ble_device.BDAddr, 0, sizeof(app_ble_params.remote_ble_device.BDAddr));
      memset(app_ble_params.remote_ble_device.ManufacturerData, 0,
             sizeof(app_ble_params.remote_ble_device.ManufacturerData));

      APP_setevent(&app_evt_current, EVT_APP_BLE_DISCONNECTED);
      break;

    case W6X_BLE_EVT_INDICATION_STATUS_ENABLED_ID:
      LogInfo(" -> BLE INDICATION ENABLED [Service: %" PRIu16 ", Charac: %" PRIu16 "].\n",
              p_param_ble_data->service_idx, p_param_ble_data->charac_idx);
      APP_setevent(&app_evt_current, EVT_APP_BLE_INDICATION_STATUS_ENABLED);
      break;

    case W6X_BLE_EVT_INDICATION_STATUS_DISABLED_ID:
      LogInfo(" -> BLE INDICATION DISABLED [Service: %" PRIu16 ", Charac: %" PRIu16 "].\n",
              p_param_ble_data->service_idx, p_param_ble_data->charac_idx);
      APP_setevent(&app_evt_current, EVT_APP_BLE_INDICATION_STATUS_DISABLED);
      break;

    case W6X_BLE_EVT_NOTIFICATION_STATUS_ENABLED_ID:
      LogInfo(" -> BLE NOTIFICATION ENABLED [Service: %" PRIu16 ", Charac: %" PRIu16 "].\n",
              p_param_ble_data->service_idx, p_param_ble_data->charac_idx);
      APP_setevent(&app_evt_current, EVT_APP_BLE_NOTIFICATION_STATUS_ENABLED);
      break;

    case W6X_BLE_EVT_NOTIFICATION_STATUS_DISABLED_ID:
      LogInfo(" -> BLE NOTIFICATION DISABLED [Service: %" PRIu16 ", Charac: %" PRIu16 "].\n",
              p_param_ble_data->service_idx, p_param_ble_data->charac_idx);
      APP_setevent(&app_evt_current, EVT_APP_BLE_NOTIFICATION_STATUS_DISABLED);
      break;

    case W6X_BLE_EVT_NOTIFICATION_DATA_ID:
      LogInfo(" -> BLE NOTIFICATION DATA.\n");
      APP_setevent(&app_evt_current, EVT_APP_BLE_NOTIFICATION_DATA);
      break;

    case W6X_BLE_EVT_WRITE_ID:
      app_ble_params.remote_ble_device.conn_handle = p_param_ble_data->remote_ble_device.conn_handle;
      app_ble_params.service_idx = p_param_ble_data->service_idx;
      app_ble_params.charac_idx = p_param_ble_data->charac_idx;
      app_ble_params.available_data_length = p_param_ble_data->available_data_length;
      LogInfo(" -> BLE WRITE [Conn_Handle: %" PRIu16 ", Service: %" PRIu16 ", Charac: %" PRIu16
              ", length %" PRIu32 "]\n",
              p_param_ble_data->remote_ble_device.conn_handle, p_param_ble_data->service_idx,
              p_param_ble_data->charac_idx, p_param_ble_data->available_data_length);

      /* When we receive a FUOTA write event, we put the data in the queue to be processed in a different task */
      if (app_ble_params.service_idx == FUOTA_SERVICE_INDEX)
      {
        APP_Ble_WriteEvent_t evt;
        evt.conn_handle = app_ble_params.remote_ble_device.conn_handle;
        evt.charac_idx = p_param_ble_data->charac_idx;
        evt.data_len = p_param_ble_data->available_data_length;
        evt.data = pvPortMalloc(evt.data_len);
        memcpy(evt.data, a_APP_AvailableData, evt.data_len);
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        xQueueSendFromISR(bleWriteQueue, &evt, &xHigherPriorityTaskWoken);
        if (xHigherPriorityTaskWoken) { portYIELD_FROM_ISR(xHigherPriorityTaskWoken); }
      }
      else
      {
        for (uint32_t i = 0; i < p_param_ble_data->available_data_length; i++)
        {
          LogInfo("0x%02" PRIX16 "\n", a_APP_AvailableData[i]);
        }
      }

      /* Manage Wi-Fi configuration */
      if ((app_ble_params.service_idx == WIFI_COMMISSIONING_SERVICE_INDEX) &&
          (app_ble_params.charac_idx == WIFI_CONFIGURE_CHAR_INDEX))
      {
        LogInfo("WIFI Configure Charac\n");
        if (a_APP_AvailableData[0] == CONFIGURE_TYPE_SSID) /* Wi-Fi AP SSID */
        {
          memcpy(APP_ConnectOpts.SSID, &a_APP_AvailableData[1], app_ble_params.available_data_length - 1);
          APP_ConnectOpts.SSID[app_ble_params.available_data_length - 1] = '\0';
          LogInfo("SSID NAME\n");
        }

        else if (a_APP_AvailableData[0] == CONFIGURE_TYPE_PWD) /* Wi-Fi AP Password */
        {
          memcpy(APP_ConnectOpts.Password, &a_APP_AvailableData[1], app_ble_params.available_data_length - 1);
          APP_ConnectOpts.Password[app_ble_params.available_data_length - 1] = '\0';
          LogInfo("PWD\n");
        }
      }

      APP_setevent(&app_evt_current, EVT_APP_BLE_WRITE);
      break;

    case W6X_BLE_EVT_READ_ID:
      LogInfo(" -> BLE READ.\n");
      APP_setevent(&app_evt_current, EVT_APP_BLE_READ);
      break;

    case W6X_BLE_EVT_PASSKEY_ENTRY_ID:
      app_ble_params.remote_ble_device.conn_handle = p_param_ble_data->remote_ble_device.conn_handle;
      LogInfo(" -> BLE PassKey Entry [Conn_Handle %" PRIu16 "]\n", app_ble_params.remote_ble_device.conn_handle);
      APP_setevent(&app_evt_current, EVT_APP_BLE_PASSKEY_ENTRY);
      break;

    case W6X_BLE_EVT_PASSKEY_CONFIRM_ID:
      app_ble_params.remote_ble_device.conn_handle = p_param_ble_data->remote_ble_device.conn_handle;
      app_ble_params.PassKey = p_param_ble_data->PassKey;
      LogInfo(" -> BLE PassKey received = %06" PRIu32 " [Conn_Handle %" PRIu16 "]\n", app_ble_params.PassKey,
              app_ble_params.remote_ble_device.conn_handle);
      APP_setevent(&app_evt_current, EVT_APP_BLE_PASSKEY_CONFIRM);
      break;

    case W6X_BLE_EVT_PAIRING_CONFIRM_ID:
      app_ble_params.remote_ble_device.conn_handle = p_param_ble_data->remote_ble_device.conn_handle;
      LogInfo(" -> BLE Pairing Confirm [Conn_Handle %" PRIu16 "]\n", app_ble_params.remote_ble_device.conn_handle);
      APP_setevent(&app_evt_current, EVT_APP_BLE_PAIRING_CONFIRM);
      break;

    case W6X_BLE_EVT_PAIRING_COMPLETED_ID:
      LogInfo(" -> BLE Pairing Complete\n");
      APP_setevent(&app_evt_current, EVT_APP_BLE_PAIRING_COMPLETED);
      break;

    case W6X_BLE_EVT_PASSKEY_DISPLAY_ID:
      app_ble_params.PassKey = p_param_ble_data->PassKey;
      LogInfo(" -> BLE PASSKEY  = %06" PRIu32 "\n", app_ble_params.PassKey);
      APP_setevent(&app_evt_current, EVT_APP_BLE_PASSKEY_DISPLAYED);
      break;

    case W6X_BLE_EVT_PAIRING_FAILED_ID:
      LogInfo(" -> BLE Pairing Failed [Conn_Handle: %" PRIu16 "]\n", app_ble_params.remote_ble_device.conn_handle);
      APP_setevent(&app_evt_current, EVT_APP_BLE_PAIRING_FAILED);
      break;

    case W6X_BLE_EVT_PAIRING_CANCELED_ID:
      LogInfo(" -> BLE Pairing Canceled [Conn_Handle %" PRIu16 "]\n", app_ble_params.remote_ble_device.conn_handle);
      APP_setevent(&app_evt_current, EVT_APP_BLE_PAIRING_CANCELED);
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
