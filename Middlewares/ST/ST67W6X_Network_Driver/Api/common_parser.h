/**
  ******************************************************************************
  * @file    common_parser.h
  * @author  GPM Application Team
  * @brief   This file provides the W6x common parser definitions
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
#ifndef COMMON_PARSER_H
#define COMMON_PARSER_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Includes ------------------------------------------------------------------*/
#include <inttypes.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/
/* Exported variables --------------------------------------------------------*/
/* Exported macros -----------------------------------------------------------*/
/** @defgroup ST67W6X_Utilities_Common_Macros ST67W6X Utility Common Macros
  * @ingroup  ST67W6X_Utilities_Common
  * @{
  */

#ifndef MAC2STR
/** MAC buffer to string macros */
#define MAC2STR(a) (a)[0], (a)[1], (a)[2], (a)[3], (a)[4], (a)[5]
/** MAC string format */
#define MACSTR "%02" PRIx16 ":%02" PRIx16 ":%02" PRIx16 ":%02" PRIx16 ":%02" PRIx16 ":%02" PRIx16
#endif /* MAC2STR */

#ifndef IP2STR
/** IP buffer to string macros */
#define IP2STR(a) (a)[0], (a)[1], (a)[2], (a)[3]
/** IP string format */
#define IPSTR "%" PRIu16 ".%" PRIu16 ".%" PRIu16 ".%" PRIu16
#endif /* IP2STR */

#ifndef NIP2STR
/** Macro to convert a uint32_t IP address to a string */
#define NIP2STR(ip)               \
  (uint8_t)(((ip) >> 24) & 0xFF), \
  (uint8_t)(((ip) >> 16) & 0xFF), \
  (uint8_t)(((ip) >> 8) & 0xFF),  \
  (uint8_t)((ip) & 0xFF)
/** Macro to format the IP address as a string, same as IPSTR */
#define NIP2STR_FMT "%" PRIu16 ".%" PRIu16 ".%" PRIu16 ".%" PRIu16
#endif /* NIP2STR */

/** @} */

/* Exported functions ------------------------------------------------------- */
/** @defgroup ST67W6X_Utilities_Common_Functions ST67W6X Utility Common Functions
  * @ingroup  ST67W6X_Utilities_Common
  * @{
  */

/**
  * @brief  Extract a hex number from a string
  * @param  ptr: pointer to string
  * @param  cnt: pointer to the number of parsed digit
  * @retval Hex value or -1 on error
  */
int32_t Parser_StrToHex(char *ptr, uint8_t *cnt);

/**
  * @brief  Parses and returns number from string
  * @param  ptr: pointer to string
  * @param  cnt: pointer to the number of parsed digit
  * @param  value: pointer to the parsed value
  * @retval integer value
  */
int32_t Parser_StrToInt(char *ptr, uint8_t *cnt, int32_t *value);

/**
  * @brief  Parses and returns IP address
  * @param  ptr: pointer to string
  * @param  ip: IP buffer
  */
void Parser_StrToIP(char *ptr, uint8_t ip[4]);

/**
  * @brief  Parses and returns MAC address
  * @param  ptr: pointer to string
  * @param  mac: MAC buffer
  */
void Parser_StrToMAC(char *ptr, uint8_t mac[6]);

/**
  * @brief  Convert char in Hex format to integer
  * @param  a: character to convert
  * @retval integer value
  */
uint8_t Parser_Hex2Num(char a);

/**
  * @brief  Checks if the given IP or MAC address is valid
  * @param  buff: pointer to the address buffer (4 bytes for IP, 6 bytes for MAC)
  * @param  len: length of the address buffer (4 for IP, 6 for MAC)
  * @return Operation status
  */
int32_t Parser_CheckValidAddress(uint8_t *buff, uint32_t len);

/**
  * @brief  Formats an IPv6 address into a human-readable string
  *
  * This function takes a 16-byte IPv6 address and formats it into a standard
  * IPv6 notation string (e.g., "2001:0db8:85a3:0000:0000:8a2e:0370:7334").
  *
  * @param  addr  Pointer to the 16-byte IPv6 address
  * @param  str   Pointer to the buffer where the formatted string will be stored
  * @param  size  Size of the buffer pointed to by str
  */
void format_ipv6_address(uint8_t *addr, char *str, size_t size);

/**
  * @brief  Checks if a formatted IPv6 address string is all zeros
  *
  * This function checks if a formatted IPv6 address string is all zeros.
  *
  * @param  str  Pointer to the formatted IPv6 address string
  * @return 1 if the address string is all zeros, 0 otherwise
  */
int32_t is_ipv6_address_str_zero(const char *str);

/** @} */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* COMMON_PARSER_H */
