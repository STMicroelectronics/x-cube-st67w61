/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    fota.c
  * @author  GPM Application Team
  * @brief   Test a FOTA with a server
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
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "fota.h"
#include "app_config.h"
#include "w6x_api.h"
#include "w6x_version.h"
#include "main_app.h"
#include "common_parser.h"
#include "shell.h"

/* USER CODE BEGIN Includes */
#include "fota_flash.h"
#include "fota_header.h"
#include "sha256.h"
#include "cJSON.h"

/* USER CODE END Includes */

/* Global variables ----------------------------------------------------------*/
/* USER CODE BEGIN GV */

/* USER CODE END GV */

/* Private defines -----------------------------------------------------------*/
#ifndef FOTA_TIMEOUT
/** Timeout value to set the FOTA timer to when the FOTA application encountered an error for the first time.
  * This allows to tune the timeout value before doing a retry attempt. (not applicable if FOTA timer is not used)*/
#define FOTA_TIMEOUT                20000
#endif /* FOTA_TIMEOUT */

#ifndef FOTA_TASK_STACK_SIZE
/** Stack size of the FOTA application, this value needs to take into account the HTTP client
  * and NCP FWU static data allocation */
#define FOTA_TASK_STACK_SIZE        1800
#endif /* FOTA_TASK_STACK_SIZE */

#ifndef FOTA_URI_MAX_SIZE
/** The max size of the URI supported, this because the buffer
  * that will receive this info is allocated at compile time (static) */
#define FOTA_URI_MAX_SIZE           256
#endif /* FOTA_URI_MAX_SIZE */

#ifndef FOTA_HTTP_URI
/** Default URI for the ST67 binary, should be smaller in bytes size than the value defined by FOTA_URI_MAX_SIZE */
#define FOTA_HTTP_URI               "/download/st67w611m_mission_t01_v2.0.97.bin.ota"
#endif /* FOTA_HTTP_URI */

#ifndef FOTA_HTTP_SERVER_ADDR
/** Default HTTP server address */
#define FOTA_HTTP_SERVER_ADDR       "192.168.8.105"
#endif /* FOTA_HTTP_SERVER_ADDR */

#ifndef FOTA_HTTP_SERVER_PORT
/** Default HTTP port */
#define FOTA_HTTP_SERVER_PORT       8000
#endif /* FOTA_HTTP_SERVER_PORT */

/** As specified in RFC 1035 Domain Implementation and Specification
  * from November 1987, domain names are 255 octets or less */
#define FOTA_MAX_DOMAIN_NAME_SIZE   255U

/** FOTA time to wait for user acknowledgment of FOTA completion before reboot/update applies */
#define FOTA_ACK_TIME               portMAX_DELAY

/** FOTA HTTP request timeout value in milliseconds.
  * When timeout is reached the HTTP request will be considered as a failure.
  */
#define FOTA_HTTP_TIMEOUT_IN_MS     60000U

/** Set the priority of the FOTA task using FreeRTOS priority evaluation system */
#define FOTA_TASK_PRIORITY          24

/** Size of the buffer used to transfer the FWU header to the ST67 in one shot (required by ST67) */
#define FWU_HEADER_SIZE             512

/** Multiple of data length that should be written in ST67, recommendation to ensure correct write into ST67 memory */
#define FWU_SECTOR_ALIGNMENT        256

/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private typedef -----------------------------------------------------------*/
/** @brief  Structure containing parameters for the FOTA */
typedef struct
{
  /** String that contains a domain name or a IPv4 address */
  char server_name[FOTA_MAX_DOMAIN_NAME_SIZE];
  /** Server port of the server */
  uint16_t server_port;
  /** URI (Uniform Resource Identifier) of the resource to fetch : */
  /**  - FOTA ST67 binary location on the server */
  uint8_t uri_st67[FOTA_URI_MAX_SIZE + 1];

  /* USER CODE BEGIN fota_struct_1 */
  /**  - FOTA JSON header descriptor location */
  uint8_t uri_header[FOTA_URI_MAX_SIZE + 1];
  /**  - FOTA STM32 binary location */
  uint8_t uri_stm32[FOTA_URI_MAX_SIZE + 1];

  /* USER CODE END fota_struct_1 */

} FOTAUpdateParams_t;

/** @brief  FOTA State Enumeration Definition. */
typedef enum
{
  FOTA_STATE_RESET   = 0x00U, /*!< FOTA not yet initialized or disabled */
  FOTA_STATE_READY   = 0x01U, /*!< FOTA initialized and ready for use   */
  FOTA_STATE_BUSY    = 0x02U, /*!< FOTA process is ongoing              */
  FOTA_STATE_ERROR   = 0x03U  /*!< FOTA error state                     */
} FOTA_StateTypeDef;

/** @brief  Structure used for the HTTP download containing information to help with the ST67 binary transfer */
typedef struct
{
  /** Buffer to accumulate data, needs to be allocate dynamically before use */
  uint8_t *fwu_buffer;
  /** Current data length of the fwu_buffer */
  size_t fwu_buffer_len;
  /** Size of the ST67 binary to receive */
  size_t fwu_total_to_receive;
  /** Data length already accumulated */
  size_t fwu_data_accumulated;
  /** Tells if the ST67 binary header already has been transferred */
  bool header_transferred;
  /** Return the code status of the HTTP data receive callback.
    * If 0 it means FOTA transfer operations in HTTP receive callback finished with success
    * else -1 for error
    * @note that current implementation doesn't stop HTTP download on error cached in the callback */
  int32_t http_xfer_error_code;
} FOTA_HttpXferTypeDef;

/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private macros ------------------------------------------------------------*/
#ifndef HAL_SYS_RESET
/** HAL System software reset function */
extern void HAL_NVIC_SystemReset(void);
/** HAL System software reset macro */
#define HAL_SYS_RESET() do{ HAL_NVIC_SystemReset(); } while(0);
#endif /* HAL_SYS_RESET */

/* USER CODE BEGIN PM */
/** Stringify version */
#define XSTR(x) #x

/** Macro to stringify version */
#define MSTR(x) XSTR(x)

/** Application version */
#define HOST_APP_VERSION_STR      \
  MSTR(HOST_APP_VERSION_MAIN) "." \
  MSTR(HOST_APP_VERSION_SUB1) "." \
  MSTR(HOST_APP_VERSION_SUB2)

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/** FOTA HTTP transfer context arguments */
static FOTA_HttpXferTypeDef fota_args;

/** FOTA HTTP connection settings */
static W6X_HTTP_connection_t fota_settings;

/** HTTP request method to use */
const static char fota_get_method[] = "GET";

/** FOTA state variable */
static FOTA_StateTypeDef fota_state = FOTA_STATE_RESET;

/** FOTA parameters shared between task and shell cmd */
static FOTAUpdateParams_t fota_params;

/** FOTA Event group handle, different event bit are defined in the fota.h file */
static EventGroupHandle_t fota_event_group_handle = NULL;

/** FOTA Event group handling interaction with tasks different from FOTA,
  * different event bit are defined in the fota.h file */
