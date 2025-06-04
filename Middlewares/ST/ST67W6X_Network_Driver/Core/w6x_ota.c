/**
  ******************************************************************************
  * @file    w6x_ota.c
  * @author  GPM Application Team
  * @brief   This file provides code for W6x OTA API
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

/* Includes ------------------------------------------------------------------*/
#include <string.h>
#include "w6x_api.h"       /* Prototypes of the functions implemented in this file */
#include "w61_at_api.h"    /* Prototypes of the functions called by this file */
#include "w6x_internal.h"
#include "w61_io.h"        /* Prototypes of the BUS functions to be registered */
#include "common_parser.h" /* Common Parser functions */

/* Global variables ----------------------------------------------------------*/
/* Private typedef -----------------------------------------------------------*/
/* Private defines -----------------------------------------------------------*/
/* Private macros ------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/** @defgroup ST67W6X_Private_OTA_Variables ST67W6X OTA Variables
  * @ingroup  ST67W6X_Private_OTA
  * @{
  */
static W61_Object_t *p_DrvObj = NULL; /*!< Global W61 context pointer */

/** W6X OTA init error string */
static const char W6X_OTA_Uninit_str[] = "W6X OTA module not initialized";

/** @} */

/* Private function prototypes -----------------------------------------------*/
/* Functions Definition ------------------------------------------------------*/
/** @addtogroup ST67W6X_API_OTA_Public_Functions
  * @{
  */

W6X_Status_t W6X_OTA_Starts(uint32_t enable)
{
  W6X_Status_t ret = W6X_STATUS_ERROR;
  p_DrvObj = W61_ObjGet();
  NULL_ASSERT(p_DrvObj, W6X_Obj_Null_str);

  /* OTA start on W61 currently supports only value 1 or 0 as parameters, all other value will raise an error */
  if ((enable != 0) && (enable != 1))
  {
    return ret;
  }

  return TranslateErrorStatus(W61_OTA_starts(p_DrvObj, enable));
}

W6X_Status_t W6X_OTA_Finish(void)
{
  W6X_Status_t ret = W6X_STATUS_ERROR;
  NULL_ASSERT(p_DrvObj, W6X_OTA_Uninit_str);

  ret = TranslateErrorStatus(W61_OTA_Finish(p_DrvObj));

  p_DrvObj = NULL; /* Reset the pointer to avoid using it after OTA finish */
  return ret;
}

W6X_Status_t W6X_OTA_Send(uint8_t *buff, uint32_t len)
{
  W6X_Status_t ret = W6X_STATUS_ERROR;
  NULL_ASSERT(p_DrvObj, W6X_OTA_Uninit_str);
  NULL_ASSERT(buff, "buff not defined");

  if (len == 0)
  {
    return W6X_STATUS_OK;
  }

  return TranslateErrorStatus(W61_OTA_Send(p_DrvObj, buff, len));
}

/** @} */
