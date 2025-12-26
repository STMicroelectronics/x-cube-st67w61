/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    fota.h
  * @author  GPM Application Team
  * @brief   FOTA test definition
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
#ifndef FOTA_H
#define FOTA_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "event_groups.h"

/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/** Event bits for FOTA update */
/** Start the FOTA update procedure */
#define FOTA_UPDATE_BIT                      (1 << 0)

/** Error occurred */
#define FOTA_ERROR_BIT                       (1 << 1)

/** Notice completion of FOTA transfer to user */
#define FOTA_COMPLETE_XFER_USER_NOTIF_BIT    (1 << 2)

/** FOTA awaiting for user inputs before reboot */
#define FOTA_WAIT_USER_ACK_BIT               (1 << 3)

/** Notice completion of FOTA update to user */
#define FOTA_COMPLETE_UPDATE_USER_NOTIF_BIT  (1 << 4)

/** Return Busy code */
#define FOTA_BUSY                     -2

/** Return Error code */
#define FOTA_ERR                      -1

/** Return Success code */
#define FOTA_SUCCESS                  0

/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported variables --------------------------------------------------------*/
/* Callback function pointers typedef */
/** FOTA callback for operations to do after successful completion */
typedef int32_t (*FOTA_SuccessfulCompletionCallback_t)(void);

/** FOTA callback for operations to do after error on completion */
typedef int32_t (*FOTA_ErrorOnCompletionCallback_t)(void);

/* USER CODE BEGIN EV */

/* USER CODE END EV */

/* Exported macros -----------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions --------------------------------------------------------*/
/**
  * @brief  Return the event group handler used to get events from FOTA destined to other tasks
  * @return EventGroupHandle_t return the event group if it exist, NULL otherwise
  */
EventGroupHandle_t Fota_AppGetEventGroupHandle(void);

/**
  * @brief  Return the event group handler used to interact with FOTA task for event management
  * @return EventGroupHandle_t return the event group if it exist, NULL otherwise
  */
EventGroupHandle_t Fota_GetEventGroupHandle(void);

/**
  * @brief  Create, initialize FOTA and start the FOTA task
  * @return int32_t return FOTA_SUCCESS if operation succeeded, FOTA_ERR otherwise
  */
int32_t Fota_StartFotaTask(void);

/**
  * @brief  Delete the FOTA task and related feature allocated/used
  * @return int32_t return FOTA_SUCCESS if operation succeeded, FOTA_ERR otherwise
  */
int32_t Fota_DeleteFotaTask(void);

/**
  * @brief  Register FOTA callbacks
  * @param  success_cb Callback called on successful completion of the FOTA operation
  * @param  error_cb Callback called on error when completing the FOTA operation
  * */
void Fota_RegisterCallbacks(FOTA_SuccessfulCompletionCallback_t success_cb,
                            FOTA_ErrorOnCompletionCallback_t error_cb);

/**
  * @brief  De init FOTA callbacks by setting NULL values
  * */
void Fota_DeinitCallbacks(void);

/**
  * @brief  Blocking function that waits for events regarding FOTA completion (success or error event),
  *         the waiting time is defined by the FOTA_ACK_TIME define (wait for ever by default).
  *         When the FOTA reaches completion, it call adequate callback function,
  *         it notifies the FOTA task of it's state and exit the blocking loop.
  * @note   Only one call at the time can be done to this function.
  * @return return FOTA_SUCCESS if operation succeeded, FOTA_BUSY is the function is already in use, FOTA_ERR otherwise
  * */
int32_t Fota_WaitForFOTACompletion(void);

/**
  * @brief  Create a timer or configure an existing one that will trigger the FOTA update
  * @param  delay_ms delay in milliseconds before the FOTA update is triggered
  * @return int32_t return FOTA_SUCCESS if operation succeeded, FOTA_ERR otherwise
  */
int32_t Fota_SetFotaTimer(uint32_t delay_ms);

/**
  * @brief  Start the FOTA timer
  * @note   If the timer has not been created, it will do nothing and return a success status
  * @return int32_t return FOTA_SUCCESS if operation succeeded, FOTA_ERR otherwise
  */
int32_t Fota_StartFotaTimer(void);

/**
  * @brief  Stop the FOTA timer
  * @note   If the timer has not been created, it will do nothing and return a success status
  * @return int32_t return FOTA_SUCCESS if operation succeeded, FOTA_ERR otherwise
  */
int32_t Fota_StopFotaTimer(void);

/**
  * @brief  Reset the FOTA timer
  * @note   If the timer has not been created, it will do nothing and return a success status
  * @return int32_t return FOTA_SUCCESS if operation succeeded, FOTA_ERR otherwise
  */
int32_t Fota_ResetFotaTimer(void);

/**
  * @brief  Trigger the FOTA update, it will set the FOTA_UPDATE_BIT event
  */
void Fota_TriggerFotaUpdate(void);

/* USER CODE BEGIN EF */

/* USER CODE END EF */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* FOTA_H */