static EventGroupHandle_t fota_app_event_group_handle = NULL;

/** FOTA timer handler */
static TimerHandle_t fota_timer = NULL;

/** FOTA task handler */
static TaskHandle_t fota_task = NULL;

/** FOTA callback for operations to do after successful completion */
static FOTA_SuccessfulCompletionCallback_t fota_success_cb = NULL;

/** FOTA callback for operations to do after error on completion */
static FOTA_ErrorOnCompletionCallback_t fota_error_cb = NULL;

/** Index to notify FOTA when HTTP request is finished (regardless of error code) */
const UBaseType_t fota_notify_index = 1;

/* USER CODE BEGIN PV */
/** Host application version structure */
const W6X_Version_t host_version =
{
  .Major = HOST_APP_VERSION_MAIN,
  .Sub1 = HOST_APP_VERSION_SUB1,
  .Sub2 = HOST_APP_VERSION_SUB2
};

/** SHA256 context from mbedtls code */
static mbedtls_sha256_context_t ctx;

/** SHA256 output variable */
static uint8_t sha256[32] = {0};

/** FOTA header structure */
static FotaHeader_t header = {0};

/** Initialize cJSON memory allocator */
static cJSON_Hooks json_hooks =
{
  .malloc_fn = pvPortMalloc,
  .free_fn = vPortFree
};

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/**
  * @brief  Start the ST67 FOTA, download the ST67 binary and transfers the binary to the ST67
  * @param  http_server_addr: Address of the HTTP server were the ST67 binary is located
  * @param  http_server_port: Port of the HTTP server were the ST67 binary is located
  * @param  uri: URI location of the ST67 binary
  * @return int32_t Return FOTA_SUCCESS if success, FOTA_ERR otherwise
  * */
static int32_t Fota_st67FotaTransfer(char *http_server_addr, uint16_t http_server_port, const uint8_t *uri);

/**
  * @brief  FOTA update task to launch the Firmware update Over The Air procedure
  * @param  pvParameters: Task parameters
  */
static void Fota_FotaTask(void *pvParameters);

/**
  * @brief  Callback function for the FOTA timer, the timer will call this function after the delay expires
  * @param  xTimer Timer handler
  */
static void Fota_TimerCallback(TimerHandle_t xTimer);

/**
  * @brief  FOTA shell function
  * @param  argc: number of arguments
  * @param  argv: pointer to the arguments
  * @retval ::SHELL_STATUS_OK on success
  * @retval ::SHELL_STATUS_UNKNOWN_ARGS if wrong arguments
  * @retval ::SHELL_STATUS_ERROR otherwise
  */
int32_t fota_over_http_shell_trigger(int32_t argc, char **argv);

/**
  * @brief  HTTP callback called on server response to a HTTP request.
  * @param  arg Argument passed to the callback function
  * @param  httpc_result HTTP status code returned by the server
  * @param  rx_content_len Length of the content to be received from the server
  * @param  srv_res Today this param is always equal to 0, this is set by the HTTP client function
  * @param  err Today this param is always equal to 0, this is set by the HTTP client function
  */
static void Fota_HttpResponseCb(void *arg, W6X_HTTP_Status_Code_e httpc_result, uint32_t rx_content_len,
                                uint32_t srv_res, int32_t err);

/**
  * @brief  HTTP callback called on each data reception.
  * @param  arg Argument passed to the callback function
  * @param  p Structure containing the received data and its length
  * @param  err Today this param is always equal to 0, this is set by the HTTP client function
  * @return int32_t 0 in case of success, -1 otherwise
  */
static int32_t Fota_HttpRecvCb(void *arg, W6X_HTTP_buffer_t *p, int32_t err);

/* USER CODE BEGIN PFP */
/**
  * @brief  Download the STM32 binary and write it to the STM32 flash memory
  * @param  http_server_addr: Address of the HTTP server were the STM32 binary is located
  * @param  http_server_port: Port of the HTTP server were the STM32 binary is located
  * @param  uri: URI location of the STM32 binary
  * @param  integrity_output: Pointer to the buffer that will receive the SHA256 hash of the downloaded STM32 binary
  * @return int32_t Return FOTA_SUCCESS if success, FOTA_ERR otherwise
  */
static int32_t Fota_Stm32Update(char *http_server_addr, uint16_t http_server_port, const uint8_t *uri,
                                uint8_t *integrity_output);

/**
  * @brief  Download the FOTA header and parse it to a structure
  * @param  http_server_addr: Address of the HTTP server were the FOTA header binary is located
  * @param  http_server_port: Port of the HTTP server were the FOTA header is located
  * @param  uri: URI location of the FOTA header binary
  * @param  header Structure to be filled with the header information
  * @return int32_t Return FOTA_SUCCESS if success, FOTA_ERR otherwise
  */
static int32_t Fota_DownloadHeader(char *http_server_addr, uint16_t http_server_port, const uint8_t *uri,
                                   FotaHeader_t *header);

/**
  * @brief  HTTP callback called on each data reception.
  * @param  arg Argument passed to the callback function
  * @param  p Structure containing the received data and its length
  * @param  err Today this param is always equal to 0, this is set by the HTTP client function
  * @return int32_t 0 in case of success, -1 otherwise
  */
static int32_t Fota_HttpStm32RecvCb(void *arg, W6X_HTTP_buffer_t *p, int32_t err);

/**
  * @brief  HTTP callback called on server response to a HTTP request.
  * @param  arg Argument passed to the callback function
  * @param  httpc_result HTTP status code returned by the server
  * @param  rx_content_len Length of the content to be received from the server
  * @param  srv_res Today this param is always equal to 0, this is set by the HTTP client function
  * @param  err Today this param is always equal to 0, this is set by the HTTP client function
  */
static void Fota_HttpStm32ResponseCb(void *arg, W6X_HTTP_Status_Code_e httpc_result, uint32_t rx_content_len,
                                     uint32_t srv_res, int32_t err);

/**
  * @brief  HTTP callback called on each data reception.
  * @param  arg Argument passed to the callback function
  * @param  p Structure containing the received data and its length
  * @param  err Today this param is always equal to 0, this is set by the HTTP client function
  * @return int32_t 0 in case of success, -1 otherwise
  */
static int32_t Fota_HttpHeaderRecvCb(void *arg, W6X_HTTP_buffer_t *p, int32_t err);

/* USER CODE END PFP */

/* Functions Definition ------------------------------------------------------*/
EventGroupHandle_t Fota_AppGetEventGroupHandle(void)
{
  return fota_app_event_group_handle;
}

EventGroupHandle_t Fota_GetEventGroupHandle(void)
{
  return fota_event_group_handle;
}

void Fota_RegisterCallbacks(FOTA_SuccessfulCompletionCallback_t success_cb, FOTA_ErrorOnCompletionCallback_t error_cb)
{
  fota_success_cb = success_cb;
  fota_error_cb = error_cb;
}

void Fota_DeinitCallbacks(void)
{
  fota_success_cb = NULL;
  fota_error_cb = NULL;
}

