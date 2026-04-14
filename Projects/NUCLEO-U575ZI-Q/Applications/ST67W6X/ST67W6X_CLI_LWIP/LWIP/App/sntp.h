/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    sntp.h
  * @author  ST67 Application Team
  * @brief   This is simple "SNTP" client definition
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

/*
 * Portions of this file are based on LwIP's http client example application, LwIP version is 2.2.1.
 * Which is licensed under modified BSD-3 Clause license as indicated below.
 * See https://savannah.nongnu.org/projects/lwip/ for more information.
 *
 * Reference source :
 * https://github.com/lwip-tcpip/lwip/blob/master/src/apps/sntp/sntp.c
 *
 */

/*
 * Copyright (c) 2007-2009 Frederic Bernon, Simon Goldschmidt
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
 * SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 *
 * This file is part of the lwIP TCP/IP stack.
 *
 * Author: Frederic Bernon, Simon Goldschmidt
 *
 */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef SNTP_H
#define SNTP_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include <time.h>

#include "lwip/ip_addr.h"
#include "lwip/opt.h"
#include "lwip/prot/iana.h"

/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/**
  * @brief  Date time structure definition
  */
typedef struct
{
  uint32_t seconds;      /*!< Seconds */
  uint32_t minutes;      /*!< Minutes */
  uint32_t hours;        /*!< Hours */
  uint32_t day;          /*!< Day */
  uint32_t month;        /*!< Month */
  uint32_t year;         /*!< Year */
  uint32_t day_of_week;  /*!< Day of the week */
  char raw[26];          /*!< Raw time string */
  time_t timestamp;      /*!< Unix timestamp in s */
  uint32_t current_time; /*!< Time from the last server response in seconds */
} date_time_t;

/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/** SNTP server port */
#define SNTP_PORT                   LWIP_IANA_PORT_SNTP

/** SNTP update delay - in milliseconds
  * Default is 1 hour. Must not be below 60 seconds by specification (i.e. 60000)
  */
#define SNTP_UPDATE_DELAY           360000

/** SNTP receive timeout - in milliseconds
  * Also used as retry timeout - this shouldn't be too low.
  * Default is 15 seconds. Must not be below 15 seconds by specification (i.e. 15000)
  */
#define SNTP_RECV_TIMEOUT           15000

/** Default retry timeout (in milliseconds) if the response
  * received is invalid.
  * This is doubled with each retry until SNTP_RETRY_TIMEOUT_MAX is reached.
  */
#define SNTP_RETRY_TIMEOUT          SNTP_RECV_TIMEOUT

/** Delay before to start first request */
#define SNTP_STARTUP_DELAY          5000U

/** SNTP server address
  * For a list of some public NTP servers, see this link:
  * http://support.ntp.org/bin/view/Servers/NTPPoolServers
  */
#define SNTP_SERVER_ADDRESS         "pool.ntp.org"

/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported variables --------------------------------------------------------*/
/* USER CODE BEGIN EV */

/* USER CODE END EV */

/* Exported macros -----------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions --------------------------------------------------------*/
/**
  * @brief  Initialize the SNTP client module
  * @param  timezone_offset: timezone_offset in hours
  * @retval 0 on success, -1 on failure
  * @note   Send out request after a delay
  */
int32_t sntp_init(int16_t timezone_offset);

/**
  * @brief  Stop the SNTP client module
  */
void sntp_stop(void);

/**
  * @brief  Get the current time from the SNTP server
  * @param  dt: pointer to date_time_t structure
  * @retval 0 on success, -1 on failure
  */
int32_t sntp_gettime(date_time_t *dt);

/**
  * @brief  Set the SNTP server address
  * @param  server_addr: pointer to ip_addr_t structure
  */
void sntp_setserver(const ip_addr_t *server_addr);

/* USER CODE BEGIN EF */

/* USER CODE END EF */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* SNTP_H */
