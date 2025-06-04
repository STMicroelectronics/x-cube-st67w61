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
#include "w61_at_rx_parser.h"
#include "w61_io.h" /* SPI_XFER_MTU_BYTES */
#include "common_parser.h" /* Common Parser functions */

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

#define OTA_DATA_SEND_DELAY               (4U)        /*!< OTA Delay between the command send OTA data AT command and the effective data being sent */

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

/** W61 context pointer error string */
static const char W61_Obj_Null_str[] = "Object pointer not initialized";

/** sscanf parsing error string */
static const char W61_Parsing_Error_str[] = "Parsing of the result failed";

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
  {0xCC, 26, 0xC0, 0,  15, 0xC0, 15, 0, "wifi_hp_poffset0"},
  {0xCC, 27, 0xC0, 16, 15, 0xC0, 31, 0, "wifi_hp_poffset1"},
  {0xCC, 28, 0xC4, 0,  15, 0xC4, 15, 0, "wifi_hp_poffset2"},
  {0xCC, 29, 0xC4, 16, 15, 0xC4, 31, 1, "wifi_lp_poffset0"},
  {0xCC, 30, 0xC8, 0,  15, 0xC8, 15, 1, "wifi_lp_poffset1"},
  {0xCC, 31, 0xC8, 16, 15, 0xC8, 31, 1, "wifi_lp_poffset2"},
  {0xD0, 26, 0xCC, 0,  25, 0xCC, 25, 2, "ble_poffset0"},
  {0xD0, 27, 0xD0, 0,  25, 0xD0, 25, 2, "ble_poffset1"},
  {0xD0, 28, 0xD4, 0,  25, 0xD4, 25, 2, "ble_poffset2"},
  {0xEC, 7,  0XEC, 0,  6,  0XEC, 6,  3, "xtal0"},
  {0xF0, 31, 0XF4, 26, 6,  0XF0, 30, 3, "xtal1"},
  {0xEC, 23, 0XF4, 20, 6,  0XF0, 28, 3, "xtal2"},
};

/** @} */

/* Private function prototypes -----------------------------------------------*/
/* Functions Definition ------------------------------------------------------*/
/** @addtogroup ST67W61_AT_System_Functions
  * @{
  */

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

W61_Status_t W61_RegisterBusIO(W61_Object_t *Obj,
                               W61_IO_Init_cb_t     IO_Init,
                               W61_IO_DeInit_cb_t   IO_DeInit,
                               W61_IO_Delay_cb_t    IO_Delay,
                               W61_IO_Send_cb_t     IO_Send,
                               W61_IO_Receive_cb_t  IO_Receive)
{
  if (!Obj || !IO_Init || !IO_DeInit || !IO_Delay || !IO_Send || !IO_Receive)
  {
    return W61_STATUS_ERROR;
  }

  Obj->fops.IO_Init = IO_Init;
  Obj->fops.IO_DeInit = IO_DeInit;
  Obj->fops.IO_Send = IO_Send;
  Obj->fops.IO_Delay = IO_Delay;
  Obj->fops.IO_Receive = IO_Receive;

  return W61_STATUS_OK;
}

W61_Status_t W61_Init(W61_Object_t *Obj)
{
  W61_Status_t ret = W61_STATUS_ERROR;
  W61_NULL_ASSERT_STR(Obj, W61_Obj_Null_str);

  Obj->NcpTimeout = W61_NCP_TIMEOUT;
  Obj->CmdResp = pvPortMalloc(W61_ATD_CMDRSP_STRING_SIZE);
  if (Obj->CmdResp == NULL)
  {
    LogError("Unable to allocate CmdResp\n");
    return ret;
  }

  Obj->xCmdMutex = xSemaphoreCreateMutex();
  if (Obj->xCmdMutex == NULL)
  {
    goto __error;
  }

  if (Obj->fops.IO_Init == NULL)
  {
    LogError("IO_Init callback not defined\n");
    goto __error;
  }
  if (Obj->fops.IO_Init() != 0)
  {
    LogError("IO_Init failed\n");
    ret = W61_STATUS_ERROR;
    goto __error;
  }

  ret = (W61_Status_t) W61_ATD_RxParserInit(Obj);
  if (ret != W61_STATUS_OK)
  {
    LogError("Could not init RxParser\n");
    goto __error;
  }

  /* Wait message ready */
  ret = W61_WaitReady(Obj);
  if (ret != W61_STATUS_OK)
  {
    LogError("Wait ready failed\n");
    goto __error;
  }

__error:
  return ret;
}

