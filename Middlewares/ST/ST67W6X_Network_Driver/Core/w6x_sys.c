/**
  ******************************************************************************
  * @file    w6x_sys.c
  * @author  GPM Application Team
  * @brief   This file provides code for W6x System API
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
#include <string.h>
#include "w6x_api.h"       /* Prototypes of the functions implemented in this file */
#include "w6x_version.h"
#include "w61_at_api.h"    /* Prototypes of the functions called by this file */
#include "w6x_internal.h"
#include "w61_io.h"        /* Prototypes of the BUS functions to be registered */
#include "common_parser.h" /* Common Parser functions */

/* Private defines -----------------------------------------------------------*/
/** @defgroup ST67W6X_Private_System_Constants ST67W6X System Constants
  * @ingroup  ST67W6X_Private_System
  * @{
  */

#define W6X_FS_READ_BLOCK_SIZE  256 /*!< File system read block size */

#ifndef HAL_SYS_RESET
/** HAL System software reset function */
extern void HAL_NVIC_SystemReset(void);
/** HAL System software reset macro */
#define HAL_SYS_RESET() do{ HAL_NVIC_SystemReset(); } while(0);
#endif /* HAL_SYS_RESET */

/** @} */

/* Private macros ------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/** @defgroup ST67W6X_Private_System_Variables ST67W6X System Variables
  * @ingroup  ST67W6X_Private_System
  * @{
  */

static W61_Object_t *p_DrvObj = NULL;                 /*!< Global W61 context pointer */

/** W6X System init error string */
static const char W6X_Sys_Uninit_str[] = "W6X System module not initialized";

static W6X_App_Cb_t W6X_CbHandler;                    /*!< W6X Applicative Callbacks handler */

static W6X_FS_FilesListFull_t *W6X_FilesList = NULL;  /*!< List of files */

static W6X_ModuleInfo_t *p_module_info = NULL;        /*!< W61 module info */

/** @} */

/* Private function prototypes -----------------------------------------------*/
/** @defgroup ST67W6X_Private_System_Functions ST67W6X System Functions
  * @ingroup  ST67W6X_Private_System
  */

/* Functions Definition ------------------------------------------------------*/
/** @addtogroup ST67W6X_API_System_Public_Functions
  * @{
  */

W6X_Status_t W6X_Init(void)
{
  W6X_Status_t ret = W6X_STATUS_ERROR;
  uint32_t clock_source = 0;

  p_DrvObj = W61_ObjGet();
  NULL_ASSERT(p_DrvObj, W6X_Obj_Null_str);

  /* Register the Bus IO callbacks */
  ret = TranslateErrorStatus(W61_RegisterBusIO(p_DrvObj,
                                               BusIo_SPI_Init,
                                               BusIo_SPI_DeInit,
                                               BusIo_SPI_Delay,
                                               BusIo_SPI_Send,
                                               BusIo_SPI_Receive));
  /* Configure the default power save mode and WakeUp pin */
  W61_LowPowerConfig(p_DrvObj, 28, 2);

  if (ret != W6X_STATUS_OK)
  {
    LogError("Register Bus IO failed\n");
    goto _err;
  }

  /* Initialize the W61 module */
  ret = TranslateErrorStatus(W61_Init(p_DrvObj));
  if (ret != W6X_STATUS_OK)
  {
    LogError("W61 Init failed\n");
    goto _err;
  }

  /* Configure the current W61 clock used */
  ret = TranslateErrorStatus(W61_GetClockSource(p_DrvObj, &clock_source));
  if (ret != W6X_STATUS_OK)
  {
    LogError("Get W61 clock source failed\n");
    goto _err;
  }

  if (clock_source != W6X_CLOCK_MODE)
  {
    /* Set the chosen clock source depending of the hardware configuration */
    ret = TranslateErrorStatus(W61_SetClockSource(p_DrvObj, W6X_CLOCK_MODE));
    if (ret != W6X_STATUS_OK)
    {
      LogError("Set W61 clock failed\n");
      goto _err;
    }
    /* Reboot the host to apply the clock change */
    HAL_SYS_RESET();
  }

  /* Get the W61 info */
  ret = TranslateErrorStatus(W61_GetModuleInfo(p_DrvObj));
  if (ret != W6X_STATUS_OK)
  {
    LogError("Get W61 Info failed\n");
    goto _err;
  }

  /* Store the W61 information */
  p_module_info = (W6X_ModuleInfo_t *)&p_DrvObj->ModuleInfo;

  /* Display the W61 information in banner */
  (void)W6X_ModuleInfoDisplay();

#if (LFS_ENABLE == 1)
  /* Initialize the File system with user certificates */
  easyflash_init();
#endif /* LFS_ENABLE */

  /* Set the wake-up pin to the ST67W611M */
  (void)W61_SetWakeUpPin(p_DrvObj, p_DrvObj->LowPowerCfg.WakeUpPinIn);

#if (W6X_POWER_SAVE_AUTO == 1)
  /* Enable the power mode */
  (void)W6X_SetPowerMode(1);
#endif /* W6X_POWER_SAVE_AUTO */

  ret = W6X_STATUS_OK;

_err:
  return ret;
}

