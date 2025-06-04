/**
  ******************************************************************************
  * @file    w6x_version.h
  * @author  GPM Application Team
  * @brief   This file provides the version of W6x_Network_Driver
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
#ifndef W6X_VERSION_H
#define W6X_VERSION_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Exported constants --------------------------------------------------------*/
/** @addtogroup ST67W6X_API_System_Public_Constants
  * @{
  */

#define W6X_VERSION_MAIN   1    /*!< [31:24] main version */
#define W6X_VERSION_SUB1   0    /*!< [23:16] sub1 version */
#define W6X_VERSION_SUB2   0    /*!< [15:8]  sub2 version */

#define W6X_MAIN_SHIFT     24   /*!< main version shift */
#define W6X_SUB1_SHIFT     16   /*!< sub1 version shift */
#define W6X_SUB2_SHIFT     8    /*!< sub2 version shift */

/** ST67W61_Network_Driver version */
#define W6X_VERSION        ((W6X_VERSION_MAIN  << W6X_MAIN_SHIFT)\
                            |(W6X_VERSION_SUB1 << W6X_SUB1_SHIFT)\
                            |(W6X_VERSION_SUB2 << W6X_SUB2_SHIFT))

/** @} */

/* Exported macros -----------------------------------------------------------*/
/** @defgroup ST67W6X_API_System_Public_Macros ST67W6X System Macros
  * @ingroup  ST67W6X_API_System
  * @{
  */
/** ST67W61_Network_Driver version preprocessor macro */
#define XSTR(x) #x

/** ST67W61_Network_Driver version preprocessor macro */
#define MSTR(x) XSTR(x)

/** ST67W61_Network_Driver version string */
#define W6X_VERSION_STR    MSTR(W6X_VERSION_MAIN) "." MSTR(W6X_VERSION_SUB1) "." MSTR(W6X_VERSION_SUB2)

/** @} */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* W6X_VERSION_H */
