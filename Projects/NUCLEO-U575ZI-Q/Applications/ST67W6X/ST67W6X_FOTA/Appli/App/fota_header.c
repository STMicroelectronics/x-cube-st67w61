/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    fota_header.c
  * @author  ST67 Application Team
  * @brief   This file provides code for parsing and handling FOTA headers.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025-2026 STMicroelectronics.
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
#include <inttypes.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include "fota_header.h"
#include "logging.h"
#include "cJSON.h"

/* Functions Definition ------------------------------------------------------*/
int32_t fota_header_ParseJsonToStruct(const uint8_t *buffer, size_t buffer_size, FotaHeader_t *header)
{
  int32_t ret = FOTA_HEADER_ERR;
  cJSON *json_object; /* Pointer to a JSON object to get each item in the FOTA header */
  size_t array_size;

  if ((buffer == NULL) || (header == NULL)) /* Check if the buffer and the header are not NULL */
  {
    return FOTA_HEADER_ERR_INVALID_ARGS;
  }

  /* Buffer provided should be at least the size of the structure for storing information,
     else there might be missing information  */
  if (buffer_size < sizeof(FotaHeader_t))
  {
    return FOTA_HEADER_ERR_BUFFER_TOO_SMALL;
  }

  /* Parse the buffer into a JSON format, expected size should be the one of the struct */
  cJSON *p_fw_header_json = cJSON_ParseWithLength((const char *)buffer, buffer_size);
  if (p_fw_header_json == NULL)
  {
    goto _err1;
  }
  /* Looking for the magic number in the JSON */
  json_object = cJSON_GetObjectItem(p_fw_header_json, MAGIC_NUM);
  if (cJSON_IsString(json_object) && (json_object->valuestring != NULL))
  {
    /* Verify the magic number is the one expected */
    if (strncmp(json_object->valuestring, MAGIC_NUMBER, strlen(MAGIC_NUMBER)) != 0)
    {
      ret = FOTA_HEADER_ERR_INVALID_MAGIC;
      goto _err1;
    }
    (void)memcpy(header->magic_num, json_object->valuestring, strlen(MAGIC_NUMBER));
  }
  else
  {
    goto _err1;
  }

  /* Looking for the protocol version in the JSON */
  json_object = cJSON_GetObjectItem(p_fw_header_json, PROTOCOL_VERSION);
  if (cJSON_IsString(json_object) && (json_object->valuestring != NULL))
  {
    /* Verify the protocol version is the one expected */
    if (strncmp(json_object->valuestring, FOTA_PROTOCOL_VERSION, strlen(FOTA_PROTOCOL_VERSION)) != 0)
    {
      ret = FOTA_HEADER_ERR_INVALID_HEADER_VER;
      goto _err1;
    }
    (void)memcpy(header->protocol_version, json_object->valuestring, strlen(FOTA_PROTOCOL_VERSION));
  }
  else
  {
    goto _err1;
  }

  /* Get the information fields if we found them */
  json_object = cJSON_GetObjectItem(p_fw_header_json, "info");
  if (!cJSON_IsNull(json_object))
  {
    /* Looking for the NCP firmware type info in the JSON */
    const cJSON *info_fw_type = cJSON_GetObjectItem(json_object, INFO_FIRMWARE_TYPE);
    if (cJSON_IsString(info_fw_type) && (info_fw_type->valuestring != NULL))
    {
      (void)memcpy(header->firmware_type, info_fw_type->valuestring, sizeof(header->firmware_type));
    }
    /* Looking for the target STM32 info in the JSON */
    const cJSON *info_target = cJSON_GetObjectItem(json_object, INFO_DATA_TYPE);
    if (cJSON_IsString(info_target) && (info_target->valuestring != NULL))
    {
      (void)memcpy(header->data_type, info_target->valuestring, sizeof(header->data_type));
    }
    /* Looking for the STM32 board name info in the JSON */
    const cJSON *info_board_name = cJSON_GetObjectItem(json_object, INFO_PREFIX_BOARD_NAME);
    if (cJSON_IsString(info_board_name) && (info_board_name->valuestring != NULL))
    {
      (void)memcpy(header->prefix_board_name, info_board_name->valuestring, sizeof(header->prefix_board_name));
    }
    /* Looking for the STM32 board revision info in the JSON */
    const cJSON *info_board_rev = cJSON_GetObjectItem(json_object, INFO_BOARD_REVISION);
    if (cJSON_IsString(info_board_rev) && (info_board_rev->valuestring != NULL))
    {
      (void)memcpy(header->board_revision, info_board_rev->valuestring, sizeof(header->board_revision));
    }
  }

  /* Looking for the file hash in the JSON */
  json_object = cJSON_GetObjectItem(p_fw_header_json, FILE_HASH);
  if (!cJSON_IsArray(json_object))
  {
    LogError("file_hash is not a valid JSON array\n");
    goto _err1;
  }
  /* Check the expected size of the hash array and the one from the JSON file */
  array_size = cJSON_GetArraySize(json_object);
  if (array_size != sizeof(header->file_hash))
  {
    LogError("Invalid file_hash size\n");
    goto _err1;
  }

  /* Copy the file hash from the JSON to the header structure */
  for (size_t i = 0; i < array_size; i++)
  {
    cJSON *item = cJSON_GetArrayItem(json_object, i);
    if (!cJSON_IsNumber(item))
    {
      LogError("Invalid value in file_hash\n");
      goto _err1;
    }
    header->file_hash[i] = (uint8_t)item->valueint;
  }

  /* Looking for the host application version in the JSON */
  json_object = cJSON_GetObjectItem(p_fw_header_json, STM32_APP_VER);
  if (cJSON_IsString(json_object) && (json_object->valuestring != NULL))
  {
    (void)memcpy(header->stm32_app_ver, json_object->valuestring, sizeof(header->stm32_app_ver));
  }
  else
  {
    goto _err1;
  }

  /* Looking for the ST67 application version in the JSON */
  json_object = cJSON_GetObjectItem(p_fw_header_json, ST67_VER);
  if (cJSON_IsString(json_object) && (json_object->valuestring != NULL))
  {
    (void)memcpy(header->st67_ver, json_object->valuestring, sizeof(header->st67_ver));
  }
  else
  {
    goto _err1;
  }
  ret = FOTA_HEADER_SUCCESS;
_err1:
  cJSON_Delete(p_fw_header_json); /* Free the JSON object */
  return ret;
}