W6X_Status_t W6X_DeInit(void)
{
  W6X_Status_t ret;
  /* Set to hibernate*/
  (void)W61_SetPowerMode(p_DrvObj, 1, 0);
  /* DeInit and power-off the ST67 by resetting the CHIP_EN pin */
  ret = TranslateErrorStatus(W61_DeInit(p_DrvObj));

  if (W6X_FilesList != NULL)
  {
    /* Free the files list */
    vPortFree(W6X_FilesList);
    W6X_FilesList = NULL;
  }

  p_module_info = NULL; /* Reset the module info */
  p_DrvObj = NULL; /* Reset the global pointer */

  return ret;
}

W6X_Status_t W6X_ModuleInfoDisplay(void)
{
  W6X_Status_t ret = W6X_STATUS_ERROR;
  NULL_ASSERT(p_DrvObj, W6X_Sys_Uninit_str);
  NULL_ASSERT(p_module_info, "Module info not available");

  /* Display the W61 information */
  LogInfo("--------------- ST67W6X info ------------\n");
  LogInfo("ST67W6X MW Version:       " W6X_VERSION_STR "\n");
  LogInfo("AT Version:               %s\n", p_module_info->AT_Version);
  LogInfo("SDK Version:              %s\n", p_module_info->SDK_Version);
  LogInfo("MAC Version:              %s\n", p_module_info->MAC_Version);
  LogInfo("Build Date:               %s\n", p_module_info->Build_Date);
  LogInfo("Module ID:                %s\n", p_module_info->ModuleID);
  LogInfo("BOM ID:                   %" PRIu16 "\n", p_module_info->BomID);
  LogInfo("Manufacturing Year:       20%02" PRIu16 "\n", p_module_info->Manufacturing_Year);
  LogInfo("Manufacturing Week:       %02" PRIu16 "\n", p_module_info->Manufacturing_Week);
  LogInfo("Battery Voltage:          %" PRIu32 ".%" PRIu32 " V\n", p_module_info->BatteryVoltage / 1000,
          p_module_info->BatteryVoltage % 1000);
  LogInfo("Trim Wi-Fi hp:            %" PRIi16 ",%" PRIi16 ",%" PRIi16 ",%" PRIi16 ",%" PRIi16 ",%" PRIi16 ",%" PRIi16
          ",%" PRIi16 ",%" PRIi16 ",%" PRIi16 ",%" PRIi16 ",%" PRIi16 ",%" PRIi16 ",%" PRIi16 "\n",
          p_module_info->trim_wifi_hp[0], p_module_info->trim_wifi_hp[1],
          p_module_info->trim_wifi_hp[2], p_module_info->trim_wifi_hp[3],
          p_module_info->trim_wifi_hp[4], p_module_info->trim_wifi_hp[5],
          p_module_info->trim_wifi_hp[6], p_module_info->trim_wifi_hp[7],
          p_module_info->trim_wifi_hp[8], p_module_info->trim_wifi_hp[9],
          p_module_info->trim_wifi_hp[10], p_module_info->trim_wifi_hp[11],
          p_module_info->trim_wifi_hp[12], p_module_info->trim_wifi_hp[13]);
  LogInfo("Trim Wi-Fi lp:            %" PRIi16 ",%" PRIi16 ",%" PRIi16 ",%" PRIi16 ",%" PRIi16 ",%" PRIi16 ",%" PRIi16
          ",%" PRIi16 ",%" PRIi16 ",%" PRIi16 ",%" PRIi16 ",%" PRIi16 ",%" PRIi16 ",%" PRIi16 "\n",
          p_module_info->trim_wifi_lp[0], p_module_info->trim_wifi_lp[1],
          p_module_info->trim_wifi_lp[2], p_module_info->trim_wifi_lp[3],
          p_module_info->trim_wifi_lp[4], p_module_info->trim_wifi_lp[5],
          p_module_info->trim_wifi_lp[6], p_module_info->trim_wifi_lp[7],
          p_module_info->trim_wifi_lp[8], p_module_info->trim_wifi_lp[9],
          p_module_info->trim_wifi_lp[10], p_module_info->trim_wifi_lp[11],
          p_module_info->trim_wifi_lp[12], p_module_info->trim_wifi_lp[13]);
  LogInfo("Trim BLE:                 %" PRIi16 ",%" PRIi16 ",%" PRIi16 ",%" PRIi16 ",%" PRIi16 "\n",
          p_module_info->trim_ble[0], p_module_info->trim_ble[1], p_module_info->trim_ble[2],
          p_module_info->trim_ble[3], p_module_info->trim_ble[4]);
  LogInfo("Trim XTAL:                %" PRIi16 "\n", p_module_info->trim_xtal);
  LogInfo("MAC Address:              " MACSTR "\n", MAC2STR(p_module_info->Mac_Address));
  LogInfo("Anti-rollback Bootloader: %" PRIu16 "\n", p_module_info->AntiRollbackBootloader);
  LogInfo("Anti-rollback App:        %" PRIu16 "\n", p_module_info->AntiRollbackApp);
  LogInfo("-----------------------------------------\n");

  return W6X_STATUS_OK;
}