int32_t Fota_StartFotaTask(void)
{
  BaseType_t xReturned;

  /* Create the event groups */
  fota_event_group_handle = xEventGroupCreate();

  fota_app_event_group_handle = xEventGroupCreate();

  /* Create the task */
  xReturned = xTaskCreate(Fota_FotaTask, (char *)"FOTA task", FOTA_TASK_STACK_SIZE >> 2,
                          NULL, FOTA_TASK_PRIORITY, &fota_task);

  if (xReturned != pdPASS)
  {
    LogError("Error: xTaskCreate failed to create FOTA task\n");
    return FOTA_ERR;
  }
  fota_state = FOTA_STATE_READY;

  return FOTA_SUCCESS;
}

int32_t Fota_DeleteFotaTask(void)
{
  /* Delete the timer used for FOTA */
  if (fota_timer != NULL)
  {
    if (xTimerDelete(fota_timer, 0) != pdPASS)
    {
      return FOTA_ERR;
    }
  }

  /* Delete the event groups used for FOTA */
  if (fota_event_group_handle != NULL)
  {
    vEventGroupDelete(fota_event_group_handle);
  }

  if (fota_app_event_group_handle != NULL)
  {
    vEventGroupDelete(fota_app_event_group_handle);
  }

  /* Delete the FOTA task */
  if (fota_task != NULL)
  {
    vTaskDelete(fota_task);
  }

  Fota_DeinitCallbacks();

  fota_state = FOTA_STATE_RESET;

  return FOTA_SUCCESS;
}

int32_t Fota_SetFotaTimer(uint32_t delay_ms)
{
  /* Create the timer if it doesn't exist already, else change the existing one settings */
  if (fota_timer == NULL)
  {
    fota_timer = xTimerCreate("FOTATimer", pdMS_TO_TICKS(delay_ms), pdTRUE, (void *)0, Fota_TimerCallback);
    if (fota_timer == NULL)
    {
      LogError("Failed to create FOTA timer\n");
      return FOTA_ERR;
    }
  }
  else
  {
    if (xTimerChangePeriod(fota_timer, pdMS_TO_TICKS(delay_ms), 0) != pdPASS)
    {
      LogError("Failed to change FOTA timer period\n");
      return FOTA_ERR;
    }
  }
  return FOTA_SUCCESS;
}

int32_t Fota_StopFotaTimer(void)
{
  /* Stop the timer if the timer exist */
  if (fota_timer != NULL)
  {
    if (xTimerStop(fota_timer, 0) != pdPASS)
    {
      LogError("Failed to stop FOTA timer\n");
      return FOTA_ERR;
    }
  }
  return FOTA_SUCCESS;
}

int32_t Fota_StartFotaTimer(void)
{

  /* Start the timer if the timer exist */
  if (fota_timer != NULL)
  {
    if (xTimerStart(fota_timer, 0) != pdPASS)
    {
      LogError("Failed to start FOTA timer\n");
      return FOTA_ERR;
    }
    return FOTA_SUCCESS;
  }
  return FOTA_ERR;
}

int32_t Fota_ResetFotaTimer(void)
{
  /* Reset the timer if the timer exist */
  if (fota_timer != NULL)
  {
    if (xTimerReset(fota_timer, 0) != pdPASS)
    {
      LogError("Failed to reset FOTA timer\n");
      return FOTA_ERR;
    }
  }
  return FOTA_SUCCESS;
}

void Fota_TriggerFotaUpdate(void)
{
  /* Set the FOTA_UPDATE_BIT to trigger the FOTA update event */
  (void)xEventGroupSetBits(fota_event_group_handle, FOTA_UPDATE_BIT);
}

/* USER CODE BEGIN FD */

/* USER CODE END FD */

