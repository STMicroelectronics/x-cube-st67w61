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

/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/
/* Exported variables --------------------------------------------------------*/
/* Exported macros -----------------------------------------------------------*/
/** @addtogroup ST67W61_AT_Common_Macros
  * @{
  */

/** Macro to check if the pointer is NULL and return the error code */
#define W61_NULL_ASSERT(p) if ((p) == NULL) { return ret; }

/** Macro to check if the pointer is NULL does not return the error code */
#define W61_NULL_ASSERT_VOID(p) if ((p) == NULL) { return; }

/** Macro to check if the pointer is NULL and return the error code with error string log */
#define W61_NULL_ASSERT_STR(p, s) if ((p) == NULL) { LogError("%s\n", (s)); return ret; }

/** @} */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /*W61_AT_INTERNAL_H */
