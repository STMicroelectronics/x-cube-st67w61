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

/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Global variables ----------------------------------------------------------*/
/* USER CODE BEGIN GV */

/* USER CODE END GV */

/* Private typedef -----------------------------------------------------------*/
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
/** Event when BLE battery level is received */
#define EVT_APP_BLE_BAS_LEVEL                       (1<<21)

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
                                 EVT_APP_BLE_BAS_LEVEL)

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

/** P2P Server Switch Characteristic value to switch on */
#define NOTIFICATION_DATA_ON                "\001\001"
/** P2P Server Switch Characteristic value to switch off */
#define NOTIFICATION_DATA_OFF               "\001\0"

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

/** Advertising Data */
char a_AdvData[36] =
{
  '0', 'F', /* Manuf data length */
  'F', 'F', /* Manuf data Flag */
  '3', '0', '0', '0', /*  */
  '0', '2', /* Blue ST SDK v2  */
  '9', 'A', /* Board ID */
  '8', '3', /* FW ID */
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

/** OnOff Switch value */
int32_t OnOffSwitch = 0;

/** Application event group */
EventGroupHandle_t app_evt_current;

/** BLE parameters */
APP_Ble_Data_t app_ble_params =
{
  /* Initialize remote device struct */
  .remote_ble_device.RSSI = 0,
  .remote_ble_device.IsConnected = 0,
  .remote_ble_device.conn_handle = 0xff,
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
    .service_index = 0,
    .char_index = 0,
    .char_uuid = P2PSERVER_LED_CHAR_UUID,
    .uuid_type = W6X_BLE_UUID_TYPE_128,
    .char_property = W6X_BLE_CHAR_PROP_WRITE_WITHOUT_RESP,
    .char_permission = W6X_BLE_CHAR_PERM_WRITE,
    .desc = "BLE P2PServer LED charac"
  },
  {
    .service_index = 0,
    .char_index = 1,
    .char_uuid = P2PSERVER_SWITCH_CHAR_UUID,
    .uuid_type = W6X_BLE_UUID_TYPE_128,
    .char_property = W6X_BLE_CHAR_PROP_NOTIFY,
    .char_permission = W6X_BLE_CHAR_PERM_READ,
    .desc = "BLE P2PServer SWITCH charac"
  },
};

/** List of bonded BLE devices */
W6X_Ble_Bonded_Devices_Result_t app_ble_BondedDeviceList = {0};

/** Application information */
static const APP_Info_t app_info =
{
  .name = "ST67W6X BLE P2P Server",
  .version = HOST_APP_VERSION_STR
};

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/**
  * @brief  Set event group to release the waiting task
  * @param  app_event: Event group
  * @param  evt: Event to set
  */
static void APP_setevent(EventGroupHandle_t *app_event, uint32_t evt);

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
  * @brief  BLE Send Notification
  */
