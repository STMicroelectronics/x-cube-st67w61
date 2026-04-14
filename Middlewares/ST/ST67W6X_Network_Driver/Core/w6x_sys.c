/**
  ******************************************************************************
  * @file    w6x_sys.c
  * @author  ST67 Application Team
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
#include <stdbool.h>
#include "w6x_api.h"       /* Prototypes of the functions implemented in this file */
#include "w6x_version.h"
#include "w61_at_api.h"    /* Prototypes of the functions called by this file */
#include "w6x_internal.h"
#include "w61_io.h"        /* Prototypes of the BUS functions to be registered */
#include "common_parser.h" /* Common Parser functions */

#if (!defined(ST67_ARCH) || ((ST67_ARCH != W6X_ARCH_T02) && (ST67_ARCH != W6X_ARCH_T01)))
#error "ST67_ARCH not defined or invalid. Supported values are: W6X_ARCH_T01 or W6X_ARCH_T02"
#error "Please define ST67_ARCH in the compiler preprocessor macros"
#endif /* ST67_ARCH */

/* Private defines -----------------------------------------------------------*/
/** @defgroup ST67W6X_Private_System_Constants ST67W6X System Constants
  * @ingroup  ST67W6X_Private_System
  * @{
  */

#define W6X_FS_READ_BLOCK_SIZE  256U /*!< File system read block size */

#define ANT_DIVERSITY_PIN       0U   /*!< Antenna diversity GPIO pin number */

#ifndef HAL_SYS_RESET
/** HAL System software reset function */
/* coverity[misra_c_2012_rule_8_5_violation : FALSE] */
/* coverity[misra_c_2012_rule_8_6_violation : FALSE] */
extern void HAL_NVIC_SystemReset(void);
/** HAL System software reset macro */
#define HAL_SYS_RESET() do{ HAL_NVIC_SystemReset(); } while(false);
#endif /* HAL_SYS_RESET */

/** @} */

/* Private macros ------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/** @defgroup ST67W6X_Private_System_Variables ST67W6X System Variables
  * @ingroup  ST67W6X_Private_System
  * @{
  */

static W61_Object_t *p_DrvObj = NULL;                 /*!< Global W61 context pointer */

#if (W6X_ASSERT_ENABLE == 1)
/** W6X System init error string */
static const char W6X_Sys_Uninit_str[] = "W6X System module not initialized";
#endif /* W6X_ASSERT_ENABLE */

static W6X_App_Cb_t W6X_CbHandler;                    /*!< W6X Applicative Callbacks handler */

static W6X_FS_FilesListFull_t *W6X_FilesList = NULL;  /*!< List of files */

static W6X_ModuleInfo_t *p_module_info = NULL;        /*!< W61 module info */

/** Status string table for W6X_StatusToStr */
static const char *const W6X_status_strings[] =
{
  "OK",
  "BUSY",
  "ERROR",
  "TIMEOUT",
  "UNEXPECTED RESPONSE",
  "NOT SUPPORTED",
  "UNKNOWN"
};

/** Module ID string table for W6X_ModuleIDToStr */
static const char *const W6X_module_strings[] =
{
  "Undefined",
  "-B",
  "-U",
  "-P"
};

/** @} */

/** @defgroup ST67W6X_Private_FWU_Variables ST67W6X Firmware updates Variables
  * @ingroup  ST67W6X_Private_FWU
  * @{
  */