/* Private Functions Definition ----------------------------------------------*/
static void Fota_FotaTask(void *pvParameters)
{
  /* Store error code from W6x */
  int32_t ret;
  /* Store error code from FOTA app */
  int32_t ret_fota;
  /* Event group bit masking result */
  EventBits_t uxBits;
  /* Boolean used to skip or not the ST67 update */
  bool skip_st67_update = false;
  /* Boolean used to evaluate if a fota is requested */
  bool fota_requested = false;

  /* USER CODE BEGIN fota_task_1 */
  /* Store error code from FOTA header functions */
  int32_t ret_header;
  /* W6x module information structure */
  W6X_ModuleInfo_t *st67_info;
  /* Boolean used to skip or not the STM32 update */
  bool skip_stm32_update = false;

  /* USER CODE END fota_task_1 */

  fota_params.server_port = FOTA_HTTP_SERVER_PORT;

  /* Copy default value to the shared FOTA parameters structure */
  (void)memcpy(fota_params.server_name, FOTA_HTTP_SERVER_ADDR, sizeof(FOTA_HTTP_SERVER_ADDR));

  (void)memcpy(fota_params.uri_st67, FOTA_HTTP_URI, sizeof(FOTA_HTTP_URI));

  /* USER CODE BEGIN fota_task_2 */
  (void)memcpy(fota_params.uri_header, FOTA_HTTP_URI_HEADER, sizeof(FOTA_HTTP_URI_HEADER));

  (void)memcpy(fota_params.uri_stm32, FOTA_HTTP_URI_STM32, sizeof(FOTA_HTTP_URI_STM32));

  /* Get W6x module information */
  st67_info = W6X_GetModuleInfo();

  /* Set the memory hooks for cJSON usage */
  cJSON_InitHooks(&json_hooks);

  /* Check if the STM32 FLASH bank are swapped or not */
  bool swap_status = false;
  (void)fota_flash_IsFlashBankSwapped(&swap_status);
  if (swap_status)
  {
    LogInfo("Application started from bank 2\n");
  }
  else
  {
    LogInfo("Application started from bank 1\n");
  }

  /* USER CODE END fota_task_2 */

  for (;;)
  {
    /* Wait for the FOTA bit to be set */
    uxBits = xEventGroupWaitBits(fota_event_group_handle, FOTA_UPDATE_BIT | FOTA_WAIT_USER_ACK_BIT | FOTA_ERROR_BIT,
                                 pdTRUE, pdFALSE, FOTA_ACK_TIME);

    if ((uxBits & FOTA_ERROR_BIT) != 0)
    {
      /* In the case that an ERROR bit is present,
         we clear it and inform that previous none cleared error was present.
         Only configure the timer when it has previously been set */
      if (fota_timer != NULL)
      {
        Fota_SetFotaTimer(FOTA_TIMEOUT);
      }

      LogError("Error bit was raised during the FOTA update,\n"
               "If FOTA timer has been configured, it will be reset.\n"
               "FOTA can be re-attempted by triggering an event again\n");
      (void)Fota_ResetFotaTimer();
      (void)Fota_StartFotaTimer();
    }
    else if ((uxBits & FOTA_UPDATE_BIT) != 0) /* FOTA update is requested */
    {
      (void)Fota_StopFotaTimer();

      fota_requested = false;
      skip_st67_update = false;

      /* USER CODE BEGIN fota_task_3 */
      skip_stm32_update = false;

      /* Download and parse the FOTA header */
      ret_fota = Fota_DownloadHeader(fota_params.server_name, fota_params.server_port,
                                     fota_params.uri_header, &header);
      if (ret_fota != FOTA_SUCCESS)
      {
        goto _jump1;
      }
      /* Print the FOTA header content */
      fota_header_Print(&header);

      /* Compare current SDK version of ST67 with the one in the FOTA header downloaded */
      ret_header = fota_header_CmpVer(st67_info->SDK_Version, header.st67_ver);
      if ((ret_header == FOTA_HEADER_VER_OLDER) || (ret_header == FOTA_HEADER_VER_EQ))
      {
        /* If versions are equal or the one in the header is older we can skip ST67 update */
        LogInfo("ST67 binary version is older or the same as the current one\n");
        ret_fota = FOTA_SUCCESS;
        skip_st67_update = true;
      }
      else if (ret_header != FOTA_HEADER_VER_NEWER)
      {
        /* If the version of the header is not newer it means we can't ensure proper update of ST67 version */
        LogInfo("ST67 binary version is newer than the current one\n");
        ret_fota = FOTA_ERR;
        goto _jump1;
      }

      /* USER CODE END fota_task_3 */

      if (skip_st67_update == false)
      {
        fota_requested = true;
        /* Download and transfer the ST67 binary to the ST67 */
        ret_fota = Fota_st67FotaTransfer(fota_params.server_name, fota_params.server_port, fota_params.uri_st67);
      }

      /* USER CODE BEGIN fota_task_4 */
      if (ret_fota == FOTA_SUCCESS)
      {
        /* Compare current STM32 version with the one in the FOTA header downloaded */
        ret_header = fota_header_CmpVer(host_version, header.stm32_app_ver);
        if ((ret_header == FOTA_HEADER_VER_OLDER) || (ret_header == FOTA_HEADER_VER_EQ))
        {
          /* If versions are equal or the one in the header is older we can skip STM32 update */
          LogInfo("STM32 binary version is older or the same as the current one\n");
          ret_fota = FOTA_SUCCESS;
          skip_stm32_update = true;
        }
        else if (ret_header != FOTA_HEADER_VER_NEWER)
        {
          /* If the version of the header is not newer it means we can't ensure proper update of STM32 version */
          LogInfo("STM32 binary version is newer than the current one\n");
          ret_fota = FOTA_ERR;
          goto _jump1;
        }

        if (skip_stm32_update == false)
        {
          fota_requested = true;
          /* Download the STM32 binary, write the STM32 bin to FLASH and provide a SHA256 of the STM32 binary */
          ret_fota = Fota_Stm32Update(fota_params.server_name, fota_params.server_port,
                                      fota_params.uri_stm32, sha256);
          if (ret_fota != FOTA_SUCCESS)
          {
            goto _jump1;
          }

          /* Check the integrity of the STM32 binary by comparing the SHA256 hash with the one in the FOTA header */
          if (memcmp(sha256, header.file_hash, sizeof(header.file_hash)))
          {
            LogError("STM32 binary integrity check is not correct\n");
            ret_fota = FOTA_ERR;
            goto _jump1;
          }
          else
          {
            LogInfo("STM32 binary integrity check passes\n");
          }
        }
      }

_jump1:

      /* USER CODE END fota_task_4 */

      if (ret_fota != FOTA_SUCCESS)
      {
        LogError("Failed to FOTA update (error: %" PRIi32 ")\n", ret_fota);
        /* Notifying that an error occurred */
        (void)xEventGroupSetBits(fota_event_group_handle, FOTA_ERROR_BIT);
        (void)xEventGroupSetBits(fota_app_event_group_handle, FOTA_ERROR_BIT);
      }
      else if (fota_requested)
      {
        /* FOTA transfer was completed successfully */
        LogInfo("FOTA transfer done\n");
        (void)xEventGroupSetBits(fota_app_event_group_handle, FOTA_COMPLETE_XFER_USER_NOTIF_BIT);
        LogInfo("FOTA task waiting for application acknowledgment\n");
      }
      else
      {
        LogInfo("Because no newer firmware was detected,"
                "no update was downloaded or installed\n");
        (void)xEventGroupSetBits(fota_event_group_handle, FOTA_WAIT_USER_ACK_BIT);
        fota_state = FOTA_STATE_READY;
        (void)Fota_ResetFotaTimer();
        (void)Fota_StartFotaTimer();
      }
    }
    /* FOTA update is finished, host can now reboot */
    else if ((uxBits & FOTA_WAIT_USER_ACK_BIT) != 0)
    {
      ret = W6X_STATUS_OK;
      LogInfo("FOTA task done\n");

      if (skip_st67_update == false)
      {
        LogInfo("Apply ST67 new binary...\n");

        /* Finish FWU on NCP side */
        ret = W6X_FWU_Finish();

        if (ret == W6X_STATUS_OK)
        {
          LogInfo("ST67 binary installation completed\n");
        }
        else
        {
          LogError("ST67 binary installation failed\n");
        }
      }

      /* USER CODE BEGIN fota_task_5 */
      if (skip_stm32_update == false)
      {
        vTaskDelay(pdMS_TO_TICKS(1000)); /* Wait for the last trace to be printed */

        if (ret == W6X_STATUS_OK)
        {
          if (fota_flash_SwapFlashBanks() != 0)
          {
            ret = W6X_STATUS_ERROR;
          }
          else
          {
            LogInfo("Rebooting....\n");
            /* Perform a system reset */
            HAL_SYS_RESET();
            /* Because of NVIC reset, any code beyond this point won't be executed */
          }
        }
      }

      /* USER CODE END fota_task_5 */

      if (ret != W6X_STATUS_OK)
      {
        LogError("Failed to finish FOTA, %" PRIi32 "\n", ret);
        /* Notifying that an error occurred */
        (void)xEventGroupSetBits(fota_event_group_handle, FOTA_ERROR_BIT);
        (void)xEventGroupSetBits(fota_app_event_group_handle, FOTA_ERROR_BIT);
      }
      else
      {
        (void)xEventGroupSetBits(fota_app_event_group_handle, FOTA_COMPLETE_UPDATE_USER_NOTIF_BIT);
      }
    }
  }
}

