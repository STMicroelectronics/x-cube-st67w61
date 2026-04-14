/**
  ******************************************************************************
  * @file    common_parser.c
  * @author  ST67 Application Team
  * @brief   This file provides code for W6x common parser functions
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

/* Includes ------------------------------------------------------------------*/
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include "common_parser.h"

/* Private macros ------------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/** @addtogroup ST67W6X_Utilities_Common_Functions
  * @{
  */

/* Functions Definition ------------------------------------------------------*/
int32_t Parser_StrToHex(char *ptr, uint8_t *cnt)
{
  uint32_t sum = 0;
  uint8_t i = 0;

  if (isxdigit((int32_t)(*ptr)) == 0)
  {
    return -1;
  }

  /* Loop on pointer content while it is a hexadecimal character */
  while (isxdigit((int32_t)(*ptr)) > 0)
  {
    sum <<= 4U;
    sum += Parser_Hex2Num(*ptr);
    ptr++;
    i++;
  }

  /* Save number of characters used for number */
  if (cnt != NULL)
  {
    *cnt = i;
  }

  /* Return the concatenated number */
  return (int32_t)sum;
}

int32_t Parser_StrToInt(char *ptr, uint8_t *cnt, int32_t *value)
{
  uint8_t minus = 0U;
  uint8_t i = 0;
  int32_t sum = 0;
  uint8_t len = 0;
  if (value == NULL)
  {
    return 0;
  }

  len = strlen(ptr);
  /* Trim '\r' and '\n' characters */
  while (len > i)
  {
    if ((ptr[i] == '\r') || (ptr[i] == '\n'))
    {
      len = i;
    }
    else
    {
      i++;
    }
  }
  i = 0;
  /* Check for minus character */
  if (*ptr == '-')
  {
    minus = 1U;
    ptr++;
    i++;
  }

  /* Loop on pointer content while it is a numeric character */
  while (isdigit((int32_t)(*ptr)) > 0)
  {
    sum = (10 * sum) + ((*ptr) - '0');
    ptr++;
    i++;
  }

  /* Save number of characters used for number */
  if (cnt != NULL)
  {
    *cnt = i;
  }

  /* Minus detected */
  if (minus == 1U)
  {
    *value = 0 - sum;
  }
  else
  {
    *value = sum;
  }

  /* Verify the total length of the string equals the number of numeric characters found */
  if (len != i)
  {
    return 0;
  }
  /* Return the concatenated number */
  return 1;
}

void Parser_StrToIP(char *ptr, uint8_t ip[4])
{
  uint8_t count_byte = 0;
  int32_t tmp_nb = 0;
  char *end_ptr = NULL;

  while ((*ptr != '\0') && (count_byte < 4U))
  {
    /* Convert the current segment to an integer */
    tmp_nb = strtol(ptr, &end_ptr, 10);

    /* Validate the parsed number (must be between 0 and 255) */
    if ((tmp_nb < 0) || (tmp_nb > 0xFF) || (end_ptr == ptr))
    {
      /* If parsing fails or the number is out of range, set IP to 0 and return */
      (void)memset(ip, 0, 4);
      return;
    }

    /* Store the parsed number in the IP array */
    ip[count_byte++] = (uint8_t)tmp_nb;

    /* Move the pointer to the next segment */
    ptr = end_ptr;

    /* Skip the '.' delimiter if present */
    if (*ptr == '.')
    {
      ptr++;
    }
  }

  /* If exactly 4 bytes are not parsed, the IP is invalid */
  if ((count_byte != 4U) || (*ptr != '\0'))
  {
    (void)memset(ip, 0, 4);
  }
}

int32_t Parser_CheckValidAddress(uint8_t *buff, uint32_t len)
{
  uint32_t count_full = 0;
  uint32_t count_zero = 0;

  for (int32_t i = 0; i < len; i++)
  {
    if (buff[i] == 0xFFU)
    {
      count_full++; /* Count the number of 255 */
    }
    if (buff[i] == 0U)
    {
      count_zero++; /* Count the number of 0 */
    }

    /* Return invalid address if all bytes contains only 255 or only 0 */
    if ((count_full == len) || (count_zero == len))
    {
      return -1;
    }
  }

  return 0;
}

void Parser_StrToMAC(char *ptr, uint8_t mac[6])
{
  uint8_t count_byte = 0;
  uint8_t hexcnt;
  int32_t mac_string_len = 17; /* Maximum length of a MAC address string */

  /* Loop on pointer content while non empty */
  while ((*ptr != '\0') && (mac_string_len > 0))
  {
    hexcnt = 1;
    if (*ptr != ':') /* Skip ':' */
    {
      int32_t hex_value = Parser_StrToHex(ptr, &hexcnt);
      if (hex_value < 0)
      {
        /* If parsing fails, set MAC to 0 and return */
        (void)memset(mac, 0, 6);
        return;
      }
      mac[count_byte++] = (uint8_t)hex_value;
    }
    ptr = ptr + hexcnt;
    mac_string_len -= hexcnt; /* Decrement the length of the MAC address string */
  }

  /* If exactly 6 bytes are not parsed, the MAC Address is invalid */
  if ((count_byte != 6U) || (*ptr != '\0'))
  {
    (void)memset(mac, 0, 6);
  }
}

uint8_t Parser_Hex2Num(char a)
{
  /* Char is num */
  if (((uint8_t)a >= 0x30U) && ((uint8_t)a <= 0x39U))
  {
    return (uint8_t)a - 0x30U;
  }
  /* Char is uppercase character A - Z (hex) */
  else if (((uint8_t)a >= 0x41U) && ((uint8_t)a <= 0x46U))
  {
    return (uint8_t)a - 0x41U + 10U;
  }
  /* Char is lowercase character a - f (hex) */
  else if (((uint8_t)a >= 0x61U) && ((uint8_t)a <= 0x66U))
  {
    return (uint8_t)a - 0x61U + 10U;
  }
  else
  {
    return 0U;
  }
}

/** @} */
