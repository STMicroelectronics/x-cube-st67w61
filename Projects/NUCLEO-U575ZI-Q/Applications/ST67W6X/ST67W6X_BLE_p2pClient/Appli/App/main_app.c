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

/* USER CODE END Includes */

/* Global variables ----------------------------------------------------------*/
/* USER CODE BEGIN GV */

/* USER CODE END GV */

/* Private typedef -----------------------------------------------------------*/
/**
  * @brief  BLE characteristic information structure
  */
typedef struct
{
  uint8_t service_index;                    /*!< Service index */
  uint8_t char_index;                       /*!< Characteristic index */
  const char *service_uuid;                 /*!< Service UUID */
  uint8_t char_property;                    /*!< Characteristic property */
  uint8_t char_permission;                  /*!< Characteristic permission */
  const char *desc;                         /*!< Characteristic description */
} APP_Ble_Char_t;

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
#define EVT_APP_BUTTON                              (1UL << 0U)
/** Event when BLE is connected */
#define EVT_APP_BLE_CONNECTED                       (1UL << 1U)
/** Event when BLE is disconnected */
#define EVT_APP_BLE_DISCONNECTED                    (1UL << 2U)
/** Event when BLE connection parameters are updated */
#define EVT_APP_BLE_CONNECTION_PARAM_UPDATE         (1UL << 3U)
/** Event when BLE characteristic is read */
#define EVT_APP_BLE_READ                            (1UL << 4U)
/** Event when BLE characteristic is written */
#define EVT_APP_BLE_WRITE                           (1UL << 5U)
/** Event when BLE service is found */
#define EVT_APP_BLE_SERVICE_FOUND                   (1UL << 6U)
/** Event when BLE characteristic is found */
#define EVT_APP_BLE_CHAR_FOUND                      (1UL << 7U)
/** Event when BLE indication is enabled */
#define EVT_APP_BLE_INDICATION_STATUS_ENABLED       (1UL << 8U)
/** Event when BLE indication is disabled */
#define EVT_APP_BLE_INDICATION_STATUS_DISABLED      (1UL << 9U)
/** Event when BLE notification is enabled */
#define EVT_APP_BLE_NOTIFICATION_STATUS_ENABLED     (1UL << 10U)
/** Event when BLE notification is disabled */
#define EVT_APP_BLE_NOTIFICATION_STATUS_DISABLED    (1UL << 11U)
/** Event when BLE notification data is received */
#define EVT_APP_BLE_NOTIFICATION_DATA               (1UL << 12U)
/** Event when BLE MTU size is updated */
#define EVT_APP_BLE_MTU_SIZE                        (1UL << 13U)
/** Event when BLE pairing failed */
#define EVT_APP_BLE_PAIRING_FAILED                  (1UL << 14U)
/** Event when BLE pairing is completed */
#define EVT_APP_BLE_PAIRING_COMPLETED               (1UL << 15U)
/** Event when BLE pairing confirmation is requested */
#define EVT_APP_BLE_PAIRING_CONFIRM                 (1UL << 16U)
/** Event when BLE passkey entry is requested */
#define EVT_APP_BLE_PASSKEY_ENTRY                   (1UL << 17U)
/** Event when BLE passkey is displayed */
#define EVT_APP_BLE_PASSKEY_DISPLAYED               (1UL << 18U)
/** Event when BLE passkey confirmation is requested */
#define EVT_APP_BLE_PASSKEY_CONFIRM                 (1UL << 19U)
/** Event when BLE pairing is canceled */
#define EVT_APP_BLE_PAIRING_CANCELED                (1UL << 20U)
/** Event when BLE scan is done */
#define EVT_APP_BLE_SCAN_DONE                       (1UL << 21U)

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
                                 EVT_APP_BLE_SCAN_DONE)

/*
 The following 128bits UUIDs have been generated from the random UUID
 generator:
 0000fe40cc7a482a984a7f2ed5b3e58f: Service 128bits UUID
 0000fe418e2245419d4c21edae82ed19: Characteristic 128bits UUID
 0000fe428e2245419d4c21edae82ed19: Characteristic 128bits UUID
 */
/** P2P Server Service UUID */
#define P2PSERVER_SERVICE_UUID              "0000fe40cc7a482a984a7f2ed5b3e58f"
/** P2P Server Led control Characteristic UUID */
#define P2PSERVER_LED_CHAR_UUID             "0000fe418e2245419d4c21edae82ed19"
/** P2P Server Switch Characteristic UUID */
#define P2PSERVER_SWITCH_CHAR_UUID          "0000fe428e2245419d4c21edae82ed19"

/* P2P Server Led control Characteristic first byte possible values */
#define P2PSERVER_LED_CONTROL               0x01U  /*!< Led control */
#if (TEST_AUTOMATION_ENABLE == 1)
#define P2PSERVER_QUIT_CONTROL              0xFFU  /*!< Quit automation */
#endif /* TEST_AUTOMATION_ENABLE */

/** Generic Passkey */
#define BLE_GENERIC_PASSKEY                 123456