static int32_t Fota_st67FotaTransfer(char *http_server_addr, uint16_t http_server_port, const uint8_t *uri)
{
  int32_t ret = FOTA_ERR;
  int32_t ret_w6x;
  uint32_t http_task_notification_value;
  ip_addr_t addr = {0};
  int8_t is_ip = 0;

  memset(&fota_args, 0, sizeof(fota_args));
  memset(&fota_settings, 0, sizeof(fota_settings));

  fota_args.fwu_buffer = NULL;
  fota_args.fwu_buffer_len = 0;
  fota_args.fwu_data_accumulated = 0;
  fota_args.fwu_total_to_receive = 0;
  fota_args.header_transferred = false;
  fota_args.http_xfer_error_code = -1;

  /* Sets the callback and it's argument to trigger on received data */
  fota_settings.recv_fn = Fota_HttpRecvCb;
  fota_settings.recv_fn_arg = &fota_args;
  fota_settings.result_fn = Fota_HttpResponseCb;
  fota_settings.callback_arg = &fota_args;

  /* Check if http_server_addr is an IPv4 address or not. If it is IPv4,
     it will also give an uint32_t representation of this IP stored in numeric_ipv4 */
  is_ip = W6X_Net_Inet_pton(AF_INET, http_server_addr, &addr.u_addr.ip4.addr);
  if (is_ip != 1)
  {
    /* Store the resolved IP address in a temporary variable (the variable is only used for the if scope) */
    uint8_t server_ip_addr[4];
    /* Resolve IP Address from the input URL */
    if (W6X_Net_ResolveHostAddress(http_server_addr, server_ip_addr) != W6X_STATUS_OK)
    {
      LogError("IP Address identification failed\n");
      goto _err1;
    }
    else
    {
      LogDebug("IP Address from Hostname [%s]: " IPSTR "\n", http_server_addr, IP2STR(server_ip_addr));
    }
    /* Store the address in a uint32_t representation of the IP address */
    addr.u_addr.ip4.addr = ATON(server_ip_addr);
  }

  fota_settings.server_name = http_server_addr;

  fota_args.fwu_buffer = pvPortMalloc(FWU_HEADER_SIZE);
  if (fota_args.fwu_buffer == NULL)
  {
    LogError("Failed to allocate buffer for FWU header transmission\n");
    goto _err1;
  }

  /* Terminate FWU transmission on NCP side to ensure clear state */
  ret_w6x = W6X_FWU_Starts(0);
  if (ret_w6x != W6X_STATUS_OK)
  {
    LogError("Failed to terminate the NCP FWU transmission, %" PRIi32 "\n", ret_w6x);
    goto _err1;
  }

  /* Starts FWU on NCP side */
  ret_w6x = W6X_FWU_Starts(1);
  if (ret_w6x != W6X_STATUS_OK)
  {
    LogError("Failed to start the NCP FWU ,  %" PRIi32 "\n", ret_w6x);
    goto _err1;
  }

  LogDebug("FOTA update started: server=%s, port=%" PRIu32 ", uri=%s\n", http_server_addr,
           http_server_port, uri);

  /* Send the HTTP request. Non-blocking function. The response will be received by the callback */
  ret_w6x = W6X_HTTP_Client_Request(&addr, http_server_port, (const char *)uri, fota_get_method,
                                    NULL, 0, NULL, NULL, NULL, NULL, &fota_settings);
  if (ret_w6x != W6X_STATUS_OK)
  {
    LogError("Failed to download and transfer binary to the ST67 , %" PRIi32 "\n", ret_w6x);
    goto _err1;
  }
  /* We wait until the HTTP download has been done */
  http_task_notification_value = ulTaskNotifyTakeIndexed(fota_notify_index, pdTRUE,
                                                         pdMS_TO_TICKS(FOTA_HTTP_TIMEOUT_IN_MS));

  if ((fota_args.http_xfer_error_code != 0) || (fota_notify_index != http_task_notification_value))
  {
    LogError("Failed to receive all the data from the server either because of a timeout or caught error\n");
    goto _err1;
  }

  ret = FOTA_SUCCESS;
_err1:
  if (fota_args.fwu_buffer != NULL)
  {
    vPortFree(fota_args.fwu_buffer);
  }

  return ret;
}

/* USER CODE BEGIN fota_fd_1 */
static int32_t Fota_DownloadHeader(char *http_server_addr,
                                   uint16_t http_server_port, const uint8_t *uri,
                                   FotaHeader_t *header)
{
  int32_t ret = FOTA_ERR;
  int32_t ret_w6x;
  uint32_t http_task_notification_value;
  ip_addr_t addr = {0};
  int8_t is_ip = 0;

  memset(&fota_args, 0, sizeof(fota_args));
  memset(&fota_settings, 0, sizeof(fota_settings));

  fota_args.fwu_buffer = NULL;
  fota_args.fwu_buffer_len = 0;
  fota_args.fwu_data_accumulated = 0;
  fota_args.fwu_total_to_receive = 0;
  fota_args.header_transferred = false;
  fota_args.http_xfer_error_code = -1;

  /* Sets the callback and it's argument to trigger on received data */
  fota_settings.recv_fn = Fota_HttpHeaderRecvCb;
  fota_settings.recv_fn_arg = &fota_args;
  fota_settings.result_fn = Fota_HttpResponseCb;
  fota_settings.callback_arg = &fota_args;

  /* Check if http_server_addr is an IPv4 address or not. If it is IPv4,
     it will also give an uint32_t representation of this IP stored in numeric_ipv4 */
  is_ip = W6X_Net_Inet_pton(AF_INET, http_server_addr, &addr.u_addr.ip4.addr);
  if (is_ip != 1)
  {
    /* Store the resolved IP address in a temporary variable (the variable is only used for the if scope) */
    uint8_t server_ip_addr[4];

    /* Resolve IP Address from the input URL */
    if (W6X_Net_ResolveHostAddress(http_server_addr, server_ip_addr) != W6X_STATUS_OK)
    {
      LogError("IP Address identification failed\n");
      goto _err2;
    }
    else
    {
      LogDebug("IP Address from Hostname [%s]: " IPSTR "\n", http_server_addr, IP2STR(server_ip_addr));
    }
    /* Store the address in a uint32_t representation of the IP address */
    addr.u_addr.ip4.addr = ATON(server_ip_addr);
  }

  fota_settings.server_name = http_server_addr;

  fota_args.fwu_buffer = pvPortMalloc(FWU_HEADER_SIZE);
  if (fota_args.fwu_buffer == NULL)
  {
    LogError("Failed to allocate buffer for FOTA the header\n");
    goto _err2;
  }

  LogDebug("FOTA update started: server=%s, port=%" PRIu32 ", uri=%s\n", http_server_addr,
           http_server_port, uri);

  /* Send the HTTP request. Non-blocking function. The response will be received by the callback */
  ret_w6x = W6X_HTTP_Client_Request(&addr, http_server_port, (const char *)uri, fota_get_method,
                                    NULL, 0, NULL, NULL, NULL, NULL, &fota_settings);
  if (ret_w6x != W6X_STATUS_OK)
  {
    LogError("Failed to download and transfer binary to the ST67 , %" PRIi32 "\n", ret_w6x);
    goto _err2;
  }
  /* We wait until the HTTP download has been done */
  http_task_notification_value = ulTaskNotifyTakeIndexed(fota_notify_index, pdTRUE,
                                                         pdMS_TO_TICKS(FOTA_HTTP_TIMEOUT_IN_MS));

  if ((fota_args.http_xfer_error_code != 0) || (fota_notify_index != http_task_notification_value))
  {
    LogError("Failed to receive all the data from the server either because of a timeout or caught error\n");
    goto _err2;
  }

  /* Parse the received buffer into header structure,
     the buffer is assumed to contain JSON format data for the parsing */
  if (fota_header_ParseJsonToStruct(fota_args.fwu_buffer, fota_args.fwu_data_accumulated,
                                    header) != FOTA_HEADER_SUCCESS)
  {
    LogError("Failed to parse FOTA header\n");
    goto _err2;
  }

  ret = FOTA_SUCCESS;
_err2:
  if (fota_args.fwu_buffer != NULL)
  {
    vPortFree(fota_args.fwu_buffer);
  }

  return ret;
}

