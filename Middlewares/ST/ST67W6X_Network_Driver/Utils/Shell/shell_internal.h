/**
  ******************************************************************************
  * @file    shell_internal.h
  * @author  GPM Application Team
  * @brief   This file is part of the shell module.
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
#ifndef SHELL_INTERNAL_H
#define SHELL_INTERNAL_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* Exported functions ------------------------------------------------------- */
/** @defgroup ST67W6X_Utilities_Shell_Functions ST67W6X Utility Shell Functions
  * @ingroup  ST67W6X_Utilities_Shell
  * @{
  */

/**
  * @brief  Shell handler
  * @param  data the input data
  */
void shell_handler(uint8_t data);
/**
  * @brief  This function will initialize shell
  * @param  shell_printf the print function
  */
void shell_init(void (*shell_printf)(char *fmt, ...));

/** @} */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* SHELL_INTERNAL_H */
