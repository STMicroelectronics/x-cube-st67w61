/**
  ******************************************************************************
  * @file    w61_at_sys.c
  * @author  GPM Application Team
  * @brief   This file provides code for W61 System AT module
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

/* Includes ------------------------------------------------------------------*/
#include <inttypes.h>
#include <stdio.h>
#include "w61_at_api.h"
#include "w61_at_common.h"
#include "w61_at_internal.h"
#include "w61_io.h" /* SPI_XFER_MTU_BYTES */
#include "common_parser.h" /* Common Parser functions */
#include "cmsis_compiler.h" /* __CLZ */

#if (SYS_DBG_ENABLE_TA4 >= 1)
#include "trcRecorder.h"
#endif /* SYS_DBG_ENABLE_TA4 */

/* Private typedef -----------------------------------------------------------*/
/** @addtogroup ST67W61_AT_System_Types
  * @{
  */

/**
  * @brief  Version information structure
  */
typedef struct
{
  char *dest;               /*!< Destination buffer */
  const char *prefix;       /*!< Prefix string */
  size_t dest_size;         /*!< Destination buffer size */
} W61_VersionInfo_t;

/**
  * @brief  EFuse structure
  */
typedef struct
{
  uint32_t efuse_addr;      /*!< EFUSE Address */
  uint32_t efuse_len;       /*!< EFUSE Length */
  uint8_t *data;            /*!< Data buffer */
  const char *desc;         /*!< EFUSE Description */
} W61_Efuse_t;

/**
  * @brief  Trim structure
  */
typedef struct
{
  uint32_t en_addr;         /*!< EFUSE Trim Enable Address */
  uint32_t en_offset;       /*!< EFUSE Trim Enable Offset */
  uint32_t value_addr;      /*!< EFUSE Trim Value Address */
  uint32_t value_offset;    /*!< EFUSE Trim Value Offset */
  uint32_t value_len;       /*!< EFUSE Trim Value Length */
  uint32_t parity_addr;     /*!< EFUSE Trim Parity Address */
  uint32_t parity_offset;   /*!< EFUSE Trim Parity Offset */
  uint32_t type;            /*!< Trim type */
  const char *desc;         /*!< Trim Description */
} W61_Trim_t;

/** @} */

/* Private defines -----------------------------------------------------------*/
/** @addtogroup ST67W61_AT_System_Constants
  * @{
  */

#define EFUSE_DEFAULT_MAC_ADDR_ADDR       (0x14)      /*!< EFUSE Address of default MAC address */
#define EFUSE_DEFAULT_MAC_ADDR_LEN        (7)         /*!< EFUSE Length of default MAC address */

#define EFUSE_CUSTOM_MAC_ADDR_1_ADDR      (0x64)      /*!< EFUSE Address of customer MAC address 1 */
#define EFUSE_CUSTOM_MAC_ADDR_1_LEN       (7)         /*!< EFUSE Length of customer MAC address 1 */

#define EFUSE_CUSTOM_MAC_ADDR_2_ADDR      (0x70)      /*!< EFUSE Address of customer MAC address 2 */
#define EFUSE_CUSTOM_MAC_ADDR_2_LEN       (7)         /*!< EFUSE Length of customer MAC address 2 */

#define EFUSE_ANTI_ROLL_BACK_EN_ADDR      (0x7C)      /*!< EFUSE Address of Anti-Rollback enable */
#define EFUSE_ANTI_ROLL_BACK_EN_LEN       (4)         /*!< EFUSE Length of Anti-Rollback enable */

#define EFUSE_PART_NUMBER_ADDR            (0x100)     /*!< EFUSE Address of Part Number */
#define EFUSE_PART_NUMBER_LEN             (24)        /*!< EFUSE Length of Part Number */

#define EFUSE_MANUF_BOM_ADDR              (0x118)     /*!< EFUSE Address of BOM ID + Manufacturing info */
#define EFUSE_MANUF_BOM_LEN               (4)         /*!< EFUSE Length of BOM ID + Manufacturing info */

#define EFUSE_BOOT2_ANTI_ROLL_BACK_ADDR   (0x170)     /*!< EFUSE Address of Boot2 Anti-Rollback counter */
#define EFUSE_BOOT2_ANTI_ROLL_BACK_LEN    (16)        /*!< EFUSE Length of Boot2 Anti-Rollback counter */

#define EFUSE_APP_ANTI_ROLL_BACK_ADDR     (0x180)     /*!< EFUSE Address of Application Anti-Rollback counter */
#define EFUSE_APP_ANTI_ROLL_BACK_LEN      (32)        /*!< EFUSE Length of Application Anti-Rollback counter */

#define CLOCK_TIMEOUT                     (4000U)     /*!< Clock timeout in ms */

/** @} */

/* Private macros ------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/** @defgroup ST67W61_AT_System_Variables ST67W61 AT Driver System Variables
  * @ingroup  ST67W61_AT_System
  * @{
  */

/** Global W61 context */
W61_Object_t W61_Obj = {0};

#if (W61_ASSERT_ENABLE == 1)
/** W61 context pointer error string */
static const char W61_Obj_Null_str[] = "Object pointer not initialized";
#endif /* W61_ASSERT_ENABLE */

/** EFUSE MAC Addresses table */
static const W61_Efuse_t efuse_mac_table[] =
{
  {EFUSE_DEFAULT_MAC_ADDR_ADDR, EFUSE_DEFAULT_MAC_ADDR_LEN, NULL, "Default MAC Address"},
  {EFUSE_CUSTOM_MAC_ADDR_1_ADDR, EFUSE_CUSTOM_MAC_ADDR_1_LEN, NULL, "Custom MAC Address 1"},
  {EFUSE_CUSTOM_MAC_ADDR_2_ADDR, EFUSE_CUSTOM_MAC_ADDR_2_LEN, NULL, "Custom MAC Address 2"},
};

/** EFUSE RF and XTAL Trimming table */
static const W61_Trim_t trim_table[] =
{
  /* Wi-Fi High Power Trimming */
  {0xCC, 26, 0xC0, 0,  15, 0xC0, 15, 0, "wifi_hp_poffset0"},
  {0xCC, 27, 0xC0, 16, 15, 0xC0, 31, 0, "wifi_hp_poffset1"},
  {0xCC, 28, 0xC4, 0,  15, 0xC4, 15, 0, "wifi_hp_poffset2"},
  /* Wi-Fi Low Power Trimming */
  {0xCC, 29, 0xC4, 16, 15, 0xC4, 31, 1, "wifi_lp_poffset0"},
  {0xCC, 30, 0xC8, 0,  15, 0xC8, 15, 1, "wifi_lp_poffset1"},
  {0xCC, 31, 0xC8, 16, 15, 0xC8, 31, 1, "wifi_lp_poffset2"},
  /* BLE Trimming */
  {0xD0, 26, 0xCC, 0,  25, 0xCC, 25, 2, "ble_poffset0"},
  {0xD0, 27, 0xD0, 0,  25, 0xD0, 25, 2, "ble_poffset1"},
  {0xD0, 28, 0xD4, 0,  25, 0xD4, 25, 2, "ble_poffset2"},
  /* XTAL Trimming */
  {0xEC, 7,  0XEC, 0,  6,  0XEC, 6,  3, "xtal0"},
  {0xF0, 31, 0XF4, 26, 6,  0XF0, 30, 3, "xtal1"},
  {0xEC, 23, 0XF4, 20, 6,  0XF0, 28, 3, "xtal2"},
};