static void APP_Ble_Evt_Write(void);

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
  int32_t data_index = 0;
  uint8_t bd_address[W6X_BLE_BD_ADDR_SIZE];
  const char hex_chars[] = "0123456789ABCDEF";
  char ble_device_name[W6X_BLE_DEVICE_NAME_SIZE];

  /* USER CODE BEGIN main_app_1 */

  /* USER CODE END main_app_1 */

  LowPowerManagerInit();

  /* Initialize the logging utilities */
  LoggingInit();
  /* Initialize the shell utilities on UART instance */
  ShellInit();

  LogInfo("#### Welcome to %s Application #####\n", app_info.name);
  LogInfo("# build: %s %s\n", __TIME__, __DATE__);
  LogInfo("--------------- Host info ---------------\n");
  LogInfo("Host FW Version:          %s\n", app_info.version);

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
    LogError("failed to initialize ST67W6X Driver, %" PRIi32 "\n", ret);
    goto _err;
  }

  /* Initialize the ST67W6X BLE module */
  ret = W6X_Ble_Init(W6X_BLE_MODE_SERVER, a_APP_AvailableData, sizeof(a_APP_AvailableData) - 1);
  if (ret)
  {
    LogError("failed to initialize ST67W6X BLE component, %" PRIi32 "\n", ret);
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

  sprintf(ble_device_name, W6X_BLE_HOSTNAME "_%02" PRIX16, bd_address[5]);
  ret = W6X_Ble_SetDeviceName(ble_device_name);
  if (ret)
  {
    LogError("failed to set device name, %" PRIi32 "\n", ret);
    goto _err;
  }

  /* Configure the ST67W6X BLE module
   * - Set the Tx power to 0 (minimal)
   * - Set the Bluetooth Device Address
   * - Set the Advertising data
   */
  LogInfo("Configure BLE\n");
  ret = W6X_Ble_SetTxPower(0);
  if (ret)
  {
    LogError("failed to set TX power, %" PRIi32 "\n", ret);
    goto _err;
  }

  /* Fill adv data with BD Address */
  while (data_index < 6)
  {
    a_AdvData[20 + (2 * data_index)] = hex_chars[(bd_address[data_index] & 0xF0) >> 4];
    a_AdvData[20 + (2 * data_index) + 1] = hex_chars[bd_address[data_index] & 0x0F];
    data_index++;
  }
  ret = W6X_Ble_SetAdvData((const char *) a_AdvData);
  if (ret)
  {
    LogError("failed to set ADV data, %" PRIi32 "\n", ret);
    goto _err;
  }
  LogInfo("BLE configuration is done\n");

  /* Create the BLE Service */
  LogInfo("\nBLE Service Creation\n");
  ret = W6X_Ble_CreateService(0, P2PSERVER_SERVICE_UUID, W6X_BLE_UUID_TYPE_128);

  if (ret != 0)
  {
    LogError("failed to Create service, %" PRIi32 "\n", ret);
    goto _err;
  }
  LogInfo("- BLE service created\n");

  /* Create the BLE Characteristics */
  for (uint8_t i = 0; i < sizeof(app_ble_char) / sizeof(APP_Ble_Char_t); i++)
  {
    ret = W6X_Ble_CreateCharacteristic(app_ble_char[i].service_index, app_ble_char[i].char_index,
                                       app_ble_char[i].char_uuid, app_ble_char[i].uuid_type,
                                       app_ble_char[i].char_property, app_ble_char[i].char_permission);

    if (ret != 0)
    {
      LogError("failed to create %s, %" PRIi32 "\n", app_ble_char[i].desc, ret);
      goto _err;
    }
    LogInfo("- %s created\n", app_ble_char[i].desc);
  }

  /* Register the BLE Characteristics */
  ret = W6X_Ble_RegisterCharacteristics();

  if (ret != 0)
  {
    LogError("failed to Register characteristics, %" PRIi32 "\n", ret);
    goto _err;
  }
  LogInfo("- BLE services and charac registered\n");
  LogInfo("BLE service and charac creation is done\n");

  /* Setup security mode */
  ret = W6X_Ble_SetSecurityParam(W6X_BLE_SEC_IO_KEYBOARD_DISPLAY);
  if (ret)
  {
    LogError("BLE Set Security Parameters failed, %" PRIi32 "\n", ret);
    goto _err;
  }
  LogInfo("Set BLE security parameters\n");

  /* Start the BLE Advertising mode: the ST67W6X can be detected as BLE device */
  LogInfo("\nStart BLE advertising\n");
  ret = W6X_Ble_AdvStart();
  if (ret)
  {
    LogError("failed to start advertising, %" PRIi32 "\n", ret);
    goto _err;
  }

  LogInfo("BLE advertising is started\n");

  while (1)
  {
    /* Wait to receive a BLE event */
    eventBits = xEventGroupWaitBits(app_evt_current, EVT_APP_ALL_BIT, pdTRUE, pdFALSE, portMAX_DELAY);

    /* Process button */
    if (eventBits & EVT_APP_BUTTON)
    {
      if (app_ble_params.remote_ble_device.IsConnected != 0x00)
      {
        BLE_Send_Notification();
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
          if (ret)
          {
            LogError("failed to unpair bonded devices, %" PRIi32 "\n", ret);
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
    }

    /* Process BLE read */
    if (eventBits & EVT_APP_BLE_READ)
    {
    }

    /* Process BLE write */
    if (eventBits & EVT_APP_BLE_WRITE)
    {
      APP_Ble_Evt_Write();

#if (TEST_AUTOMATION_ENABLE == 1)
      /* Send notification to p2p Client board */
      vTaskDelay(500);
      BLE_Send_Notification();
#endif /* TEST_AUTOMATION_ENABLE */
    }

    /* Process BLE passkey entry */
    if (eventBits & EVT_APP_BLE_PASSKEY_ENTRY)
    {
      app_ble_params.PassKey = BLE_GENERIC_PASSKEY;
      if (app_ble_params.remote_ble_device.conn_handle != 0xff)
      {
        ret = W6X_Ble_SecuritySetPassKey(app_ble_params.remote_ble_device.conn_handle, app_ble_params.PassKey);
        if (ret)
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
        if (ret)
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
        if (ret)
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

  /* De-initialize the ST67W6X Driver */
  W6X_DeInit();

  /* USER CODE BEGIN main_app_Err_2 */

  /* USER CODE END main_app_Err_2 */
  LogInfo("##### Application end\n");
}

void BLE_Send_Notification(void)
{
  int32_t ret;
  uint32_t SentDatalen;
  /* USER CODE BEGIN BLE_Send_Notification_1 */

  /* USER CODE END BLE_Send_Notification_1 */

  LogInfo("BLE Send Notification\n");
  if (OnOffSwitch == 0)
  {
    ret = W6X_Ble_ServerSendNotification(0, 1, NOTIFICATION_DATA_ON, strlen(NOTIFICATION_DATA_ON),
                                         &SentDatalen, 6000);
    OnOffSwitch = 1;
  }
  else
  {
    ret = W6X_Ble_ServerSendNotification(0, 1, NOTIFICATION_DATA_OFF, strlen(NOTIFICATION_DATA_ON),
                                         &SentDatalen, 6000);
    OnOffSwitch = 0;
  }
  if (ret != 0)
  {
    LogError("Send Notification FAILED, %" PRIi32 "\n", ret);
  }
  return;
  /* USER CODE BEGIN BLE_Send_Notification_End */

  /* USER CODE END BLE_Send_Notification_End */
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

  switch (event_id)
  {
    case W6X_BLE_EVT_CONNECTED_ID:
      LogInfo(" -> BLE CONNECTED: Conn_Handle: %" PRIu16 "\n", p_param_ble_data->remote_ble_device.conn_handle);

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
      LogInfo(" -> BLE CONNECTION PARAM UPDATE\n");
      APP_setevent(&app_evt_current, EVT_APP_BLE_CONNECTION_PARAM_UPDATE);
      break;

    case W6X_BLE_EVT_DISCONNECTED_ID:
      LogInfo(" -> BLE DISCONNECTED.\n");

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
      LogInfo(" -> BLE NOTIFICATION.\n");
      APP_setevent(&app_evt_current, EVT_APP_BLE_NOTIFICATION_DATA);
      break;

    case W6X_BLE_EVT_WRITE_ID:
      LogInfo(" -> BLE WRITE.\n");
      app_ble_params.remote_ble_device.conn_handle = p_param_ble_data->remote_ble_device.conn_handle;
      app_ble_params.service_idx = p_param_ble_data->service_idx;
      app_ble_params.charac_idx = p_param_ble_data->charac_idx;
      app_ble_params.available_data_length = p_param_ble_data->available_data_length;
      LogInfo(" -> Conn_Handle: %" PRIu16 ", Service: %" PRIu16 ", Charac: %" PRIu16 ", length %" PRIu32 ", Data:\n",
              p_param_ble_data->remote_ble_device.conn_handle, p_param_ble_data->service_idx,
              p_param_ble_data->charac_idx, p_param_ble_data->available_data_length);

      for (uint32_t i = 0; i < p_param_ble_data->available_data_length; i++)
      {
        LogInfo("0x%02" PRIX16 "\n", a_APP_AvailableData[i]);
      }

      APP_setevent(&app_evt_current, EVT_APP_BLE_WRITE);
      break;

    case W6X_BLE_EVT_READ_ID:
      LogInfo(" -> BLE READ.\n");
      APP_setevent(&app_evt_current, EVT_APP_BLE_READ);
      break;

    case W6X_BLE_EVT_PASSKEY_ENTRY_ID:
      app_ble_params.remote_ble_device.conn_handle = p_param_ble_data->remote_ble_device.conn_handle;
      LogInfo(" -> BLE PassKey Entry: Conn_Handle: %" PRIu16 "\n", app_ble_params.remote_ble_device.conn_handle);
      APP_setevent(&app_evt_current, EVT_APP_BLE_PASSKEY_ENTRY);
      break;

    case W6X_BLE_EVT_PASSKEY_CONFIRM_ID:
      app_ble_params.remote_ble_device.conn_handle = p_param_ble_data->remote_ble_device.conn_handle;
      app_ble_params.PassKey = p_param_ble_data->PassKey;
      LogInfo(" -> BLE PassKey received = %06" PRIu32 ", Conn_Handle: %" PRIu16 "\n", app_ble_params.PassKey,
              app_ble_params.remote_ble_device.conn_handle);
      APP_setevent(&app_evt_current, EVT_APP_BLE_PASSKEY_CONFIRM);
      break;

    case W6X_BLE_EVT_PAIRING_CONFIRM_ID:
      app_ble_params.remote_ble_device.conn_handle = p_param_ble_data->remote_ble_device.conn_handle;
      LogInfo(" -> BLE Pairing Confirm: Conn_Handle: %" PRIu16 "\n", app_ble_params.remote_ble_device.conn_handle);
      APP_setevent(&app_evt_current, EVT_APP_BLE_PAIRING_CONFIRM);
      break;

    case W6X_BLE_EVT_PAIRING_COMPLETED_ID:
      LogInfo(" -> BLE Pairing Complete\n\n");
      APP_setevent(&app_evt_current, EVT_APP_BLE_PAIRING_COMPLETED);
      break;

    case W6X_BLE_EVT_PASSKEY_DISPLAY_ID:
      app_ble_params.PassKey = p_param_ble_data->PassKey;
      LogInfo(" -> BLE PASSKEY  = %06" PRIu32 "\n", app_ble_params.PassKey);
      APP_setevent(&app_evt_current, EVT_APP_BLE_PASSKEY_DISPLAYED);
      break;

    case W6X_BLE_EVT_PAIRING_FAILED_ID:
      LogInfo(" -> BLE Pairing Failed: Conn_Handle: %" PRIu16 "\n", app_ble_params.remote_ble_device.conn_handle);
      APP_setevent(&app_evt_current, EVT_APP_BLE_PAIRING_FAILED);
      break;

    case W6X_BLE_EVT_PAIRING_CANCELED_ID:
      LogInfo(" -> BLE Pairing Canceled: Conn_Handle: %" PRIu16 "\n", app_ble_params.remote_ble_device.conn_handle);
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

static void APP_Ble_Evt_Write(void)
{
  /* USER CODE BEGIN APP_Ble_Evt_Write_1 */

  /* USER CODE END APP_Ble_Evt_Write_1 */
  if (a_APP_AvailableData[1] == 0x01)
  {
    HAL_GPIO_WritePin(LED_GREEN_GPIO_Port, LED_GREEN_Pin, GPIO_PIN_SET);
  }
  else if (a_APP_AvailableData[1] == 0x00)
  {
    HAL_GPIO_WritePin(LED_GREEN_GPIO_Port, LED_GREEN_Pin, GPIO_PIN_RESET);
  }
  return;
  /* USER CODE BEGIN APP_Ble_Evt_Write_End */

  /* USER CODE END APP_Ble_Evt_Write_End */
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