W6X_App_Cb_t *W6X_GetCbHandler(void)
{
  /* Return the W6X callback handler */
  return &W6X_CbHandler;
}

W6X_ModuleInfo_t *W6X_GetModuleInfo(void)
{
  /* Return the W6X module info */
  return p_module_info;
}

W6X_Status_t W6X_RegisterAppCb(W6X_App_Cb_t *App_cb)
{
  W6X_Status_t ret = W6X_STATUS_ERROR;
  NULL_ASSERT(App_cb, W6X_Sys_Uninit_str);

  /* Register the application callback */
  W6X_CbHandler = *App_cb;

  return W6X_STATUS_OK;
}

W6X_Status_t W6X_FS_WriteFile(char *filename)
{
#if (LFS_ENABLE == 1)
  W6X_FS_FilesListFull_t *files_list = NULL;
  uint32_t file_lfs_index = 0;
  uint32_t file_ncp_index = 0;
  uint8_t buf[W6X_FS_READ_BLOCK_SIZE + 1] = {0};
  int32_t read_len = 0;
  uint32_t offset = 0;
  uint32_t file_lfs_size = 0;
  W6X_Status_t ret = W6X_STATUS_ERROR;
  NULL_ASSERT(p_DrvObj, W6X_Sys_Uninit_str);

  ret = W6X_FS_ListFiles(&files_list); /* Get the files list available in the NCP */
  if (ret != W6X_STATUS_OK)
  {
    if (ret == W6X_STATUS_ERROR)
    {
      LogError("Unable to list files\n");
    }
    goto _err;
  }

  /* Check if the file exists in the lfs_files_list to be copied in NCP */
  for (; file_lfs_index < files_list->nb_files; file_lfs_index++)
  {
    if (strncmp(files_list->lfs_files_list[file_lfs_index].name, filename, strlen(filename)) == 0)
    {
      /* File found in the lfs_files_list */
      file_lfs_size = files_list->lfs_files_list[file_lfs_index].size;
      break;
    }
  }

  if (file_lfs_index == files_list->nb_files)
  {
    /* File not found in the lfs_files_list */
    LogError("File not found in the Host LFS. Verify the filename and littlefs.bin generation");
    goto _err;
  }

  /* Check if the file exists in the NCP */
  for (; file_ncp_index < files_list->ncp_files_list.nb_files; file_ncp_index++)
  {
    if (strncmp(files_list->ncp_files_list.filename[file_ncp_index], filename, strlen(filename)) == 0)
    {
      uint32_t size = 0;
      /* Get the NCP file size */
      ret = TranslateErrorStatus(W61_FS_GetSizeFile(p_DrvObj, filename, &size));
      if (ret != W6X_STATUS_OK)
      {
        goto _err;
      }

      /* File found. Must to check if the size of Host lfs file and NCP file are equal */
      if (files_list->lfs_files_list[file_lfs_index].size == size)
      {
        uint8_t ncp_buf[W6X_FS_READ_BLOCK_SIZE + 1] = {0};
        uint32_t read_offset = 0;
        uint32_t part_len;

        while (read_offset < size) /* Read and compare the files by block */
        {
          part_len = (size - read_offset) < W6X_FS_READ_BLOCK_SIZE ? (size - read_offset) : W6X_FS_READ_BLOCK_SIZE;
          /* Read a part of the NCP file */
          ret = W6X_FS_ReadFile(filename, read_offset, ncp_buf, part_len);
          if (ret != W6X_STATUS_OK)
          {
            goto _err;
          }
          /* Read a part of the Host file */
          read_len = ef_get_env_blob_offset(filename, buf, part_len, NULL, read_offset);
          if ((read_len <= 0) || (read_len != part_len))
          {
            goto _err;
          }

          /* Compare the two buffers */
          if (memcmp(ncp_buf, buf, part_len) != 0)
          {
            /* File content is different: Delete operation requested */
            LogDebug("File content is different: Delete operation requested\n");
            W61_FS_DeleteFile(p_DrvObj, filename);
            break;
          }

          if (size == read_offset + part_len)
          {
            /* File already exists in the NCP and the content is the same */
            LogDebug("File already exists in the NCP and the content is the same\n");
            return W6X_STATUS_OK;
          }

          read_offset += W6X_FS_READ_BLOCK_SIZE;
        }
      }
      else
      {
        /* File already exists in the NCP but the size is different: Delete operation requested */
        LogDebug("File size is different: Delete operation requested\n");
        W61_FS_DeleteFile(p_DrvObj, filename);
      }
      break;
    }
  }

  /* Create the file entry */
  ret = TranslateErrorStatus(W61_FS_CreateFile(p_DrvObj, filename));
  if (ret != W6X_STATUS_OK)
  {
    if (ret == W6X_STATUS_ERROR)
    {
      LogError("Unable to create file in NCP\n");
    }
    goto _err;
  }

  /* Copy the file content */
  do
  {
    /* Read data in Host lfs */
    read_len = ef_get_env_blob_offset(filename, buf, W6X_FS_READ_BLOCK_SIZE, NULL, offset);

    if (read_len <= 0)
    {
      LogError("Unable to read file in Host LFS\n");
      goto _err;
    }

    /* Write data to the file */
    ret = TranslateErrorStatus(W61_FS_WriteFile(p_DrvObj, filename, offset, buf, read_len));
    if (ret != W6X_STATUS_OK)
    {
      if (ret == W6X_STATUS_ERROR)
      {
        LogError("Unable to write file in NCP\n");
      }
      goto _err;
    }
    offset += read_len;
  } while (offset < file_lfs_size);

  LogDebug("File copied to NCP\n");

_err:
  return ret;
#else
  /* Cannot write data to file if no LFS service is available */
  LogError("Host LFS service is not available\n");
  return W6X_STATUS_NOT_SUPPORTED;
#endif /* LFS_ENABLE */
}