/** Part Number description per module ID table */
static const W61_ModuleID_t module_id[] =
{
  {W61_MODULE_ID_B, "C6AFDBD111400004"},
  {W61_MODULE_ID_U, "72AFDBD110400005"},
  {W61_MODULE_ID_P, "A6AFDBD110400006"},
};

/** @} */

/* Private function prototypes -----------------------------------------------*/
/** @addtogroup ST67W61_AT_System_Functions
  * @{
  */

/**
  * @brief  Callback function to handle Module AT version responses
  * @param  data: pointer to the modem_cmd_handler_data structure
  * @param  len: length of the data
  * @param  argv: array of argument strings
  * @param  argc: number of argument
  * @return 0 on success, negative value on error
 */
MODEM_CMD_DECLARE(on_cmd_at_version);

/**
  * @brief  Callback function to handle Module Wi-Fi MAC SW version responses
  * @param  data: pointer to the modem_cmd_handler_data structure
  * @param  len: length of the data
  * @param  argv: array of argument strings
  * @param  argc: number of argument
  * @return 0 on success, negative value on error
 */
MODEM_CMD_DECLARE(on_cmd_mac_sw_version);

/**
  * @brief  Callback function to handle Module SDK version responses
  * @param  data: pointer to the modem_cmd_handler_data structure
  * @param  len: length of the data
  * @param  argv: array of argument strings
  * @param  argc: number of argument
  * @return 0 on success, negative value on error
 */
MODEM_CMD_DECLARE(on_cmd_sdk_version);

/**
  * @brief  Callback function to handle Module BLE Controller version responses
  * @param  data: pointer to the modem_cmd_handler_data structure
  * @param  len: length of the data
  * @param  argv: array of argument strings
  * @param  argc: number of argument
  * @return 0 on success, negative value on error
 */
MODEM_CMD_DECLARE(on_cmd_bt_controller_version);

/**
  * @brief  Callback function to handle Module BLE Stack version responses
  * @param  data: pointer to the modem_cmd_handler_data structure
  * @param  len: length of the data
  * @param  argv: array of argument strings
  * @param  argc: number of argument
  * @return 0 on success, negative value on error
 */
MODEM_CMD_DECLARE(on_cmd_bt_stack_version);

/**
  * @brief  Callback function to handle read efuse responses
  * @param  data: pointer to the modem_cmd_handler_data structure
  * @param  len: length of the data
  * @param  argv: array of argument strings (unused)
  * @param  argc: number of argument (unused)
  * @return 0 on success, negative value on error
 */
MODEM_CMD_DIRECT_DECLARE(on_cmd_read_efuse);

/**
  * @brief  Callback function to handle list files responses
  * @param  data: pointer to the modem_cmd_handler_data structure
  * @param  len: length of the data
  * @param  argv: array of argument strings
  * @param  argc: number of argument
  * @return 0 on success, negative value on error
 */
MODEM_CMD_DECLARE(on_cmd_fs_listfiles);

/**
  * @brief  Callback function to handle read file content responses
  * @param  data: pointer to the modem_cmd_handler_data structure
  * @param  len: length of the data
  * @param  argv: array of argument strings (unused)
  * @param  argc: number of argument (unused)
  * @return 0 on success, negative value on error
 */
MODEM_CMD_DIRECT_DECLARE(on_cmd_fs_readfile);

/**
  * @brief  Callback function to handle AT command responses
  * @param  data: pointer to the modem_cmd_handler_data structure
  * @param  len: length of the data
  * @param  argv: array of argument strings
  * @param  argc: number of argument
  * @return 0 on success, negative value on error
 */
MODEM_CMD_DECLARE(on_cmd_atcmd);

/* Functions Definition ------------------------------------------------------*/
W61_Object_t *W61_ObjGet(void)
{
  return &W61_Obj;
}

W61_Status_t W61_RegisterULcb(W61_Object_t *Obj,
                              W61_UpperLayer_wifi_sta_cb_t   UL_wifi_sta_cb,
                              W61_UpperLayer_wifi_ap_cb_t    UL_wifi_ap_cb,
                              W61_UpperLayer_net_cb_t        UL_net_cb,
                              W61_UpperLayer_mqtt_cb_t       UL_mqtt_cb,
                              W61_UpperLayer_ble_cb_t        UL_ble_cb)
{
  if (!Obj)
  {
    return W61_STATUS_ERROR;
  }

  if (UL_wifi_sta_cb)
  {
    Obj->ulcbs.UL_wifi_sta_cb = UL_wifi_sta_cb;
  }
  if (UL_wifi_ap_cb)
  {
    Obj->ulcbs.UL_wifi_ap_cb = UL_wifi_ap_cb;
  }
  if (UL_net_cb)
  {
    Obj->ulcbs.UL_net_cb = UL_net_cb;
  }
  if (UL_mqtt_cb)
  {
    Obj->ulcbs.UL_mqtt_cb = UL_mqtt_cb;
  }
  if (UL_ble_cb)
  {
    Obj->ulcbs.UL_ble_cb = UL_ble_cb;
  }

  return W61_STATUS_OK;
}

W61_Status_t W61_Init(W61_Object_t *Obj)
{
  W61_Status_t ret;
  W61_NULL_ASSERT_STR(Obj, W61_Obj_Null_str);

  /* Initialize the Modem handler
     Start the SPI driver
     Boot the ST67W611M module */
  ret = (W61_Status_t) W61_AT_ModemInit(Obj);
  if (ret != W61_STATUS_OK)
  {
    SYS_LOG_ERROR("Could not init Modem handler\n");
    goto __error;
  }

__error:
  return ret;
}

W61_Status_t W61_DeInit(W61_Object_t *Obj)
{
  W61_NULL_ASSERT_STR(Obj, W61_Obj_Null_str);
  /* Deinitialize the Modem handler
     Shutdown the ST67W611M module
     Stop the SPI driver */
  W61_AT_ModemDeInit(Obj);
  return W61_STATUS_OK;
}

W61_Status_t W61_WaitForReady(W61_Object_t *Obj, uint32_t time_ms)
{
  BaseType_t xReturned;
  struct modem *mdm = (struct modem *) &Obj->Modem;
  W61_NULL_ASSERT_STR(Obj, W61_Obj_Null_str);

  /* Wait for the module to be ready */
  xReturned = xSemaphoreTake(mdm->sem_if_ready, pdMS_TO_TICKS(time_ms));
  if (xReturned != pdPASS)
  {
    SYS_LOG_ERROR("sem_if_ready not received\n");
    return W61_STATUS_ERROR;
  }
  else
  {
    /** Need to wait after catching ready event to ensure module is fully operational .*/
    vTaskDelay(pdMS_TO_TICKS(100));
    return W61_STATUS_OK;
  }
}

