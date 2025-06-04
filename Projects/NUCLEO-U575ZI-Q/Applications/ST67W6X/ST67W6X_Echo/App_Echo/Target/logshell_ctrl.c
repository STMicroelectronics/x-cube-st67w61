/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    logshell_ctrl.c
  * @author  GPM Application Team
  * @brief   logshell_ctrl (uart interface)
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
#include <stdint.h>
#include <string.h>

/* Application */
#include "main.h"
#include "app_config.h"
#include "logshell_ctrl.h"
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

/* Global variables ----------------------------------------------------------*/
#define UART_HANDLE   huart1
extern UART_HandleTypeDef UART_HANDLE;

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
#if defined(__ICCARM__)
/* New definition from EWARM V9, compatible with EWARM8 */
int iar_fputc(int ch);
#define PUTCHAR_PROTOTYPE int iar_fputc(int ch)
#elif defined ( __GNUC__) && !defined(__clang__)
/* With GCC, small printf (option LD Linker->Libraries->Small printf
   set to 'Yes') calls __io_putchar() */
#define PUTCHAR_PROTOTYPE int __io_putchar(int ch)
#else
#define PUTCHAR_PROTOTYPE int fputc(int ch, FILE *f)
#endif /* __ICCARM__ */

/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
#if (LOG_OUTPUT_MODE == LOG_OUTPUT_UART)
xTaskHandle LogOutputTaskHandle = NULL;
#endif /* LOG_OUTPUT_MODE */

QueueHandle_t xLogQueue = NULL;

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Functions Definition ------------------------------------------------------*/
/**
  * @brief  Retargets the C library __write function to the IAR function iar_fputc.
  * @param  file: file descriptor.
  * @param  ptr: pointer to the buffer where the data is stored.
  * @param  len: length of the data to write in bytes.
  * @retval length of the written data in bytes.
  */
#if defined(__ICCARM__)
size_t __write(int file, unsigned char const *ptr, size_t len)
{
  size_t idx;
  unsigned char const *pdata = ptr;
  /* USER CODE BEGIN __write_1 */

  /* USER CODE END __write_1 */

  for (idx = 0; idx < len; idx++)
  {
    iar_fputc((int)*pdata);
    pdata++;
  }
  return len;
  /* USER CODE BEGIN __write_End */

  /* USER CODE END __write_End */
}
#endif /* __ICCARM__ */

/**
  * @brief  Redirect console output to COM
  */
PUTCHAR_PROTOTYPE
{
  /* USER CODE BEGIN PUTCHAR_PROTOTYPE_1 */

  /* USER CODE END PUTCHAR_PROTOTYPE_1 */
  HAL_UART_Transmit(&UART_HANDLE, (unsigned char *)&ch, 1, 1000);
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
  printf("%s", message);

#elif (LOG_OUTPUT_MODE == LOG_OUTPUT_UART) /* UART in interrupt mode */
  HAL_StatusTypeDef hal_status = HAL_OK;

  LogOutputTaskHandle = xTaskGetCurrentTaskHandle();

  DisableSuppressTicksAndSleep(1 << CFG_TICKLESS_LOG_ID);

  hal_status = HAL_UART_Transmit_IT(&UART_HANDLE, (uint8_t *) message, (uint16_t) strlen(message));
  /* configASSERT( xHalStatus == HAL_OK ); */

  if (hal_status == HAL_OK)
  {
    /* Wait for completion event (should be within 1 or 2 ms) */
    (void) ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
  }
  EnableSuppressTicksAndSleep(1 << CFG_TICKLESS_LOG_ID);

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

void LoggingInit(void)
{
  /* USER CODE BEGIN LoggingInit_1 */

  /* USER CODE END LoggingInit_1 */
  xLogQueue = vLoggingInit(LogOutput);
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
