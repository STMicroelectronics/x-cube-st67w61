/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    logshell_ctrl.c
  * @author  ST67 Application Team
  * @brief   logshell_ctrl (uart interface)
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
#include <stdint.h>
#include <string.h>

/* Application */
#include "main.h"
#include "app_config.h"
#include "logshell_ctrl.h"
#include "bsp_conf.h"
#include "logging.h"

#include "freertos_tickless.h"
#include "FreeRTOS.h"  /* "include FreeRTOS.h" must appear in source files before "include queue.h" */
#include "queue.h"
#include "event_groups.h"

#include <stdio.h> /* Standard I/O .h-file */
#if defined(__ICCARM__)
#include <LowLevelIOInterface.h>
#endif /* __ICCARM__ */

/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

#ifndef UART_HANDLE
#if (LOG_OUTPUT_MODE == LOG_OUTPUT_UART)
#error "UART instance not selected. Please use platform settings panel in STM32CubeMX GUI."
#endif /* LOG_OUTPUT_MODE */
#endif /* UART_HANDLE */

/* Global variables ----------------------------------------------------------*/

/* USER CODE BEGIN GV */

/* USER CODE END GV */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private defines -----------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */
#ifndef LOG_OUTPUT_MODE
#warning "Missing LOG_OUTPUT_MODE definition. Please use app_config.h"
#endif /* LOG_OUTPUT_MODE */

/* Private macros ------------------------------------------------------------*/
#ifdef UART_HANDLE
/**
  * \def PUTCHAR_PROTOTYPE
  * Common macro to redirect console output to COM
  */
#if defined(__ICCARM__)
/* New definition from EWARM V9, compatible with EWARM8 */
/**
  * @brief  Redirect console output to COM
  * @param  ch: Character to send
  * @retval Character sent
  */
int iar_fputc(int ch);
#define PUTCHAR_PROTOTYPE int iar_fputc(int ch)
#elif defined ( __GNUC__) && !defined(__clang__)
/* With GCC, small printf (option LD Linker->Libraries->Small printf
   set to 'Yes') calls __io_putchar() */
/**
  * @brief  Redirect console output to COM
  * @param  ch: Character to send
  * @retval Character sent
  */
int __io_putchar(int ch);
#define PUTCHAR_PROTOTYPE int __io_putchar(int ch)
#else
/**
  * @brief  Redirect console output to COM
  * @param  ch: Character to send
  * @param  f: Not used
  * @retval Character sent
  */
int fputc(int ch, FILE *f);
#define PUTCHAR_PROTOTYPE int fputc(int ch, FILE *f)
#endif /* __ICCARM__ */
#endif /* UART_HANDLE */

/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
#if defined(UART_HANDLE) && (LOG_OUTPUT_MODE == LOG_OUTPUT_UART)
/** Task handle of the LogOutput task */
xTaskHandle LogOutputTaskHandle = NULL;
#endif /* LOG_OUTPUT_MODE */

#if defined(UART_HANDLE)
/** Queue used by the logging and shell tasks */
QueueHandle_t xLogQueue = NULL;
#endif /* UART_HANDLE */

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Functions Definition ------------------------------------------------------*/
#if defined(UART_HANDLE)
#if defined(__ICCARM__)
/**
  * @brief  Retargets the C library __write function to the IAR function iar_fputc
  * @param  file: file descriptor
  * @param  ptr: pointer to the buffer where the data is stored
  * @param  len: length of the data to write in bytes
  * @retval length of the written data in bytes
  */
/* coverity[misra_c_2012_rule_21_2_violation : FALSE] */
size_t __write(int file, unsigned char const *ptr, size_t len)
{
  size_t idx;
  unsigned char const *pdata = ptr;
  /* USER CODE BEGIN __write_1 */

  /* USER CODE END __write_1 */

  for (idx = 0; idx < len; idx++)
  {
    (void)iar_fputc((int)*pdata);
    pdata++;
  }
  return len;
  /* USER CODE BEGIN __write_End */

  /* USER CODE END __write_End */
}
#endif /* __ICCARM__ */

PUTCHAR_PROTOTYPE
{
  /* USER CODE BEGIN PUTCHAR_PROTOTYPE_1 */

  /* USER CODE END PUTCHAR_PROTOTYPE_1 */
  (void)HAL_UART_Transmit(&UART_HANDLE, (unsigned char *)&ch, 1, 1000);
  return ch;
  /* USER CODE BEGIN PUTCHAR_PROTOTYPE_End */

  /* USER CODE END PUTCHAR_PROTOTYPE_End */
}

#if (LOG_OUTPUT_MODE == LOG_OUTPUT_UART)
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
  BaseType_t higher_priority_tas_woken = pdFALSE;
  /* USER CODE BEGIN HAL_UART_TxCpltCallback_1 */

  /* USER CODE END HAL_UART_TxCpltCallback_1 */

  vTaskNotifyGiveFromISR(LogOutputTaskHandle, &higher_priority_tas_woken);

  portYIELD_FROM_ISR(higher_priority_tas_woken);
  /* USER CODE BEGIN HAL_UART_TxCpltCallback_End */

  /* USER CODE END HAL_UART_TxCpltCallback_End */
}
#endif /* LOG_OUTPUT_MODE */

void LogOutput(const char *message)
{
  /* USER CODE BEGIN LogOutput_1 */

  /* USER CODE END LogOutput_1 */
#if (LOG_OUTPUT_MODE == LOG_OUTPUT_PRINTF) /* Direct printf via putchar */
  /* Transmit if bytes available to transmit */
  (void)printf("%s", message);

#elif (LOG_OUTPUT_MODE == LOG_OUTPUT_UART) /* UART in interrupt mode */
  HAL_StatusTypeDef hal_status = HAL_OK;

  LogOutputTaskHandle = xTaskGetCurrentTaskHandle();

  DisableSuppressTicksAndSleep(1UL << CFG_TICKLESS_LOG_ID);

  hal_status = HAL_UART_Transmit_IT(&UART_HANDLE, (uint8_t *) message, (uint16_t) strlen(message));

  if (hal_status == HAL_OK)
  {
    /* Wait for completion event (should be within 1 or 2 ms) */
    (void)ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
  }
  EnableSuppressTicksAndSleep(1UL << CFG_TICKLESS_LOG_ID);

#elif (LOG_OUTPUT_MODE == LOG_OUTPUT_ITM) /* ITM */
  /* Transmit if bytes available to transmit */
  for (int32_t i = 0; i < strlen(message); i++)
  {
    ITM_SendChar(message[i]);
  }
#endif /* LOG_OUTPUT_MODE */
  /* USER CODE BEGIN LogOutput_End */

  /* USER CODE END LogOutput_End */
}

#endif /* UART_HANDLE */

void LoggingInit(void)
{
  /* USER CODE BEGIN LoggingInit_1 */

  /* USER CODE END LoggingInit_1 */
#ifdef UART_HANDLE
  xLogQueue = vLoggingInit(LogOutput);
#endif /* UART_HANDLE */
  /* USER CODE BEGIN LoggingInit_End */

  /* USER CODE END LoggingInit_End */
}

void ShellInit(void)
{
  /* USER CODE BEGIN ShellInit_1 */

  /* USER CODE END ShellInit_1 */
}

/* USER CODE BEGIN FD */

/* USER CODE END FD */
