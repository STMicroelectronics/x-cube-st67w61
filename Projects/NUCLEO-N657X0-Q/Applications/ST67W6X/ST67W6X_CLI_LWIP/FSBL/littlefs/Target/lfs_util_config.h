/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    lfs_util_config.h
  * @author  ST67 Application Team
  * @brief   lfs utility user configuration
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef LFS_UTIL_CONFIG_H
#define LFS_UTIL_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <inttypes.h>

#include "FreeRTOS.h"
#include "logging.h"

/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported constants --------------------------------------------------------*/
/** Disable assertions */
#define LFS_NO_ASSERT

/** Enable thread-safe */
#define LFS_THREADSAFE

/** Enable read-only filesystem */
#define LFS_READONLY

/* #define LFS_YES_TRACE */

/** Disable debug messages */
#define LFS_NO_DEBUG

/** Disable warning messages */
#define LFS_NO_WARN

/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macros -----------------------------------------------------------*/
/* Logging functions */
/**
  * \def LFS_TRACE( ... )
  * Send a message to the log with trace level
  */
/**
  * \def LFS_DEBUG( ... )
  * Send a message to the log with debug level
  */
/**
  * \def LFS_WARN( ... )
  * Send a message to the log with warn level
  */
/**
  * \def LFS_ERROR( ... )
  * Send a message to the log with error level
  */
/**
  * \def LFS_ASSERT( ... )
  * Send a message to the log with assert level
  */
#ifdef LFS_YES_TRACE
/** Trace logging */
#define LFS_TRACE_(fmt, ...) \
  LogInfo("%s:%d:trace: " fmt "%s\n", __FILE__, __LINE__, __VA_ARGS__)
#define LFS_TRACE(...) LFS_TRACE_(__VA_ARGS__, "")
#else
#define LFS_TRACE(...)
#endif /* LFS_YES_TRACE */

#ifndef LFS_NO_DEBUG
/** Debug logging */
#define LFS_DEBUG_(fmt, ...) \
  LogDebug("%s:%d:debug: " fmt "%s\n", __FILE__, __LINE__, __VA_ARGS__)
#define LFS_DEBUG(...) LFS_DEBUG_(__VA_ARGS__, "")
#else
#define LFS_DEBUG(...)
#endif /* LFS_NO_DEBUG */

#ifndef LFS_NO_WARN
/** Warning logging */
#define LFS_WARN_(fmt, ...) \
  LogWarn("%s:%d:warn: " fmt "%s\n", __FILE__, __LINE__, __VA_ARGS__)
#define LFS_WARN(...) LFS_WARN_(__VA_ARGS__, "")
#else
#define LFS_WARN(...)
#endif /* LFS_NO_WARN */

#ifndef LFS_NO_ERROR
/** Error logging */
#define LFS_ERROR_(fmt, ...) \
  LogError("%s:%d:error: " fmt "%s\n", __FILE__, __LINE__, __VA_ARGS__)
#define LFS_ERROR(...) LFS_ERROR_(__VA_ARGS__, "")
#else
#define LFS_ERROR(...)
#endif /* LFS_NO_ERROR */

/* Runtime assertions */
#ifndef LFS_NO_ASSERT
#define LFS_ASSERT(test) assert(test)
#else
#define LFS_ASSERT(test)
#endif /* LFS_NO_ASSERT */

/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions --------------------------------------------------------*/
/* Builtin functions, these may be replaced by more efficient
 * toolchain-specific implementations. LFS_NO_INTRINSICS falls back to a more
 * expensive basic C implementation for debugging purposes
 */

/**
  * @brief  Returns the maximum of two uint32_t numbers
  * @param  a First number
  * @param  b Second number
  * @return The maximum of a and b
  */
static inline uint32_t lfs_max(uint32_t a, uint32_t b)
{
  return (a > b) ? a : b;
}

/**
  * @brief  Returns the minimum of two uint32_t numbers
  * @param  a First number
  * @param  b Second number
  * @return The minimum of a and b
  */
static inline uint32_t lfs_min(uint32_t a, uint32_t b)
{
  return (a < b) ? a : b;
}