W61_Status_t W61_ResetToFactoryDefault(W61_Object_t *Obj)
{
  W61_Status_t ret;
  W61_NULL_ASSERT_STR(Obj, W61_Obj_Null_str);

  /* Send the factory reset command */
  ret = W61_AT_Common_SetExecute(Obj, (uint8_t *)"AT+RESTORE\r\n", 2000);
  if (ret != W61_STATUS_OK)
  {
    SYS_LOG_ERROR("Restore factory default failed\n");
    goto __err;
  }

  /* Wait for the module to be ready */
  ret = W61_WaitForReady(Obj, W61_READY_DEFAULT_TIMEOUT_MS);
  if (ret != W61_STATUS_OK)
  {
    SYS_LOG_ERROR("sem_if_ready not received\n");
    goto __err;
  }

  /* Send an AT command to check the module is available */
  ret = W61_AT_Common_SetExecute(Obj, (uint8_t *)"AT\r\n", W61_NCP_TIMEOUT);
  if (ret != W61_STATUS_OK)
  {
    SYS_LOG_ERROR("AT command failed after factory reset\n");
    goto __err;
  }
  vTaskDelay(100); /* Add mandatory delay */

__err:
  return ret;
}

W61_Status_t W61_Reset(W61_Object_t *Obj)
{
  W61_Status_t ret;
  W61_NULL_ASSERT_STR(Obj, W61_Obj_Null_str);

  ret = W61_AT_Common_SetExecute(Obj, (uint8_t *)"AT+RST\r\n", 2000);
  if (ret != W61_STATUS_OK)
  {
    SYS_LOG_ERROR("Reset command failed\n");
    goto __err;
  }

  /* Wait for the module to be ready */
  ret = W61_WaitForReady(Obj, W61_READY_DEFAULT_TIMEOUT_MS);
  if (ret != W61_STATUS_OK)
  {
    SYS_LOG_ERROR("sem_if_ready not received\n");
    goto __err;
  }

  ret = W61_AT_Common_SetExecute(Obj, (uint8_t *)"AT\r\n", W61_NCP_TIMEOUT);
  if (ret != W61_STATUS_OK)
  {
    SYS_LOG_ERROR("AT command failed after reset\n");
    goto __err;
  }

  /* Add mandatory delay */
  vTaskDelay(100);

__err:
  return ret;
}

W61_Status_t W61_GetNcpHeapState(W61_Object_t *Obj, uint32_t *RemainingHeap, uint32_t *LwipHeap)
{
  char *argv[CONFIG_MODEM_CMD_HANDLER_MAX_PARAM_COUNT];
  char cmd[W61_CMD_MATCH_BUFF_SIZE];
  uint16_t argc = 0;
  W61_Status_t ret;
  W61_NULL_ASSERT_STR(Obj, W61_Obj_Null_str);

  /* Query the remaining heap and LWIP heap sizes. The response is in the form of
     +SYSRAM:<remaining RAM size>,<lwip heap size> */
  strncpy(cmd, "AT+SYSRAM?\r\n", sizeof(cmd));
  ret = W61_AT_Common_Query_Parse(Obj, cmd, "+SYSRAM:", &argc, argv, W61_NCP_TIMEOUT);
  if (ret != W61_STATUS_OK)
  {
    return ret;
  }
  if (argc < 2)
  {
    return W61_STATUS_ERROR;
  }

  *RemainingHeap = (uint32_t)atoi(argv[0]);
  *LwipHeap = (uint32_t)atoi(argv[1]);

  return ret;
}

W61_Status_t W61_GetStoreMode(W61_Object_t *Obj, uint32_t *mode)
{
  char *argv[CONFIG_MODEM_CMD_HANDLER_MAX_PARAM_COUNT];
  char cmd[W61_CMD_MATCH_BUFF_SIZE];
  uint16_t argc = 0;
  W61_Status_t ret;
  W61_NULL_ASSERT_STR(Obj, W61_Obj_Null_str);

  /* Query the AT parameter store mode. The response is in the form of
     +SYSSTORE:<store_mode> */
  strncpy(cmd, "AT+SYSSTORE?\r\n", sizeof(cmd));
  ret = W61_AT_Common_Query_Parse(Obj, cmd, "+SYSSTORE:", &argc, argv, W61_NCP_TIMEOUT);
  if (ret != W61_STATUS_OK)
  {
    return ret;
  }
  if (argc < 1)
  {
    return W61_STATUS_ERROR;
  }

  *mode = (uint32_t)atoi(argv[0]);

  return ret;
}

W61_Status_t W61_ReadEFuse(W61_Object_t *Obj, uint32_t addr, uint32_t nbytes, uint8_t *data)
{
  W61_Status_t ret;
  char cmd[W61_CMDRSP_STRING_SIZE];
  W61_NULL_ASSERT_STR(Obj, W61_Obj_Null_str);
  W61_NULL_ASSERT(data);
  struct modem *mdm = (struct modem *) &Obj->Modem;
  struct modem_cmd_handler_data *data_mdm = (struct modem_cmd_handler_data *)mdm->modem_cmd_handler.cmd_handler_data;

  struct modem_cmd handlers[] =
  {
    MODEM_CMD_DIRECT("+EFUSE-R:", on_cmd_read_efuse),
  };

  if (data_mdm == NULL)
  {
    return W61_STATUS_ERROR;
  }
  (void)xSemaphoreTake(data_mdm->sem_tx_lock, portMAX_DELAY);

  mdm->rx_data = data;

  /* Read the content of the eFuse at the specified address. The parameters are:
     - <nbytes>: The number of bytes to be read.
     - <addr>: This is the address of eFuse, it needs to be filled in as a string.
     - <reload>: The read operation reload from the eFuse address.
     The response is in the form of
     +EFUSE-R:<nbytes>,[data] */
  snprintf((char *)cmd, W61_CMDRSP_STRING_SIZE, "AT+EFUSE-R=%" PRIu32 ",\"0x%03" PRIx32 "\",1\r\n", nbytes, addr);
  ret = W61_Status(modem_cmd_send_ext(&mdm->iface,
                                      &mdm->modem_cmd_handler,
                                      handlers,
                                      ARRAY_SIZE(handlers),
                                      (const uint8_t *)cmd,
                                      mdm->sem_response,
                                      W61_NCP_TIMEOUT,
                                      MODEM_NO_TX_LOCK));

  (void)xSemaphoreGive(data_mdm->sem_tx_lock);
  return ret;
}

W61_Status_t W61_RestoreGPIO(W61_Object_t *Obj, uint32_t pin)
{
  char cmd[W61_CMDRSP_STRING_SIZE];
  W61_NULL_ASSERT_STR(Obj, W61_Obj_Null_str);
  /* Restore the specified GPIO to its default state */
  snprintf(cmd, W61_CMDRSP_STRING_SIZE, "AT+IORST=%" PRIu32 "\r\n", pin);
  return W61_AT_Common_SetExecute(Obj, (uint8_t *)cmd, W61_NCP_TIMEOUT);
}

W61_Status_t W61_FS_DeleteFile(W61_Object_t *Obj, char *filename)
{
  char cmd[W61_CMDRSP_STRING_SIZE];
  W61_NULL_ASSERT_STR(Obj, W61_Obj_Null_str);
  W61_NULL_ASSERT(filename);

  /* Delete the specified file in the filesystem. The parameters are:
     - <type>: 0
     - <operation>: 0 (Delete a file)
     - <filename>: The name of the file to delete */
  snprintf(cmd, W61_CMDRSP_STRING_SIZE, "AT+FS=0,0,\"%s\"\r\n", filename);
  return W61_AT_Common_SetExecute(Obj, (uint8_t *)cmd, W61_NCP_TIMEOUT);
}

