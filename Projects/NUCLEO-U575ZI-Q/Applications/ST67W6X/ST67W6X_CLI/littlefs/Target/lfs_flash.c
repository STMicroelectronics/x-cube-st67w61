/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    lfs_flash.c
  * @author  GPM Application Team
  * @brief   Host flash interface
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
#include <errno.h>

#include "lfs.h"

#include "FreeRTOS.h"
#include "semphr.h"

#include "lfs_port.h"

#include "logging.h"

#define DBG_TAG "LFS"

/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Global variables ----------------------------------------------------------*/
/* USER CODE BEGIN GV */

/* USER CODE END GV */

/* Private defines -----------------------------------------------------------*/
/* USER CODE BEGIN PD */
/** This value must be aligned with prebuild script argument */
#define LITTLEFS_SECTION_SIZE 0x4000

/* USER CODE END PD */

/* Private macros ------------------------------------------------------------*/
/* USER CODE BEGIN PM */
#if defined(__GNUC__)
#define STR2(x) #x
#define ICSTR(x) STR2(x)

#define INCBIN(name, file)                  \
  __asm__(".section .rodata\n"              \
          ".global " ICSTR(name) "_start\n" \
          ".balign 8\n"                     \
          ICSTR(name) "_start:\n"           \
          ".incbin \"" file "\"\n"          \
          ".global " ICSTR(name) "_end\n"   \
          ".balign 1\n"                     \
          ICSTR(name) "_end:\n"             \
          ".byte 0\n"                       \
         );                                 \
  extern __attribute__((aligned(8))) const char name ## _start[]; \
  extern __attribute__((aligned(8))) const char name ## _end[]
#endif /* __GNUC__ */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */
#if defined(__CC_ARM) || defined(__CLANG_ARM) /* ARM C Compiler */
#elif defined(__ICCARM__) || defined(__ICCRX__) /* For IAR Compiler */
/* The file system binary must be configured into the project linker settings: --image_input */
extern __attribute__((aligned(8))) const char littlefs_start[];

#elif defined(__GNUC__)
INCBIN(littlefs, "../../littlefs/littlefs.bin");

#endif /* __CC_ARM | __GNUC__ */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
static int32_t lfs_flash_giant_lock(const struct lfs_config *c);
static int32_t lfs_flash_giant_unlock(const struct lfs_config *c);
static int32_t lfs_flash_read(const struct lfs_config *c, lfs_block_t block,
                              lfs_off_t off, void *buffer, lfs_size_t size);
static int32_t lfs_flash_prog(const struct lfs_config *c, lfs_block_t block,
                              lfs_off_t off, const void *buffer, lfs_size_t size);
static int32_t lfs_flash_erase(const struct lfs_config *c, lfs_block_t block);
static int32_t lfs_flash_sync(const struct lfs_config *c);

/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Functions Definition ------------------------------------------------------*/
lfs_t *lfs_flash_init(struct lfs_context *lfs_flash_ctx, struct lfs_config *cfg)
{
  int32_t ret;
  lfs_t *lfs = &lfs_flash_ctx->lfs;

  if (lfs->cfg == cfg)
  {
    return lfs;
  }
  cfg->context = lfs_flash_ctx;
  cfg->read = lfs_flash_read;
  cfg->prog = lfs_flash_prog;
  cfg->erase = lfs_flash_erase;
  cfg->sync = lfs_flash_sync;
  cfg->lock = lfs_flash_giant_lock;
  cfg->unlock = lfs_flash_giant_unlock;
  cfg->block_count = 0;
  lfs_flash_ctx->flash_addr = 0;

  /* USER CODE BEGIN lfs_flash_init_1 */
  lfs_flash_ctx->flash_addr = (uint32_t)(&littlefs_start);
  cfg->block_count = (uint32_t)LITTLEFS_SECTION_SIZE / cfg->block_size;

  /* USER CODE END lfs_flash_init_1 */

  if (lfs_flash_ctx->flash_addr == 0)
  {
    LogWarn("LFS binary address not defined\n");
    return NULL;
  }

#if configUSE_RECURSIVE_MUTEXES
  lfs_flash_ctx->fs_giant_lock = xSemaphoreCreateRecursiveMutex();
#else
  lfs_flash_ctx->fs_giant_lock = xSemaphoreCreateMutex();
#endif /* configUSE_RECURSIVE_MUTEXES */

  /* mount the filesystem */
  ret = lfs_mount(lfs, cfg);

#ifndef LFS_READONLY
  /* reformat if we can't mount the filesystem
   * this should only happen on the first boot */
  if (ret == LFS_ERR_CORRUPT)
  {
    LogWarn("try to reformat\n");
    ret = lfs_format(lfs, cfg);
    if (ret)
    {
      LogError("reformat fail\n");
      errno = LFS_ERR_CORRUPT;
      return NULL;
    }

    LogInfo("reformat success\n");
    ret = lfs_mount(lfs, cfg);
    if (ret)
    {
      errno = ret;
      return NULL;
    }
  }
  else if (ret != LFS_ERR_OK)
  {
    errno = ret;
    return NULL;
  }
#else
  if (ret != LFS_ERR_OK)
  {
    errno = ret;
    return NULL;
  }
#endif /* LFS_READONLY */

  LogInfo("mount success\n");

  return lfs;
}

