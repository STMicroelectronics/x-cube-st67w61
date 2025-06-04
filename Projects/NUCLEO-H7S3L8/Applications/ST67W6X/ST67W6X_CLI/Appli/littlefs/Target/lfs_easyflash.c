/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    lfs_easyflash.c
  * @author  GPM Application Team
  * @brief   Adapts LittleFS to EasyFlash4
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
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>

#include <FreeRTOS.h>
#include <semphr.h>

#define DBG_TAG "LFS"

#include "lfs_port.h"
#include "easyflash.h"

#include "logging.h"

/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Global variables ----------------------------------------------------------*/
/* USER CODE BEGIN GV */

/* USER CODE END GV */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private defines -----------------------------------------------------------*/
/**
  * the following defines are used to configure the littlefs binary
  * the size of the read buffer, program buffer, lookahead buffer,
  * cache buffer, block size and block cycles are defined here.
  * the values must be aligned to the command line arguments:
  *
  * mklfs.exe -c lfs -b <BLOCK_SIZE> -p <PROG_BUF_SIZE> -r <READ_BUF_SIZE> -s <FLASH_SECTOR_SIZE> -i littlefs.bin
  *
  * Note: the FLASH_SECTOR_SIZE option must be adjusted depending on the number of certificates files.
  */
#define READ_BUF_SIZE       256   /*!< Size of the read buffer */
#define PROG_BUF_SIZE       256   /*!< Size of the program buffer */
#define LOOKAHEAD_BUF_SIZE  256   /*!< Size of the lookahead buffer */
#define CACHE_BUF_SIZE      512   /*!< Size of the cache buffer */
#define BLOCK_SIZE          2048  /*!< Size of the block */
#define BLOCK_CYCLES        500   /*!< Number of block cycles */

/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macros ------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* Pointer to the littlefs instance */
static lfs_t *lfs = NULL;

/* Buffer for storing file paths */
static char path_buffer[64];

#ifndef LFS_EF_NAMESPACE
/* Default namespace for littlefs */
#define LFS_EF_NAMESPACE "/"
#endif /* LFS_EF_NAMESPACE */

/* Semaphore for thread-safe access */
static SemaphoreHandle_t env_giant_lock = NULL;

/* Context for littlefs */
static struct lfs_context lfs_ctx = { .partition_name = "PSM" };

/* Configuration for littlefs */
static struct lfs_config lfs_cfg =
{
  .read_size = READ_BUF_SIZE,
  .prog_size = PROG_BUF_SIZE,
  .lookahead_size = LOOKAHEAD_BUF_SIZE,
  .cache_size = CACHE_BUF_SIZE,
  .block_size = BLOCK_SIZE,
  .block_cycles = BLOCK_CYCLES
};

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/

/* Lock the semaphore for thread-safe access */
static void ef_giant_lock(void);

/* Unlock the semaphore for thread-safe access */
static void ef_giant_unlock(void);

/* Generate a key path for key-value storage */
static int32_t gen_kv_key_path(char *buf, size_t buf_len, const char *prefix, const char *path);

/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Functions Definition ------------------------------------------------------*/
EfErrCode easyflash_init(void)
{
  struct lfs_info stat_t = {0};

  /* Initialize the littlefs instance */
  lfs = lfs_flash_init(&lfs_ctx, &lfs_cfg);
  if (lfs == NULL)
  {
    LogError("littlefs backend init failed.\n");
    return EF_ENV_INIT_FAILED;
  }

  /* Create a mutex for thread-safe access */
#if configUSE_RECURSIVE_MUTEXES
  env_giant_lock = xSemaphoreCreateRecursiveMutex();
#else
  env_giant_lock = xSemaphoreCreateMutex();
#endif /* configUSE_RECURSIVE_MUTEXES */

  /* Initialize the namespace */
  if (lfs_stat(lfs, LFS_EF_NAMESPACE, &stat_t) == LFS_ERR_OK)
  {
    if (stat_t.type == LFS_TYPE_DIR)
    {
      return EF_NO_ERR;
    }
    else if (stat_t.type == LFS_TYPE_REG)
    {
      LogError("namespace directory conflicts with standard file.\n");
      return EF_ENV_INIT_FAILED;
    }
  }

  return EF_NO_ERR;
}

