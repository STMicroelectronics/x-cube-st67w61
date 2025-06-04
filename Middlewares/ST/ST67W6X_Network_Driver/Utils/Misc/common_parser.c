/**
  ******************************************************************************
  * @file    common_parser.c
  * @author  GPM Application Team
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
#include "common_parser.h"

/* Private macros ------------------------------------------------------------*/
/** @addtogroup ST67W6X_Utilities_Common_Macros
  * @{
  */

/** Convert a character to a number */
#define CHAR2NUM(x)                     ((x) - '0')

/** Check if the character x is a number */
#define CHARISNUM(x)                    isdigit((unsigned char)(x))

/** Check if the character x is a hexadecimal number */
#define CHARISHEXNUM(x)                 isxdigit((unsigned char)(x))

/** @} */
/* Private function prototypes -----------------------------------------------*/
/** @addtogroup ST67W6X_Utilities_Common_Functions
  * @{
  */

/**
  * @brief  Convert char in Hex format to integer.
  * @param  a: character to convert
  * @retval integer value.
  */
static uint8_t Hex2Num(char a);

/* Functions Definition ------------------------------------------------------*/
uint32_t Parser_StrToHex(char *ptr, uint8_t *cnt)
{
  uint32_t sum = 0;
  uint8_t i = 0;

  /* Loop on pointer content while it is a hexadecimal character */
  while (CHARISHEXNUM(*ptr))
  {
    sum <<= 4;
    sum += Hex2Num(*ptr);
    ptr++;
    i++;
  }

  /* Save number of characters used for number */
  if (cnt != NULL)
  {
    *cnt = i;
  }

  /* Return the concatenated number */
  return sum;
}

int32_t Parser_StrToInt(char *ptr, uint8_t *cnt, int32_t *value)
{
  uint8_t minus = 0;
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
    minus = 1;
    ptr++;
    i++;
  }

  /* Loop on pointer content while it is a numeric character */
  while (CHARISNUM(*ptr))
  {
    sum = 10 * sum + CHAR2NUM(*ptr);
    ptr++;
    i++;
  }

  /* Save number of characters used for number */
  if (cnt != NULL)
  {
    *cnt = i;
  }

  /* Minus detected */
  if (minus)
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
  uint8_t octet_count = 0;
  int32_t tmp_nb = 0;
  char *end_ptr = NULL;

  while (*ptr && octet_count < 4)
  {
    /* Convert the current segment to an integer */
    tmp_nb = strtol(ptr, &end_ptr, 10);

    /* Validate the parsed number (must be between 0 and 255) */
    if ((tmp_nb < 0) || (tmp_nb > 0xFF) || (end_ptr == ptr))
    {
      /* If parsing fails or the number is out of range, set IP to 0 and return */
      memset(ip, 0, 4);
      return;
    }

    /* Store the parsed number in the IP array */
    ip[octet_count++] = (uint8_t)tmp_nb;

    /* Move the pointer to the next segment */
    ptr = end_ptr;

    /* Skip the '.' delimiter if present */
    if (*ptr == '.')
    {
      ptr++;
    }
  }

  /* If we didn't parse exactly 4 octets, the IP is invalid */
  if ((octet_count != 4) || (*ptr != '\0'))
  {
    memset(ip, 0, 4);
  }
}

int32_t Parser_CheckIP(uint8_t ip[4])
{
  uint32_t count_full = 0;
  uint32_t count_zero = 0;

  for (int32_t i = 0; i < 4; i++)
  {
    if (ip[i] == 0xFF)
    {
      count_full++; /* Count the number of 255 */
    }
    if (ip[i] == 0)
    {
      count_zero++; /* Count the number of 0 */
    }

    /* If the IP contains only 255 or only 0, the IP address is invalid */
    if ((count_full == 4) || (count_zero == 4))
    {
      return -1;
    }
  }

  return 0;
}

void Parser_StrToMAC(char *ptr, uint8_t mac[6])
{
  uint8_t hexnum = 0;
  uint8_t hexcnt;
  int32_t mac_string_len = 17; /* Maximum length of a MAC address string */

  /* Loop on pointer content while non empty */
  while ((*ptr) && (mac_string_len > 0))
  {
    hexcnt = 1;
    if (*ptr != ':') /* Skip ':' */
    {
      mac[hexnum++] = Parser_StrToHex(ptr, &hexcnt);
    }
    ptr = ptr + hexcnt;
    mac_string_len -= hexcnt; /* Decrement the length of the MAC address string */
  }
}

/* Private Functions Definition ----------------------------------------------*/
static uint8_t Hex2Num(char a)
{
  /* Char is num */
  if ((a >= '0') && (a <= '9'))
  {
    return a - '0';
  }
  /* Char is lowercase character A - Z (hex) */
  else if ((a >= 'a') && (a <= 'f'))
  {
    return (a - 'a') + 10;
  }
  /* Char is uppercase character A - Z (hex) */
  else if ((a >= 'A') && (a <= 'F'))
  {
    return (a - 'A') + 10;
  }

  return 0;
}

/** @} */