int32_t fota_header_CmpVer(const W6X_Version_t current_ver, const uint8_t *new_ver)
{
  int32_t major;
  int32_t minor;
  int32_t patch;
  uint32_t current_v;
  uint32_t new_v;
  const char *p = (const char *)new_ver;
  char *endptr;

  /* Scan for the x.y.z format version and saved them into int32_t variables */
  major = (int32_t)strtol(p, &endptr, 10);
  if (*endptr != '.')
  {
    goto _err;
  }
  p = endptr + 1; /* Move past the '.' */
  minor = (int32_t)strtol(p, &endptr, 10);
  if (*endptr != '.')
  {
    goto _err;
  }
  p = endptr + 1; /* Move past the '.' */
  patch = (int32_t)strtol(p, &endptr, 10);

  /* Convert the version string to an integer for comparison */
  new_v = ((major & 0xFF) << 16) | ((minor & 0xFF) << 8) | (patch & 0xFF);

  /* Convert the version string to an integer for comparison */
  current_v = ((current_ver.Major & 0xFF) << 16) | ((current_ver.Sub1 & 0xFF) << 8) | (current_ver.Sub2 & 0xFF);

  /* Compare the two version numbers */
  if (current_v > new_v)
  {
    return FOTA_HEADER_VER_OLDER;
  }
  else if (current_v < new_v)
  {
    return FOTA_HEADER_VER_NEWER;
  }
  else
  {
    /* Versions are equal */
  }

  return FOTA_HEADER_VER_EQ; /* Versions x.y.z are equal */

_err:
  LogError("Invalid version format\n");
  return FOTA_HEADER_ERR;
}

int32_t fota_header_Print(const FotaHeader_t *header)
{
  if (header == NULL)
  {
    return FOTA_HEADER_ERR_INVALID_ARGS;
  }
  LogDebug("\nFOTA header:\n");   /* Debug print header structure content */
  LogDebug("Magic number: %.16s\n", header->magic_num);
  LogDebug("Header protocol version: %.16s\n", header->protocol_version);
  LogDebug("Data type: %.16s\n", header->data_type);
  LogDebug("Firmware type: %.16s\n", header->firmware_type);
  LogDebug("stm32_app_ver version: %.16s\n", header->stm32_app_ver);
  LogDebug("st67_ver version: %.16s\n", header->st67_ver);
  LogDebug("File hash:\n");
  for (uint32_t i = 0; i < 32; i++)
  {
    LogDebug("%02X", header->file_hash[i]); /* Print each byte as a two-digit hexadecimal */
  }
  LogDebug("\n\n");

  return FOTA_HEADER_SUCCESS;
}