/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macros ------------------------------------------------------------*/
#ifndef ADDR2STR
/** BD Address buffer to string macros */
#define ADDR2STR(a) (a)[0], (a)[1], (a)[2], (a)[3], (a)[4], (a)[5]
/** BD Address string format */
#define ADDRSTR "%02" PRIx16 ":%02" PRIx16 ":%02" PRIx16 ":%02" PRIx16 ":%02" PRIx16 ":%02" PRIx16
#endif /* ADDR2STR */

#ifndef UUID128TOSTR
/** 128-bit UUID buffer to string macros */
#define UUID128TOSTR(a) \
  (a)[0], (a)[1], (a)[2], (a)[3], (a)[4], (a)[5], (a)[6], (a)[7], \
  (a)[8], (a)[9], (a)[10], (a)[11], (a)[12], (a)[13], (a)[14], (a)[15]
#endif /* UUID128TOSTR */

#ifndef UUID128STR
/** 128-bit UUID string format */
#define UUID128STR \
  "%02" PRIx16 "%02" PRIx16 "%02" PRIx16 "%02" PRIx16 "%02" PRIx16 "%02" PRIx16 "%02" PRIx16 "%02" PRIx16 \
  "%02" PRIx16 "%02" PRIx16 "%02" PRIx16 "%02" PRIx16 "%02" PRIx16 "%02" PRIx16 "%02" PRIx16 "%02" PRIx16
#endif /* UUID128STR */

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

/** BLE scan results */
W6X_Ble_Scan_Result_t app_ble_scan_results = {0};

/** OnOff Switch data */
int32_t OnOffSwitch[W6X_BLE_MAX_CONN_NBR] = {0};

/** BLE scan active state */
bool scan_mode = false;

/** Connection handle */
uint32_t connection_index = 0xFFU;

/** Application event group */
EventGroupHandle_t app_evt_current;

/** P2P Services and Characteristics indexes: must be identified at discovery time */
uint8_t p2p_service_index[W6X_BLE_MAX_CONN_NBR] = {0};                    /*!< Index of the Peer to Peer Service on remote Servers */
uint8_t p2p_led_characteristic_index[W6X_BLE_MAX_CONN_NBR] = {0};         /*!< Index of the LED characteristic on remote Servers */
uint8_t p2p_switch_characteristic_index[W6X_BLE_MAX_CONN_NBR] = {0};      /*!< Index of the SWITCH characteristic on remote Servers */

/** Connected devices information */
W6X_Ble_Connected_Device_t app_ble_connected_device_list[W6X_BLE_MAX_CONN_NBR] = {0};