W6X_Status_t W6X_FS_ReadFile(char *filename, uint32_t offset, uint8_t *data, uint32_t len)
{
  W6X_Status_t ret = W6X_STATUS_ERROR;
  NULL_ASSERT(p_DrvObj, W6X_Sys_Uninit_str);

  /* Read data from the file */
  return TranslateErrorStatus(W61_FS_ReadFile(p_DrvObj, filename, offset, data, len));
}

W6X_Status_t W6X_FS_DeleteFile(char *filename)
{
  W6X_Status_t ret = W6X_STATUS_ERROR;
  NULL_ASSERT(p_DrvObj, W6X_Sys_Uninit_str);

  /* Delete the file */
  return TranslateErrorStatus(W61_FS_DeleteFile(p_DrvObj, filename));
}

W6X_Status_t W6X_FS_GetSizeFile(char *filename, uint32_t *size)
{
  W6X_Status_t ret = W6X_STATUS_ERROR;
  NULL_ASSERT(p_DrvObj, W6X_Sys_Uninit_str);

  /* Get the size of the file */
  return TranslateErrorStatus(W61_FS_GetSizeFile(p_DrvObj, filename, size));
}

W6X_Status_t W6X_FS_ListFiles(W6X_FS_FilesListFull_t **files_list)
{
  W6X_Status_t ret = W6X_STATUS_ERROR;
  NULL_ASSERT(p_DrvObj, W6X_Sys_Uninit_str);

  if (W6X_FilesList == NULL)
  {
    W6X_FilesList = pvPortMalloc(sizeof(W6X_FS_FilesListFull_t));
    if (W6X_FilesList == NULL)
    {
      LogError("Unable to allocate memory for files list\n");
      goto _err;
    }
  }
  /* Clear the list */
  memset(W6X_FilesList, 0, sizeof(W6X_FS_FilesListFull_t));

#if (LFS_ENABLE == 1)
  /* List the host files */
  ef_print_env(W6X_FilesList->lfs_files_list, &W6X_FilesList->nb_files);
#endif /* LFS_ENABLE */

  /* List the NCP files */
  ret = TranslateErrorStatus(W61_FS_ListFiles(p_DrvObj, (W61_FS_FilesList_t *)&W6X_FilesList->ncp_files_list));
  if (ret != W6X_STATUS_OK)
  {
    goto _err;
  }

  *files_list = W6X_FilesList;

_err:
  return ret;
}

