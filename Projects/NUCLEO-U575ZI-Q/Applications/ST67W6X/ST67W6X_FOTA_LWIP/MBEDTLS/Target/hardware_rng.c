/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    hardware_rng.c
  * @author  ST67 Application Team
  * @brief   mbedtls alternate entropy data function.
  *          the mbedtls_hardware_poll() is customized to use the STM32 RNG
  *          to generate random data, required for TLS encryption algorithms.
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
#include "mbedtls_config.h"

#ifdef MBEDTLS_ENTROPY_HARDWARE_ALT
#include "main.h"
#include "string.h"
#include "entropy_poll.h"
#include "stm32u5xx_hal.h"
#endif /* MBEDTLS_ENTROPY_HARDWARE_ALT */

/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Global variables ----------------------------------------------------------*/
#if defined(MBEDTLS_ENTROPY_HARDWARE_ALT) && defined(HAL_RNG_MODULE_ENABLED)
extern RNG_HandleTypeDef hrng;
#endif /* MBEDTLS_ENTROPY_HARDWARE_ALT && HAL_RNG_MODULE_ENABLED */

/* USER CODE BEGIN GV */

/* USER CODE END GV */

/* Functions Definition ------------------------------------------------------*/
#ifdef MBEDTLS_ENTROPY_HARDWARE_ALT
int mbedtls_hardware_poll(void *data, unsigned char *output, size_t len, size_t *olen)
{
  /* USER CODE BEGIN mbedtls_hardware_poll_1 */

  /* USER CODE END mbedtls_hardware_poll_1 */
#if defined(HAL_RNG_MODULE_ENABLED)
  uint32_t index;
  uint32_t randomValue = 0U;

  for (index = 0; index < len / 4; index++)
  {
    if (HAL_RNG_GenerateRandomNumber(&hrng, &randomValue) == HAL_OK)
    {
      *olen += 4;
      (void)memset(&(output[index * 4]), (int)randomValue, 4);
    }
    else
    {
      Error_Handler();
    }
  }
#endif /* HAL_RNG_MODULE_ENABLED */

  return 0;

  /* USER CODE BEGIN mbedtls_hardware_poll_End */

  /* USER CODE END mbedtls_hardware_poll_End */
}
#endif /* MBEDTLS_ENTROPY_HARDWARE_ALT */

/* USER CODE BEGIN FD */

/* USER CODE END FD */