/** Application information */
static const APP_Info_t app_info =
{
  .name = "ST67W6X BLE P2P Client",
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
  * @brief  BLE scan callback
  * @param  Ble_scan_results: Scan results
  */
static void APP_ble_scan_cb(W6X_Ble_Scan_Result_t *Ble_scan_results);

/**
  * @brief  Handle BLE notification event
  */
static void APP_ble_handle_notification_event(void);

/**
  * @brief  Validate BLE connection handle
  * @param  conn_handle: Connection handle
  * @retval true if valid, false otherwise
  */
static bool APP_ble_is_valid_conn_handle(uint32_t conn_handle);

/**
  * @brief  Write P2P data
  * @param  conn_handle: Connection handle to write data
  */
static void APP_ble_write_p2p_data(uint8_t conn_handle);

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
  int8_t device_connected = 0;
  uint8_t bd_address[W6X_BLE_BD_ADDR_SIZE];
  char ble_device_name[W6X_BLE_DEVICE_NAME_SIZE];

  for (uint32_t i = 0; i < W6X_BLE_MAX_CONN_NBR; i++)
  {
    app_ble_connected_device_list[i].conn_role = W6X_BLE_CONN_ROLE_UNKNOWN;
    app_ble_connected_device_list[i].conn_handle = 0xFFU;
  }
  /* USER CODE BEGIN main_app_1 */

  /* USER CODE END main_app_1 */

#if (TEST_AUTOMATION_ENABLE == 1)
  /* Start the task performance measurement */
  task_perf_start();
#endif /* TEST_AUTOMATION_ENABLE */

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

  app_evt_current = xEventGroupCreate();

  /* Initialize the ST67W6X Driver */
  ret = W6X_Init();
  if (ret != W6X_STATUS_OK)
  {
    LogError("Failed to initialize ST67W6X Driver, %" PRIi32 "\n", ret);
    goto _err;
  }

  /* Initialize the ST67W6X BLE module */
  ret = W6X_Ble_Init(W6X_BLE_MODE_CLIENT, a_APP_AvailableData, sizeof(a_APP_AvailableData) - 1U);
  if (ret != W6X_STATUS_OK)
  {
    LogError("Failed to initialize ST67W6X BLE component, %" PRIi32 "\n", ret);
    goto _err;
  }
  LogInfo("Ble init is done\n");

  /* USER CODE BEGIN main_app_3 */

  /* USER CODE END main_app_3 */

  ret = W6X_Ble_GetBDAddress(bd_address);
  if (ret == W6X_STATUS_OK)
  {
    LogInfo("BD Address: " ADDRSTR "\n", ADDR2STR(bd_address));
  }
  else
  {
    LogError("BD Address identification failed, %" PRIi32 "\n", ret);
    goto _err;
  }

  (void)sprintf(ble_device_name, W6X_BLE_HOSTNAME "_%02" PRIX16, bd_address[5]);
  ret = W6X_Ble_SetDeviceName(ble_device_name);
  if (ret != W6X_STATUS_OK)
  {
    LogError("Failed to set device name, %" PRIi32 "\n", ret);
    goto _err;
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
    goto _err;
  }

  /* Set Security Parameters */
  ret = W6X_Ble_SetSecurityParam(W6X_BLE_SEC_IO_NO_INPUT_OUTPUT);
  if (ret != W6X_STATUS_OK)
  {
    LogError("Set BLE Security Parameters failed, %" PRIi32 "\n", ret);
    goto _err;
  }
  LogInfo("Set BLE security parameters\n");

#if (TEST_AUTOMATION_ENABLE == 1)
  /* Automatically launch BLE Scan */
  if ((app_ble_connected_device_list[0].IsConnected == 0x00) && (app_ble_connected_device_list[1].IsConnected == 0x00))
  {
    /* Scan and connect */
    ret = W6X_Ble_StartScan(&APP_ble_scan_cb);
    if (ret != W6X_STATUS_OK)
    {
      LogError("BLE Start Scan failed, %" PRIi32 "\n", ret);
    }
    else
    {
      LogInfo("BLE Start Scan success\n");
    }
    scan_mode = true;
  }
#endif /* TEST_AUTOMATION_ENABLE */

  while (true)
  {
    /* Wait to receive a BLE event */
    eventBits = xEventGroupWaitBits(app_evt_current, EVT_APP_ALL_BIT, pdTRUE, pdFALSE, portMAX_DELAY);

    /* Process button */
    if ((eventBits & EVT_APP_BUTTON) != 0UL)
    {
      device_connected = 0;

      for (int32_t i = 0; i < W6X_BLE_MAX_CONN_NBR; i++)
      {
        if (app_ble_connected_device_list[i].IsConnected != 0x00)
        {
          /* Write p2p Characteristic of remote device */
          APP_ble_write_p2p_data(app_ble_connected_device_list[i].conn_handle);
          device_connected++;
        }
      }

      /* If no device connected, start/stop BLE scan */
      if (device_connected == 0x00)
      {
        if (!scan_mode)
        {
          /* Start scan */
          ret = W6X_Ble_StartScan(&APP_ble_scan_cb);
          if (ret != W6X_STATUS_OK)
          {
            LogError("BLE Start Scan failed, %" PRIi32 "\n", ret);
          }
          else
          {
            LogInfo("BLE Start Scan success\n");
          }
          scan_mode = true;
        }
        else
        {
          /* Stop scan */
          APP_setevent(&app_evt_current, EVT_APP_BLE_SCAN_DONE);
        }
      }
    }

    /* Process BLE passkey entry */
    if ((eventBits & EVT_APP_BLE_PASSKEY_ENTRY) != 0UL)
    {
      if (APP_ble_is_valid_conn_handle(connection_index))
      {
        /* BLE_GENERIC_PASSKEY must be replaced by remote BLE device Passkey */
        app_ble_connected_device_list[connection_index].PassKey = BLE_GENERIC_PASSKEY;
        ret = W6X_Ble_SecurityEnterRemotePassKey(connection_index,
                                                 app_ble_connected_device_list[connection_index].PassKey);
        if (ret != W6X_STATUS_OK)
        {
          LogError("Failed to set passkey, %" PRIi32 "\n", ret);
        }
        else
        {
          LogInfo("Set passkey\n");
        }
      }
    }

    /* Process BLE passkey confirm */
    if ((eventBits & EVT_APP_BLE_PASSKEY_CONFIRM) != 0UL)
    {
      if (APP_ble_is_valid_conn_handle(connection_index))
      {
        ret = W6X_Ble_SecurityPassKeyConfirm(connection_index);
        if (ret != W6X_STATUS_OK)
        {
          LogError("Failed to send passkey confirm, %" PRIi32 "\n", ret);
        }
        else
        {
          LogInfo("Sent passkey confirm\n");
        }
      }
    }

    if ((eventBits & EVT_APP_BLE_SCAN_DONE) != 0UL)
    {
      if (scan_mode)
      {
        ret = W6X_Ble_StopScan();
        scan_mode = false;
        if (ret != W6X_STATUS_OK)
        {
          LogError("BLE Stop Scan failed, %" PRIi32 "\n", ret);
        }
        else
        {
          W6X_Ble_Print_Scan(&app_ble_scan_results);
          LogInfo("BLE Stop Scan success\n");

          if (app_ble_scan_results.Detected_Peripheral != NULL)
          {
            /* If p2pServer device found, establish connection with it */
            for (uint32_t count = 0U; (count < app_ble_scan_results.Count); count++)
            {
              if (strncmp((char *)app_ble_scan_results.Detected_Peripheral[count].DeviceName,
                          "p2pS_", sizeof("p2pS_") - 1U) == 0)
              {
                LogInfo("Peer 2 Peer Server found, establish connection\n");
                vTaskDelay(pdMS_TO_TICKS(1000));
                for (uint32_t i = 0; i < W6X_BLE_MAX_CONN_NBR; i++)
                {
                  if (app_ble_connected_device_list[i].IsConnected == 0x00U)
                  {
                    ret = W6X_Ble_Connect(i, app_ble_scan_results.Detected_Peripheral[count].BDAddr);
                    break;
                  }
                }

                if (ret != W6X_STATUS_OK)
                {
                  LogError("BLE Connection failed, %" PRIi32 "\n", ret);
                }
                else
                {
                  LogInfo("BLE Connection success\n");
                  break;  /* exit loop to let the connection process to be completed */
                }
              }
            }
          }

#if (TEST_AUTOMATION_ENABLE == 1)
          /* If no connection re-launch BLE Scan */
          vTaskDelay(pdMS_TO_TICKS(2000));
          if ((app_ble_connected_device_list[0].IsConnected == 0x00U) &&
              (app_ble_connected_device_list[1].IsConnected == 0x00U))
          {
            /* Scan and connect */
            ret = W6X_Ble_StartScan(&APP_ble_scan_cb);
            if (ret != W6X_STATUS_OK)
            {
              LogError("BLE Start Scan failed, %" PRIi32 "\n", ret);
            }
            else
            {
              LogInfo("BLE Start Scan success\n");
            }
            scan_mode = true;
          }
#endif /* TEST_AUTOMATION_ENABLE */
        }
      }
    }

    /* Process BLE pairing confirm */
    if ((eventBits & EVT_APP_BLE_PAIRING_CONFIRM) != 0UL)
    {
      if (APP_ble_is_valid_conn_handle(connection_index))
      {
        ret = W6X_Ble_SecurityPairingConfirm(connection_index);
        if (ret != W6X_STATUS_OK)
        {
          LogError("Pairing Confirm Failed, %" PRIi32 "\n", ret);
        }
        else
        {
          LogInfo("Pairing Confirm\n");
        }
      }
    }

    /* Process BLE pairing complete */
    if ((eventBits & EVT_APP_BLE_PAIRING_COMPLETED) != 0UL)
    {
      LogInfo("Pairing Completed\n");
    }

    if ((eventBits & EVT_APP_BLE_DISCONNECTED) != 0UL)
    {
#if (TEST_AUTOMATION_ENABLE == 1)
      /* If no connection re-launch BLE Scan */
      vTaskDelay(pdMS_TO_TICKS(2000));
      /* Scan and connect */
      ret = W6X_Ble_StartScan(&APP_ble_scan_cb);
      if (ret != W6X_STATUS_OK)
      {
        LogError("BLE Start Scan failed, %" PRIi32 "\n", ret);
      }
      else
      {
        LogInfo("BLE Start Scan success\n");
      }
      scan_mode = true;
#endif /* TEST_AUTOMATION_ENABLE */
    }

    if ((eventBits & EVT_APP_BLE_SERVICE_FOUND) != 0UL)
    {
    }

    if ((eventBits & EVT_APP_BLE_CHAR_FOUND) != 0UL)
    {
    }

    if ((eventBits & EVT_APP_BLE_CONNECTED) != 0UL)
    {
      if (APP_ble_is_valid_conn_handle(connection_index))
      {
        /* Print BLE connection info */
        LogInfo("Connection index: %" PRIu32 ", Connection role: %s, Remote BD Addr: " ADDRSTR "\n",
                connection_index, W6X_Ble_RoleToStr(app_ble_connected_device_list[connection_index].conn_role),
                ADDR2STR(app_ble_connected_device_list[connection_index].BDAddr));

        /* Service Discovery */
        (void)W6X_Ble_RemoteServiceDiscovery(connection_index);

        /* Char discovery */
        for (uint32_t i = 0; app_ble_connected_device_list[connection_index].Service[i].service_idx != 0U; i++)
        {
          (void)W6X_Ble_RemoteCharDiscovery(connection_index,
                                            app_ble_connected_device_list[connection_index].Service[i].service_idx);

          /* Identify Peer to Peer service */
          char service_uuid_128[32 + 1];
          char charac_uuid_128[32 + 1];

          (void)snprintf(service_uuid_128, 33, UUID128STR,
                         UUID128TOSTR(app_ble_connected_device_list[connection_index].Service[i].service_uuid));
          if (strcmp(service_uuid_128, P2PSERVER_SERVICE_UUID) == 0)
          {
            W6X_Ble_Characteristic_t *charac;
            p2p_service_index[connection_index] =
              app_ble_connected_device_list[connection_index].Service[i].service_idx;

            /* Identify Peer to Peer LED and Switch characteristics */
            for (uint32_t j = 0;
                 app_ble_connected_device_list[connection_index].Service[i].charac[j].char_idx != 0U;
                 j++)
            {
              charac = &app_ble_connected_device_list[connection_index].Service[i].charac[j];
              (void)snprintf(charac_uuid_128, 33, UUID128STR, UUID128TOSTR(charac->char_uuid));
              if (strcmp(charac_uuid_128, P2PSERVER_LED_CHAR_UUID) == 0)
              {
                p2p_led_characteristic_index[connection_index] = charac->char_idx;
              }
              if (strcmp(charac_uuid_128, P2PSERVER_SWITCH_CHAR_UUID) == 0)
              {
                p2p_switch_characteristic_index[connection_index] = charac->char_idx;
                /* Subscribe to p2p Switch characteristic */
                (void)W6X_Ble_ClientSubscribeChar(connection_index, charac->char_value_handle, 1);
              }
            }
          }
        }

#if (TEST_AUTOMATION_ENABLE == 1)
        /* Write characteristic to remote p2pServer */
        vTaskDelay(pdMS_TO_TICKS(500));
        if (app_ble_connected_device_list[0].IsConnected != 0x00U)
        {
          /* Write p2p Characteristic */
          APP_ble_write_p2p_data(app_ble_connected_device_list[0].conn_handle);
        }
        if (app_ble_connected_device_list[1].IsConnected != 0x00U)
        {
          /* Write p2p Characteristic */
          APP_ble_write_p2p_data(app_ble_connected_device_list[1].conn_handle);
        }
#endif /* TEST_AUTOMATION_ENABLE */
      }
    }

    if ((eventBits & EVT_APP_BLE_NOTIFICATION_DATA) != 0UL)
    {
      APP_ble_handle_notification_event();

#if (TEST_AUTOMATION_ENABLE == 1)
      /* Write characteristic to remote p2pServer */
      vTaskDelay(pdMS_TO_TICKS(500));
      if (app_ble_connected_device_list[0].IsConnected != 0x00U)
      {
        /* Write p2p Characteristic */
        APP_ble_write_p2p_data(app_ble_connected_device_list[0].conn_handle);
      }
      if (app_ble_connected_device_list[1].IsConnected != 0x00U)
      {
        /* Write p2p Characteristic */
        APP_ble_write_p2p_data(app_ble_connected_device_list[1].conn_handle);
      }

      if ((a_APP_AvailableData[0] == P2PSERVER_QUIT_CONTROL) && (a_APP_AvailableData[1] == 0xFFU))
      {
        LogInfo(" -> Quit Automation Notification received\n");
        break;
      }
#endif /* TEST_AUTOMATION_ENABLE */
    }
  }

  /* USER CODE BEGIN main_app_Last */

  /* USER CODE END main_app_Last */

_err:
  /* USER CODE BEGIN main_app_Err_1 */

  /* USER CODE END main_app_Err_1 */
  /* De-initialize the ST67W6X BLE module */
  W6X_Ble_DeInit();

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
    APP_setevent(&app_evt_current, EVT_APP_BUTTON);
  }
  /* USER CODE BEGIN EXTI_Falling_Callback_End */

  /* USER CODE END EXTI_Falling_Callback_End */
}

