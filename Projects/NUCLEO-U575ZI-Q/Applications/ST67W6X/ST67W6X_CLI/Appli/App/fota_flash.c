/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    fota_flash.c
  * @author  ST67 Application Team
  * @brief   This file provides code for Flash operations used in FOTA.
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
#include "fota_flash.h"
#include "logging.h"

/* Private function prototypes -----------------------------------------------*/
static uint32_t GetPage(uint32_t Addr);

/* Functions Definition ------------------------------------------------------*/
int32_t fota_flash_WriteToFlash(uint32_t address, const uint8_t *data, uint32_t size)
{
  int32_t ret = -1;

  /* Check for alignment */
  if ((address % QUADWORD_SIZE_BYTE) != 0)
  {
    return ret;
  }

  uint32_t program_count = (size % QUADWORD_SIZE_BYTE) != 0 ?
                           (size / QUADWORD_SIZE_BYTE) + 1 :
                           (size / QUADWORD_SIZE_BYTE);

  uint64_t *data_64 = (uint64_t *)data;
  uint32_t addr = address;

  if (HAL_FLASH_Unlock() != HAL_OK) /* Unlock the Flash */
  {
    return ret;
  }

  /* Program the Flash */
  for (uint32_t i = 0; i < program_count; ++i)
  {
    if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_QUADWORD, addr, (uint32_t) data_64) == HAL_OK)
    {
      addr += QUADWORD_SIZE_BYTE;
      data_64 += QUADWORD_SIZE_BYTE / 8U;
    }
    else
    {
      goto _err;
    }
  }
  ret = 0;

_err:
  (void)HAL_FLASH_Lock(); /* Lock the Flash */
  return ret;
}

int32_t fota_flash_WriteToFlashBurst(uint32_t address, const uint8_t *data, uint32_t size)
{
  int32_t ret = -1;

  /* Check for alignment */
  if (((address % 32U) != 0) || ((size % BURST_SIZE_BYTE) != 0))
  {
    return ret;
  }

  uint32_t program_count = (size / BURST_SIZE_BYTE);
  uint32_t data_32 = (uint32_t)data;
  uint32_t addr = address;

  if (HAL_FLASH_Unlock() != HAL_OK) /* Unlock the Flash */
  {
    return ret;
  }

  for (uint32_t i = 0; i < program_count; ++i)
  {
    if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_BURST, addr, (uint32_t)data) == HAL_OK)
    {
      addr += BURST_SIZE_BYTE;
      data_32 += BURST_SIZE_BYTE;
    }
    else
    {
      goto _err;
    }
  }
  ret = 0;

_err:
  (void)HAL_FLASH_Lock(); /* Lock the Flash */
  return ret;
}

int32_t fota_flash_EraseFlash(uint32_t address, uint32_t size)
{
  int32_t ret = -1;
  uint32_t page_error = 0;
  FLASH_EraseInitTypeDef erase_init;
  bool swap_status = false;

  if (HAL_FLASH_Unlock() != HAL_OK) /* Unlock the Flash */
  {
    return ret;
  }

  if (fota_flash_IsFlashBankSwapped(&swap_status) != 0)
  {
    goto _err;
  }

  /* Configure erase parameters */
  erase_init.TypeErase = FLASH_TYPEERASE_PAGES;
  erase_init.Banks = swap_status ? FLASH_BANK_1 : FLASH_BANK_2;
  erase_init.Page = GetPage(address);
  erase_init.NbPages = GetPage(address + size - 1) - erase_init.Page + 1;

  /* Perform the erase operation */
  if (HAL_FLASHEx_Erase(&erase_init, &page_error) != HAL_OK)
  {
    goto _err;
  }
  ret = 0;

_err:
  (void)HAL_FLASH_Lock(); /* Lock the Flash */
  return ret;
}

uint32_t fota_flash_GetStartAddrFlashBank(void)
{
  /* From the application point of view, it always executes from Bank 1
     because the address re-mapping is done by hardware. */
  return (FLASH_BASE + FLASH_BANK_SIZE);
}

