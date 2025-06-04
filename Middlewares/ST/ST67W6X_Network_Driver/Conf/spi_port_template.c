/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    spi_port_template.c
  * @author  GPM Application Team
  * @brief   SPI bus interface porting layer implementation
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

/**
  * This file is based on QCC74xSDK provided by Qualcomm.
  * See https://git.codelinaro.org/clo/qcc7xx/QCCSDK-QCC74x for more information.
  *
  * Reference source: examples/stm32_spi_host/QCC743_SPI_HOST/Core/Src/spi_iface.c
  */

/* Includes ------------------------------------------------------------------*/
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#include "spi_port.h"
#include "main.h"

/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Global variables ----------------------------------------------------------*/
extern SPI_HandleTypeDef hspi1;

/* USER CODE BEGIN GV */

/* USER CODE END GV */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private defines -----------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macros ------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
static spi_transaction_complete_t spi_port_transaction_complete_cb = NULL;

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Functions Definition ------------------------------------------------------*/
#ifdef __ICCARM__
void *spi_port_memcpy(void *dest, const void *src, unsigned int len)
#endif /* __ICCARM__ */
#ifdef __GNUC__
void *memcpy(void *dest, const void *src, unsigned int len)
#endif /* __ICCARM__ */
{
  /* USER CODE BEGIN memcpy_1 */

  /* USER CODE END memcpy_1 */
  uint8_t *d = (uint8_t *)dest;
  const uint8_t *s = (const uint8_t *)src;

  /* Copy bytes until the destination address is aligned to 4 bytes */
  while (((uint32_t) d % 4 != 0) && len > 0)
  {
    *d++ = *s++;
    len--;
  }

  /* Copy 4-byte blocks */
  uint32_t *d32 = (uint32_t *)d;
  const uint32_t *s32 = (const uint32_t *)s;
  while (len >= 4)
  {
    *d32++ = *s32++;
    len -= 4;
  }

  /* Copy remaining bytes */
  d = (uint8_t *)d32;
  s = (const uint8_t *)s32;
  while (len > 0)
  {
    *d++ = *s++;
    len--;
  }

  return dest;
  /* USER CODE BEGIN memcpy_End */

  /* USER CODE END memcpy_End */
}

int32_t spi_port_init(spi_transaction_complete_t transaction_complete_cb)
{
  /* USER CODE BEGIN spi_port_init_1 */

  /* USER CODE END spi_port_init_1 */
  /* Powering up the NCP using GPIO CHIP_EN */
  HAL_GPIO_WritePin(CHIP_EN_GPIO_Port, CHIP_EN_Pin, GPIO_PIN_SET);

  if (transaction_complete_cb != NULL)
  {
    spi_port_transaction_complete_cb = transaction_complete_cb;
  }

  return 0;
  /* USER CODE BEGIN spi_port_init_End */

  /* USER CODE END spi_port_init_End */
}

int32_t spi_port_deinit(void)
{
  /* USER CODE BEGIN spi_port_deinit_1 */

  /* USER CODE END spi_port_deinit_1 */
  /* Switch off the NCP using GPIO CHIP_EN */
  HAL_GPIO_WritePin(CHIP_EN_GPIO_Port, CHIP_EN_Pin, GPIO_PIN_RESET);

  spi_port_transaction_complete_cb = NULL;

  return 0;
  /* USER CODE BEGIN spi_port_deinit_End */

  /* USER CODE END spi_port_deinit_End */
}

int32_t spi_port_transfer(void *tx_buf, void *rx_buf, uint16_t len, uint32_t timeout)
{
  /* USER CODE BEGIN spi_port_transfer_1 */

  /* USER CODE END spi_port_transfer_1 */
  HAL_StatusTypeDef status;

#if defined (__DCACHE_PRESENT) && (__DCACHE_PRESENT == 1U)
  SCB_CleanInvalidateDCache_by_Addr(rx_buf, len);
#endif /* __DCACHE_PRESENT */

  /* Check whether host data is to be transmitted to the NCP, otherwise read only data from the NCP */
  if (tx_buf != NULL)
  {
#if defined (__DCACHE_PRESENT) && (__DCACHE_PRESENT == 1U)
    SCB_CleanDCache_by_Addr(tx_buf, len);
#endif /* __DCACHE_PRESENT */
    status = HAL_SPI_TransmitReceive(&hspi1, tx_buf, rx_buf, len, timeout);
  }
  else
  {
    status = HAL_SPI_Receive(&hspi1, rx_buf, len, timeout);
  }

  return (status == HAL_OK ? 0 : -1);
  /* USER CODE BEGIN spi_port_transfer_End */

  /* USER CODE END spi_port_transfer_End */
}