W61_Status_t W61_DeInit(W61_Object_t *Obj)
{
  W61_Status_t ret = W61_STATUS_ERROR;
  W61_NULL_ASSERT_STR(Obj, W61_Obj_Null_str);

  ret = (W61_Status_t) Obj->fops.IO_DeInit();

  W61_ATD_RxParserDeInit(Obj);

  vSemaphoreDelete(Obj->xCmdMutex);

  if (Obj->CmdResp)
  {
    vPortFree(Obj->CmdResp);
    Obj->CmdResp = NULL;
  }

  return ret;
}

W61_Status_t W61_WaitReady(W61_Object_t *Obj)
{
  W61_Status_t ret = W61_STATUS_ERROR;
  W61_NULL_ASSERT_STR(Obj, W61_Obj_Null_str);

  /* Wait message ready */
  if (W61_ATlock(Obj, W61_AT_LOCK_TIMEOUT))
  {
    /* Flush the read buffer if any the 'ready' message is received */
    (void)W61_ATD_Recv(Obj, Obj->CmdResp, W61_ATD_RSP_SIZE, 4000);
    W61_ATunlock(Obj);
  }
  else
  {
    LogError("IO Busy\n");
    ret = W61_STATUS_BUSY;
    goto __err;
  }

  /* Test a first AT command to check if the system is able to receive a response */
  if (W61_ATlock(Obj, W61_AT_LOCK_TIMEOUT))
  {
    snprintf((char *)Obj->CmdResp, W61_ATD_CMDRSP_STRING_SIZE, "AT\r\n");
    ret = W61_AT_SetExecute(Obj, Obj->CmdResp, Obj->NcpTimeout);
    W61_ATunlock(Obj);
  }
  else
  {
    ret = W61_STATUS_BUSY;
  }

  if (ret == W61_STATUS_TIMEOUT)
  {
    LogError("No received response from NCP\n");
    goto __err;
  }
  else if (ret != W61_STATUS_OK)
  {
    LogError("Disable store more failed\n");
    goto __err;
  }

__err:
  return ret;
}

W61_Status_t W61_SetTimeout(W61_Object_t *Obj, uint32_t Timeout)
{
  W61_Status_t ret = W61_STATUS_ERROR;
  W61_NULL_ASSERT_STR(Obj, W61_Obj_Null_str);

  Obj->NcpTimeout = Timeout;
  return W61_STATUS_OK;
}

W61_Status_t W61_ResetToFactoryDefault(W61_Object_t *Obj)
{
  W61_Status_t ret = W61_STATUS_ERROR;
  W61_NULL_ASSERT_STR(Obj, W61_Obj_Null_str);

  if (W61_ATlock(Obj, W61_AT_LOCK_TIMEOUT))
  {
    snprintf((char *)Obj->CmdResp, W61_ATD_CMDRSP_STRING_SIZE, "AT+RESTORE\r\n");
    ret = W61_AT_SetExecute(Obj, Obj->CmdResp, 2000);
    W61_ATunlock(Obj);
  }
  else
  {
    ret = W61_STATUS_BUSY;
  }

  if (ret != W61_STATUS_OK)
  {
    LogError("Restore factory default failed\n");
    goto __err;
  }

  /* Wait message ready */
  ret = W61_WaitReady(Obj);
  if (ret != W61_STATUS_OK)
  {
    LogError("Wait ready failed\n");
  }

  /* Add mandatory delay */
  vTaskDelay(100);

__err:
  return ret;
}

W61_Status_t W61_GetNcpHeapState(W61_Object_t *Obj, uint32_t *RemainingHeap, uint32_t *LwipHeap)
{
  W61_Status_t ret = W61_STATUS_ERROR;
  W61_NULL_ASSERT_STR(Obj, W61_Obj_Null_str);

  if (W61_ATlock(Obj, W61_AT_LOCK_TIMEOUT))
  {
    snprintf((char *)Obj->CmdResp, W61_ATD_CMDRSP_STRING_SIZE, "AT+SYSRAM?\r\n");
    ret = W61_AT_Query(Obj, Obj->CmdResp, Obj->CmdResp, Obj->NcpTimeout);
    if (ret == W61_STATUS_OK)
    {
      if (sscanf((char *)Obj->CmdResp, "+SYSRAM:%" SCNu32 ",%" SCNu32, RemainingHeap, LwipHeap) != 2)
      {
        LogError("%s\n", W61_Parsing_Error_str);
        ret = W61_STATUS_ERROR;
      }
    }
    W61_ATunlock(Obj);
  }
  else
  {
    ret = W61_STATUS_BUSY;
  }

  if (ret != W61_STATUS_OK)
  {
    ret = W61_STATUS_ERROR;
  }

  return ret;
}