W61_Status_t W61_FS_CreateFile(W61_Object_t *Obj, char *filename)
{
  char cmd[W61_CMDRSP_STRING_SIZE];
  W61_NULL_ASSERT_STR(Obj, W61_Obj_Null_str);
  W61_NULL_ASSERT(filename);

  /* Create a new file in the filesystem. The parameters are:
     - <type>: 0
     - <operation>: 1 (Create a file)
     - <filename>: The name of the file to create */
  snprintf(cmd, W61_CMDRSP_STRING_SIZE, "AT+FS=0,1,\"%s\"\r\n", filename);
  return W61_AT_Common_SetExecute(Obj, (uint8_t *)cmd, W61_NCP_TIMEOUT);
}

W61_Status_t W61_FS_WriteFile(W61_Object_t *Obj, char *filename, uint32_t offset, uint8_t *data, uint32_t len)
{
  char cmd[W61_CMDRSP_STRING_SIZE];
  W61_NULL_ASSERT_STR(Obj, W61_Obj_Null_str);
  W61_NULL_ASSERT(filename);
  W61_NULL_ASSERT(data);

  /* Write the specified file in the filesystem. The parameters are:
    - <type>: 0
    - <operation>: 2 (Write data to a file)
    - <filename>: The name of the file to write
    - <offset>: The offset in the file to start writing
    - <len>: The length of data to write
    The data to write is sent in the next request part */
  snprintf(cmd, W61_CMDRSP_STRING_SIZE, "AT+FS=0,2,\"%s\",%" PRIu32 ",%" PRIu32 "\r\n", filename, offset, len);
  return W61_AT_Common_RequestSendData(Obj, (uint8_t *)cmd, data, len, W61_SYS_TIMEOUT, true);
}

W61_Status_t W61_FS_ReadFile(W61_Object_t *Obj, char *filename, uint32_t offset, uint8_t *data, uint32_t len)
{
  char cmd[W61_CMDRSP_STRING_SIZE];
  W61_NULL_ASSERT_STR(Obj, W61_Obj_Null_str);
  W61_NULL_ASSERT(filename);
  W61_NULL_ASSERT(data);

  W61_Status_t ret;
  struct modem *mdm = (struct modem *) &Obj->Modem;
  struct modem_cmd_handler_data *data_mdm = (struct modem_cmd_handler_data *)mdm->modem_cmd_handler.cmd_handler_data;

  struct modem_cmd handlers[] =
  {
    MODEM_CMD_DIRECT("+FS:READ,", on_cmd_fs_readfile),
  };

  if (data_mdm == NULL)
  {
    return W61_STATUS_ERROR;
  }
  (void)xSemaphoreTake(data_mdm->sem_tx_lock, portMAX_DELAY);

  mdm->rx_data = (void *)data;

  /* Read the specified file in the filesystem. The parameters are:
    - <type>: 0
    - <operation>: 3 (Read data from a file)
    - <filename>: The name of the file to read
    - <offset>: The offset in the file to start reading
    - <len>: The length of data to read
    The response is in the form of
    +FS:READ,<nbytes>,[data] */
  snprintf(cmd, W61_CMDRSP_STRING_SIZE, "AT+FS=0,3,\"%s\",%" PRIu32 ",%" PRIu32 "\r\n", filename, offset, len);
  ret = W61_Status(modem_cmd_send_ext(&mdm->iface,
                                      &mdm->modem_cmd_handler,
                                      handlers,
                                      ARRAY_SIZE(handlers),
                                      (const uint8_t *)cmd,
                                      mdm->sem_response,
                                      W61_NCP_TIMEOUT,
                                      MODEM_NO_TX_LOCK));

  (void)xSemaphoreGive(data_mdm->sem_tx_lock);
  return ret;
}

W61_Status_t W61_FS_GetSizeFile(W61_Object_t *Obj, char *filename, uint32_t *size)
{
  char *argv[CONFIG_MODEM_CMD_HANDLER_MAX_PARAM_COUNT];
  char cmd[W61_CMD_MATCH_BUFF_SIZE];
  uint16_t argc = 0;
  W61_Status_t ret;
  W61_NULL_ASSERT_STR(Obj, W61_Obj_Null_str);
  W61_NULL_ASSERT(filename);
  W61_NULL_ASSERT(size);

  /* Get the size of the specified file in the filesystem. The parameters are:
    - <type>: 0
    - <operation>: 4 (Get the size of a file)
    - <filename>: The name of the file to read
    The response is in the form of
    +FS:SIZE,<size> */
  snprintf(cmd, W61_CMD_MATCH_BUFF_SIZE, "AT+FS=0,4,\"%s\"\r\n", filename);
  ret = W61_AT_Common_Query_Parse(Obj, cmd, "+FS:SIZE", &argc, argv, W61_SYS_TIMEOUT);
  if (ret != W61_STATUS_OK)
  {
    return ret;
  }
  if (argc < 2)
  {
    return W61_STATUS_ERROR;
  }

  *size = (uint32_t)atoi(argv[1]);

  return ret;
}

W61_Status_t W61_FS_ListFiles(W61_Object_t *Obj, W61_FS_FilesList_t *files_list)
{
  W61_Status_t ret;
  W61_NULL_ASSERT_STR(Obj, W61_Obj_Null_str);
  W61_NULL_ASSERT_STR(files_list, "File list pointer is NULL");

  struct modem *mdm = (struct modem *) &Obj->Modem;
  struct modem_cmd_handler_data *data = (struct modem_cmd_handler_data *)mdm->modem_cmd_handler.cmd_handler_data;

  struct modem_cmd handlers[] =
  {
    MODEM_CMD("", on_cmd_fs_listfiles, 1U, ""),
  };

  if (data == NULL)
  {
    return W61_STATUS_ERROR;
  }
  (void)xSemaphoreTake(data->sem_tx_lock, portMAX_DELAY);

  mdm->rx_data = (void *)files_list;

  files_list->nb_files = 0; /* Reset the number of files */

  /* Get the files list in the filesystem. The parameters are:
    - <type>: 0
    - <operation>: 5 (List files in a directory)
    - <dirname>: The name of the directory to list files from
    The multiline responses are in the form of
    +FS:LIST,<nfiles>,[filename] */
  ret = W61_Status(modem_cmd_send_ext(&mdm->iface,
                                      &mdm->modem_cmd_handler,
                                      handlers,
                                      ARRAY_SIZE(handlers),
                                      (const uint8_t *)"AT+FS=0,5,\".\"\r\n",
                                      mdm->sem_response,
                                      W61_NCP_TIMEOUT,
                                      MODEM_NO_TX_LOCK));

  (void)xSemaphoreGive(data->sem_tx_lock);
  return ret;
}