size_t ef_get_env_blob_offset(const char *key, void *value_buf, size_t buf_len, size_t *saved_value_len, int32_t offset)
{
  lfs_file_t file;
  int32_t ret;
  int32_t read_len;

  if (lfs == NULL)
  {
    return 0;
  }

  if (key == NULL || value_buf == NULL || buf_len == 0)
  {
    return 0;
  }

  /* Lock the semaphore */
  ef_giant_lock();

  /* Generate the key path */
  ret = gen_kv_key_path(path_buffer, sizeof(path_buffer), LFS_EF_NAMESPACE, key);
  if (ret >= sizeof(path_buffer))
  {
    LogError("key name is too long to truncated.\n");
    ret = 0;
    goto _err;
  }

  /* Open the file for reading */
  ret = lfs_file_open(lfs, &file, path_buffer, LFS_O_RDONLY);
  if (ret != LFS_ERR_OK)
  {
    errno = -ret;
    ret = 0;
    goto _err;
  }

  /* Seek to the specified offset */
  ret = lfs_file_seek(lfs, &file, offset, LFS_SEEK_SET);
  if (ret < 0)
  {
    errno = -ret;
    lfs_file_close(lfs, &file);
    ret = 0;
    goto _err;
  }

  /* Read the data from the file */
  read_len = lfs_file_read(lfs, &file, value_buf, buf_len);
  if (read_len < 0)
  {
    LogError("lfs_file_read failed with errno:%" PRIi32 ".\n", ret);
    errno = -ret;
    lfs_file_close(lfs, &file);
    ret = 0;
    goto _err;
  }

  /* If saved_value_len is not NULL, store the length of the saved value */
  if (saved_value_len != NULL)
  {
    /* Get the size of the file */
    ret = lfs_file_size(lfs, &file);
    if (ret < 0)
    {
      LogError("lfs_file_size failed with errno:%" PRIi32 ".\n", ret);
      lfs_file_close(lfs, &file);
      errno = -ret;
      ret = 0;
      goto _err;
    }
    *saved_value_len = ret;
  }
  lfs_file_close(lfs, &file); /* Close the file */
  ret = read_len;
_err:
  ef_giant_unlock(); /* Unlock the semaphore */
  return ret;
}

size_t ef_get_env_blob(const char *key, void *value_buf, size_t buf_len, size_t *saved_value_len)
{
  return ef_get_env_blob_offset(key, value_buf, buf_len, saved_value_len, 0);
}

#ifndef LFS_READONLY
EfErrCode ef_set_env_blob(const char *key, const void *value_buf, size_t buf_len)
{
  lfs_file_t file;
  int32_t ret;

  if (lfs == NULL)
  {
    return EF_ENV_INIT_FAILED;
  }

  if (key == NULL || value_buf == NULL || buf_len == 0)
  {
    return EF_ENV_ARG_ERR;
  }

  /* Lock the semaphore */
  ef_giant_lock();

  /* Generate the key path */
  ret = gen_kv_key_path(path_buffer, sizeof(path_buffer), LFS_EF_NAMESPACE, key);
  if (ret >= sizeof(path_buffer))
  {
    LogError("key name is too long to truncated.\n");
    ret = EF_ENV_NAME_ERR;
    goto _err;
  }

  /* Open the file for writing */
  ret = lfs_file_open(lfs, &file, path_buffer, LFS_O_RDWR | LFS_O_CREAT);
  if (ret != LFS_ERR_OK)
  {
    errno = -ret;
    LogError("lfs_file_open failed with errno:%" PRIi32 "\n", ret);
    ret = EF_WRITE_ERR;
    goto _err;
  }

  /* Write the data to the file */
  ret = lfs_file_write(lfs, &file, value_buf, buf_len);
  if (ret != buf_len)
  {
    errno = -ret;
    LogError("lfs_file_write failed with errno:%" PRIi32 ".\n", ret);
    lfs_file_close(lfs, &file);
    ret = EF_WRITE_ERR;
    goto _err;
  }

  /* Truncates the size of the file to the specified size */
  ret = lfs_file_truncate(lfs, &file, buf_len);
  if (ret != LFS_ERR_OK)
  {
    errno = -ret;
    LogError("lfs_file_truncate failed with errno:%" PRIi32 "\n", ret);
    lfs_file_close(lfs, &file);
    ret = EF_WRITE_ERR;
    goto _err;
  }
  lfs_file_close(lfs, &file); /* Close the file */
  ret = EF_NO_ERR;
_err:
  ef_giant_unlock(); /* Unlock the semaphore */
  return (EfErrCode)ret;
}