/* USER CODE BEGIN FD */

/* USER CODE END FD */

/* Private Functions Definition ----------------------------------------------*/
static bool APP_ble_is_valid_conn_handle(uint32_t conn_handle)
{
  return (conn_handle < W6X_BLE_MAX_CONN_NBR);
}

static void APP_wifi_cb(W6X_event_id_t event_id, void *event_args)
{
  /* USER CODE BEGIN APP_wifi_cb_1 */

  /* USER CODE END APP_wifi_cb_1 */
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
  uint8_t service_index = 0;
  uint8_t charac_index = 0;
  W6X_Ble_Service_t *service = NULL;
  char tmp_UUID[33];
  uint8_t uuid_size = 0;

  switch (event_id)
  {
    case W6X_BLE_EVT_CONNECTED_ID:
      connection_index = p_param_ble_data->remote_ble_device.conn_handle;
      if (!APP_ble_is_valid_conn_handle(connection_index))
      {
        LogError(" -> BLE CONNECTED invalid Connection Handle = %" PRIu32 "\n", connection_index);
        break;
      }
      LogInfo(" -> BLE CONNECTED [Connection Handle: %" PRIu32 "]\n", connection_index);

      /* Fill remote device structure */
      app_ble_connected_device_list[connection_index].conn_handle = connection_index;
      app_ble_connected_device_list[connection_index].conn_role = p_param_ble_data->remote_ble_device.conn_role;
      app_ble_connected_device_list[connection_index].IsConnected = p_param_ble_data->remote_ble_device.IsConnected;
      (void)memcpy(app_ble_connected_device_list[connection_index].BDAddr, p_param_ble_data->remote_ble_device.BDAddr,
                   sizeof(app_ble_connected_device_list[connection_index].BDAddr));
      APP_setevent(&app_evt_current, EVT_APP_BLE_CONNECTED);
      break;

    case W6X_BLE_EVT_CONNECTION_PARAM_ID:
      connection_index = p_param_ble_data->remote_ble_device.conn_handle;
      if (!APP_ble_is_valid_conn_handle(connection_index))
      {
        LogError(" -> BLE CONNECTION PARAM UPDATE invalid Connection Handle = %" PRIu32 "\n", connection_index);
        break;
      }
      LogInfo(" -> BLE CONNECTION PARAM UPDATE [Connection Handle: %" PRIu32 "]\n", connection_index);
      APP_setevent(&app_evt_current, EVT_APP_BLE_CONNECTION_PARAM_UPDATE);
      break;

    case W6X_BLE_EVT_DISCONNECTED_ID:
      connection_index = p_param_ble_data->remote_ble_device.conn_handle;
      if (!APP_ble_is_valid_conn_handle(connection_index))
      {
        LogError(" -> BLE DISCONNECTED invalid Connection Handle = %" PRIu32 "\n", connection_index);
        break;
      }
      LogInfo(" -> BLE DISCONNECTED [Connection Handle: %" PRIu32 "]\n", connection_index);

      /* Reinitialize remote device struct */
      app_ble_connected_device_list[connection_index].IsConnected = 0;
      app_ble_connected_device_list[connection_index].conn_handle = 0xFFU;
      app_ble_connected_device_list[connection_index].conn_role = W6X_BLE_CONN_ROLE_UNKNOWN;
      (void)memset(app_ble_connected_device_list[connection_index].BDAddr, 0,
                   sizeof(app_ble_connected_device_list[connection_index].BDAddr));
      for (uint32_t i = 0; i < W6X_BLE_MAX_SERVICE_NBR; i++)
      {
        (void)memset(&app_ble_connected_device_list[connection_index].Service[i], 0, sizeof(W6X_Ble_Service_t));
      }
      app_ble_connected_device_list[connection_index].bd_addr_type = 0;
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
      connection_index = p_param_ble_data->remote_ble_device.conn_handle;
      if (!APP_ble_is_valid_conn_handle(connection_index))
      {
        LogError(" -> BLE NOTIFICATION invalid Connection Handle = %" PRIu32 "\n", connection_index);
        break;
      }
      LogInfo(" -> BLE NOTIFICATION DATA [Connection Handle: %" PRIu32 ", length %" PRIu32 "]\n",
              connection_index, p_param_ble_data->available_data_length);
      for (uint32_t i = 0; i < p_param_ble_data->available_data_length; i++)
      {
        LogInfo("0x%02" PRIX16 "\n", a_APP_AvailableData[i]);
      }
      APP_setevent(&app_evt_current, EVT_APP_BLE_NOTIFICATION_DATA);
      break;

    case W6X_BLE_EVT_WRITE_ID:
      LogInfo(" -> BLE WRITE.\n");
      break;

    case W6X_BLE_EVT_READ_ID:
      connection_index = p_param_ble_data->remote_ble_device.conn_handle;
      if (!APP_ble_is_valid_conn_handle(connection_index))
      {
        LogError(" -> BLE READ invalid Connection Handle = %" PRIu32 "\n", connection_index);
        break;
      }
      LogInfo(" -> READ DATA [Connection Handle: %" PRIu32 ", length %" PRIu32 "]\n",
              connection_index, p_param_ble_data->available_data_length);
      for (uint32_t i = 0; i < p_param_ble_data->available_data_length; i++)
      {
        LogInfo("0x%02" PRIX16 "\n", a_APP_AvailableData[i]);
      }
      break;

    case W6X_BLE_EVT_SERVICE_FOUND_ID:
      connection_index = p_param_ble_data->remote_ble_device.conn_handle;
      if (!APP_ble_is_valid_conn_handle(connection_index))
      {
        LogError(" -> BLE Service Discovered invalid Connection Handle = %" PRIu32 "\n", connection_index);
        break;
      }
      service_index = p_param_ble_data->Service.service_idx;
      app_ble_connected_device_list[connection_index].Service[service_index - 1U] = p_param_ble_data->Service;

      service = &app_ble_connected_device_list[connection_index].Service[service_index - 1U];
      (void)memset(tmp_UUID, 0x20, sizeof(tmp_UUID));

      uuid_size = (service->uuid_type == W6X_BLE_UUID_TYPE_16) ? 2U : 16U;
      for (uint32_t i = 0; i < uuid_size; i++)
      {
        (void)sprintf(&tmp_UUID[i * 2U], "%02" PRIx16, service->service_uuid[i]);
      }

      LogInfo(" -> BLE Service Discovered [Connection Handle = %" PRIu32 "]: Service idx = %" PRIu16 ", UUID = %s\n",
              connection_index, service->service_idx, tmp_UUID);
      APP_setevent(&app_evt_current, EVT_APP_BLE_SERVICE_FOUND);
      break;

    case W6X_BLE_EVT_CHAR_FOUND_ID:
      connection_index = p_param_ble_data->remote_ble_device.conn_handle;
      if (!APP_ble_is_valid_conn_handle(connection_index))
      {
        LogError(" -> BLE Characteristic Discovered invalid Connection Handle = %" PRIu32 "\n", connection_index);
        break;
      }
      service_index = p_param_ble_data->Service.service_idx;
      charac_index = p_param_ble_data->Service.charac[0].char_idx;
      service = &app_ble_connected_device_list[connection_index].Service[service_index - 1U];

      service->charac[charac_index - 1U] = p_param_ble_data->Service.charac[0];
      (void)memset(tmp_UUID, 0x20, sizeof(tmp_UUID));

      uuid_size = (service->charac[charac_index - 1U].uuid_type == W6X_BLE_UUID_TYPE_16) ? 2U : 16U;
      for (uint32_t i = 0; i < uuid_size; i++)
      {
        (void)sprintf(&tmp_UUID[i * 2U], "%02" PRIx16, service->charac[charac_index - 1U].char_uuid[i]);
      }

      LogInfo(" -> BLE Characteristic Discovered [Connection Handle = %" PRIu32 "]: Service idx = %" PRIu16
              ", Charac idx = %" PRIu16 ", UUID = %s\n", connection_index, service->service_idx,
              service->charac[charac_index - 1U].char_idx, tmp_UUID);
      APP_setevent(&app_evt_current, EVT_APP_BLE_CHAR_FOUND);
      break;

    case W6X_BLE_EVT_PASSKEY_ENTRY_ID:
      connection_index = p_param_ble_data->remote_ble_device.conn_handle;
      if (!APP_ble_is_valid_conn_handle(connection_index))
      {
        LogError(" -> BLE PassKey Entry invalid Connection Handle = %" PRIu32 "\n", connection_index);
        break;
      }
      LogInfo(" -> BLE PassKey Entry [Connection Handle: %" PRIu32 "]\n", connection_index);
      APP_setevent(&app_evt_current, EVT_APP_BLE_PASSKEY_ENTRY);
      break;

    case W6X_BLE_EVT_PASSKEY_CONFIRM_ID:
      connection_index = p_param_ble_data->remote_ble_device.conn_handle;
      if (!APP_ble_is_valid_conn_handle(connection_index))
      {
        LogError(" -> BLE PassKey Confirm invalid Connection Handle = %" PRIu32 "\n", connection_index);
        break;
      }
      app_ble_connected_device_list[connection_index].PassKey = p_param_ble_data->PassKey;
      LogInfo(" -> BLE PassKey received = %06" PRIu32 " [Connection Handle: %" PRIu32 "]\n",
              app_ble_connected_device_list[connection_index].PassKey, connection_index);
      APP_setevent(&app_evt_current, EVT_APP_BLE_PASSKEY_CONFIRM);
      break;

    case W6X_BLE_EVT_PAIRING_CONFIRM_ID:
      connection_index = p_param_ble_data->remote_ble_device.conn_handle;
      if (!APP_ble_is_valid_conn_handle(connection_index))
      {
        LogError(" -> BLE Pairing Confirm invalid Connection Handle = %" PRIu32 "\n", connection_index);
        break;
      }
      LogInfo(" -> BLE Pairing Confirm [Connection Handle: %" PRIu32 "]\n", connection_index);
      APP_setevent(&app_evt_current, EVT_APP_BLE_PAIRING_CONFIRM);
      break;

    case W6X_BLE_EVT_PAIRING_COMPLETED_ID:
      LogInfo(" -> BLE Pairing Complete\n");
      APP_setevent(&app_evt_current, EVT_APP_BLE_PAIRING_COMPLETED);
      break;

    case W6X_BLE_EVT_PASSKEY_DISPLAY_ID:
      LogInfo(" -> BLE PASSKEY  = %06" PRIu32 "\n", p_param_ble_data->PassKey);
      APP_setevent(&app_evt_current, EVT_APP_BLE_PASSKEY_DISPLAYED);
      break;

    case W6X_BLE_EVT_PAIRING_FAILED_ID:
      connection_index = p_param_ble_data->remote_ble_device.conn_handle;
      if (!APP_ble_is_valid_conn_handle(connection_index))
      {
        LogError(" -> BLE Pairing Failed invalid Connection Handle = %" PRIu32 "\n", connection_index);
        break;
      }
      LogInfo(" -> BLE Pairing Failed [Connection Handle: %" PRIu32 "]\n", connection_index);
      APP_setevent(&app_evt_current, EVT_APP_BLE_PAIRING_FAILED);
      break;

    case W6X_BLE_EVT_PAIRING_CANCELED_ID:
      connection_index = p_param_ble_data->remote_ble_device.conn_handle;
      if (!APP_ble_is_valid_conn_handle(connection_index))
      {
        LogError(" -> BLE Pairing Canceled invalid Connection Handle = %" PRIu32 "\n", connection_index);
        break;
      }
      LogInfo(" -> BLE Pairing Canceled [Connection Handle: %" PRIu32 "]\n", connection_index);
      APP_setevent(&app_evt_current, EVT_APP_BLE_PAIRING_CANCELED);
      break;

    default:
      /* BLE events unmanaged */
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

static void APP_ble_handle_notification_event(void)
{
  /* USER CODE BEGIN APP_Ble_Evt_Notif_1 */

  /* USER CODE END APP_Ble_Evt_Notif_1 */
  if (a_APP_AvailableData[0] == P2PSERVER_LED_CONTROL)
  {
    if (a_APP_AvailableData[1] == 0x01U)
    {
      HAL_GPIO_WritePin(LED_GREEN_GPIO_Port, LED_GREEN_Pin, GPIO_PIN_SET);
    }
    else if (a_APP_AvailableData[1] == 0x00U)
    {
      HAL_GPIO_WritePin(LED_GREEN_GPIO_Port, LED_GREEN_Pin, GPIO_PIN_RESET);
    }
    else
    {
      /* Invalid LED control command */
    }
  }
  return;
  /* USER CODE BEGIN APP_Ble_Evt_Notif_End */

  /* USER CODE END APP_Ble_Evt_Notif_End */
}

static void APP_ble_scan_cb(W6X_Ble_Scan_Result_t *Ble_scan_results)
{
  /* USER CODE BEGIN APP_ble_scan_cb_1 */

  /* USER CODE END APP_ble_scan_cb_1 */
  LogInfo(" Cb informed APP that BLE SCAN DONE.\n");
  app_ble_scan_results.Count = 0;
  if ((Ble_scan_results->Count == 0U) || (Ble_scan_results->Detected_Peripheral == NULL))
  {
    LogInfo("No scan results\n");
  }
  else
  {
    if (app_ble_scan_results.Detected_Peripheral != NULL)
    {
      vPortFree(app_ble_scan_results.Detected_Peripheral);
    }
    app_ble_scan_results.Detected_Peripheral = pvPortMalloc(sizeof(W6X_Ble_Device_t) * Ble_scan_results->Count);
    if (app_ble_scan_results.Detected_Peripheral == NULL)
    {
      LogError("Memory allocation failed\n");
      return;
    }

    (void)memset(app_ble_scan_results.Detected_Peripheral, 0, sizeof(W6X_Ble_Device_t) * Ble_scan_results->Count);

    for (uint32_t count = 0; count < Ble_scan_results->Count; count++)
    {
      app_ble_scan_results.Detected_Peripheral[count].RSSI = Ble_scan_results->Detected_Peripheral[count].RSSI;
      (void)memcpy(app_ble_scan_results.Detected_Peripheral[count].BDAddr,
                   Ble_scan_results->Detected_Peripheral[count].BDAddr,
                   sizeof(app_ble_scan_results.Detected_Peripheral[count].BDAddr));
      (void)memcpy(app_ble_scan_results.Detected_Peripheral[count].DeviceName,
                   Ble_scan_results->Detected_Peripheral[count].DeviceName,
                   sizeof(app_ble_scan_results.Detected_Peripheral[count].DeviceName));
      (void)memcpy(app_ble_scan_results.Detected_Peripheral[count].ManufacturerData,
                   Ble_scan_results->Detected_Peripheral[count].ManufacturerData,
                   sizeof(app_ble_scan_results.Detected_Peripheral[count].ManufacturerData));
    }
    app_ble_scan_results.Count = Ble_scan_results->Count;
    APP_setevent(&app_evt_current, EVT_APP_BLE_SCAN_DONE);
  }
  /* USER CODE BEGIN APP_ble_scan_cb_End */

  /* USER CODE END APP_ble_scan_cb_End */
}

static void APP_ble_write_p2p_data(uint8_t conn_handle)
{
  /* USER CODE BEGIN APP_ble_write_p2p_data_1 */

  /* USER CODE END APP_ble_write_p2p_data_1 */
  W6X_Status_t ret;
  uint32_t SentDatalen;
  uint8_t data[3] = {0};

  if (!APP_ble_is_valid_conn_handle(conn_handle))
  {
    LogError("BLE Write P2P Data skipped: invalid Connection Handle = %" PRIu32 "\n", conn_handle);
    return;
  }

  if (app_ble_connected_device_list[conn_handle].IsConnected == 0U)
  {
    LogError("BLE Write P2P Data skipped: Connection Handle = %" PRIu32 " not connected\n", conn_handle);
    return;
  }

  data[0] = P2PSERVER_LED_CONTROL;
  if (OnOffSwitch[conn_handle] == 0)
  {
    data[1] = 0x01;
    OnOffSwitch[conn_handle] = 1;
  }
  else
  {
    OnOffSwitch[conn_handle] = 0;
  }

  ret = W6X_Ble_ClientWriteData(conn_handle,
                                p2p_service_index[conn_handle],
                                p2p_led_characteristic_index[conn_handle],
                                data, 2U, &SentDatalen, 6000U);
  if (ret == W6X_STATUS_OK)
  {
    LogInfo("Write P2P Data DONE\n");
  }
  else
  {
    LogError("Write P2P Data FAILED\n");
  }

  return;
  /* USER CODE BEGIN BLE_Write_P2P_Data_End */

  /* USER CODE END BLE_Write_P2P_Data_End */
}

static void APP_setevent(EventGroupHandle_t *app_event, uint32_t evt)
{
  /* USER CODE BEGIN APP_setevent_1 */

  /* USER CODE END APP_setevent_1 */
  BaseType_t xHigherPriorityTaskWoken = pdFALSE;
#if ((tskKERNEL_VERSION_MAJOR > 10) || ((tskKERNEL_VERSION_MAJOR == 10) && (tskKERNEL_VERSION_MINOR >= 6)))
  if (xPortIsInsideInterrupt())
#else
  if (__get_IPSR() != 0U)
#endif /* tskKERNEL_VERSION_MAJOR */
  {
    (void)xEventGroupSetBitsFromISR(*app_event, evt, &xHigherPriorityTaskWoken);
    if (xHigherPriorityTaskWoken)
    {
      portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
  }
  else
  {
    (void)xEventGroupSetBits(*app_event, evt);
  }
  /* USER CODE BEGIN APP_setevent_End */

  /* USER CODE END APP_setevent_End */
}

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