#if (W6X_ASSERT_ENABLE == 1)
/** W6X FWU init error string */
static const char W6X_FWU_Uninit_str[] = "W6X FWU module not initialized";
#endif /* W6X_ASSERT_ENABLE */

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
  W6X_Status_t ret;
  uint32_t clock_source = 0U;
  int32_t Netmode = -1;

  p_DrvObj = W61_ObjGet();
  NULL_ASSERT(p_DrvObj, W6X_Obj_Null_str);

  /* Configure the default power save mode and WakeUp pin */
  (void)W61_LowPowerConfig(p_DrvObj, 28, 2);

  /* Initialize the W61 module */
  ret = TranslateErrorStatus(W61_Init(p_DrvObj));
  if (ret != W6X_STATUS_OK)
  {
    SYS_LOG_ERROR("W61 Init failed\n");
    goto _err;
  }

  /* Get the current Network mode (0: on host, 1: on NCP) */
  ret = TranslateErrorStatus(W61_GetNetMode(p_DrvObj, &Netmode));
  if (ret != W6X_STATUS_OK)
  {
    SYS_LOG_ERROR("Get Net mode failed\n");
    goto _err;
  }

  if (Netmode == 1)
  {
    p_DrvObj->NetCtx.Supported = 1; /* AT Network module supported */
#if (ST67_ARCH == W6X_ARCH_T02)
    LogError("SDK T02 variant is required for the ST67_ARCH selected\n");
    ret = W6X_STATUS_ERROR;
    goto _err;
#endif /* ST67_ARCH */
  }
  else if (Netmode == 0)
  {
    /* AT Network module not supported. The network interface must be linked to the IP stack on host */
    p_DrvObj->NetCtx.Supported = 0;
#if (ST67_ARCH == W6X_ARCH_T01)
    LogError("SDK T01 variant is required for the ST67_ARCH selected\n");
    ret = W6X_STATUS_ERROR;
    goto _err;
#endif /* ST67_ARCH */
  }
  else
  {
    SYS_LOG_ERROR("Invalid Net mode %d\n", Netmode);
    ret = W6X_STATUS_ERROR;
    goto _err;
  }

  /* Configure the current W61 clock used */
  ret = TranslateErrorStatus(W61_GetClockSource(p_DrvObj, &clock_source));
  if (ret != W6X_STATUS_OK)
  {
    SYS_LOG_ERROR("Get W61 clock source failed\n");
    goto _err;
  }

  if (clock_source != W6X_CLOCK_MODE)
  {
    /* Set the chosen clock source depending of the hardware configuration */
    ret = TranslateErrorStatus(W61_SetClockSource(p_DrvObj, W6X_CLOCK_MODE));
    if (ret != W6X_STATUS_OK)
    {
      SYS_LOG_ERROR("Set W61 clock failed\n");
      goto _err;
    }
    HAL_SYS_RESET(); /* Reboot the host to apply the clock change */
  }

  /* Get the W61 info */
  ret = TranslateErrorStatus(W61_GetModuleInfo(p_DrvObj));
  if (ret != W6X_STATUS_OK)
  {
    SYS_LOG_ERROR("Get W61 Info failed\n");
    goto _err;
  }

  /* Store the W61 information */
  p_module_info = (W6X_ModuleInfo_t *)&p_DrvObj->ModuleInfo;

  (void)W6X_ModuleInfoDisplay(); /* Display the W61 information in banner */

#if (LFS_ENABLE == 1)
  (void)easyflash_init(); /* Initialize the File system with user certificates */
#endif /* LFS_ENABLE */
  if (p_DrvObj->ModuleInfo.ModuleID.ModuleID == W61_MODULE_ID_B)
  {
    /* Restore the antenna diversity pin to floating input */
    (void)W61_RestoreGPIO(p_DrvObj, ANT_DIVERSITY_PIN);
  }

  /* Set the wake-up pin to the ST67W611M */
  (void)W61_SetWakeUpPin(p_DrvObj, p_DrvObj->LowPowerCfg.WakeUpPinIn);

#if (W6X_POWER_SAVE_AUTO == 1)
  (void)W6X_SetPowerMode(1U); /* Enable the power save mode */
#else
  (void)W6X_SetPowerMode(0U); /* Disable the power save mode */
#endif /* W6X_POWER_SAVE_AUTO */

  ret = W6X_STATUS_OK;

_err:
  return ret;
}

void W6X_DeInit(void)
{
  if (p_DrvObj == NULL)
  {
    return; /* Nothing to do */
  }
  /* Set to hibernate */
  (void)W61_SetPowerMode(p_DrvObj, 1U, 0U);
  /* DeInit and power-off the ST67 by resetting the CHIP_EN pin */
  (void)W61_DeInit(p_DrvObj);

  if (W6X_FilesList != NULL)
  {
    /* Free the files list */
    vPortFree(W6X_FilesList);
    W6X_FilesList = NULL;
  }

  p_module_info = NULL; /* Reset the module info */
  p_DrvObj = NULL; /* Reset the global pointer */

}