EfErrCode ef_del_env(const char *key)
{
  int32_t ret;

  if (lfs == NULL)
  {
    return EF_ENV_INIT_FAILED;
  }

  if (key == NULL)
  {
    return EF_ENV_ARG_ERR;
  }

  /* Lock the semaphore */
  ef_giant_lock();

  /* Generate the key path */
  ret = gen_kv_key_path(path_buffer, sizeof(path_buffer), LFS_EF_NAMESPACE, key);
  if (ret >= sizeof(path_buffer))
  {
    LogError("key name is too long to truncated.\n");
    ret = EF_ENV_NAME_ERR;
    goto _err;
  }
  lfs_remove(lfs, path_buffer); /* Remove the file */
  ret = EF_NO_ERR;
_err:
  ef_giant_unlock(); /* Unlock the semaphore */
  return (EfErrCode)ret;
}
#endif /* LFS_READONLY */

/* clear all env */
EfErrCode ef_env_set_default(void)
{
  /* clear all kv */
  lfs_dir_t dir = {0};
  struct lfs_info info_t = {0};
  int32_t ret;

  if (lfs == NULL)
  {
    return EF_ENV_INIT_FAILED;
  }

  /* Lock the semaphore */
  ef_giant_lock();

  /* Open the directory */
  ret = lfs_dir_open(lfs, &dir, LFS_EF_NAMESPACE);
  if (ret != LFS_ERR_OK)
  {
    errno = -ret;
    ret = EF_READ_ERR;
    goto _err;
  }

  /* Read all files in the directory */
  while (1)
  {
    /* Read the next file */
    ret = lfs_dir_read(lfs, &dir, &info_t);
    if (ret < 0)
    {
      errno = -ret;
      ret = EF_READ_ERR;
      break;
    }
    else if (ret == 0)
    {
      ret = EF_NO_ERR;
      break;
    }

    /* Skip the current and parent directory */
    if (strcmp(info_t.name, ".") == 0 || strcmp(info_t.name, "..") == 0)
    {
      continue;
    }

    /* Check the file type */
    if (info_t.type != LFS_TYPE_REG)
    {
      LogError("Unexpected file type! name:%s, size: %" PRIu32 ", type: %" PRIu16 "\n",
               info_t.name, info_t.size, info_t.type);
      ret = EF_ENV_NAME_ERR;
      break;
    }

    /* Generate the file path */
    ret = snprintf(path_buffer, sizeof(path_buffer), "%s/%s", LFS_EF_NAMESPACE, info_t.name);
    assert(ret <= sizeof(path_buffer) - 1);
#ifndef LFS_READONLY
    /* Remove the file */
    ret = lfs_remove(lfs, path_buffer);
    if (ret < 0)
    {
      errno = -ret;
      ret = EF_WRITE_ERR;
      break;
    }
#endif /* LFS_READONLY */
  }
  lfs_dir_close(lfs, &dir); /* Close the directory */
_err:
  ef_giant_unlock(); /* Unlock the semaphore */
  return (EfErrCode)ret;
}