W6X_Status_t W6X_SetPowerMode(uint32_t ps_mode)
{
  W6X_Status_t ret = W6X_STATUS_ERROR;
  NULL_ASSERT(p_DrvObj, W6X_Sys_Uninit_str);

  /* Set the power save mode */
  return TranslateErrorStatus(W61_SetPowerMode(p_DrvObj, (ps_mode == 0) ? 0 : 2, 0));
}

W6X_Status_t W6X_GetPowerMode(uint32_t *ps_mode)
{
  W6X_Status_t ret = W6X_STATUS_ERROR;
  uint32_t mode = 0;
  NULL_ASSERT(p_DrvObj, W6X_Sys_Uninit_str);

  /* Get the power save mode */
  ret = TranslateErrorStatus(W61_GetPowerMode(p_DrvObj, &mode));
  *ps_mode = (mode == 0) ? 0 : 1;

  return ret;
}

W6X_Status_t W6X_RestoreDefaultConfig(void)
{
  W6X_Status_t ret = W6X_STATUS_ERROR;
  W61_Ble_Mode_e ble_mode;
  uint8_t *ble_BuffRecvData = NULL;
  int32_t ble_BuffRecvDataSize = 0;
  uint8_t isWiFiMode;
  uint8_t isNetMode;
  uint8_t isBleMode;
  uint32_t ps_mode = 0;
  NULL_ASSERT(p_DrvObj, W6X_Sys_Uninit_str);

  /* Check if the modules are initialized */
  isWiFiMode = p_DrvObj->WiFi_event_cb != NULL ? 1 : 0;
  isNetMode = p_DrvObj->Net_event_cb != NULL ? 1 : 0;
  isBleMode = p_DrvObj->Ble_event_cb != NULL ? 1 : 0;

  /* Save the current power save mode */
  if (W6X_GetPowerMode(&ps_mode) != W6X_STATUS_OK)
  {
    goto _err;
  }

  /* DeInit the modules */
  if (isWiFiMode)
  {
    W6X_WiFi_DeInit();
  }

  if (isNetMode)
  {
    W6X_Net_DeInit();
  }

  if (isBleMode)
  {
    ble_BuffRecvData = p_DrvObj->BleCtx.AppBuffRecvData;
    ble_BuffRecvDataSize = p_DrvObj->BleCtx.AppBuffRecvDataSize;
    if (W61_Ble_GetInitMode(p_DrvObj, &ble_mode) != W61_STATUS_OK)
    {
      goto _err;
    }

    W6X_Ble_DeInit();
  }

  /* Reset the W61 module to factory default */
  if (W61_ResetToFactoryDefault(p_DrvObj) != W61_STATUS_OK)
  {
    goto _err;
  }

  /* Set the wake-up pin to the ST67W611M */
  if (W61_SetWakeUpPin(p_DrvObj, p_DrvObj->LowPowerCfg.WakeUpPinIn) != W61_STATUS_OK)
  {
    goto _err;
  }

  /* Restore the power save mode */
  if (W6X_SetPowerMode(ps_mode) != W6X_STATUS_OK)
  {
    goto _err;
  }

  /* ReInit the modules used */
  if ((isWiFiMode) && (W6X_WiFi_Init() != W6X_STATUS_OK))
  {
    goto _err;
  }

  if ((isNetMode) && (W6X_Net_Init() != W6X_STATUS_OK))
  {
    goto _err;
  }

  if ((isBleMode) && (W6X_Ble_Init((W6X_Ble_Mode_e)ble_mode, ble_BuffRecvData, ble_BuffRecvDataSize) != W6X_STATUS_OK))
  {
    goto _err;
  }

  return W6X_STATUS_OK;
_err:
  LogError("Restore default config failed.\n");
  return ret;
}

