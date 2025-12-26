/**
  ******************************************************************************
  * @file    w6x_internal.h
  * @author  GPM Application Team
  * @brief   This file contains the internal definitions of the API
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef W6X_INTERNAL_H
#define W6X_INTERNAL_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Includes ------------------------------------------------------------------*/
#include "w6x_default_config.h"

/** @defgroup ST67W6X_Private ST67W6X Internal
  */

/** @defgroup ST67W6X_Private_Common ST67W6X Common
  * @ingroup  ST67W6X_Private
  */

/** @defgroup ST67W6X_Private_System ST67W6X System
  * @ingroup  ST67W6X_Private
  */

/** @defgroup ST67W6X_Private_WiFi ST67W6X Wi-Fi
  * @ingroup  ST67W6X_Private
  */

/** @defgroup ST67W6X_Private_Net ST67W6X Net
  * @ingroup  ST67W6X_Private
  */

/** @defgroup ST67W6X_Private_HTTP ST67W6X HTTP
  * @ingroup  ST67W6X_Private
  */

/** @defgroup ST67W6X_Private_MQTT ST67W6X MQTT
  * @ingroup  ST67W6X_Private
  */

/** @defgroup ST67W6X_Private_BLE ST67W6X BLE
  * @ingroup  ST67W6X_Private
  */

/** @defgroup ST67W6X_Private_FWU ST67W6X Firmware updates
  * @ingroup  ST67W6X_Private
  */

/** @defgroup ST67W6X_Private_Netif ST67W6X Network Interface
  * @ingroup  ST67W6X_Private
  */

/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/
/** @defgroup ST67W6X_Private_Common_Constants ST67W6X Common Constants
  * @ingroup  ST67W6X_Private_Common
  * @{
  */

#if (W6X_ASSERT_ENABLE == 1)
/** W61 context pointer error string */
static const char W6X_Obj_Null_str[] = "W61 Object pointer not initialized";
#endif /* W6X_ASSERT_ENABLE */

/** @} */

/* Exported variables --------------------------------------------------------*/
/* Exported macros -----------------------------------------------------------*/
/** @defgroup ST67W6X_Private_Common_Macros ST67W6X Common Macros
  * @ingroup  ST67W6X_Private_Common
  * @{
  */

/**
  * \def NULL_ASSERT(p, s)
  * Pointer NULL check and return error
  */

/**
  * \def NULL_ASSERT_VOID(p, s)
  * Pointer NULL check and no return
  */

#if (W6X_ASSERT_ENABLE == 1)
#define NULL_ASSERT(p, s) if ((p) == NULL) { LogError("%s\n", (s)); return W6X_STATUS_ERROR; }
#define NULL_ASSERT_VOID(p, s) if ((p) == NULL) { LogError("%s\n", (s)); return; }
#else
#define NULL_ASSERT(p, s)
#define NULL_ASSERT_VOID(p, s)
#endif /* W6X_ASSERT_ENABLE */

/** Translate W61 to W6X status */
#define TranslateErrorStatus(ret_w61)  TranslateErrorStatus_W61_W6X((ret_w61), __func__)

/** Translate W61 to W6X status and return error */
#define RETURN_ERROR_STATUS(err61, str) \
  if ((ret = TranslateErrorStatus((err61))) == W6X_STATUS_ERROR) { LogError("%s\n", (str)); } return ret;

/** @} */

/* Exported functions ------------------------------------------------------- */
/** @defgroup ST67W6X_Private_Common_Functions ST67W6X Common Functions
  * @ingroup  ST67W6X_Private_Common
  * @{
  */

/**
  * @brief  Translate W61 status to W6X status
  * @param  ret_w61: W61 status
  * @param  func_name: Function name
  * @retval W6X status
  */
W6X_Status_t TranslateErrorStatus_W61_W6X(W61_Status_t ret_w61, char const *func_name);

/** @} */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* W6X_INTERNAL_H */