EfErrCode ef_print_env(EfLfsInfo_t file_list[20], uint32_t *nb_files)
{
  /* clear all kv */
  lfs_dir_t dir = {0};
  struct lfs_info info_t = {0};
  int32_t ret;

  if (lfs == NULL)
  {
    return EF_ENV_INIT_FAILED;
  }

  /* Lock the semaphore */
  ef_giant_lock();

  /* Open the directory */
  ret = lfs_dir_open(lfs, &dir, LFS_EF_NAMESPACE);
  if (ret != LFS_ERR_OK)
  {
    errno = -ret;
    ret = EF_READ_ERR;
    goto _err;
  }

  *nb_files = 0;

  /* Read all files in the directory */
  while (1)
  {
    /* Read the next file */
    ret = lfs_dir_read(lfs, &dir, &info_t);
    if (ret < 0)
    {
      errno = -ret;
      break;
    }
    else if (ret == 0)
    {
      break;
    }

    /* Skip the current and parent directory */
    if (strcmp(info_t.name, ".") == 0 || strcmp(info_t.name, "..") == 0)
    {
      continue;
    }

    strncpy(file_list[*nb_files].name, info_t.name, EF_LFS_NAME_MAX); /* Copy the filename */
    file_list[*nb_files].name[EF_LFS_NAME_MAX] = '\0'; /* Null terminate the filename if truncated */
    file_list[*nb_files].size = info_t.size;        /* Copy the file size */
    (*nb_files)++;
  }
  lfs_dir_close(lfs, &dir); /* Close the directory */
  ret = EF_NO_ERR;
_err:
  ef_giant_unlock(); /* Unlock the semaphore */
  return (EfErrCode)ret;
}

/* Private functions ---------------------------------------------------------*/
static void ef_giant_lock(void)
{
#if configUSE_RECURSIVE_MUTEXES
  xSemaphoreTakeRecursive(env_giant_lock, portMAX_DELAY);
#else
  xSemaphoreTake(env_giant_lock, portMAX_DELAY);
#endif /* configUSE_RECURSIVE_MUTEXES */
}

static void ef_giant_unlock(void)
{
#if configUSE_RECURSIVE_MUTEXES
  xSemaphoreGiveRecursive(env_giant_lock);
#else
  xSemaphoreGive(env_giant_lock);
#endif /* configUSE_RECURSIVE_MUTEXES */
}

static int32_t gen_kv_key_path(char *buf, size_t buf_len, const char *prefix, const char *path)
{
  int32_t i = 0;
  int32_t j = 0;
  int32_t prefix_len = strlen(prefix);
  int32_t path_len;

  i = prefix_len + 1; /* prefix + '/' */

  for (j = 0; path[j] != 0; j++) /* cal full path string length */
  {
    switch (path[j])
    {
      case '}':
      case '/':
        i += 2;
        break;
      default:
        i++;
    }
  }
  path_len = j;

  if (i > buf_len - 1) /* oversize */
  {
    return i;
  }

  memcpy(buf, prefix, prefix_len); /* do strcat */
  buf[prefix_len] = '/';

  for (j = 0, i = 0; j < path_len; j++)
  {
    switch (path[j])
    {
      case '}':
      case '/':
        (buf + prefix_len + 1)[i++] = '}';
        (buf + prefix_len + 1)[i++] = path[j] ^ 0x20;
        break;
      default:
        (buf + prefix_len + 1)[i++] = path[j];
    }
  }

  (buf + prefix_len + 1)[i] = 0;
  return prefix_len + 1 + i;
}

#if defined(__ARMCC_VERSION)
__attribute__((weak, noreturn))
void __aeabi_assert(const char *expr, const char *file, int32_t line)
{
  char str[12], *p;

  fputs("*** assertion failed: ", stderr);
  fputs(expr, stderr);
  fputs(", file ", stderr);
  fputs(file, stderr);
  fputs(", line ", stderr);

  p = str + sizeof(str);
  *--p = '\0';
  *--p = '\n';
  while (line > 0)
  {
    *--p = '0' + (line % 10);
    line /= 10;
  }
  fputs(p, stderr);

  abort();
}

__attribute__((weak))
void abort(void)
{
  for (;;);
}
#endif /* __ARMCC_VERSION */

/* USER CODE BEGIN FD */

/* USER CODE END FD */