/**
  * @brief  Aligns a value down to the nearest multiple of a given alignment
  * @param  a The value to align
  * @param  alignment The alignment value
  * @return The aligned value
  */
static inline uint32_t lfs_aligndown(uint32_t a, uint32_t alignment)
{
  return a - (a % alignment);
}

/**
  * @brief  Aligns a value up to the nearest multiple of a given alignment
  * @param  a The value to align
  * @param  alignment The alignment value
  * @return The aligned value
  */
static inline uint32_t lfs_alignup(uint32_t a, uint32_t alignment)
{
  return lfs_aligndown(a + alignment - 1, alignment);
}

/**
  * @brief  Finds the smallest power of 2 greater than or equal to a
  * @param  a The value to check
  * @return The smallest power of 2 greater than or equal to a
  */
static inline uint32_t lfs_npw2(uint32_t a)
{
#if !defined(LFS_NO_INTRINSICS) && (defined(__GNUC__) || defined(__CC_ARM))
  return 32 - __builtin_clz(a - 1);
#else
  uint32_t rv = 0;
  uint32_t sv;
  a -= 1;
  sv = (a > 0xffff) << 4;
  a >>= sv;
  rv |= sv;
  sv = (a > 0xff) << 3;
  a >>= sv;
  rv |= sv;
  sv = (a > 0xf) << 2;
  a >>= sv;
  rv |= sv;
  sv = (a > 0x3) << 1;
  a >>= sv;
  rv |= sv;
  return (rv | (a >> 1)) + 1;
#endif /* LFS_NO_INTRINSICS */
}

/**
  * @brief  Counts the number of trailing binary zeros in a given number
  * @param  a The value to check
  * @return The number of trailing binary zeros in a
  */
static inline uint32_t lfs_ctz(uint32_t a)
{
#if !defined(LFS_NO_INTRINSICS) && defined(__GNUC__)
  return __builtin_ctz(a);
#else
  return lfs_npw2((a & -a) + 1) - 1;
#endif /* LFS_NO_INTRINSICS */
}

/**
  * @brief  Counts the number of binary ones in a given number
  * @param  a The value to check
  * @return The number of binary ones in a
  */
static inline uint32_t lfs_popc(uint32_t a)
{
#if !defined(LFS_NO_INTRINSICS) && (defined(__GNUC__) || defined(__CC_ARM))
  return __builtin_popcount(a);
#else
  a = a - ((a >> 1) & 0x55555555);
  a = (a & 0x33333333) + ((a >> 2) & 0x33333333);
  return (((a + (a >> 4)) & 0xf0f0f0f) * 0x1010101) >> 24;
#endif /* LFS_NO_INTRINSICS */
}

/**
  * @brief  Compares two unsigned integers
  * @param  a First number
  * @param  b Second number
  * @return The difference between a and b
  */
static inline int32_t lfs_scmp(uint32_t a, uint32_t b)
{
  return (int)(unsigned)(a - b);
}

/**
  * @brief  Converts a 32-bit little-endian value to native order
  * @param  a The value to convert
  * @return The converted value
  */