W61_Status_t W61_GetModuleInfo(W61_Object_t *Obj)
{
  W61_Status_t ret;
  uint32_t data[8] = {0};
  W61_NULL_ASSERT_STR(Obj, W61_Obj_Null_str);
  char *argv[CONFIG_MODEM_CMD_HANDLER_MAX_PARAM_COUNT];
  char cmd[W61_CMD_MATCH_BUFF_SIZE];
  uint16_t argc = 0;
  struct modem *mdm = (struct modem *) &Obj->Modem;

  memset(&Obj->ModuleInfo, 0, sizeof(W61_ModuleInfo_t));

  /* ====================================== */
  /* Get the module info */
  /* ====================================== */

  struct modem_cmd handlers[] =
  {
    MODEM_CMD_ARGS_MAX("AT version:", on_cmd_at_version, 1U, 10U, ".()"),
    MODEM_CMD_ARGS_MAX("component_version_macsw_", on_cmd_mac_sw_version, 1U, 10U, "."),
    MODEM_CMD_ARGS_MAX("component_version_sdk_", on_cmd_sdk_version, 1U, 10U, "."),
    MODEM_CMD_ARGS_MAX("lib_version_btblecontroller_", on_cmd_bt_controller_version, 1U, 10U, "."),
    MODEM_CMD_ARGS_MAX("component_version_btble_", on_cmd_bt_stack_version, 1U, 10U, "."),
  };

  /* Get the version of the available software components in the ST67W611M */
  ret = W61_Status(modem_cmd_send(&mdm->iface,
                                  &mdm->modem_cmd_handler,
                                  handlers,
                                  ARRAY_SIZE(handlers),
                                  (const uint8_t *)"AT+GMR\r\n",
                                  mdm->sem_response,
                                  W61_NCP_TIMEOUT));
  if (ret != W61_STATUS_OK)
  {
    goto _err;
  }

  /* ====================================== */
  /* Battery voltage. return the voltage value in mV */
  /* ====================================== */
  strncpy(cmd, "AT+VBAT?\r\n", sizeof(cmd));
  ret = W61_AT_Common_Query_Parse(Obj, cmd, "+VBAT:", &argc, argv, W61_NCP_TIMEOUT);
  if (ret != W61_STATUS_OK)
  {
    return ret;
  }
  if (argc < 1)
  {
    return W61_STATUS_ERROR;
  }

  Obj->ModuleInfo.BatteryVoltage = (uint32_t)atoi(argv[0]);

  /* ====================================== */
  /* RF and XTAL Trimming */
  /* ====================================== */
  for (uint32_t i = 0; i < sizeof(trim_table) / sizeof(trim_table[0]); i++)
  {
    ret = W61_ReadEFuse(Obj, trim_table[i].en_addr, 4, (uint8_t *)data);
    if (ret != W61_STATUS_OK)
    {
      if (ret == W61_STATUS_ERROR)
      {
        SYS_LOG_ERROR("Unable to read %s\n", trim_table[i].desc);
      }
      goto _err;
    }

    if ((data[0] >> trim_table[i].en_offset) & 0x1) /* Check if trim is enabled */
    {
      uint32_t cnt = 0;
      int32_t k;
      int32_t step = 0;
      int8_t pwr_offset[14];
      int8_t pwr_offset_tmp[3];
      uint32_t trim_parity;
      uint32_t trim_value;

      /* Get the trim offset and parity values */
      ret = W61_ReadEFuse(Obj, trim_table[i].parity_addr, 4, (uint8_t *)&data[0]);
      if (ret != W61_STATUS_OK)
      {
        if (ret == W61_STATUS_ERROR)
        {
          SYS_LOG_ERROR("Unable to read %s\n", trim_table[i].desc);
        }
        goto _err;
      }
      ret = W61_ReadEFuse(Obj, trim_table[i].value_addr, 4, (uint8_t *)&data[1]);
      if (ret != W61_STATUS_OK)
      {
        if (ret == W61_STATUS_ERROR)
        {
          SYS_LOG_ERROR("Unable to read %s\n", trim_table[i].desc);
        }
        goto _err;
      }

      /* Parity */
      trim_parity = data[0] >> trim_table[i].parity_offset & 0x1;

      /* Trim offset */
      trim_value = data[1] >> trim_table[i].value_offset & ((1 << trim_table[i].value_len) - 1);

      for (k = 0; k < trim_table[i].value_len; k++) /* Count number of bits set */
      {
        if (trim_value & (1 << k))
        {
          cnt++;
        }
      }

      if ((cnt & 0x1) != trim_parity) /* Check parity */
      {
        continue;
      }

      if (trim_table[i].type == 2) /* BLE */
      {
        for (k = 0; k < 5; k++)
        {
          /* Calculate the 5 channels offset from the 25 bits value */
          Obj->ModuleInfo.trim_ble[k] = (trim_value >> (k * 5)) & 0x1f;
          if (Obj->ModuleInfo.trim_ble[k] >= 16)
          {
            Obj->ModuleInfo.trim_ble[k] -= 32;
          }
        }
      }
      else if ((trim_table[i].type == 0) || (trim_table[i].type == 1)) /* Wi-Fi */
      {
        /* Calculate the 14 channels offset from the 15 bits value */
        for (k = 0; k < 3; k++)
        {
          pwr_offset_tmp[k] = (trim_value >> (k * 5)) & 0x1f;

          if (pwr_offset_tmp[k] >= 16)
          {
            pwr_offset_tmp[k] -= 32;
          }
        }

        pwr_offset[0] = pwr_offset_tmp[0];
        pwr_offset[6] = pwr_offset_tmp[1];
        pwr_offset[12] = pwr_offset_tmp[2];

        step = (pwr_offset_tmp[1] - pwr_offset_tmp[0]) * 100 / 6;
        for (k = 1; k < 6; k++)
        {
          pwr_offset[k] = ((step * k) + 50) / 100 + pwr_offset_tmp[0];
        }

        step = (pwr_offset_tmp[2] - pwr_offset_tmp[1]) * 100 / 6;
        for (k = 7; k < 12; k++)
        {
          pwr_offset[k] = ((step * (k - 6)) + 50) / 100 + pwr_offset_tmp[1];
        }

        pwr_offset[13] = (step * 7 + 50) / 100 + pwr_offset_tmp[1];

        if (trim_table[i].type == 0) /* Wi-Fi high-performance */
        {
          memcpy(Obj->ModuleInfo.trim_wifi_hp, pwr_offset, sizeof(Obj->ModuleInfo.trim_wifi_hp));
        }
        else /* Wi-Fi low-power */
        {
          memcpy(Obj->ModuleInfo.trim_wifi_lp, pwr_offset, sizeof(Obj->ModuleInfo.trim_wifi_lp));
        }
      }
      else if (trim_table[i].type == 3) /* XTAL */
      {
        Obj->ModuleInfo.trim_xtal = trim_value;
      }
    }
  }

  /* ====================================== */
  /* Module part number */
  /* ====================================== */
  ret = W61_ReadEFuse(Obj, EFUSE_PART_NUMBER_ADDR, EFUSE_PART_NUMBER_LEN, (uint8_t *)data);
  if (ret != W61_STATUS_OK)
  {
    if (ret == W61_STATUS_ERROR)
    {
      SYS_LOG_ERROR("Unable to read Part number\n");
    }
    goto _err;
  }

  if (data[0] || data[1] || data[2])
  {
    uint32_t char_cnt = 0;
    uint8_t *byte = (uint8_t *)data;
    char part_number[EFUSE_PART_NUMBER_LEN + 1] = {0};

    for (char_cnt = 0; char_cnt < EFUSE_PART_NUMBER_LEN; char_cnt++)
    {
      if ((byte[char_cnt] == 0) || (byte[char_cnt] == 0x03)) /* Check the end of string or the end of the part number */
      {
        break;
      }
      snprintf(part_number + char_cnt, 2, "%c", byte[char_cnt]);
    }
    strncpy(Obj->ModuleInfo.ModuleID.ModuleName, part_number, sizeof(Obj->ModuleInfo.ModuleID.ModuleName) - 1);
    Obj->ModuleInfo.ModuleID.ModuleName[sizeof(Obj->ModuleInfo.ModuleID.ModuleName) - 1] = '\0';

    /* Check the module ID */
    for (int32_t i = 0; i < sizeof(module_id) / sizeof(module_id[0]); i++)
    {
      if (strncmp((char *)Obj->ModuleInfo.ModuleID.ModuleName, (char *)module_id[i].ModuleName,
                  sizeof(module_id[i].ModuleName)) == 0)
      {
        Obj->ModuleInfo.ModuleID.ModuleID = module_id[i].ModuleID;
        break;
      }
    }
  }

  /* ====================================== */
  /* BOM ID + Manufacturing Year/Week */
  /* ====================================== */
  ret = W61_ReadEFuse(Obj, EFUSE_MANUF_BOM_ADDR, EFUSE_MANUF_BOM_LEN, (uint8_t *)data);
  if (ret != W61_STATUS_OK)
  {
    if (ret == W61_STATUS_ERROR)
    {
      SYS_LOG_ERROR("Unable to read BOM ID & Manufacturing Year/Week\n");
    }
    goto _err;
  }

  if (data[0])
  {
    Obj->ModuleInfo.BomID = ((data[0] >> 8) & 0xFF) | ((data[0] & 0xFF) << 8);
    Obj->ModuleInfo.Manufacturing_Year = (data[0] >> 16) & 0xFF;
    Obj->ModuleInfo.Manufacturing_Week = (data[0] >> 24) & 0xFF;
  }

  /* ====================================== */
  /* MAC Address */
  /* ====================================== */
  for (int32_t i = 0; i < sizeof(efuse_mac_table) / sizeof(efuse_mac_table[0]); i++)
  {
    uint32_t cnt = 0;
    uint32_t byte_cnt = 0;
    uint32_t bit_cnt = 0;
    uint8_t *byte;

    /* Search the MAC Address in each EFUSE slot. Use always the last available slot */
    ret = W61_ReadEFuse(Obj, efuse_mac_table[i].efuse_addr, efuse_mac_table[i].efuse_len,
                        (uint8_t *)data);
    if (ret != W61_STATUS_OK)
    {
      if (ret == W61_STATUS_ERROR)
      {
        SYS_LOG_ERROR("Unable to read %s\n", efuse_mac_table[i].desc);
      }
      goto _err;
    }

    byte = (uint8_t *)data;

    for (byte_cnt = 0; byte_cnt < 6; byte_cnt++)
    {
      for (bit_cnt = 0; bit_cnt < 8; bit_cnt++)
      {
        if ((byte[byte_cnt] & (1 << bit_cnt)) == 0)
        {
          cnt += 1; /* Count all cleared bits */
        }
      }
    }

    if ((cnt & 0x3f) == ((data[1] >> 16) & 0x3f)) /* Check the cleared bits count with 'CRC' byte */
    {
      for (byte_cnt = 0; byte_cnt < 6; byte_cnt++)
      {
        Obj->ModuleInfo.Mac_Address[byte_cnt] = byte[5 - byte_cnt]; /* Get the MAC address */
      }
    }
  }

  /* ====================================== */
  /* Anti-rollback */
  /* ====================================== */
  ret = W61_ReadEFuse(Obj, EFUSE_ANTI_ROLL_BACK_EN_ADDR, EFUSE_ANTI_ROLL_BACK_EN_LEN,
                      (uint8_t *)data);
  if (ret != W61_STATUS_OK)
  {
    if (ret == W61_STATUS_ERROR)
    {
      SYS_LOG_ERROR("Unable to read Anti-rollback enable\n");
    }
    goto _err;
  }

  if ((data[0] >> 12) & 0x1) /* Anti-rollback enabled */
  {
    W61_Efuse_t efuse_antirollback_table[] =
    {
      {
        EFUSE_BOOT2_ANTI_ROLL_BACK_ADDR, EFUSE_BOOT2_ANTI_ROLL_BACK_LEN,
        &Obj->ModuleInfo.AntiRollbackBootloader, "Bootloader"
      },
      {
        EFUSE_APP_ANTI_ROLL_BACK_ADDR, EFUSE_APP_ANTI_ROLL_BACK_LEN,
        &Obj->ModuleInfo.AntiRollbackApp, "Application"
      }
    };

    for (int32_t i = 0; i < sizeof(efuse_antirollback_table) / sizeof(efuse_antirollback_table[0]); i++)
    {
      uint8_t half_index = efuse_antirollback_table[i].efuse_len / 8;
      ret = W61_ReadEFuse(Obj, efuse_antirollback_table[i].efuse_addr, efuse_antirollback_table[i].efuse_len,
                          (uint8_t *)data);
      if (ret != W61_STATUS_OK)
      {
        if (ret == W61_STATUS_ERROR)
        {
          SYS_LOG_ERROR("Unable to read Anti-rollback %s\n", efuse_antirollback_table[i].desc);
        }
        goto _err;
      }

      for (int32_t j = 0; j < half_index; j++) /* Get the first cleared bit position */
      {
        *efuse_antirollback_table[i].data += 32 - __CLZ(data[j] | data[j + half_index]);
      }
    }
  }

_err:
  return ret;
}