static int32_t Fota_Stm32Update(char *http_server_addr, uint16_t http_server_port, const uint8_t *uri,
                                uint8_t *integrity_output)
{
  int32_t ret = FOTA_ERR;
  int32_t ret_w6x;
  uint32_t http_task_notification_value;
  ip_addr_t addr = {0};
  int8_t is_ip = 0;

  memset(&fota_args, 0, sizeof(fota_args));
  memset(&fota_settings, 0, sizeof(fota_settings));

  fota_args.fwu_buffer = NULL;
  fota_args.fwu_buffer_len = 0;
  fota_args.fwu_data_accumulated = 0;
  fota_args.fwu_total_to_receive = 0;
  fota_args.header_transferred = false;
  fota_args.http_xfer_error_code = -1;

  /* Sets the callback and it's argument to trigger on received data */
  fota_settings.recv_fn = Fota_HttpStm32RecvCb;
  fota_settings.recv_fn_arg = &fota_args;
  fota_settings.result_fn = Fota_HttpStm32ResponseCb;
  fota_settings.callback_arg = &fota_args;

  /* Check if http_server_addr is an IPv4 address or not. If it is IPv4,
     it will also give an uint32_t representation of this IP stored in numeric_ipv4 */
  is_ip = W6X_Net_Inet_pton(AF_INET, http_server_addr, &addr.u_addr.ip4.addr);
  if (is_ip != 1)
  {
    /* Store the resolved IP address in a temporary variable (the variable is only used for the if scope) */
    uint8_t server_ip_addr[4];
    /* Resolve IP Address from the input URL */
    if (W6X_Net_ResolveHostAddress(http_server_addr, server_ip_addr) != W6X_STATUS_OK)
    {
      LogError("IP Address identification failed\n");
      goto _err3;
    }
    else
    {
      LogDebug("IP Address from Hostname [%s]: " IPSTR "\n", http_server_addr, IP2STR(server_ip_addr));
    }
    /* Store the address in a uint32_t representation of the IP address */
    addr.u_addr.ip4.addr = ATON(server_ip_addr);
  }

  fota_settings.server_name = http_server_addr;

  fota_args.fwu_buffer = pvPortMalloc(4096);
  if (fota_args.fwu_buffer == NULL)
  {
    LogError("Failed to allocate buffer for FWU header transmission\n");
    goto _err3;
  }

  /* Back 2 mass erase */
  if (fota_flash_MassEraseFlashBank() != 0)
  {
    LogError("Flash bank mass erase failed\n");
    goto _err3;
  }
  LogDebug("Flash bank mass erase done\n");

  LogDebug("FOTA update started: server=%s, port=%" PRIu32 ", uri=%s\n", http_server_addr,
           http_server_port, uri);

  /* Initialize SHA256 context */
  mbedtls_sha256_init(&ctx);
  /* Start SHA256 calculation */
  mbedtls_sha256_starts(&ctx);

  /* Send the HTTP request. Non-blocking function. The response will be received by the callback */
  ret_w6x = W6X_HTTP_Client_Request(&addr, http_server_port, (const char *)uri, fota_get_method,
                                    NULL, 0, NULL, NULL, NULL, NULL, &fota_settings);
  if (ret_w6x != W6X_STATUS_OK)
  {
    LogError("Failed to download and transfer binary to the ST67 , %" PRIi32 "\n", ret_w6x);
    goto _err3;
  }

  /* We wait until the HTTP download has been done */
  http_task_notification_value = ulTaskNotifyTakeIndexed(fota_notify_index, pdTRUE,
                                                         pdMS_TO_TICKS(FOTA_HTTP_TIMEOUT_IN_MS));

  if ((fota_args.http_xfer_error_code != 0) || (fota_notify_index != http_task_notification_value))
  {
    LogError("Failed to receive all the data from the server either because of a timeout or caught error\n");
    goto _err3;
  }
  /* Finish SHA256 calculation and give the output result */
  mbedtls_sha256_finish(&ctx, integrity_output);

  ret = FOTA_SUCCESS;
_err3:
  if (fota_args.fwu_buffer != NULL)
  {
    vPortFree(fota_args.fwu_buffer);
  }

  return ret;
}

/* USER CODE END fota_fd_1 */

static void Fota_TimerCallback(TimerHandle_t xTimer)
{
  /* Call the function that triggers the FOTA update using the according event group */
  (void)Fota_TriggerFotaUpdate();
}

int32_t Fota_WaitForFOTACompletion(void)
{
  EventBits_t uxBits;
  if (fota_state == FOTA_STATE_BUSY)
  {
    return FOTA_BUSY;
  }

  fota_state = FOTA_STATE_BUSY;

  uxBits = xEventGroupWaitBits(fota_app_event_group_handle, FOTA_COMPLETE_XFER_USER_NOTIF_BIT | FOTA_ERROR_BIT,
                               pdTRUE, pdFALSE, FOTA_ACK_TIME);
  if ((uxBits & FOTA_COMPLETE_XFER_USER_NOTIF_BIT) != 0)
  {
    if (fota_success_cb != NULL)
    {
      fota_success_cb();
    }

    /* User tells that everything is done on it's side and that FOTA task can now proceed
       with the next steps (booting on the new software) */
    (void)xEventGroupSetBits(fota_event_group_handle, FOTA_WAIT_USER_ACK_BIT);
  }
  else
  {
    if (fota_error_cb != NULL)
    {
      fota_error_cb();
    }

    fota_state = FOTA_STATE_READY;
    return FOTA_ERR;
  }

  uxBits = xEventGroupWaitBits(fota_app_event_group_handle, FOTA_COMPLETE_UPDATE_USER_NOTIF_BIT | FOTA_ERROR_BIT,
                               pdTRUE, pdFALSE, FOTA_ACK_TIME);
  if ((uxBits & FOTA_COMPLETE_UPDATE_USER_NOTIF_BIT) != 0)
  {
    fota_state = FOTA_STATE_READY;
    return FOTA_SUCCESS;
  }
  else
  {
    fota_state = FOTA_STATE_READY;
    return FOTA_ERR;
  }
}