W61_Status_t W61_GetStoreMode(W61_Object_t *Obj, uint32_t *mode)
{
  W61_Status_t ret = W61_STATUS_ERROR;
  W61_NULL_ASSERT_STR(Obj, W61_Obj_Null_str);

  if (W61_ATlock(Obj, W61_AT_LOCK_TIMEOUT))
  {
    snprintf((char *)Obj->CmdResp, W61_ATD_CMDRSP_STRING_SIZE, "AT+SYSSTORE?\r\n");
    ret = W61_AT_Query(Obj, Obj->CmdResp, Obj->CmdResp, Obj->NcpTimeout);
    if (ret == W61_STATUS_OK)
    {
      if (sscanf((char *)Obj->CmdResp, "+SYSSTORE:%" SCNu32, mode) != 1)
      {
        LogError("%s\n", W61_Parsing_Error_str);
        ret = W61_STATUS_ERROR;
      }
    }

    W61_ATunlock(Obj);
  }
  else
  {
    ret = W61_STATUS_BUSY;
  }

  return ret;
}

W61_Status_t W61_ReadEFuse(W61_Object_t *Obj, uint32_t addr, uint32_t nbytes, uint8_t *data)
{
  W61_Status_t ret = W61_STATUS_ERROR;
  uint32_t size = 0;
  char *str_token;
  W61_NULL_ASSERT_STR(Obj, W61_Obj_Null_str);
  W61_NULL_ASSERT(data);

  if (W61_ATlock(Obj, W61_AT_LOCK_TIMEOUT))
  {
    snprintf((char *)Obj->CmdResp, W61_ATD_CMDRSP_STRING_SIZE,
             "AT+EFUSE-R=%" PRIu32 ",\"0x%03" PRIx32 "\",1\r\n",
             nbytes, addr);
    ret = W61_AT_Query(Obj, Obj->CmdResp, Obj->CmdResp, Obj->NcpTimeout);
    if (ret == W61_STATUS_OK)
    {
      if (sscanf((char *)Obj->CmdResp, "+EFUSE-R:%" SCNu32, &size) != 1)
      {
        LogError("%s\n", W61_Parsing_Error_str);
        ret = W61_STATUS_ERROR;
      }
      else
      {
        str_token = strstr((char *)Obj->CmdResp, ",");
        if (str_token == NULL)
        {
          ret = W61_STATUS_ERROR;
        }
        else
        {
          memcpy(data, (void *)(str_token + 1), size);
        }
      }
    }

    W61_ATunlock(Obj);
  }
  else
  {
    ret = W61_STATUS_BUSY;
  }

  return ret;
}

W61_Status_t W61_FS_DeleteFile(W61_Object_t *Obj, char *filename)
{
  W61_Status_t ret = W61_STATUS_ERROR;
  W61_NULL_ASSERT_STR(Obj, W61_Obj_Null_str);
  W61_NULL_ASSERT(filename);

  if (W61_ATlock(Obj, W61_AT_LOCK_TIMEOUT))
  {
    /* Operation 0: Delete a file */
    snprintf((char *)Obj->CmdResp, W61_ATD_CMDRSP_STRING_SIZE, "AT+FS=0,0,\"%s\"\r\n", filename);
    ret = W61_AT_SetExecute(Obj, Obj->CmdResp, Obj->NcpTimeout);
    W61_ATunlock(Obj);
  }
  else
  {
    ret = W61_STATUS_BUSY;
  }

  return ret;
}

W61_Status_t W61_FS_CreateFile(W61_Object_t *Obj, char *filename)
{
  W61_Status_t ret = W61_STATUS_ERROR;
  W61_NULL_ASSERT_STR(Obj, W61_Obj_Null_str);
  W61_NULL_ASSERT(filename);

  if (W61_ATlock(Obj, W61_AT_LOCK_TIMEOUT))
  {
    /* Operation 1: Create a file */
    snprintf((char *)Obj->CmdResp, W61_ATD_CMDRSP_STRING_SIZE, "AT+FS=0,1,\"%s\"\r\n", filename);
    ret = W61_AT_SetExecute(Obj, Obj->CmdResp, Obj->NcpTimeout);
    W61_ATunlock(Obj);
  }
  else
  {
    ret = W61_STATUS_BUSY;
  }

  return ret;
}

W61_Status_t W61_FS_WriteFile(W61_Object_t *Obj, char *filename, uint32_t offset, uint8_t *data, uint32_t len)
{
  W61_Status_t ret = W61_STATUS_ERROR;
  W61_NULL_ASSERT_STR(Obj, W61_Obj_Null_str);
  W61_NULL_ASSERT(filename);
  W61_NULL_ASSERT(data);

  if (W61_ATlock(Obj, W61_AT_LOCK_TIMEOUT))
  {
    /* Operation 2: Write data to a file */
    snprintf((char *)Obj->CmdResp, W61_ATD_CMDRSP_STRING_SIZE,
             "AT+FS=0,2,\"%s\",%" PRIu32 ",%" PRIu32 "\r\n",
             filename, offset, len);
    ret = W61_AT_SetExecute(Obj, Obj->CmdResp, W61_SYS_TIMEOUT);
    if (ret == W61_STATUS_OK)
    {
      /* Send the chunk of data */
      ret = W61_AT_RequestSendData(Obj, data, len, W61_SYS_TIMEOUT);
    }

    W61_ATunlock(Obj);
  }
  else
  {
    ret = W61_STATUS_BUSY;
  }

  return ret;
}