static inline uint32_t lfs_fromle32(uint32_t a)
{
#if (defined(  BYTE_ORDER  ) && defined(  ORDER_LITTLE_ENDIAN  ) &&   BYTE_ORDER   ==   ORDER_LITTLE_ENDIAN  ) || \
    (defined(__BYTE_ORDER  ) && defined(__ORDER_LITTLE_ENDIAN  ) && __BYTE_ORDER   == __ORDER_LITTLE_ENDIAN  ) || \
    (defined(__BYTE_ORDER__) && defined(__ORDER_LITTLE_ENDIAN__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
  return a;
#elif !defined(LFS_NO_INTRINSICS) && ( \
    (defined(  BYTE_ORDER  ) && defined(  ORDER_BIG_ENDIAN  ) &&   BYTE_ORDER   ==   ORDER_BIG_ENDIAN  ) || \
    (defined(__BYTE_ORDER  ) && defined(__ORDER_BIG_ENDIAN  ) && __BYTE_ORDER   == __ORDER_BIG_ENDIAN  ) || \
    (defined(__BYTE_ORDER__) && defined(__ORDER_BIG_ENDIAN__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__))
  return __builtin_bswap32(a);
#else
  return (((uint8_t *)&a)[0] <<  0) |
         (((uint8_t *)&a)[1] <<  8) |
         (((uint8_t *)&a)[2] << 16) |
         (((uint8_t *)&a)[3] << 24);
#endif /* LFS_NO_INTRINSICS */
}

/**
  * @brief  Converts a 32-bit little-endian value to native order
  * @param  a The value to convert
  * @return The converted value
  */
static inline uint32_t lfs_tole32(uint32_t a)
{
  return lfs_fromle32(a);
}

/**
  * @brief  Converts a 32-bit big-endian value to native order
  * @param  a The value to convert
  * @return The converted value
  */
static inline uint32_t lfs_frombe32(uint32_t a)
{
#if !defined(LFS_NO_INTRINSICS) && ( \
    (defined(  BYTE_ORDER  ) && defined(  ORDER_LITTLE_ENDIAN  ) &&   BYTE_ORDER   ==   ORDER_LITTLE_ENDIAN  ) || \
    (defined(__BYTE_ORDER  ) && defined(__ORDER_LITTLE_ENDIAN  ) && __BYTE_ORDER   == __ORDER_LITTLE_ENDIAN  ) || \
    (defined(__BYTE_ORDER__) && defined(__ORDER_LITTLE_ENDIAN__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__))
  return __builtin_bswap32(a);
#elif (defined(  BYTE_ORDER  ) && defined(  ORDER_BIG_ENDIAN  ) &&   BYTE_ORDER   ==   ORDER_BIG_ENDIAN  ) || \
    (defined(__BYTE_ORDER  ) && defined(__ORDER_BIG_ENDIAN  ) && __BYTE_ORDER   == __ORDER_BIG_ENDIAN  ) || \
    (defined(__BYTE_ORDER__) && defined(__ORDER_BIG_ENDIAN__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)
  return a;
#else
  return (((uint8_t *)&a)[0] << 24) |
         (((uint8_t *)&a)[1] << 16) |
         (((uint8_t *)&a)[2] <<  8) |
         (((uint8_t *)&a)[3] <<  0);
#endif /* LFS_NO_INTRINSICS */
}

/**
  * @brief  Converts a 32-bit big-endian value to native order
  * @param  a The value to convert
  * @return The converted value
  */
static inline uint32_t lfs_tobe32(uint32_t a)
{
  return lfs_frombe32(a);
}

/**
  * @brief  Calculates the CRC-32 checksum of a buffer with polynomial = 0x04c11db7
  * @param  crc The initial CRC value
  * @param  buffer The buffer to calculate the CRC for
  * @param  size The size of the buffer
  * @return The calculated CRC-32 checksum
  */
static inline uint32_t lfs_crc(uint32_t crc, const void *buffer, size_t size)
{
  static const uint32_t rtable[16] =
  {
    0x00000000, 0x1db71064, 0x3b6e20c8, 0x26d930ac,
    0x76dc4190, 0x6b6b51f4, 0x4db26158, 0x5005713c,
    0xedb88320, 0xf00f9344, 0xd6d6a3e8, 0xcb61b38c,
    0x9b64c2b0, 0x86d3d2d4, 0xa00ae278, 0xbdbdf21c,
  };

  const uint8_t *data = buffer;

  for (size_t i = 0; i < size; i++)
  {
    crc = (crc >> 4) ^ rtable[(crc ^ (data[i] >> 0)) & 0xf];
    crc = (crc >> 4) ^ rtable[(crc ^ (data[i] >> 4)) & 0xf];
  }

  return crc;
}

/**
  * @brief  Allocates memory for littlefs
  * @note   Only used if buffers are not provided to littlefs
  * @param  size The size of the memory to allocate
  * @return A pointer to the allocated memory
  */
static inline void *lfs_malloc(size_t size)
{
  return pvPortMalloc(size);
}

/**
  * @brief  Frees memory allocated for littlefs
  * @note   Only used if buffers are not provided to littlefs
  * @param  p A pointer to the memory to free
  */
static inline void lfs_free(void *p)
{
  vPortFree(p);
}

/* USER CODE BEGIN EF */

/* USER CODE END EF */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* LFS_UTIL_CONFIG_H */
