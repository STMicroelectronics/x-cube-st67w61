/**
  ******************************************************************************
  * @file    w6x_sys_shell.c
  * @author  GPM Application Team
  * @brief   This file provides code for W6x System Shell Commands
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

#include "w6x_api.h"
#include "shell.h"
#include "logging.h"
#include "common_parser.h" /* Common Parser functions */
#include "FreeRTOS.h"

/* Global variables ----------------------------------------------------------*/
/* Private typedef -----------------------------------------------------------*/
/* Private defines -----------------------------------------------------------*/
/** @addtogroup ST67W6X_Private_System_Constants
  * @{
  */

#ifndef HAL_SYS_RESET
/** HAL System software reset function */
extern void HAL_NVIC_SystemReset(void);
/** HAL System software reset macro */
#define HAL_SYS_RESET() do{ HAL_NVIC_SystemReset(); } while(0);
#endif /* HAL_SYS_RESET */

/** @} */

/* Private macros ------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/** @addtogroup ST67W6X_Private_System_Functions
  * @{
  */

/**
  * @brief  Get info shell function
  * @param  argc: number of arguments
  * @param  argv: pointer to the arguments
  * @retval ::SHELL_STATUS_OK on success
  * @retval ::SHELL_STATUS_UNKNOWN_ARGS if wrong arguments
  * @retval ::SHELL_STATUS_ERROR otherwise
  */
int32_t W6X_Shell_GetInfo(int32_t argc, char **argv);

/**
  * @brief  Reset shell function
  * @param  argc: number of arguments
  * @param  argv: pointer to the arguments
  * @retval ::SHELL_STATUS_OK on success
  * @retval ::SHELL_STATUS_UNKNOWN_ARGS if wrong arguments
  * @retval ::SHELL_STATUS_ERROR otherwise
  */
int32_t W6X_Shell_Reset(int32_t argc, char **argv);

/**
  * @brief  Write file shell function
  * @param  argc: number of arguments
  * @param  argv: pointer to the arguments
  * @retval ::SHELL_STATUS_OK on success
  * @retval ::SHELL_STATUS_UNKNOWN_ARGS if wrong arguments
  * @retval ::SHELL_STATUS_ERROR otherwise
  */
int32_t W6X_Shell_FS_WriteFile(int32_t argc, char **argv);

/**
  * @brief  Read file shell function
  * @param  argc: number of arguments
  * @param  argv: pointer to the arguments
  * @retval ::SHELL_STATUS_OK on success
  * @retval ::SHELL_STATUS_UNKNOWN_ARGS if wrong arguments
  * @retval ::SHELL_STATUS_ERROR otherwise
  */
int32_t W6X_Shell_FS_ReadFile(int32_t argc, char **argv);

/**
  * @brief  Delete file shell function
  * @param  argc: number of arguments
  * @param  argv: pointer to the arguments
  * @retval ::SHELL_STATUS_OK on success
  * @retval ::SHELL_STATUS_UNKNOWN_ARGS if wrong arguments
  * @retval ::SHELL_STATUS_ERROR otherwise
  */
int32_t W6X_Shell_FS_DeleteFile(int32_t argc, char **argv);

/**
  * @brief  Get files list shell function
  * @param  argc: number of arguments
  * @param  argv: pointer to the arguments
  * @retval ::SHELL_STATUS_OK on success
  * @retval ::SHELL_STATUS_UNKNOWN_ARGS if wrong arguments
  * @retval ::SHELL_STATUS_ERROR otherwise
  */
int32_t W6X_Shell_FS_ListFiles(int32_t argc, char **argv);

/**
  * @brief  Low power shell function
  * @param  argc: number of arguments
  * @param  argv: pointer to the arguments
  * @retval ::SHELL_STATUS_OK on success
  * @retval ::SHELL_STATUS_UNKNOWN_ARGS if wrong arguments
  */
int32_t W6X_Shell_LowPower(int32_t argc, char **argv);

/**
  * @brief  AT command shell function
  * @param  argc: number of arguments
  * @param  argv: pointer to the arguments
  * @retval ::SHELL_STATUS_OK on success
  * @retval ::SHELL_STATUS_UNKNOWN_ARGS if wrong arguments
  */
int32_t W6X_Shell_ATCommand(int32_t argc, char **argv);

/* Private Functions Definition ----------------------------------------------*/
int32_t W6X_Shell_GetInfo(int32_t argc, char **argv)
{
  if (argc != 1)
  {
    return SHELL_STATUS_UNKNOWN_ARGS;
  }

  if (W6X_ModuleInfoDisplay() != W6X_STATUS_OK)
  {
    SHELL_E("Module info not available\n");
    return SHELL_STATUS_ERROR;
  }

  return SHELL_STATUS_OK;
}

SHELL_CMD_EXPORT_ALIAS(W6X_Shell_GetInfo, info, info. Display ST67W6X module info);

int32_t W6X_Shell_Reset(int32_t argc, char **argv)
{
  /* optional argument 0:HAL_Reset, 1:NCP_Restore */
  if (argc == 2)
  {
    if (strcmp(argv[1], "0") == 0)
    {
      HAL_SYS_RESET();
    }
    else if (strcmp(argv[1], "1") == 0)
    {
      if (W6X_RestoreDefaultConfig() != W6X_STATUS_OK)
      {
        SHELL_E("Unable to restore default configuration\n");
        return SHELL_STATUS_ERROR;
      }
    }
    else
    {
      return SHELL_STATUS_UNKNOWN_ARGS;
    }
  }
  else
  {
    HAL_SYS_RESET();
  }

  return SHELL_STATUS_OK;
}

SHELL_CMD_EXPORT_ALIAS(W6X_Shell_Reset, reset, reset < 0: HAL_Reset; 1: NCP_Restore >);