W61_Status_t W61_FS_ReadFile(W61_Object_t *Obj, char *filename, uint32_t offset, uint8_t *data, uint32_t len)
{
  W61_Status_t ret = W61_STATUS_ERROR;
  int32_t recv_len = 0;
  uint32_t count_digit = 0;
  uint32_t total_len = 0;
  W61_NULL_ASSERT_STR(Obj, W61_Obj_Null_str);
  W61_NULL_ASSERT(filename);
  W61_NULL_ASSERT(data);

  if (W61_ATlock(Obj, W61_AT_LOCK_TIMEOUT))
  {
    /* Operation 3: Read data from a file */
    snprintf((char *)Obj->CmdResp, W61_ATD_CMDRSP_STRING_SIZE,
             "AT+FS=0,3,\"%s\",%" PRIu32 ",%" PRIu32 "\r\n",
             filename, offset, len);

    if (W61_ATsend(Obj, Obj->CmdResp, strlen((char *)Obj->CmdResp), Obj->NcpTimeout) > 0)
    {
      /* Wait the response '+FS:READ,' */
      recv_len = W61_ATD_Recv(Obj, Obj->CmdResp, W61_ATD_CMDRSP_STRING_SIZE, W61_SYS_TIMEOUT);
      if (recv_len > 0)
      {
        /* Get the total length of the data */
        if ((sscanf((char *)Obj->CmdResp, "+FS:READ,%" SCNu32 ",", &total_len) == 1) && (total_len == len))
        {
          do /* Calculate the number of digits to right shift the first data */
          {
            total_len /= 10;
            ++count_digit;
          } while (total_len != 0);

          recv_len -= (strlen("+FS:READ,") + count_digit + 1); /* Remove the '+FS:READ,' and the number of digits */

          /* Copy the first chunk of data */
          memcpy(data, (char *)Obj->CmdResp + strlen("+FS:READ,") + count_digit + 1, recv_len);
          total_len = recv_len;

          while ((total_len < len) && (recv_len > 0)) /* Loop if some remaining data */
          {
            recv_len = W61_ATD_Recv(Obj, Obj->CmdResp, W61_ATD_CMDRSP_STRING_SIZE, W61_SYS_TIMEOUT);
            if (recv_len > 0)
            {
              if (recv_len + total_len > len) /* Check if the data is bigger than the buffer. Remove the last CRLF */
              {
                memcpy(&data[total_len], (char *)Obj->CmdResp, len - total_len); /* Copy the remaining data */
                total_len = len;
              }
              else
              {
                memcpy(&data[total_len], (char *)Obj->CmdResp, recv_len); /* Copy the next chunk of data */
                total_len += recv_len;
              }
            }
            else
            {
              ret = W61_STATUS_TIMEOUT;
              break; /* Exit the loop if no data received */
            }
          }
        }
      }
      else
      {
        ret = W61_STATUS_TIMEOUT;
      }
    }

    if (recv_len > 0) /* If all receive data are OK, Check the last OK/Error response */
    {
      recv_len = W61_ATD_Recv(Obj, Obj->CmdResp, W61_ATD_CMDRSP_STRING_SIZE, Obj->NcpTimeout);
      if (recv_len > 2)
      {
        Obj->CmdResp[recv_len] = '\0'; /* Null terminate the response */
        ret = W61_AT_ParseOkErr((char *)Obj->CmdResp);
      }
      else if (recv_len == 0)
      {
        ret = W61_STATUS_TIMEOUT;
      }
    }

    W61_ATunlock(Obj);
  }
  else
  {
    ret = W61_STATUS_BUSY;
  }

  return ret;
}

W61_Status_t W61_FS_GetSizeFile(W61_Object_t *Obj, char *filename, uint32_t *size)
{
  W61_Status_t ret = W61_STATUS_ERROR;
  W61_NULL_ASSERT_STR(Obj, W61_Obj_Null_str);
  W61_NULL_ASSERT(filename);
  W61_NULL_ASSERT(size);

  if (W61_ATlock(Obj, W61_AT_LOCK_TIMEOUT))
  {
    /* Operation 4: Get the size of a file */
    snprintf((char *)Obj->CmdResp, W61_ATD_CMDRSP_STRING_SIZE, "AT+FS=0,4,\"%s\"\r\n", filename);
    ret = W61_AT_Query(Obj, Obj->CmdResp, Obj->CmdResp, W61_SYS_TIMEOUT);
    if (ret == W61_STATUS_OK)
    {
      if (sscanf((char *)Obj->CmdResp, "+FS:SIZE,%" SCNu32, size) != 1)
      {
        ret = W61_STATUS_ERROR;
      }
    }
    W61_ATunlock(Obj);
  }
  else
  {
    ret = W61_STATUS_BUSY;
  }

  return ret;
}