W6X_Status_t W6X_ExeATCommand(char *at_cmd)
{
  return TranslateErrorStatus(W61_ExeATCommand(p_DrvObj, at_cmd));
}

const char *W6X_StatusToStr(W6X_Status_t status)
{
  switch (status)
  {
    case W6X_STATUS_OK:
      return "OK";
    case W6X_STATUS_BUSY:
      return "BUSY";
    case W6X_STATUS_TIMEOUT:
      return "TIMEOUT";
    case W6X_STATUS_UNEXPECTED_RESPONSE:
      return "UNEXPECTED_RESPONSE";
    case W6X_STATUS_ERROR:
      return "ERROR";
    default:
      return "UNKNOWN";
  }
}

/** @} */

/** @addtogroup ST67W6X_Private_Common_Functions
  * @{
  */

W6X_Status_t TranslateErrorStatus_W61_W6X(W61_Status_t ret_w61, char const *func_name)
{
  W6X_Status_t ret_w6x;
  /* Translate the W61 status to the W6X status */
  switch (ret_w61)
  {
    case W61_STATUS_OK:
      ret_w6x = W6X_STATUS_OK;
      break;
    case W61_STATUS_BUSY:
      ret_w6x = W6X_STATUS_BUSY;
      break;
    case W61_STATUS_TIMEOUT:
      ret_w6x = W6X_STATUS_TIMEOUT;
      break;
    case W61_STATUS_UNEXPECTED_RESPONSE:
      ret_w6x = W6X_STATUS_UNEXPECTED_RESPONSE;
      break;
    default:
      ret_w6x = W6X_STATUS_ERROR;
      break;
  }

  if ((ret_w6x != W6X_STATUS_OK) && (W6X_CbHandler.APP_error_cb != NULL))
  {
    /* Call the error callback */
    W6X_CbHandler.APP_error_cb(ret_w6x, func_name);
  }

  return ret_w6x;
}

/** @} */