int32_t W6X_Shell_FS_WriteFile(int32_t argc, char **argv)
{
  if (argc != 2)
  {
    return SHELL_STATUS_UNKNOWN_ARGS;
  }

  if (W6X_FS_WriteFile(argv[1]) != W6X_STATUS_OK)
  {
    SHELL_E("Unable to write file\n");
    return SHELL_STATUS_ERROR;
  }
  return SHELL_STATUS_OK;
}

SHELL_CMD_EXPORT_ALIAS(W6X_Shell_FS_WriteFile, fs_write,
                       fs_write < filename >. Write file content from the Host to the NCP);

int32_t W6X_Shell_FS_ReadFile(int32_t argc, char **argv)
{
  uint8_t buf[257] = {0};
  uint32_t size = 0;
  uint32_t read_offset = 0;

  if (argc != 2)
  {
    return SHELL_STATUS_UNKNOWN_ARGS;
  }

  if (W6X_FS_GetSizeFile(argv[1], &size) != W6X_STATUS_OK)
  {
    SHELL_E("File not found\n");
    return SHELL_STATUS_ERROR;
  }

  if (size == 0)
  {
    SHELL_E("File is empty\n");
    return SHELL_STATUS_ERROR;
  }
  SHELL_PRINTF("Data:\n");
  while (read_offset < size)
  {
    if ((size - read_offset) < 256)
    {
      W6X_FS_ReadFile(argv[1], read_offset, buf, size - read_offset);
      buf[size - read_offset] = '\0';
      SHELL_PRINTF("%s", buf);
      break;
    }
    W6X_FS_ReadFile(argv[1], read_offset, buf, 256);
    buf[256] = '\0';
    SHELL_PRINTF("%s", buf);
    read_offset += 256;
  }

  return SHELL_STATUS_OK;
}

SHELL_CMD_EXPORT_ALIAS(W6X_Shell_FS_ReadFile, fs_read, fs_read < filename >. Read file content);

int32_t W6X_Shell_FS_DeleteFile(int32_t argc, char **argv)
{
  if (argc != 2)
  {
    return SHELL_STATUS_UNKNOWN_ARGS;
  }

  if (W6X_FS_DeleteFile(argv[1]) != W6X_STATUS_OK)
  {
    SHELL_E("Unable to delete file\n");
    return SHELL_STATUS_ERROR;
  }
  return SHELL_STATUS_OK;
}

SHELL_CMD_EXPORT_ALIAS(W6X_Shell_FS_DeleteFile, fs_delete,
                       fs_delete < filename >. Delete file from the NCP file system);

int32_t W6X_Shell_FS_ListFiles(int32_t argc, char **argv)
{
  W6X_FS_FilesListFull_t *files_list = NULL;

  if (argc != 1)
  {
    return SHELL_STATUS_UNKNOWN_ARGS;
  }

  if (W6X_FS_ListFiles(&files_list) != W6X_STATUS_OK)
  {
    SHELL_E("Unable to list files\n");
    return SHELL_STATUS_ERROR;
  }

#if (LFS_ENABLE == 1)
  SHELL_PRINTF("Host LFS Files:\n");
  for (uint32_t i = 0; i < files_list->nb_files; i++)
  {
    SHELL_PRINTF("%s (size: %" PRIu32 ")\n", files_list->lfs_files_list[i].name, files_list->lfs_files_list[i].size);
  }
#endif /* LFS_ENABLE */

  SHELL_PRINTF("\nNCP Files:\n");
  for (uint32_t i = 0; i < files_list->ncp_files_list.nb_files; i++)
  {
    SHELL_PRINTF("%s\n", files_list->ncp_files_list.filename[i]);
  }

  return SHELL_STATUS_OK;
}

SHELL_CMD_EXPORT_ALIAS(W6X_Shell_FS_ListFiles, fs_list, fs_list. List all files in the file system);

int32_t W6X_Shell_LowPower(int32_t argc, char **argv)
{
  uint32_t enable = 0;
  int32_t tmp = 0;

  if (argc == 1)
  {
    if (W6X_GetPowerMode(&enable) != W6X_STATUS_OK)
    {
      SHELL_E("Unable to get power mode\n");
      return SHELL_STATUS_ERROR;
    }
    SHELL_PRINTF("powersave mode is %s\n", enable ? "enabled" : "disabled");
  }
  else if (argc == 2)
  {
    if (Parser_StrToInt(argv[1], NULL, &tmp) == 0)
    {
      SHELL_E("Invalid argument\n");
      return SHELL_STATUS_UNKNOWN_ARGS;
    }
    if (W6X_SetPowerMode((uint32_t)tmp) != W6X_STATUS_OK)
    {
      SHELL_E("Unable to set power mode\n");
      return SHELL_STATUS_ERROR;
    }
  }
  else
  {
    return SHELL_STATUS_UNKNOWN_ARGS;
  }

  return SHELL_STATUS_OK;
}

SHELL_CMD_EXPORT_ALIAS(W6X_Shell_LowPower, powersave, powersave [ 0: disable; 1: enable ]);

int32_t W6X_Shell_ATCommand(int32_t argc, char **argv)
{
  if (argc != 2)
  {
    return SHELL_STATUS_UNKNOWN_ARGS;
  }

  if (W6X_ExeATCommand(argv[1]) != W6X_STATUS_OK)
  {
    SHELL_E("Unable to execute AT command\n");
    return SHELL_STATUS_ERROR;
  }

  return SHELL_STATUS_OK;
}

SHELL_CMD_EXPORT_ALIAS(W6X_Shell_ATCommand, atcmd,
                       atcmd < "AT+CMD?" >. Execute AT command);

/** @} */