W61_Status_t W61_FS_ListFiles(W61_Object_t *Obj, W61_FS_FilesList_t *files_list)
{
  W61_Status_t ret = W61_STATUS_ERROR;
  int32_t recv_len;
  W61_NULL_ASSERT_STR(Obj, W61_Obj_Null_str);
  W61_NULL_ASSERT_STR(files_list, "File list pointer is NULL");

  if (W61_ATlock(Obj, W61_AT_LOCK_TIMEOUT))
  {
    files_list->nb_files = 0; /* Reset the number of files */

    /* Operation 5: List the NCP files in root path */
    snprintf((char *)Obj->CmdResp, W61_ATD_CMDRSP_STRING_SIZE, "AT+FS=0,5,\".\"\r\n");
    if (W61_ATsend(Obj, Obj->CmdResp, strlen((char *)Obj->CmdResp), Obj->NcpTimeout) > 0)
    {
      /* Wait the first response +FS:LIST */
      recv_len = W61_ATD_Recv(Obj, Obj->CmdResp, W61_ATD_CMDRSP_STRING_SIZE, W61_SYS_TIMEOUT);
      if (recv_len > 0)
      {
        if (strncmp((char *)Obj->CmdResp, "+FS:LIST", sizeof("+FS:LIST") - 1) == 0)
        {
          ret = W61_STATUS_OK;
        }
      }
      else
      {
        ret = W61_STATUS_TIMEOUT;
      }

      if (ret == W61_STATUS_OK)
      {
        recv_len = W61_ATD_Recv(Obj, Obj->CmdResp, W61_ATD_CMDRSP_STRING_SIZE, W61_SYS_TIMEOUT);

        while (recv_len > 0) /* Loop until OK or ERROR are received and process the filename */
        {
          Obj->CmdResp[recv_len] = '\0'; /* Null terminate the response */

          if ((strncmp((char *)Obj->CmdResp, AT_OK_STRING, sizeof(AT_OK_STRING) - 1) == 0) ||
              (strncmp((char *)Obj->CmdResp, AT_ERROR_STRING, sizeof(AT_ERROR_STRING) - 1) == 0))
          {
            break; /* Exit the loop if OK or ERROR are received */
          }

          if (Obj->CmdResp[0] == '.') /* Skip the current and parent directory entries */
          {
            recv_len = W61_ATD_Recv(Obj, Obj->CmdResp, W61_ATD_CMDRSP_STRING_SIZE, W61_SYS_TIMEOUT);
            continue;
          }

          /* Copy the filename */
          snprintf(files_list->filename[files_list->nb_files], W61_SYS_FS_FILENAME_SIZE, "%s", Obj->CmdResp);

          /* Remove the CRLF in the filename string */
          files_list->filename[files_list->nb_files][strlen(files_list->filename[files_list->nb_files]) - 2] = '\0';

          files_list->nb_files++; /* Increment the number of files */

          recv_len = W61_ATD_Recv(Obj, Obj->CmdResp, W61_ATD_CMDRSP_STRING_SIZE, Obj->NcpTimeout);
        }

        ret = W61_AT_ParseOkErr((char *)Obj->CmdResp);
      }
    }
    W61_ATunlock(Obj);
  }
  else
  {
    ret = W61_STATUS_BUSY;
  }

  return ret;
}