W61_Status_t W61_FWU_starts(W61_Object_t *Obj, uint32_t enable)
{
  char cmd[W61_CMDRSP_STRING_SIZE];
  W61_NULL_ASSERT_STR(Obj, W61_Obj_Null_str);

  /* Start or stop the firmware update process. The parameter is:
     - <enable>: 1 to start the firmware update process, 0 to stop it */
  snprintf(cmd, W61_CMDRSP_STRING_SIZE, "AT+OTASTART=%" PRIu32 "\r\n", enable);
  return W61_AT_Common_SetExecute(Obj, (uint8_t *)cmd, W61_SYS_TIMEOUT);
}

W61_Status_t W61_FWU_Finish(W61_Object_t *Obj)
{
  W61_NULL_ASSERT_STR(Obj, W61_Obj_Null_str);

  /* Finish the firmware update and reboot the ST67W611M */
  return W61_AT_Common_SetExecute(Obj, (uint8_t *)"AT+OTAFIN\r\n", W61_SYS_TIMEOUT);
}

W61_Status_t W61_FWU_Send(W61_Object_t *Obj, uint8_t *buff, uint32_t len)
{
  char cmd[W61_CMDRSP_STRING_SIZE];
  W61_NULL_ASSERT_STR(Obj, W61_Obj_Null_str);
  W61_NULL_ASSERT(buff);

  /* Send a chunk of data for the firmware update. The parameter is:
     - <len>: The length of data to send
     The data to send is sent in the next request part */
  snprintf(cmd, W61_CMDRSP_STRING_SIZE, "AT+OTASEND=%" PRIu32 "\r\n", len);
  return W61_AT_Common_RequestSendData(Obj, (uint8_t *)cmd, buff, len, W61_SYS_TIMEOUT, true);
}

W61_Status_t W61_LowPowerConfig(W61_Object_t *Obj, uint32_t WakeUpPinIn, uint32_t ps_mode)
{
  W61_NULL_ASSERT_STR(Obj, W61_Obj_Null_str);

  /* In Hibernate Mode, only WakeUpPin 16 can be used */
  if ((ps_mode == 1) && (WakeUpPinIn != 16))
  {
    return W61_STATUS_ERROR;
  }

  Obj->LowPowerCfg.WakeUpPinIn = WakeUpPinIn;
  Obj->LowPowerCfg.PSMode = ps_mode;
  Obj->LowPowerCfg.WiFi_DTIM_Factor = 0; /* Disabled by default */
  Obj->LowPowerCfg.WiFi_DTIM_Interval = 0; /* Disabled by default */

  return W61_STATUS_OK;
}

