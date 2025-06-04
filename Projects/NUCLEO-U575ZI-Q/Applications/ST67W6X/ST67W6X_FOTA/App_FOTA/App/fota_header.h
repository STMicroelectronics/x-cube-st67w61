/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    fota_header.h
  * @author  GPM Application Team
  * @brief   Header file for FOTA header parsing and handling.
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
#ifndef FOTA_HEADER_H
#define FOTA_HEADER_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include <stddef.h>
#include "fota_header_struct.h"

/* Exported constants --------------------------------------------------------*/
/** @defgroup FOTA_Header_Error_Codes FOTA Header Error Codes
  * @{
  */

/** Error codes */
#define FOTA_HEADER_VER_NEWER           2  /*!< Version parsed from downloaded header is newer than current one */
#define FOTA_HEADER_VER_EQ              1  /*!< Version parsed from downloaded header is the same than current one */
#define FOTA_HEADER_SUCCESS             0  /*!< Operation successful */
#define FOTA_HEADER_ERR                -1  /*!< General error */
#define FOTA_HEADER_ERR_INVALID_ARGS   -2  /*!< Invalid arguments */
#define FOTA_HEADER_ERR_BUFFER_TOO_SMALL -3 /*!< Buffer too small */
#define FOTA_HEADER_ERR_INVALID_MAGIC  -4  /*!< Invalid magic number in the downloaded header */
#define FOTA_HEADER_ERR_INVALID_HEADER_VER -5 /*!< Invalid header version */
#define FOTA_HEADER_VER_OLDER          -6  /*!< Version parsed from downloaded header is older than current one */

/** @} */

/* Exported functions --------------------------------------------------------*/
/** @defgroup FOTA_Header_Functions FOTA Header Functions
  * @{
  */

/**
  * @brief  Parse the FOTA header from a JSON buffer.
  * @param  buffer: Pointer to the JSON buffer.
  * @param  buffer_size: Size of the JSON buffer.
  * @param  header: Pointer to the FotaHeader structure to populate.
  * @retval int32_t Status of the operation.
  */
int32_t fota_header_ParseJsonToStruct(const uint8_t *buffer, size_t buffer_size, FotaHeader_t *header);

/**
  * @brief  Compare two version strings.
  * @param  current_ver: Current version string.
  * @param  new_ver: New version string.
  * @retval int32_t Comparison result.
  */
int32_t fota_header_CmpVer(const uint8_t *current_ver, const uint8_t *new_ver);

/**
  * @brief  Print the FOTA header information.
  * @param  header: Pointer to the FotaHeader structure.
  * @retval int32_t Status of the operation.
  */
int32_t fota_header_Print(const FotaHeader_t *header);

/** @} */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* FOTA_HEADER_H */