W61_Status_t W61_GetModuleInfo(W61_Object_t *Obj)
{
  W61_Status_t ret = W61_STATUS_ERROR;
  int32_t recv_len;
  int32_t tmp = 0;
  uint32_t data_received = 0;
  uint32_t data[8] = {0};
  W61_NULL_ASSERT_STR(Obj, W61_Obj_Null_str);

  memset(&Obj->ModuleInfo, 0, sizeof(W61_ModuleInfo_t));

  /* ====================================== */
  /* Get the module info */
  /* ====================================== */
  if (W61_ATlock(Obj, W61_AT_LOCK_TIMEOUT))
  {
    snprintf((char *)Obj->CmdResp, W61_ATD_CMDRSP_STRING_SIZE, "AT+GMR\r\n");
    if (W61_ATsend(Obj, Obj->CmdResp, strlen((char *)Obj->CmdResp), Obj->NcpTimeout) > 0)
    {
      W61_VersionInfo_t version_table[] =
      {
        {(char *)Obj->ModuleInfo.AT_Version,  "AT version:", 32},
        {(char *)Obj->ModuleInfo.MAC_Version, "component_version_macsw_", 32},
        {(char *)Obj->ModuleInfo.SDK_Version, "component_version_sdk_", 32},
        {(char *)Obj->ModuleInfo.Build_Date,  "compile time:", 32},
      };

      do
      {
        recv_len = W61_ATD_Recv(Obj, Obj->CmdResp, W61_ATD_RSP_SIZE, Obj->NcpTimeout);
        if (recv_len > 0)
        {
          data_received = 1; /* At least one data received */
          /* Parse if its an OK/ERROR message */
          /* Exit the loop if OK or ERROR are received */
          if (strncmp((char *)Obj->CmdResp, AT_OK_STRING, sizeof(AT_OK_STRING) - 1) == 0)
          {
            ret = W61_STATUS_OK;
            break;
          }
          else if (strncmp((char *)Obj->CmdResp, AT_ERROR_STRING, sizeof(AT_ERROR_STRING) - 1) == 0)
          {
            ret = W61_STATUS_ERROR;
            break;
          }

          /* Check if the response is a version string */
          for (int32_t i = 0; i < sizeof(version_table) / sizeof(version_table[0]); i++)
          {
            if (strncmp(version_table[i].prefix, (char *)Obj->CmdResp, strlen(version_table[i].prefix)) == 0)
            {
              Obj->CmdResp[recv_len - sizeof(CRLF) + 1] = 0; /* Remove end of line characters */
              strncpy(version_table[i].dest, (char *)Obj->CmdResp + strlen(version_table[i].prefix),
                      version_table[i].dest_size);

              /* Split build date from AT version string */
              if (strncmp(version_table[i].prefix, "AT version:", sizeof("AT version:") - 1) == 0)
              {
                char *date = strchr(version_table[i].dest, '(');
                if (date != NULL)
                {
                  /* Stop AT_version string at '(' character */
                  *date = 0;
                }
              }
              break;
            }
          }
        }
      } while (recv_len > 0);

      if (!data_received)
      {
        ret = W61_STATUS_TIMEOUT;
      }
    }
    W61_ATunlock(Obj);
  }
  else
  {
    ret = W61_STATUS_BUSY;
  }

  if (ret != W61_STATUS_OK)
  {
    goto _err;
  }

  /* ====================================== */
  /* Battery voltage */
  /* ====================================== */
  if (W61_ATlock(Obj, W61_AT_LOCK_TIMEOUT))
  {
    snprintf((char *)Obj->CmdResp, W61_ATD_CMDRSP_STRING_SIZE, "AT+VBAT?\r\n");
    ret = W61_AT_Query(Obj, Obj->CmdResp, Obj->CmdResp, Obj->NcpTimeout);
    if (ret == W61_STATUS_OK)
    {
      if (strncmp((char *)Obj->CmdResp, "+VBAT:", sizeof("+VBAT:") - 1) == 0)
      {
        if (Parser_StrToInt((char *)Obj->CmdResp + strlen("+VBAT:"), NULL, &tmp) == 0)
        {
          ret = W61_STATUS_ERROR;
        }
        else
        {
          Obj->ModuleInfo.BatteryVoltage = (uint32_t)tmp;
        }
      }
    }
    W61_ATunlock(Obj);
  }
  else
  {
    ret = W61_STATUS_BUSY;
  }

  if (ret != W61_STATUS_OK)
  {
    goto _err;
  }

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
        LogError("Unable to read %s\n", trim_table[i].desc);
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
          LogError("Unable to read %s\n", trim_table[i].desc);
        }
        goto _err;
      }
      ret = W61_ReadEFuse(Obj, trim_table[i].value_addr, 4, (uint8_t *)&data[1]);
      if (ret != W61_STATUS_OK)
      {
        if (ret == W61_STATUS_ERROR)
        {
          LogError("Unable to read %s\n", trim_table[i].desc);
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
      LogError("Unable to read Part number\n");
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
    memcpy(Obj->ModuleInfo.ModuleID, part_number, sizeof(Obj->ModuleInfo.ModuleID));
  }

  /* ====================================== */
  /* BOM ID + Manufacturing Year/Week */
  /* ====================================== */
  ret = W61_ReadEFuse(Obj, EFUSE_MANUF_BOM_ADDR, EFUSE_MANUF_BOM_LEN, (uint8_t *)data);
  if (ret != W61_STATUS_OK)
  {
    if (ret == W61_STATUS_ERROR)
    {
      LogError("Unable to read BOM ID & Manufacturing Year/Week\n");
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
        LogError("Unable to read %s\n", efuse_mac_table[i].desc);
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
      LogError("Unable to read Anti-rollback enable\n");
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
          LogError("Unable to read Anti-rollback %s\n", efuse_antirollback_table[i].desc);
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

W61_Status_t W61_OTA_starts(W61_Object_t *Obj, uint32_t enable)
{
  W61_Status_t ret = W61_STATUS_ERROR;
  W61_NULL_ASSERT_STR(Obj, W61_Obj_Null_str);

  if (W61_ATlock(Obj, W61_AT_LOCK_TIMEOUT))
  {
    snprintf((char *)Obj->CmdResp, W61_ATD_CMDRSP_STRING_SIZE,
             "AT+OTASTART=%" PRIu32 "\r\n", enable);
    ret = W61_AT_SetExecute(Obj, Obj->CmdResp, W61_SYS_TIMEOUT);
    W61_ATunlock(Obj);
  }
  else
  {
    ret = W61_STATUS_BUSY;
  }
  return ret;
}

W61_Status_t W61_OTA_Finish(W61_Object_t *Obj)
{
  W61_Status_t ret = W61_STATUS_ERROR;
  W61_NULL_ASSERT_STR(Obj, W61_Obj_Null_str);

  if (W61_ATlock(Obj, W61_AT_LOCK_TIMEOUT))
  {
    snprintf((char *)Obj->CmdResp, W61_ATD_CMDRSP_STRING_SIZE, "AT+OTAFIN\r\n");
    ret = W61_AT_SetExecute(Obj, Obj->CmdResp, W61_SYS_TIMEOUT);
    W61_ATunlock(Obj);
  }
  else
  {
    ret = W61_STATUS_BUSY;
  }
  return ret;
}

W61_Status_t W61_OTA_Send(W61_Object_t *Obj, uint8_t *buff, uint32_t len)
{
  W61_Status_t ret = W61_STATUS_ERROR;
  W61_NULL_ASSERT_STR(Obj, W61_Obj_Null_str);
  W61_NULL_ASSERT(buff);

  if (W61_ATlock(Obj, W61_AT_LOCK_TIMEOUT))
  {
    snprintf((char *)Obj->CmdResp, W61_ATD_CMDRSP_STRING_SIZE,
             "AT+OTASEND=%" PRIu32 "\r\n", len);
    ret = W61_AT_SetExecute(Obj, Obj->CmdResp, W61_SYS_TIMEOUT);
    if (ret == W61_STATUS_OK)
    {
      Obj->fops.IO_Delay(OTA_DATA_SEND_DELAY);
      ret = W61_AT_RequestSendData(Obj, buff, len, W61_SYS_TIMEOUT);
    }
    W61_ATunlock(Obj);
  }
  else
  {
    ret = W61_STATUS_BUSY;
  }
  return ret;
}

W61_Status_t W61_LowPowerConfig(W61_Object_t *Obj, uint32_t WakeUpPinIn, uint32_t ps_mode)
{
  W61_Status_t ret = W61_STATUS_ERROR;
  W61_NULL_ASSERT_STR(Obj, W61_Obj_Null_str);

  /* In Hibernate Mode, only WakeUpPin 16 can be used*/
  if ((ps_mode == 1) && (WakeUpPinIn != 16))
  {
    return W61_STATUS_ERROR;
  }

  Obj->LowPowerCfg.WakeUpPinIn = WakeUpPinIn;
  Obj->LowPowerCfg.PSMode = ps_mode;
  Obj->LowPowerCfg.WiFi_DTIM = 0; /* Disabled by default */

  return W61_STATUS_OK;
}

W61_Status_t W61_SetPowerMode(W61_Object_t *Obj, uint32_t ps_mode, uint32_t hbn_level)
{
  W61_Status_t ret = W61_STATUS_ERROR;
  W61_NULL_ASSERT_STR(Obj, W61_Obj_Null_str);

  if (W61_ATlock(Obj, W61_AT_LOCK_TIMEOUT))
  {
    if (ps_mode == 1) /* Hibernate mode */
    {
      snprintf((char *)Obj->CmdResp, W61_ATD_CMDRSP_STRING_SIZE,
               "AT+PWR=%" PRIu32 ",%" PRIu32 "\r\n", ps_mode, hbn_level);
      int32_t cmd_len = strlen((char *)Obj->CmdResp);
      /* send only. No response from the ST67W when in hibernate ps mode*/
      if (W61_ATsend(Obj, Obj->CmdResp, cmd_len, Obj->NcpTimeout) == cmd_len)
      {
        ret = W61_STATUS_OK;
      }
    }
    else
    {
      snprintf((char *)Obj->CmdResp, W61_ATD_CMDRSP_STRING_SIZE,
               "AT+PWR=%" PRIu32 "\r\n", ps_mode);
      ret = W61_AT_SetExecute(Obj, Obj->CmdResp, Obj->NcpTimeout);
    }

    if (ret == W61_STATUS_OK)
    {
      Obj->LowPowerCfg.PSMode = ps_mode;
    }

    W61_ATunlock(Obj);
  }
  else
  {
    ret = W61_STATUS_BUSY;
  }

  return ret;
}

W61_Status_t W61_GetPowerMode(W61_Object_t *Obj, uint32_t *ps_mode)
{
  *ps_mode = Obj->LowPowerCfg.PSMode;
  return W61_STATUS_OK;
}

W61_Status_t W61_SetWakeUpPin(W61_Object_t *Obj, uint32_t wakeup_pin)
{
  W61_Status_t ret;

  if (W61_ATlock(Obj, W61_AT_LOCK_TIMEOUT))
  {
    snprintf((char *)Obj->CmdResp, W61_ATD_CMDRSP_STRING_SIZE,
             "AT+SLWKIO=%" PRIu32 ",0\r\n", wakeup_pin);
    ret = W61_AT_SetExecute(Obj, Obj->CmdResp, Obj->NcpTimeout);
    W61_ATunlock(Obj);
  }
  else
  {
    ret = W61_STATUS_BUSY;
  }
  return ret;
}

W61_Status_t W61_SetClockSource(W61_Object_t *Obj, uint32_t source)
{
  W61_Status_t ret = W61_STATUS_ERROR;
  int32_t recv_len;

  if ((source == 0) || (source > 3))
  {
    return ret;
  }

  if (W61_ATlock(Obj, W61_AT_LOCK_TIMEOUT))
  {
    snprintf((char *)Obj->CmdResp, W61_ATD_CMDRSP_STRING_SIZE,
             "AT+SET_CLOCK=%" PRIu32 "\r\n", source);

    if (W61_ATsend(Obj, Obj->CmdResp, strlen((char *)Obj->CmdResp), Obj->NcpTimeout) > 0)
    {
      recv_len = W61_ATD_Recv(Obj, Obj->CmdResp, W61_ATD_CMDRSP_STRING_SIZE, CLOCK_TIMEOUT);
      if (recv_len > 0)
      {
        /* Check if the response is an OK or 'ready' message */
        if ((strncmp((char *)Obj->CmdResp, AT_OK_STRING, sizeof(AT_OK_STRING) - 1) == 0) ||
            (strncmp((char *)Obj->CmdResp, "ready", sizeof("ready") - 1) == 0))
        {
          ret = W61_STATUS_OK;
        }
      }
      else
      {
        ret = W61_STATUS_TIMEOUT;
      }
    }
    W61_ATunlock(Obj);
  }
  else
  {
    ret = W61_STATUS_BUSY;
  }
  return ret;
}

W61_Status_t W61_GetClockSource(W61_Object_t *Obj, uint32_t *source)
{
  W61_Status_t ret = W61_STATUS_BUSY;

  if (W61_ATlock(Obj, W61_AT_LOCK_TIMEOUT))
  {
    snprintf((char *)Obj->CmdResp, W61_ATD_CMDRSP_STRING_SIZE, "AT+GET_CLOCK\r\n");
    ret = W61_AT_Query(Obj, Obj->CmdResp, Obj->CmdResp, Obj->NcpTimeout);
    if (ret == W61_STATUS_OK)
    {
      if (sscanf((char *)Obj->CmdResp, "+GET_CLOCK:%" SCNu32, source) != 1)
      {
        ret = W61_STATUS_ERROR;
      }
    }
    W61_ATunlock(Obj);
  }

  return ret;
}

W61_Status_t W61_ExeATCommand(W61_Object_t *Obj, char *at_cmd)
{
  W61_Status_t ret = W61_STATUS_OK;

  if (W61_ATlock(Obj, W61_AT_LOCK_TIMEOUT))
  {
    snprintf((char *)Obj->CmdResp, W61_ATD_CMDRSP_STRING_SIZE, "%s\r\n", at_cmd);
    uint32_t cmd_len = strlen((char *)Obj->CmdResp);
    uint32_t rcvlen = 0;

    if (W61_ATsend(Obj, (uint8_t *)Obj->CmdResp, cmd_len, W61_SYS_TIMEOUT) == cmd_len)
    {
      LogInfo("%s", Obj->CmdResp);
      /* Receive the responses and check if the returned length is greater than 0 */
      while ((rcvlen = W61_ATD_Recv(Obj, Obj->CmdResp, W61_ATD_CMDRSP_STRING_SIZE, W61_SYS_TIMEOUT)) > 0)
      {
        Obj->CmdResp[rcvlen] = '\0';
        LogInfo("%s",  Obj->CmdResp);
        /* Stop when Response is either OK or ERROR*/
        if ((strstr((char *)Obj->CmdResp, "OK") != NULL) || (strstr((char *)Obj->CmdResp, "ERROR") != NULL))
        {
          break;
        }
      }
    }
    else
    {
      ret = W61_STATUS_ERROR;
    }
    W61_ATunlock(Obj);
  }
  return ret;
}

/** @} */