/* USER CODE BEGIN FD */

/* USER CODE END FD */

/* Private Functions Definition ----------------------------------------------*/
static int32_t lfs_flash_giant_lock(const struct lfs_config *c)
{
  struct lfs_context *ctx = c->context;
#if configUSE_RECURSIVE_MUTEXES
  xSemaphoreTakeRecursive(ctx->fs_giant_lock, portMAX_DELAY);
#else
  xSemaphoreTake(ctx->fs_giant_lock, portMAX_DELAY);
#endif /* configUSE_RECURSIVE_MUTEXES */
  return 0;
}

static int32_t lfs_flash_giant_unlock(const struct lfs_config *c)
{
  struct lfs_context *ctx = c->context;
#if configUSE_RECURSIVE_MUTEXES
  xSemaphoreGiveRecursive(ctx->fs_giant_lock);
#else
  xSemaphoreGive(ctx->fs_giant_lock);
#endif /* configUSE_RECURSIVE_MUTEXES */
  return 0;
}

/*****************************************************************************
  * @brief        Read a region in a block. Negative error codes are propagated
  *               to the user.
  * @param[in]    c
  * @param[in]    block
  * @param[in]    off
  * @param[out]   buffer
  * @param[in]    size
  *
  * @retval int
  *****************************************************************************/
static int32_t lfs_flash_read(const struct lfs_config *c, lfs_block_t block,
                              lfs_off_t off, void *buffer, lfs_size_t size)
{
  struct lfs_context *ctx = c->context;

  if (ctx->flash_addr == 0)
  {
    LogWarn("LFS binary address not defined\n");
    return -1;
  }

  uint32_t addr = ctx->flash_addr + block * c->block_size + off;
  uint8_t *data = (uint8_t *)buffer;
  uint32_t len = size;

  memcpy(data, (void *)addr, len);

  return 0;
}

/*****************************************************************************
  * @brief        Program a region in a block. The block must have previously
  *               been erased. Negative error codes are propagated to the user.
  *               May return LFS_ERR_CORRUPT if the block should be considered bad.
  * @param[in]    c
  * @param[in]    block
  * @param[in]    off
  * @param[in]    buffer
  * @param[in]    size
  *
  * @retval int
  *****************************************************************************/
static int32_t lfs_flash_prog(const struct lfs_config *c, lfs_block_t block,
                              lfs_off_t off, const void *buffer, lfs_size_t size)
{
  return 0;
}

/*****************************************************************************
  * @brief        Erase a block. A block must be erased before being programmed.
  *               The state of an erased block is undefined. Negative error codes
  *               are propagated to the user.
  *               May return LFS_ERR_CORRUPT if the block should be considered bad.
  * @param[in]    c
  * @param[in]    block
  *
  * @retval int
  *****************************************************************************/
static int32_t lfs_flash_erase(const struct lfs_config *c, lfs_block_t block)
{
  return 0;
}

/*****************************************************************************
  * @brief        Sync the state of the underlying block device. Negative error
  *               codes are propagated to the user.
  * @param[in]    c
  *
  * @retval int
  *****************************************************************************/
static int32_t lfs_flash_sync(const struct lfs_config *c)
{
  return 0;
}

/* USER CODE BEGIN PFD */

/* USER CODE END PFD */