W61_Status_t W61_SetPowerMode(W61_Object_t *Obj, uint32_t ps_mode, uint32_t hbn_level)
{
  W61_Status_t ret;
  char cmd[W61_CMDRSP_STRING_SIZE];
  W61_NULL_ASSERT_STR(Obj, W61_Obj_Null_str);

  /* Configure the power save mode. The parameters are:
     - <ps_mode>: The power save mode to set (0: No power save, 1: Hibernate, 2: Standby)
     - <hbn_level>: The hibernate level to set (only used in hibernate mode) */

  if (ps_mode == 1) /* Hibernate mode */
  {
    snprintf(cmd, W61_CMDRSP_STRING_SIZE, "AT+PWR=%" PRIu32 ",%" PRIu32 "\r\n", ps_mode, hbn_level);
    /* No response from the ST67 when in hibernate ps mode, timeout is 0 */
    ret = W61_AT_Common_SetExecute(Obj, (uint8_t *)cmd, 0);
  }
  else
  {
    snprintf(cmd, W61_CMDRSP_STRING_SIZE, "AT+PWR=%" PRIu32 "\r\n", ps_mode);
    ret = W61_AT_Common_SetExecute(Obj, (uint8_t *)cmd, W61_NCP_TIMEOUT);
  }

  if (ret == W61_STATUS_OK)
  {
    Obj->LowPowerCfg.PSMode = ps_mode;
  }

  return ret;
}

W61_Status_t W61_GetPowerMode(W61_Object_t *Obj, uint32_t *ps_mode)
{
  W61_NULL_ASSERT_STR(Obj, W61_Obj_Null_str);

  *ps_mode = Obj->LowPowerCfg.PSMode;
  return W61_STATUS_OK;
}

W61_Status_t W61_SetWakeUpPin(W61_Object_t *Obj, uint32_t wakeup_pin)
{
  char cmd[W61_CMDRSP_STRING_SIZE];
  W61_NULL_ASSERT_STR(Obj, W61_Obj_Null_str);

  /* Configure the wakeup pin for exiting low power mode. The parameter is:
     - <wakeup_pin>: The GPIO pin number to be used as wakeup pin (0..31) */
  snprintf(cmd, W61_CMDRSP_STRING_SIZE, "AT+SLWKIO=%" PRIu32 ",0\r\n", wakeup_pin);
  return W61_AT_Common_SetExecute(Obj, (uint8_t *)cmd, W61_NCP_TIMEOUT);
}

W61_Status_t W61_SetClockSource(W61_Object_t *Obj, uint32_t source)
{
  W61_Status_t ret = W61_STATUS_ERROR;
  uint32_t current_source;
  char cmd[W61_CMDRSP_STRING_SIZE];
  W61_NULL_ASSERT_STR(Obj, W61_Obj_Null_str);

  if ((source == 0) || (source > 3))
  {
    goto _err; /* Invalid clock source */
  }

  if (W61_GetClockSource(Obj, &current_source) != W61_STATUS_OK)
  {
    goto _err; /* Failed to get the current clock source */
  }

  if (current_source == source) /* If the source is already set, return OK */
  {
    return W61_STATUS_OK;
  }

  /* Set the clock source. The parameter is:
     - <source>: The clock source to set (1: Internal RC, 2: External XTAL, 3: External Clock Input) */
  snprintf(cmd, W61_CMDRSP_STRING_SIZE, "AT+SET_CLOCK=%" PRIu32 "\r\n", source);
  /* Timeout is 0, the command cannot return any response because the ST67 reboots after setting the clock source */
  ret = W61_AT_Common_SetExecute(Obj, (uint8_t *)cmd, 0);
  if (ret != W61_STATUS_OK)
  {
    SYS_LOG_ERROR("Failed to set clock source");
    goto _err;
  }

  /* Wait for the ready message from the ST67 */
  ret = W61_WaitForReady(Obj, W61_READY_DEFAULT_TIMEOUT_MS);
  if (ret != W61_STATUS_OK)
  {
    SYS_LOG_ERROR("sem_if_ready not received\n");
  }

_err:
  return ret;
}

W61_Status_t W61_GetClockSource(W61_Object_t *Obj, uint32_t *source)
{
  char *argv[CONFIG_MODEM_CMD_HANDLER_MAX_PARAM_COUNT];
  char cmd[W61_CMD_MATCH_BUFF_SIZE];
  uint16_t argc = 0;
  W61_Status_t ret;
  W61_NULL_ASSERT_STR(Obj, W61_Obj_Null_str);

  /* Query the current clock source. The response is in the form of
     +GET_CLOCK:<source> */
  strncpy(cmd, "AT+GET_CLOCK\r\n", sizeof(cmd));
  ret = W61_AT_Common_Query_Parse(Obj, cmd, "+GET_CLOCK:", &argc, argv, W61_NCP_TIMEOUT);
  if (ret != W61_STATUS_OK)
  {
    return ret;
  }
  if (argc < 1)
  {
    return W61_STATUS_ERROR;
  }

  *source = (uint32_t)atoi(argv[0]);

  return ret;
}

W61_Status_t W61_ExeATCommand(W61_Object_t *Obj, char *at_cmd)
{
  W61_Status_t ret;
  char cmd[W61_CMDRSP_STRING_SIZE];
  W61_NULL_ASSERT_STR(Obj, W61_Obj_Null_str);

  struct modem *mdm = (struct modem *) &Obj->Modem;

  struct modem_cmd handlers[] =
  {
    MODEM_CMD("", on_cmd_atcmd, 1, ""),
  };

  snprintf(cmd, W61_CMDRSP_STRING_SIZE, "%s\r\n", at_cmd);
  ret = W61_Status(modem_cmd_send(&mdm->iface,
                                  &mdm->modem_cmd_handler,
                                  handlers,
                                  ARRAY_SIZE(handlers),
                                  (const uint8_t *)cmd,
                                  mdm->sem_response,
                                  W61_NCP_TIMEOUT));

  if (ret == W61_STATUS_OK)
  {
    SYS_LOG_INFO("OK\n");
  }
  else
  {
    SYS_LOG_INFO("ERROR\n");
  }

  return ret;
}

W61_Status_t W61_GetNetMode(W61_Object_t *Obj, int32_t *Netmode)
{
  char *argv[CONFIG_MODEM_CMD_HANDLER_MAX_PARAM_COUNT];
  char cmd[W61_CMD_MATCH_BUFF_SIZE];
  uint16_t argc = 0;
  W61_Status_t ret;
  W61_NULL_ASSERT(Obj);
  W61_NULL_ASSERT(Netmode);

  /* Query the current network mode. The response is in the form of
     +CWNETMODE:<mode> */
  strncpy(cmd, "AT+CWNETMODE?\r\n", sizeof(cmd));
  ret = W61_AT_Common_Query_Parse(Obj, cmd, "+CWNETMODE:", &argc, argv, W61_NCP_TIMEOUT);
  if (ret != W61_STATUS_OK)
  {
    return ret;
  }
  if (argc < 1)
  {
    return W61_STATUS_ERROR;
  }

  *Netmode = (uint32_t)atoi(argv[0]);

  return ret;
}