W6X_Status_t W6X_ModuleInfoDisplay(void)
{
  NULL_ASSERT(p_DrvObj, W6X_Sys_Uninit_str);
  NULL_ASSERT(p_module_info, "Module info not available");

  /* Display the W61 information */
  SYS_LOG_INFO("--------------- ST67W6X info ------------\n");
  SYS_LOG_INFO("ST67W6X MW Version:       " W6X_VERSION_STR "\n");
  SYS_LOG_INFO("AT Version:               %" PRIi16 ".%" PRIi16 ".%" PRIi16 "\n",
               p_module_info->AT_Version.Major, p_module_info->AT_Version.Sub1, p_module_info->AT_Version.Sub2);
  SYS_LOG_INFO("SDK Version:              %" PRIi16 ".%" PRIi16 ".%" PRIi16,
               p_module_info->SDK_Version.Major, p_module_info->SDK_Version.Sub1, p_module_info->SDK_Version.Sub2);
  if (p_module_info->SDK_Version.Patch != 0U)
  {
    SYS_LOG_INFO(".%" PRIi16, p_module_info->SDK_Version.Patch);
  }
  SYS_LOG_INFO("\n");
  SYS_LOG_INFO("Wi-Fi MAC Version:        %" PRIi16 ".%" PRIi16 ".%" PRIi16 "\n",
               p_module_info->WiFi_MAC_Version.Major, p_module_info->WiFi_MAC_Version.Sub1,
               p_module_info->WiFi_MAC_Version.Sub2);
  SYS_LOG_INFO("BT Controller Version:    %" PRIi16 ".%" PRIi16 ".%" PRIi16 "\n",
               p_module_info->BT_Controller_Version.Major, p_module_info->BT_Controller_Version.Sub1,
               p_module_info->BT_Controller_Version.Sub2);
  SYS_LOG_INFO("BT Stack Version:         %" PRIi16 ".%" PRIi16 ".%" PRIi16 "\n",
               p_module_info->BT_Stack_Version.Major, p_module_info->BT_Stack_Version.Sub1,
               p_module_info->BT_Stack_Version.Sub2);
  SYS_LOG_INFO("Build Date:               %s\n", p_module_info->Build_Date);
  SYS_LOG_INFO("Module ID:                ");
  if (p_module_info->ModuleID.ModuleName[0] == '\0')
  {
    SYS_LOG_INFO("Undefined");
  }
  else
  {
    SYS_LOG_INFO("%s (%s)", p_module_info->ModuleID.ModuleName, W6X_ModelToStr(p_module_info->ModuleID.ModuleID));
  }
  SYS_LOG_INFO("\n");
  SYS_LOG_INFO("BOM ID:                   %" PRIu16 "\n", p_module_info->BomID);
  SYS_LOG_INFO("Manufacturing Year:       20%02" PRIu16 "\n", p_module_info->Manufacturing_Year);
  SYS_LOG_INFO("Manufacturing Week:       %02" PRIu16 "\n", p_module_info->Manufacturing_Week);
  SYS_LOG_INFO("Battery Voltage:          %" PRIu32 ".%" PRIu32 " V\n", p_module_info->BatteryVoltage / 1000U,
               p_module_info->BatteryVoltage % 1000U);
  SYS_LOG_INFO("Trim Wi-Fi hp:            %" PRIi16 ",%" PRIi16 ",%" PRIi16 ",%" PRIi16 ",%" PRIi16 ",%" PRIi16 ",%"
               PRIi16
               ",%" PRIi16 ",%" PRIi16 ",%" PRIi16 ",%" PRIi16 ",%" PRIi16 ",%" PRIi16 ",%" PRIi16 "\n",
               p_module_info->trim_wifi_hp[0], p_module_info->trim_wifi_hp[1],
               p_module_info->trim_wifi_hp[2], p_module_info->trim_wifi_hp[3],
               p_module_info->trim_wifi_hp[4], p_module_info->trim_wifi_hp[5],
               p_module_info->trim_wifi_hp[6], p_module_info->trim_wifi_hp[7],
               p_module_info->trim_wifi_hp[8], p_module_info->trim_wifi_hp[9],
               p_module_info->trim_wifi_hp[10], p_module_info->trim_wifi_hp[11],
               p_module_info->trim_wifi_hp[12], p_module_info->trim_wifi_hp[13]);
  SYS_LOG_INFO("Trim Wi-Fi lp:            %" PRIi16 ",%" PRIi16 ",%" PRIi16 ",%" PRIi16 ",%" PRIi16 ",%" PRIi16 ",%"
               PRIi16
               ",%" PRIi16 ",%" PRIi16 ",%" PRIi16 ",%" PRIi16 ",%" PRIi16 ",%" PRIi16 ",%" PRIi16 "\n",
               p_module_info->trim_wifi_lp[0], p_module_info->trim_wifi_lp[1],
               p_module_info->trim_wifi_lp[2], p_module_info->trim_wifi_lp[3],
               p_module_info->trim_wifi_lp[4], p_module_info->trim_wifi_lp[5],
               p_module_info->trim_wifi_lp[6], p_module_info->trim_wifi_lp[7],
               p_module_info->trim_wifi_lp[8], p_module_info->trim_wifi_lp[9],
               p_module_info->trim_wifi_lp[10], p_module_info->trim_wifi_lp[11],
               p_module_info->trim_wifi_lp[12], p_module_info->trim_wifi_lp[13]);
  SYS_LOG_INFO("Trim BLE:                 %" PRIi16 ",%" PRIi16 ",%" PRIi16 ",%" PRIi16 ",%" PRIi16 "\n",
               p_module_info->trim_ble[0], p_module_info->trim_ble[1], p_module_info->trim_ble[2],
               p_module_info->trim_ble[3], p_module_info->trim_ble[4]);
  SYS_LOG_INFO("Trim XTAL:                %" PRIi16 "\n", p_module_info->trim_xtal);
  SYS_LOG_INFO("MAC Address:              " MACSTR "\n", MAC2STR(p_module_info->Mac_Address));
  SYS_LOG_INFO("Anti-rollback Bootloader: %" PRIu16 "\n", p_module_info->AntiRollbackBootloader);
  SYS_LOG_INFO("Anti-rollback App:        %" PRIu16 "\n", p_module_info->AntiRollbackApp);
  SYS_LOG_INFO("-----------------------------------------\n");

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

W6X_Status_t W6X_RegisterAppCb(W6X_App_Cb_t *app_cb)
{
  NULL_ASSERT(app_cb, W6X_Sys_Uninit_str);

  /* Register the application callback */
  W6X_CbHandler = *app_cb;

  return W6X_STATUS_OK;
}

W6X_Status_t W6X_FS_WriteFileByName(char filename[W6X_SYS_FS_FILENAME_SIZE])
{
  /* Write the file to the NCP. Requires the file to be present in the host filesystem */
  return W6X_FS_WriteFileByContent(filename, NULL, 0);
}

W6X_Status_t W6X_FS_WriteFileByContent(char filename[W6X_SYS_FS_FILENAME_SIZE], const char *file, uint32_t len)
{
  W6X_FS_FilesListFull_t *files_list = NULL;
  uint32_t file_ncp_index = 0;
  uint32_t read_offset = 0;
  uint32_t part_len;
  uint32_t file_lfs_size = 0;
#if (LFS_ENABLE == 1)
  uint8_t *buf = NULL;
  uint32_t file_lfs_index = 0;
  int32_t read_len = 0;
#endif /* LFS_ENABLE */
  W6X_Status_t ret = W6X_STATUS_ERROR;
  NULL_ASSERT(p_DrvObj, W6X_Sys_Uninit_str);
  /* Allocate a buffer to read a block of the NCP file */
  uint8_t *ncp_buf = pvPortMalloc(W6X_FS_READ_BLOCK_SIZE + 1U);
  if (ncp_buf == NULL)
  {
    goto _err;
  }
  (void)memset(ncp_buf, 0, W6X_FS_READ_BLOCK_SIZE + 1U);

#if (LFS_ENABLE == 1)
  /* Allocate a buffer to read a block of the Host lfs file */
  buf = pvPortMalloc(W6X_FS_READ_BLOCK_SIZE + 1U);
  if (buf == NULL)
  {
    goto _err;
  }
  (void)memset(buf, 0, W6X_FS_READ_BLOCK_SIZE + 1U);
#else
  if (file == NULL)
  {
    SYS_LOG_ERROR("File content not available\n");
    ret = W6X_STATUS_ERROR;
    goto _err;
  }
#endif /* LFS_ENABLE */

  ret = W6X_FS_ListFiles(&files_list); /* Get the files list available in the NCP */
  if (ret != W6X_STATUS_OK)
  {
    if (ret == W6X_STATUS_ERROR)
    {
      SYS_LOG_ERROR("Unable to list files\n");
    }
    goto _err;
  }

#if (LFS_ENABLE == 1)
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

  if ((file == NULL) && (file_lfs_index == files_list->nb_files))
  {
    /* File not found in the lfs_files_list */
    SYS_LOG_ERROR("File not found in the Host LFS. Verify the filename and littlefs.bin generation");
    ret = W6X_STATUS_ERROR;
    goto _err;
  }
#endif /* LFS_ENABLE */

  /* Check if the file exists in the NCP */
  for (; file_ncp_index < files_list->ncp_files_list.nb_files; file_ncp_index++)
  {
    if (strncmp(files_list->ncp_files_list.filename[file_ncp_index], filename, strlen(filename)) == 0)
    {
      /* File found in the NCP. Must to check if the content is the same */
      uint32_t size = 0U;
      /* Get the NCP file size */
      ret = TranslateErrorStatus(W61_FS_GetSizeFile(p_DrvObj, filename, &size));
      if (ret != W6X_STATUS_OK)
      {
        goto _err;
      }

      /* File found. Must to check if the size of Host lfs file and NCP file are equal */
#if (LFS_ENABLE == 1)
      if (((file != NULL) && (len == size)) || (files_list->lfs_files_list[file_lfs_index].size == size))
#else
      if (len == size)
#endif /* LFS_ENABLE */
      {
        read_offset = 0;

        while (read_offset < size) /* Size of files are equal. Read and compare the files by block */
        {
          part_len = ((size - read_offset) < W6X_FS_READ_BLOCK_SIZE) ? (size - read_offset) : W6X_FS_READ_BLOCK_SIZE;
          /* Read a part of the NCP file */
          ret = W6X_FS_ReadFile(filename, read_offset, ncp_buf, part_len);
          if (ret != W6X_STATUS_OK)
          {
            goto _err;
          }

#if (LFS_ENABLE == 1)
          /* Read a part of the Host file */
          read_len = ef_get_env_blob_offset(filename, buf, part_len, NULL, read_offset);
          if ((read_len <= 0) || (read_len != part_len))
          {
            ret = W6X_STATUS_ERROR;
            goto _err;
          }

          /* Compare the two buffers */
          if (memcmp(ncp_buf, buf, part_len) != 0)
          {
            /* File content is different: Delete operation requested */
            SYS_LOG_DEBUG("File content is different: Delete operation requested\n");
            (void)W61_FS_DeleteFile(p_DrvObj, filename);
            break;
          }
#else
          /* Compare the two buffers */
          if (memcmp(ncp_buf, &file[read_offset], part_len) != 0)
          {
            /* File content is different: Delete operation requested */
            SYS_LOG_DEBUG("File content is different: Delete operation requested\n");
            (void)W61_FS_DeleteFile(p_DrvObj, filename);
            break;
          }
#endif /* LFS_ENABLE */
          if (size == (read_offset + part_len))
          {
            /* File already exists in the NCP and the content is the same */
            SYS_LOG_DEBUG("File already exists in the NCP and the content is the same\n");
            ret = W6X_STATUS_OK;
            goto _err;
          }

          read_offset += part_len;
        }
      }
      else
      {
        /* File already exists in the NCP but the size is different: Delete operation requested */
        SYS_LOG_DEBUG("File size is different: Delete operation requested\n");
        (void)W61_FS_DeleteFile(p_DrvObj, filename);
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
      SYS_LOG_ERROR("Unable to create file in NCP\n");
    }
    goto _err;
  }

  /* Copy the file content */
  read_offset = 0;
#if (LFS_ENABLE == 0)
  file_lfs_size = len;
#endif /* LFS_ENABLE */
  do
  {
#if (LFS_ENABLE == 1)
    /* Read data in Host lfs */
    read_len = ef_get_env_blob_offset(filename, buf, W6X_FS_READ_BLOCK_SIZE, NULL, read_offset);

    if (read_len <= 0)
    {
      SYS_LOG_ERROR("Unable to read file in Host LFS\n");
      goto _err;
    }

    /* Write data to the file */
    ret = TranslateErrorStatus(W61_FS_WriteFile(p_DrvObj, filename, read_offset, buf, read_len));
    read_offset += read_len;
#else
    part_len = ((len - read_offset) < W6X_FS_READ_BLOCK_SIZE) ? (len - read_offset) : W6X_FS_READ_BLOCK_SIZE;
    ret = TranslateErrorStatus(W61_FS_WriteFile(p_DrvObj, filename, read_offset,
                                                (uint8_t *)&file[read_offset], part_len));
    read_offset += part_len;
#endif /* LFS_ENABLE */

    if (ret != W6X_STATUS_OK)
    {
      if (ret == W6X_STATUS_ERROR)
      {
        SYS_LOG_ERROR("Unable to write file in NCP\n");
      }
      goto _err;
    }
  } while (read_offset < file_lfs_size);

  SYS_LOG_DEBUG("File copied to NCP\n");

_err:
  if (ncp_buf != NULL)
  {
    vPortFree(ncp_buf);
  }
#if (LFS_ENABLE == 1)
  if (buf != NULL)
  {
    vPortFree(buf);
  }
#endif /* LFS_ENABLE */
  return ret;
}

W6X_Status_t W6X_FS_ReadFile(char filename[W6X_SYS_FS_FILENAME_SIZE], uint32_t offset, uint8_t *data, uint32_t len)
{
  NULL_ASSERT(p_DrvObj, W6X_Sys_Uninit_str);

  /* Read data from the file */
  return TranslateErrorStatus(W61_FS_ReadFile(p_DrvObj, filename, offset, data, len));
}

W6X_Status_t W6X_FS_DeleteFile(char filename[W6X_SYS_FS_FILENAME_SIZE])
{
  NULL_ASSERT(p_DrvObj, W6X_Sys_Uninit_str);

  /* Delete the file */
  return TranslateErrorStatus(W61_FS_DeleteFile(p_DrvObj, filename));
}

W6X_Status_t W6X_FS_GetSizeFile(char filename[W6X_SYS_FS_FILENAME_SIZE], uint32_t *size)
{
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
    /* Allocate the files list to store all filenames */
    W6X_FilesList = pvPortMalloc(sizeof(W6X_FS_FilesListFull_t));
    if (W6X_FilesList == NULL)
    {
      SYS_LOG_ERROR("Unable to allocate memory for files list\n");
      goto _err;
    }
  }
  /* Clear the list */
  (void)memset(W6X_FilesList, 0, sizeof(W6X_FS_FilesListFull_t));

#if (LFS_ENABLE == 1)
  /* List the host files */
  (void)ef_print_env(W6X_FilesList->lfs_files_list, &W6X_FilesList->nb_files);
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
  NULL_ASSERT(p_DrvObj, W6X_Sys_Uninit_str);

  /* Set the power save mode */
  return TranslateErrorStatus(W61_SetPowerMode(p_DrvObj, (ps_mode == 0U) ? 0U : 2U, 0U));
}

W6X_Status_t W6X_GetPowerMode(uint32_t *ps_mode)
{
  W6X_Status_t ret;
  uint32_t mode = 0;
  NULL_ASSERT(p_DrvObj, W6X_Sys_Uninit_str);

  /* Get the power save mode */
  ret = TranslateErrorStatus(W61_GetPowerMode(p_DrvObj, &mode));
  *ps_mode = (mode == 0U) ? 0U : 1U;

  return ret;
}

W6X_Status_t W6X_Reset(uint8_t restore)
{
  NULL_ASSERT(p_DrvObj, W6X_Sys_Uninit_str);

  if (p_DrvObj->ResetCfg.PS_mode < 0) /* Power save mode not saved yet */
  {
    /* Save the current power save mode */
    if (W6X_GetPowerMode((uint32_t *)&p_DrvObj->ResetCfg.PS_mode) != W6X_STATUS_OK)
    {
      goto _err;
    }
  }

  /* Disable the power save mode */
  if (W6X_SetPowerMode(0) != W6X_STATUS_OK)
  {
    goto _err;
  }

  /* DeInit the modules */
  if (p_DrvObj->ResetCfg.WiFi_status == W61_MODULE_STATE_INIT)
  {
    W6X_WiFi_DeInit();
    /* Backup the Wi-Fi state to restore it after reset */
    p_DrvObj->ResetCfg.WiFi_status = W61_MODULE_STATE_INIT_PENDING;
  }

#if (ST67_ARCH == W6X_ARCH_T01)
  if (p_DrvObj->ResetCfg.Net_status == W61_MODULE_STATE_INIT)
  {
    W6X_Net_DeInit();
    /* Backup the Network state to restore it after reset */
    p_DrvObj->ResetCfg.Net_status = W61_MODULE_STATE_INIT_PENDING;
  }
#endif /* ST67_ARCH */

  if (p_DrvObj->ResetCfg.Ble_status == W61_MODULE_STATE_INIT)
  {
    /* Save BLE buffer and mode information */
    p_DrvObj->ResetCfg.Ble_mode = p_DrvObj->BleCtx.NetSettings.Mode;
    p_DrvObj->ResetCfg.Ble_BuffRecvData = p_DrvObj->BleCtx.AppBuffRecvData;
    p_DrvObj->ResetCfg.Ble_BuffRecvDataSize = p_DrvObj->BleCtx.AppBuffRecvDataSize;

    W6X_Ble_DeInit();
    /* Backup the BLE state to restore it after reset */
    p_DrvObj->ResetCfg.Ble_status = W61_MODULE_STATE_INIT_PENDING;
  }

  if (p_DrvObj->ResetCfg.FWU_requested == 1U)
  {
    /* Finish the firmware update and reboot the ST67W611M on the new firmware version if integrity check passes */
    if (W61_Reset(p_DrvObj, W61_RESET_MODE_FWU) != W61_STATUS_OK)
    {
      goto _err;
    }
  }
  else if (restore == 1U)
  {
    /* Reset the W61 module to factory default. This will erase all user data and reboot the ST67W611M */
    if (W61_Reset(p_DrvObj, W61_RESET_MODE_RESTORE) != W61_STATUS_OK)
    {
      goto _err;
    }
  }
  else
  {
    /* Reset the W61 module */
    if (W61_Reset(p_DrvObj, W61_RESET_MODE_SOFT) != W61_STATUS_OK)
    {
      goto _err;
    }
  }

  /* Set the wake-up pin to the ST67W611M */
  if (W61_SetWakeUpPin(p_DrvObj, p_DrvObj->LowPowerCfg.WakeUpPinIn) != W61_STATUS_OK)
  {
    goto _err;
  }

  /* Restore the power save mode */
  if (W6X_SetPowerMode((uint32_t)p_DrvObj->ResetCfg.PS_mode) != W6X_STATUS_OK)
  {
    goto _err;
  }
  p_DrvObj->ResetCfg.PS_mode = -1; /* Reset the backup variable */

  /* If Wi-Fi was started, initialize the Wi-Fi module */
  if (p_DrvObj->ResetCfg.WiFi_status == W61_MODULE_STATE_INIT_PENDING)
  {
    if (W6X_WiFi_Init() != W6X_STATUS_OK)
    {
      goto _err;
    }
  }

#if (ST67_ARCH == W6X_ARCH_T01)
  /* If Network was started, initialize the Network module */
  if (p_DrvObj->ResetCfg.Net_status == W61_MODULE_STATE_INIT_PENDING)
  {
    if (W6X_Net_Init() != W6X_STATUS_OK)
    {
      goto _err;
    }
  }
#endif /* ST67_ARCH */

  /* If BLE was started, initialize the BLE module with previous configuration */
  if (p_DrvObj->ResetCfg.Ble_status == W61_MODULE_STATE_INIT_PENDING)
  {
    if (W6X_Ble_Init((W6X_Ble_Mode_e)p_DrvObj->ResetCfg.Ble_mode,
                     p_DrvObj->ResetCfg.Ble_BuffRecvData,
                     p_DrvObj->ResetCfg.Ble_BuffRecvDataSize) != W6X_STATUS_OK)
    {
      goto _err;
    }
  }

  return W6X_STATUS_OK;
_err:
  SYS_LOG_ERROR("Restore default config failed.\n");
  return W6X_STATUS_ERROR;
}

W6X_Status_t W6X_ExeATCommand(char *at_cmd)
{
  return TranslateErrorStatus(W61_ExeATCommand(p_DrvObj, at_cmd));
}

const char *W6X_StatusToStr(W6X_Status_t status)
{
  if (status > W6X_STATUS_UNKNOWN)
  {
    status = W6X_STATUS_UNKNOWN;
  }
  return W6X_status_strings[(uint8_t)status];
}

const char *W6X_ModelToStr(W6X_ModuleID_e module_id)
{
  if (module_id > W6X_MODULE_ID_P)
  {
    module_id = W6X_MODULE_ID_UNDEF;
  }
  return W6X_module_strings[(uint8_t)module_id];
}

W6X_Status_t W6X_SdkMinVersion(uint8_t major, uint8_t sub1, uint8_t sub2)
{
  NULL_ASSERT(p_DrvObj, W6X_Obj_Null_str);

  if (W61_SdkMinVersion(p_DrvObj, major, sub1, sub2) != W61_STATUS_OK)
  {
    return W6X_STATUS_ERROR;
  }
  return W6X_STATUS_OK;
}

/** @} */

/** @addtogroup ST67W6X_API_FWU_Public_Functions
  * @{
  */

W6X_Status_t W6X_FWU_Starts(uint32_t enable)
{
  NULL_ASSERT(p_DrvObj, W6X_Obj_Null_str);

  /* Start of firmware update on W61 currently supports only value 1 or 0 as parameters,
     all other value will raise an error */
  if ((enable != 0U) && (enable != 1U))
  {
    return W6X_STATUS_ERROR;
  }

  return TranslateErrorStatus(W61_FWU_starts(p_DrvObj, enable));
}

W6X_Status_t W6X_FWU_Finish(void)
{
  W6X_Status_t ret;
  NULL_ASSERT(p_DrvObj, W6X_FWU_Uninit_str);

  /* Set the firmware update requested flag to avoid any operation
   * on the module until the end of the firmware update process */
  p_DrvObj->ResetCfg.FWU_requested = 1U;

  /* Finish the firmware update and reboot the ST67W611M on the new firmware version if integrity check passes
   * Perform a module reset to reinitialize all contexts to their initial state */
  ret = W6X_Reset(0U);

  /* Reset the firmware update requested flag to allow normal operation of the module */
  p_DrvObj->ResetCfg.FWU_requested = 0U;

  return ret;
}

W6X_Status_t W6X_FWU_Send(uint8_t *buff, uint32_t len)
{
  NULL_ASSERT(p_DrvObj, W6X_FWU_Uninit_str);
  NULL_ASSERT(buff, "buff not defined");

  if (len == 0U)
  {
    return W6X_STATUS_OK;
  }

  return TranslateErrorStatus(W61_FWU_Send(p_DrvObj, buff, len));
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