int32_t fota_flash_IsFlashBankSwapped(bool *swap_status)
{
  int32_t ret = -1;
  FLASH_OBProgramInitTypeDef adv_ob_init = {0};

  if (HAL_FLASH_Unlock() != HAL_OK) /* Unlock the Flash */
  {
    return ret;
  }

  /* Allow Access to option bytes sector */
  if (HAL_FLASH_OB_Unlock() != HAL_OK)
  {
    goto _err;
  }

  /* Get the Dual boot configuration status */
  adv_ob_init.OptionType = OB_BOOTADDR_NS0 | OB_BOOTADDR_NS1;
  HAL_FLASHEx_OBGetConfig(&adv_ob_init);

  *swap_status = (adv_ob_init.USERConfig & OB_SWAP_BANK_ENABLE) != OB_SWAP_BANK_DISABLE;
  ret = 0;

_err:
  /* Lock the Option Bytes and Flash */
  (void)HAL_FLASH_OB_Lock();
  (void)HAL_FLASH_Lock();
  return ret;
}

int32_t fota_flash_SwapFlashBanks(void)
{
  int32_t ret = -1;
  FLASH_OBProgramInitTypeDef ob_init = {0};

  if (HAL_FLASH_Unlock() != HAL_OK) /* Unlock the Flash */
  {
    return ret;
  }

  /* Unlock the Option Bytes */
  if (HAL_FLASH_OB_Unlock() != HAL_OK)
  {
    goto _err;
  }

  /* Get the current Option Bytes configuration */
  HAL_FLASHEx_OBGetConfig(&ob_init);

  /* Toggle the bank swap */
  ob_init.OptionType = OPTIONBYTE_USER;
  ob_init.USERType = OB_USER_SWAP_BANK;
  ob_init.USERConfig = (ob_init.USERConfig & OB_SWAP_BANK_ENABLE) ? OB_SWAP_BANK_DISABLE : OB_SWAP_BANK_ENABLE;

  /* Set the new Option Bytes configuration */
  if (HAL_FLASHEx_OBProgram(&ob_init) != HAL_OK)
  {
    goto _err;
  }

  /* Launch the Option Bytes programming */
  if (HAL_FLASH_OB_Launch() != HAL_OK)
  {
    goto _err;
  }
  ret = 0;

_err:
  /* Lock the Option Bytes and Flash */
  (void)HAL_FLASH_OB_Lock();
  (void)HAL_FLASH_Lock();
  return ret;
}

int32_t fota_flash_MassEraseFlashBank(void)
{
  int32_t ret = -1;
  uint32_t page_error = 0;
  FLASH_EraseInitTypeDef erase_init_struct;
  bool swap_status = false;

  if (fota_flash_IsFlashBankSwapped(&swap_status) != 0)
  {
    goto _err;
  }

  if (HAL_FLASH_Unlock() != HAL_OK) /* Unlock the Flash */
  {
    return -1;
  }

  erase_init_struct.TypeErase = FLASH_TYPEERASE_MASSERASE;
  erase_init_struct.Banks = swap_status ? FLASH_BANK_1 : FLASH_BANK_2;
  erase_init_struct.Page = 0U; /* We erase the target bank starting from the first page.*/
  erase_init_struct.NbPages = FLASH_PAGE_NB;

  if (HAL_FLASHEx_Erase(&erase_init_struct, &page_error) != HAL_OK)
  {
    goto _err;
  }
  ret = 0;

_err:
  (void)HAL_FLASH_Lock(); /* Lock the Flash */
  return ret;
}

/**
  * @brief  Gets the page of a given address
  * @param  Addr: Address of the FLASH Memory
  * @retval The page of a given address
  */
static uint32_t GetPage(uint32_t Addr)
{
  uint32_t page = 0;
  if (Addr < (FLASH_BASE + FLASH_BANK_SIZE))
  {
    /* Bank 1 */
    page = (Addr - FLASH_BASE) / FLASH_PAGE_SIZE;
  }
  else
  {
    /* Bank 2 */
    page = (Addr - (FLASH_BASE + FLASH_BANK_SIZE)) / FLASH_PAGE_SIZE;
  }

  return page;
}
