/**
  ******************************************************************************
  * @file    fota_header_struct.h
  * @author  GPM Application Team
  * @brief   FOTA header structure definition
  * @note    Auto generated file, DO NOT MODIFY
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

#ifndef FOTA_HEADER_STRUCT_H
#define FOTA_HEADER_STRUCT_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>

/* Exported constants --------------------------------------------------------*/
#define MAGIC_NUMBER                    "ST67W611"
#define FOTA_HEADER_SIZE                512
#define FOTA_PROTOCOL_VERSION           "1.0.0"

#define MAGIC_NUM                       "magic_num"
#define PROTOCOL_VERSION                "protocol_version"
#define INFO_FIRMWARE_TYPE              "firmware_type"
#define INFO_DATA_TYPE                  "data_type"
#define INFO_PREFIX_BOARD_NAME          "prefix_board_name"
#define INFO_BOARD_REVISION             "board_revision"
#define INFO_DESC                       "desc"
#define STM32_APP_VER                   "stm32_app_ver"
#define ST67_VER                        "st67_ver"
#define FILE_HASH                       "file_hash"

/* Exported types ------------------------------------------------------------*/
typedef struct
{
  uint8_t magic_num[16];
  uint8_t protocol_version[16];
  uint8_t data_type[16];
  uint8_t prefix_board_name[16];
  uint8_t board_revision[16];
  uint8_t firmware_type[16];
  uint8_t stm32_app_ver[16];
  uint8_t st67_ver[16];
  uint8_t file_hash[32];
  uint8_t reserved[];
} FotaHeader_t;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* FOTA_HEADER_STRUCT_H */
