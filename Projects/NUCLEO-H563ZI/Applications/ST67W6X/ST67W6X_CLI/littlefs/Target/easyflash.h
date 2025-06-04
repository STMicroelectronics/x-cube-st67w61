/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    easyflash.h
  * @author  GPM Application Team
  * @brief   Header file that adapts LittleFS to EasyFlash4
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef EASYFLASH_H
#define EASYFLASH_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported constants --------------------------------------------------------*/
#define EF_LFS_NAME_MAX 32

/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported types ------------------------------------------------------------*/
/* EasyFlash error code */
typedef enum
{
  EF_NO_ERR,
  EF_ERASE_ERR,
  EF_READ_ERR,
  EF_WRITE_ERR,
  EF_ENV_NAME_ERR,
  EF_ENV_NAME_EXIST,
  EF_ENV_FULL,
  EF_ENV_INIT_FAILED,
  EF_ENV_ARG_ERR,
} EfErrCode;

/* File info structure */
typedef struct
{
  /** Size of the file, only valid for REG files. Limited to 32-bits. */
  uint32_t size;

  /**
    * Name of the file stored as a null-terminated string. Limited to
    * LFS_NAME_MAX+1, which can be changed by redefining LFS_NAME_MAX to
    * reduce RAM. LFS_NAME_MAX is stored in superblock and must be
    * respected by other littlefs drivers.
    */
  char name[EF_LFS_NAME_MAX + 1];
} EfLfsInfo_t;

/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported functions --------------------------------------------------------*/
/**
  * @brief  Initialize the EasyFlash system with littlefs backend.
  * @return EfErrCode Error code indicating success or failure.
  */
EfErrCode easyflash_init(void);

/**
  * @brief  Get the value of an environment variable.
  * @param  key The key of the environment variable.
  * @param  value_buf Buffer to store the value.
  * @param  buf_len Length of the buffer.
  * @return size_t The size of the value.
  */
size_t ef_get_env_blob(const char *key, void *value_buf, size_t buf_len, size_t *saved_value_len);

/**
  * @brief  Get the offset of an environment variable blob.
  * @param  key The key of the environment variable.
  * @param  value_buf Buffer to store the value.
  * @param  buf_len Length of the buffer.
  * @param  saved_value_len Pointer to store the length of the saved value.
  * @param  offset Offset to start reading from.
  * @return size_t The size of the blob.
  */
size_t ef_get_env_blob_offset(const char *key, void *value_buf, size_t buf_len, size_t *saved_value_len,
                              int32_t offset);

/**
  * @brief  Set the value of an environment variable.
  * @param  key The key of the environment variable.
  * @param  value_buf Buffer to store the value.
  * @param  buf_len Length of the buffer.
  * @return EfErrCode Error code indicating success or failure.
  */
EfErrCode ef_set_env_blob(const char *key, const void *value_buf, size_t buf_len);

/**
  * @brief  Unused function.
  * @return EfErrCode Error code indicating success or failure.
  */
EfErrCode ef_save_env(void);

/**
  * @brief  Clear all environment variables.
  * @return EfErrCode Error code indicating success or failure.
  */
EfErrCode ef_env_set_default(void);

/**
  * @brief  Display the file list of the environment variables.
  * @param  file_list List of files.
  * @param  nb_files Number of files.
  * @return EfErrCode Error code indicating success or failure.
  */
EfErrCode ef_print_env(EfLfsInfo_t file_list[20], uint32_t *nb_files);

/* USER CODE BEGIN EF */

/* USER CODE END EF */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* EASYFLASH_H */