#if (SHELL_ENABLE == 1)
int32_t fota_over_http_shell_trigger(int32_t argc, char **argv)
{
  int32_t ret = 0;
  int32_t tmp = 0;

  if (fota_task == NULL)
  {
    SHELL_E("FOTA module is not started\n");
    return SHELL_STATUS_ERROR;
  }

  if (argc < 4)
  {
    return SHELL_STATUS_UNKNOWN_ARGS;
  }

  SHELL_CMD("\n\n***************FOTA TEST ***************\n");

  SHELL_CMD("Init FOTA callbacks for SHELL usage\n");

  /* Copy shell passed elements from argv to global struct fota_params */
  (void)strncpy((char *)fota_params.server_name, argv[1], FOTA_MAX_DOMAIN_NAME_SIZE - 1);
  fota_params.server_name[FOTA_MAX_DOMAIN_NAME_SIZE - 1] = '\0'; /* Ensure null-termination */

  /* Check if the server port is a valid number */
  if (Parser_StrToInt(argv[2], NULL, &tmp) == 0)
  {
    SHELL_E("Invalid server port\n");
    return SHELL_STATUS_UNKNOWN_ARGS;
  }
  fota_params.server_port = (uint16_t)tmp;

  (void)strncpy((char *)fota_params.uri_st67, argv[3], FOTA_URI_MAX_SIZE - 1);
  fota_params.uri_st67[FOTA_URI_MAX_SIZE - 1] = '\0'; /* Ensure null-termination */

  /* USER CODE BEGIN fota_shell_1 */
  if (argc < 6)
  {
    return SHELL_STATUS_UNKNOWN_ARGS;
  }

  (void)strncpy((char *)fota_params.uri_stm32, argv[4], FOTA_URI_MAX_SIZE - 1);
  fota_params.uri_stm32[FOTA_URI_MAX_SIZE - 1] = '\0'; /* Ensure null-termination */

  (void)strncpy((char *)fota_params.uri_header, argv[5], FOTA_URI_MAX_SIZE - 1);
  fota_params.uri_header[FOTA_URI_MAX_SIZE - 1] = '\0'; /* Ensure null-termination */

  /* USER CODE END fota_shell_1 */

  /* Set the FOTA_UPDATE_BIT to trigger the task */
  (void)xEventGroupSetBits(fota_event_group_handle, FOTA_UPDATE_BIT);

  ret = Fota_WaitForFOTACompletion();
  if (ret != FOTA_SUCCESS)
  {
    SHELL_E("***************FOTA SHELL ERROR *********\n\n");
    return SHELL_STATUS_ERROR;
  }

  SHELL_CMD("***************FOTA SHELL SUCCESS *******\n\n");
  return SHELL_STATUS_OK;
}

SHELL_CMD_EXPORT_ALIAS(fota_over_http_shell_trigger, fota_http,
                       fota_http < server IP > < server port > < ST67 resource URI >
                       [ STM32 resource URI ] [ FOTA header resource URI ]. Run firmware update over HTTP);

#endif /* SHELL_ENABLE */

static void Fota_HttpResponseCb(void *arg, W6X_HTTP_Status_Code_e httpc_result, uint32_t rx_content_len,
                                uint32_t srv_res, int32_t err)
{
  /* Save the length of the content we are about to received in the passed arg */
  FOTA_HttpXferTypeDef *args = arg;
  if (err != 0)
  {
    LogError("HTTP received error:%" PRIi32 "\n", err);
    args->http_xfer_error_code = -1;
    xTaskNotifyGiveIndexed(fota_task, fota_notify_index);
  }
  else
  {
    args->fwu_total_to_receive = rx_content_len;
    LogDebug("total len %" PRIu32 "\n", rx_content_len);
    if (httpc_result != OK)
    {
      args->http_xfer_error_code = -1;
      xTaskNotifyGiveIndexed(fota_task, fota_notify_index);
    }
  }
}

static int32_t Fota_HttpRecvCb(void *arg, W6X_HTTP_buffer_t *p, int32_t err)
{
  W6X_Status_t ret;
  FOTA_HttpXferTypeDef *args = arg;

  if (err != 0)
  {
    LogError("HTTP received error:%" PRIi32 "\n", err);
    args->http_xfer_error_code = -1;
    xTaskNotifyGiveIndexed(fota_task, fota_notify_index);
    return -1;
  }

  if ((p == NULL) || (p->length == 0) || (p->data == NULL) || (args->fwu_buffer == NULL))
  {
    LogError("Invalid HTTP buffer received or buffer in arg\n");
    args->http_xfer_error_code = -1;
    xTaskNotifyGiveIndexed(fota_task, fota_notify_index);
    return -1;
  }
  /* If the FWU head has not been send out to the ST67 */
  if (!(args->header_transferred))
  {
    /* If the data received is greater than the FWU header size, we know that the header is in the first 512 bytes
       and that we can send it to the ST67.*/
    if (args->fwu_buffer_len + p->length >= FWU_HEADER_SIZE)
    {
      /* Compute if there is remaining data to send to the ST67 */
      size_t remaining_length = FWU_HEADER_SIZE - args->fwu_buffer_len;
      /* Copy any remaining data to the FWU buffer and then send it */
      memcpy(args->fwu_buffer + args->fwu_buffer_len, p->data, remaining_length);
      ret = W6X_FWU_Send(args->fwu_buffer, FWU_HEADER_SIZE);
      if (ret != W6X_STATUS_OK)
      {
        LogError("Failed to send remaining data in buffer to W6x via FWU send, error code : %" PRIu32 "\n", ret);
        args->http_xfer_error_code = -1;
        xTaskNotifyGiveIndexed(fota_task, fota_notify_index);
        return -1;

      }
      /* A this point we know that the header has been send with success */
      args->header_transferred = 1;
      LogInfo("ST67 FWU header successfully transferred\n");
      args->fwu_buffer_len = 0;

      /* Send any remaining data to the ST67 */
      if (p->length > remaining_length)
      {
        ret = W6X_FWU_Send(p->data + remaining_length, p->length - remaining_length);
        if (ret != W6X_STATUS_OK)
        {
          LogError("Failed to send remaining data in buffer to W6x via FWU send, error code : %" PRIu32 "\n", ret);
          args->http_xfer_error_code = -1;
          xTaskNotifyGiveIndexed(fota_task, fota_notify_index);
          return -1;

        }
      }
    }
    else
    {
      /* Else we didn't receive all the FWU ST67 header so we store the data in a buffer */
      memcpy(args->fwu_buffer + args->fwu_buffer_len, p->data, p->length);
      args->fwu_buffer_len += p->length;
    }
  }
  else
  {
    /* Once the header ST67 has been received and send to the ST67, we can proceed to send data as-is */
    ret = W6X_FWU_Send(p->data, p->length);
    if (ret != W6X_STATUS_OK)
    {
      LogError("Failed to send buffer to W6x via FWU send, error code : %" PRIu32 "\n", ret);
      /* If the data length is not aligned with ST67 memory requirement,
         we log an info on the potential error source */
      if (!(p->length % FWU_SECTOR_ALIGNMENT))
      {
        LogError("Issue might be due to the received data length that is not inline with the FWU transfer buffer"
                 "(not aligned with recommended xfer size), %" PRIi32 " bytes\n",
                 p->length);
      }
      args->http_xfer_error_code = -1;
      xTaskNotifyGiveIndexed(fota_task, fota_notify_index);
      return -1;
    }
    LogDebug("FOTA data length %" PRIu32 " xfer to ST67\n", p->length);

  }
  /* Amount of data currently transferred */
  args->fwu_data_accumulated += p->length;
  /* If all data expected has been received, we can tell the FOTA task to proceed with execution flow */
  if (args->fwu_data_accumulated >= args->fwu_total_to_receive)
  {
    args->http_xfer_error_code = 0;
    LogInfo("FOTA data transfer to ST67 finished\n");
    xTaskNotifyGiveIndexed(fota_task, fota_notify_index);
  }

  return 0;
}

