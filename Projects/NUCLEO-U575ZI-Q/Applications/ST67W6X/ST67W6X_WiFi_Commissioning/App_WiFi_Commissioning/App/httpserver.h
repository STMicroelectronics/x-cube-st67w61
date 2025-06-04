/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    httpserver.h
  * @author  GPM Application Team
  * @brief   Http server declarations.
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
#ifndef HTTPSERVER_H
#define HTTPSERVER_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Includes ------------------------------------------------------------------*/
#include <stddef.h>
#include <stdint.h>
#include "main.h"

/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
typedef enum
{
  STATUS_UNKNOWN     = 0,
  STATUS_NEW         = 1,   /* New connection, no communication yet. */
  STATUS_COMM        = 2,   /* Connected. */
  STATUS_COMM_STREAM = 3,   /* Connected and stream mode. */
  STATUS_END         = 4    /* Communication terminated. */
} ClientStatusTypeDef;

typedef struct ClientInfos
{
  int32_t             descr;            /* Socket descriptor */
  char                info[64];         /* Ip & port */
  char                err;              /* Error status */
  ClientStatusTypeDef status;           /* Connection status */
  int32_t             message;          /* Message count */
  int32_t             page_done;        /* Html page sent to client. */
  long                id_counter;
} ClientInfosTypeDef;

typedef struct StaInfos
{
  int32_t                   connect_state;         /* user button state */
} StaInfosTypeDef;

typedef struct PingInfos
{
  int32_t                   count;         /* user button state */
  int32_t                   delay;
  int32_t                   loss;
} PingInfosTypeDef;

/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported variables --------------------------------------------------------*/
/* USER CODE BEGIN EV */

/* USER CODE END EV */

/* Exported macros -----------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions --------------------------------------------------------*/
void http_server_socket(void *arg);

/* USER CODE BEGIN EF */

/* USER CODE END EF */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* HTTPSERVER_H */