W61_Status_t W61_SdkMinVersion(W61_Object_t *Obj, uint32_t major, uint32_t sub1, uint32_t sub2)
{
  W61_NULL_ASSERT_STR(Obj, W61_Obj_Null_str);

  if ((Obj->ModuleInfo.SDK_Version.Major > major) ||
      ((Obj->ModuleInfo.SDK_Version.Major == major) && (Obj->ModuleInfo.SDK_Version.Sub1 > sub1)) ||
      ((Obj->ModuleInfo.SDK_Version.Major == major) && (Obj->ModuleInfo.SDK_Version.Sub1 == sub1) &&
       (Obj->ModuleInfo.SDK_Version.Sub2 >= sub2)))
  {
    return W61_STATUS_OK;
  }

  return W61_STATUS_ERROR;
}
/* Private Functions Definition ----------------------------------------------*/
MODEM_CMD_DEFINE(on_cmd_at_version)
{
  struct modem *mdm = (struct modem *) data->user_data;
  W61_Object_t *Obj = CONTAINER_OF(mdm, W61_Object_t, Modem);

  if (argc < 5)
  {
    return 0;
  }

  Obj->ModuleInfo.AT_Version.Major = (uint8_t)atoi((char *)argv[0]);
  Obj->ModuleInfo.AT_Version.Sub1 = (uint8_t)atoi((char *)argv[1]);
  Obj->ModuleInfo.AT_Version.Sub2 = (uint8_t)atoi((char *)argv[2]);
  strncpy((char *) Obj->ModuleInfo.Build_Date, (char *) argv[4], sizeof(Obj->ModuleInfo.Build_Date) - 1);
  return 0;
}

MODEM_CMD_DEFINE(on_cmd_mac_sw_version)
{
  struct modem *mdm = (struct modem *) data->user_data;
  W61_Object_t *Obj = CONTAINER_OF(mdm, W61_Object_t, Modem);

  if (argc < 3)
  {
    return 0;
  }

  Obj->ModuleInfo.WiFi_MAC_Version.Major = (uint8_t)atoi((char *)argv[0]);
  Obj->ModuleInfo.WiFi_MAC_Version.Sub1 = (uint8_t)atoi((char *)argv[1]);
  Obj->ModuleInfo.WiFi_MAC_Version.Sub2 = (uint8_t)atoi((char *)argv[2]);
  return 0;
}

MODEM_CMD_DEFINE(on_cmd_sdk_version)
{
  struct modem *mdm = (struct modem *) data->user_data;
  W61_Object_t *Obj = CONTAINER_OF(mdm, W61_Object_t, Modem);

  if (argc < 3)
  {
    return 0;
  }

  Obj->ModuleInfo.SDK_Version.Major = (uint8_t)atoi((char *)argv[0]);
  Obj->ModuleInfo.SDK_Version.Sub1 = (uint8_t)atoi((char *)argv[1]);
  Obj->ModuleInfo.SDK_Version.Sub2 = (uint8_t)atoi((char *)argv[2]);
  if (argc > 3)
  {
    Obj->ModuleInfo.SDK_Version.Patch = (uint8_t)atoi((char *)argv[3]);
  }
  return 0;
}

MODEM_CMD_DEFINE(on_cmd_bt_controller_version)
{
  struct modem *mdm = (struct modem *) data->user_data;
  W61_Object_t *Obj = CONTAINER_OF(mdm, W61_Object_t, Modem);

  if (argc < 3)
  {
    return 0;
  }

  Obj->ModuleInfo.BT_Controller_Version.Major = (uint8_t)atoi((char *)argv[0]);
  Obj->ModuleInfo.BT_Controller_Version.Sub1 = (uint8_t)atoi((char *)argv[1]);
  Obj->ModuleInfo.BT_Controller_Version.Sub2 = (uint8_t)atoi((char *)argv[2]);
  return 0;
}

MODEM_CMD_DEFINE(on_cmd_bt_stack_version)
{
  struct modem *mdm = (struct modem *) data->user_data;
  W61_Object_t *Obj = CONTAINER_OF(mdm, W61_Object_t, Modem);

  if (argc < 3)
  {
    return 0;
  }

  Obj->ModuleInfo.BT_Stack_Version.Major = (uint8_t)atoi((char *)argv[0]);
  Obj->ModuleInfo.BT_Stack_Version.Sub1 = (uint8_t)atoi((char *)argv[1]);
  Obj->ModuleInfo.BT_Stack_Version.Sub2 = (uint8_t)atoi((char *)argv[2]);
  return 0;
}

MODEM_CMD_DIRECT_DEFINE(on_cmd_read_efuse)
{
  struct modem *mdm = (struct modem *) data->user_data;

  uint8_t *ptr;
  uint32_t offset;
  uint8_t *endptr;
  uint16_t rx_data_len;

  if (data->rx_buf == NULL)
  {
    return -EINVAL;
  }
  ptr = data->rx_buf + len;
  data->rx_buf[data->rx_buf_len] = 0;

  rx_data_len = strtol((char *) ptr, (char **)&endptr, 10);
  if ((endptr == ptr) || (*endptr != ','))
  {
    SYS_LOG_ERROR("Invalid EFUSE read response format");
    return -EINVAL;
  }
  offset = endptr - data->rx_buf + 1;

  if (data->rx_buf_len >= offset + rx_data_len)
  {
    memcpy(mdm->rx_data, endptr + 1, rx_data_len);
    return offset + rx_data_len;
  }
  else
  {
    return -EAGAIN;
  }
}

MODEM_CMD_DEFINE(on_cmd_fs_listfiles)
{
  struct modem *mdm = (struct modem *) data->user_data;
  W61_FS_FilesList_t *files_list = (W61_FS_FilesList_t *) mdm->rx_data;

  if ((argc >= 1) && (argv[0][0] != '.') && (strcmp((char *) argv[0], "+FS:LIST") != 0))
  {
    /* Copy the filename */
    snprintf(files_list->filename[files_list->nb_files], W61_SYS_FS_FILENAME_SIZE, "%s", argv[0]);

    files_list->nb_files++; /* Increment the number of files */
  }
  return 0;
}

MODEM_CMD_DIRECT_DEFINE(on_cmd_fs_readfile)
{
  struct modem *mdm = (struct modem *) data->user_data;

  uint8_t *ptr;
  uint32_t offset;
  uint8_t *endptr;
  uint16_t rx_data_len;

  if (data->rx_buf == NULL)
  {
    return -EINVAL;
  }
  ptr = data->rx_buf + len;
  data->rx_buf[data->rx_buf_len] = 0;

  rx_data_len = strtol((char *) ptr, (char **)&endptr, 10);
  if ((endptr == ptr) || (*endptr != ','))
  {
    SYS_LOG_ERROR("Invalid FS read response format");
    return -EINVAL;
  }
  offset = endptr - data->rx_buf + 1;

  if (data->rx_buf_len >= (offset + rx_data_len))
  {
    memcpy(mdm->rx_data, endptr + 1, rx_data_len);
    return offset + rx_data_len;
  }
  else
  {
    return -EAGAIN;
  }
}

MODEM_CMD_DEFINE(on_cmd_atcmd)
{
  if (argc > 0)
  {
    SYS_LOG_INFO("%s\n", argv[0]);
    /*for AT+CMD?, log needs some time to push */
    vTaskDelay(5);
  }
  return 0;
}

/** @} */
