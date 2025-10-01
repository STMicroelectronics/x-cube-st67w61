/**
  ******************************************************************************
  * @file    w61_at_internal.h
  * @author  GPM Application Team
  * @brief   This file provides the internal definitions of the AT driver
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
#ifndef W61_AT_INTERNAL_H
#define W61_AT_INTERNAL_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Includes ------------------------------------------------------------------*/
#include <string.h>
#include "w61_default_config.h"

/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/
/* Exported variables --------------------------------------------------------*/
/* Exported macros -----------------------------------------------------------*/
/** @addtogroup ST67W61_AT_Common_Macros
  * @{
  */

/**
  * \def W61_ASSERT(p)
  * Macro to check if the expression is true and call the assert function if it is false
  */

/**
  * \def W61_NULL_ASSERT(p)
  * Macro to check if the pointer is NULL and return the error code
  */

/**
  * \def W61_NULL_ASSERT_VOID(p)
  * Macro to check if the pointer is NULL does not return the error code
  */

/**
  * \def W61_NULL_ASSERT_STR(p, s)
  * Macro to check if the pointer is NULL and return the error code with error string log
  */

#if (W61_ASSERT_ENABLE == 1)
/** Macro to check if the pointer is NULL and return the error code with error string log */
#define W61_ASSERT(expr) if (!(expr)) { LogError("Assert failed: file: %s, line: %d", __FILE__, __LINE__); while(1); }
#define W61_NULL_ASSERT(p) if ((p) == NULL) { return W61_STATUS_ERROR; }
#define W61_NULL_ASSERT_VOID(p) if ((p) == NULL) { return; }
#define W61_NULL_ASSERT_STR(p, s) if ((p) == NULL) { LogError("%s\n", (s)); return W61_STATUS_ERROR; }
#else
#define W61_ASSERT(expr)
#define W61_NULL_ASSERT(p)
#define W61_NULL_ASSERT_VOID(p)
#define W61_NULL_ASSERT_STR(p, s)
#endif /* W61_ASSERT_ENABLE */

/** @} */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /*W61_AT_INTERNAL_H */