/* USER CODE BEGIN PFD */
static void Fota_HttpStm32ResponseCb(void *arg, W6X_HTTP_Status_Code_e httpc_result,
                                     uint32_t rx_content_len, uint32_t srv_res, int32_t err)
{
  /* Save the length of the content we are about to received in the passed arg */
  FOTA_HttpXferTypeDef *args = arg;

  if (err != 0)
  {
    LogError("HTTP received error:%" PRIi32 "\n", err);
    args->http_xfer_error_code = -1;
    xTaskNotifyGiveIndexed(fota_task, fota_notify_index);
  }
  else
  {
    args->fwu_total_to_receive = rx_content_len;
    LogInfo("Total len %" PRIu32 "\n", rx_content_len);
    if ((rx_content_len > FLASH_BANK_SIZE) || (httpc_result != OK))
    {
      args->http_xfer_error_code = -1;
      xTaskNotifyGiveIndexed(fota_task, fota_notify_index);
    }
  }
}

static int32_t Fota_HttpHeaderRecvCb(void *arg, W6X_HTTP_buffer_t *p, int32_t err)
{
  FOTA_HttpXferTypeDef *args = arg;

  if (err != 0)
  {
    LogError("HTTP received error:%" PRIi32 "\n", err);
    args->http_xfer_error_code = -1;
    xTaskNotifyGiveIndexed(fota_task, fota_notify_index);
    return -1;
  }

  if ((p == NULL) || (p->length == 0) || (p->data == NULL) || (args->fwu_buffer == NULL))
  {
    LogError("Invalid HTTP buffer received or buffer in arg\n");
    args->http_xfer_error_code = -1;
    xTaskNotifyGiveIndexed(fota_task, fota_notify_index);
    return -1;
  }

  /* Ensure the received data fits into the allocated buffer */
  if (args->fwu_data_accumulated + p->length > FWU_HEADER_SIZE)
  {
    LogError("Received data exceeds allocated buffer size\n");
    args->http_xfer_error_code = -1;
    xTaskNotifyGiveIndexed(fota_task, fota_notify_index);
    return -1;
  }

  /* Copy the received data into the fwu_buffer */
  memcpy(args->fwu_buffer + args->fwu_data_accumulated, p->data, p->length);
  args->fwu_data_accumulated += p->length;

  LogDebug("Received %" PRIi32 " bytes of header data, total accumulated: %" PRIi32 " bytes\n",
           p->length, args->fwu_buffer_len);

  /* Check if the entire header has been received */
  if (args->fwu_data_accumulated >= args->fwu_total_to_receive)
  {
    args->http_xfer_error_code  = 0;
    LogInfo("FOTA header fully received\n");
    xTaskNotifyGiveIndexed(fota_task, fota_notify_index);
  }
  return 0;
}

static int32_t Fota_HttpStm32RecvCb(void *arg, W6X_HTTP_buffer_t *p, int32_t err)
{
  FOTA_HttpXferTypeDef *args = arg;

  if (err != 0)
  {
    LogError("HTTP received error:%" PRIi32 "\n", err);
    args->http_xfer_error_code = -1;
    xTaskNotifyGiveIndexed(fota_task, fota_notify_index);
    return -1;
  }

  if ((p == NULL) || (p->length == 0) || (p->data == NULL) || (args->fwu_buffer == NULL))
  {
    LogError("Invalid HTTP buffer received or buffer in arg\n");
    args->http_xfer_error_code = -1;
    xTaskNotifyGiveIndexed(fota_task, fota_notify_index);
    return -1;
  }

  /* Combine buffered data with the new data */
  size_t total_length = args->fwu_buffer_len + p->length;
  uint8_t *combined_data = args->fwu_buffer;

  if (args->fwu_buffer_len > 0)
  {
    memcpy(args->fwu_buffer + args->fwu_buffer_len, p->data, p->length);
  }
  else
  {
    combined_data = p->data;
  }

  /* Calculate the aligned length (multiple of QUADWORD_SIZE_BYTE bytes) */
  size_t aligned_length = (total_length / QUADWORD_SIZE_BYTE) * QUADWORD_SIZE_BYTE;
  size_t remaining_length = total_length - aligned_length;

  /* Write aligned data to flash */
  if (aligned_length > 0)
  {
    uint32_t current_address = fota_flash_GetStartAddrFlashBank() + args->fwu_data_accumulated;

    if (fota_flash_WriteToFlash(current_address, combined_data, aligned_length) != 0)
    {
      LogError("Flash write error at address 0x%08" PRIx32 " with size %" PRIu32 "\n", current_address, aligned_length);
      xTaskNotifyGiveIndexed(fota_task, fota_notify_index);
      return -1;
    }

    LogDebug("Wrote %" PRIu32 " bytes to flash at address 0x%08" PRIx32 "\n", aligned_length, current_address);

    mbedtls_sha256_update(&ctx, combined_data, aligned_length);
    /* Update the amount of data transferred */
    args->fwu_data_accumulated += aligned_length;
  }

  /* Store the remaining unaligned data */
  if (remaining_length > 0)
  {
    memcpy(args->fwu_buffer, combined_data + aligned_length, remaining_length);
    args->fwu_buffer_len = remaining_length;
  }
  else
  {
    args->fwu_buffer_len = 0;
  }

  /* Handle the last transfer */
  if (args->fwu_data_accumulated + args->fwu_buffer_len >= args->fwu_total_to_receive)
  {
    if (args->fwu_buffer_len > 0)
    {
      uint32_t current_address = fota_flash_GetStartAddrFlashBank() + args->fwu_data_accumulated;

      /* Write the remaining unaligned data to flash */
      if (fota_flash_WriteToFlash(current_address, args->fwu_buffer, args->fwu_buffer_len) != 0)
      {
        LogError("Flash write error at address 0x%08" PRIx32 " with size %" PRIu32 "\n",
                 current_address, args->fwu_buffer_len);
        xTaskNotifyGiveIndexed(fota_task, fota_notify_index);
        return -1;
      }

      LogDebug("Wrote remaining %" PRIu32 " bytes to flash at address 0x%08" PRIx32 "\n",
               args->fwu_buffer_len, current_address);

      mbedtls_sha256_update(&ctx, args->fwu_buffer, args->fwu_buffer_len);
      /* Update the amount of data transferred */
      args->fwu_data_accumulated += args->fwu_buffer_len;
      args->fwu_buffer_len = 0;
    }

    /* Notify the FOTA task that the transfer is complete */
    args->http_xfer_error_code = 0;
    LogInfo("Finished writing FOTA STM32 binary into flash\n");
    xTaskNotifyGiveIndexed(fota_task, fota_notify_index);
  }

  return 0;
}

/* USER CODE END PFD */