int32_t spi_port_transfer_dma(void *tx_buf, void *rx_buf, uint16_t len)
{
  /* USER CODE BEGIN spi_port_transfer_dma_1 */

  /* USER CODE END spi_port_transfer_dma_1 */
  HAL_StatusTypeDef status;

#if defined (__DCACHE_PRESENT) && (__DCACHE_PRESENT == 1U)
  SCB_CleanInvalidateDCache_by_Addr(rx_buf, len);
#endif /* __DCACHE_PRESENT */

  /* Check whether host data is to be transmitted to the NCP, otherwise read only data from the NCP */
  if (tx_buf != NULL)
  {
#if defined (__DCACHE_PRESENT) && (__DCACHE_PRESENT == 1U)
    SCB_CleanDCache_by_Addr(tx_buf, len);
#endif /* __DCACHE_PRESENT */
    status = HAL_SPI_TransmitReceive_DMA(&hspi1, tx_buf, rx_buf, len);
  }
  else
  {
    status = HAL_SPI_Receive_DMA(&hspi1, rx_buf, len);
  }
  return (status == HAL_OK ? 0 : -1);
  /* USER CODE BEGIN spi_port_transfer_dma_End */

  /* USER CODE END spi_port_transfer_dma_End */
}

int32_t spi_port_is_ready(void)
{
  /* USER CODE BEGIN spi_port_is_ready_1 */

  /* USER CODE END spi_port_is_ready_1 */
  /* Check whether NCP data are available on the SPI bus */
  return (int32_t)HAL_GPIO_ReadPin(SPI_RDY_GPIO_Port, SPI_RDY_Pin);
  /* USER CODE BEGIN spi_port_is_ready_End */

  /* USER CODE END spi_port_is_ready_End */
}

int32_t spi_port_set_cs(int32_t state)
{
  /* USER CODE BEGIN spi_port_set_cs_1 */

  /* USER CODE END spi_port_set_cs_1 */
  if (state)
  {
    /* Activate Chip Select before starting transfer */
    HAL_GPIO_WritePin(SPI_CS_GPIO_Port, SPI_CS_Pin, GPIO_PIN_SET);
  }
  else
  {
    /* Disable Chip Select when transfer is complete */
    HAL_GPIO_WritePin(SPI_CS_GPIO_Port, SPI_CS_Pin, GPIO_PIN_RESET);
  }

  return 0;
  /* USER CODE BEGIN spi_port_set_cs_End */

  /* USER CODE END spi_port_set_cs_End */
}

uint32_t spi_port_itm(uint32_t ch)
{
  /* USER CODE BEGIN spi_port_itm_1 */

  /* USER CODE END spi_port_itm_1 */
#if 0
  return ITM_SendChar(ch);
#endif /* 0 */
  return 0;
  /* USER CODE BEGIN spi_port_itm_End */

  /* USER CODE END spi_port_itm_End */
}

/* USER CODE BEGIN FD */

/* USER CODE END FD */

/* Weak functions redefinition -----------------------------------------------*/
void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi)
{
  /* USER CODE BEGIN HAL_SPI_TxCpltCallback_1 */

  /* USER CODE END HAL_SPI_TxCpltCallback_1 */
  spi_port_transaction_complete_cb();
  /* USER CODE BEGIN HAL_SPI_TxCpltCallback_End */

  /* USER CODE END HAL_SPI_TxCpltCallback_End */
}

void HAL_SPI_RxCpltCallback(SPI_HandleTypeDef *hspi)
{
  /* USER CODE BEGIN HAL_SPI_RxCpltCallback_1 */

  /* USER CODE END HAL_SPI_RxCpltCallback_1 */
  spi_port_transaction_complete_cb();
  /* USER CODE BEGIN HAL_SPI_RxCpltCallback_End */

  /* USER CODE END HAL_SPI_RxCpltCallback_End */
}

void HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef *hspi)
{
  /* USER CODE BEGIN HAL_SPI_TxRxCpltCallback_1 */

  /* USER CODE END HAL_SPI_TxRxCpltCallback_1 */
  spi_port_transaction_complete_cb();
  /* USER CODE BEGIN HAL_SPI_TxRxCpltCallback_End */

  /* USER CODE END HAL_SPI_TxRxCpltCallback_End */
}

/* USER CODE BEGIN WFR */

/* USER CODE END WFR */
