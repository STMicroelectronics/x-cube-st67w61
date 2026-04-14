/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    sha256.c
  * @author  ST67 Application Team
  * @brief   This file is part of mbed TLS and used for sha256 computation.
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

/**
  * Portions of this file are based on mbed TLS, which is licensed under the Apache-2.0 license as indicated below.
  * It was taken from the mbed TLS version 3.6.2 and modified for use in this project.
  *
  * Reference source:
  * https://github.com/Mbed-TLS/mbedtls/blob/v3.6.2/library/sha256.c
  */

/*
 *  FIPS-180-2 compliant SHA-256 implementation
 *
 *  Copyright The Mbed TLS Contributors
 *  SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later
 */
/*
 *  The SHA-256 Secure Hash Standard was published by NIST in 2002.
 *
 *  http://csrc.nist.gov/publications/fips/fips180-2/fips180-2.pdf
 */

#include "sha256.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define SHA256_BLOCK_SIZE 64

size_t mbedtls_internal_sha256_process_many(mbedtls_sha256_context_t *ctx, const uint8_t *data, size_t len);

/* Implementation that should never be optimized out by the compiler */
static void mbedtls_platform_zeroize(void *v, size_t num)
{
  volatile unsigned char *p = v;
  while (num--) { *p++ = 0; }
}

/*
 * 32-bit integer manipulation macros (big endian)
 */
#ifndef GET_UINT32_BE
#define GET_UINT32_BE(data , offset) \
  ( \
    (   (uint32_t) ( data )[( offset )    ] << 24 ) \
    | ( (uint32_t) ( data )[( offset ) + 1] << 16 ) \
    | ( (uint32_t) ( data )[( offset ) + 2] <<  8 ) \
    | ( (uint32_t) ( data )[( offset ) + 3]       ) \
  )
#endif /* GET_UINT32_BE */

#ifndef PUT_UINT32_BE
#define PUT_UINT32_BE(n, data, offset)                        \
  do {                                                        \
    ( data )[( offset )    ] = (unsigned char) ( (n) >> 24 ); \
    ( data )[( offset ) + 1] = (unsigned char) ( (n) >> 16 ); \
    ( data )[( offset ) + 2] = (unsigned char) ( (n) >>  8 ); \
    ( data )[( offset ) + 3] = (unsigned char) ( (n)       ); \
  } while( 0 )
#endif /* PUT_UINT32_BE */

void mbedtls_sha256_init(mbedtls_sha256_context_t *ctx)
{
  memset(ctx, 0, sizeof(mbedtls_sha256_context_t));
}

void mbedtls_sha256_free(mbedtls_sha256_context_t *ctx)
{
  if (ctx == NULL)
  {
    return;
  }

  mbedtls_platform_zeroize(ctx, sizeof(mbedtls_sha256_context_t));
}

void mbedtls_sha256_clone(mbedtls_sha256_context_t *dst, const mbedtls_sha256_context_t *src)
{
  *dst = *src;
}

/*
 * SHA-256 context setup
 */
int32_t mbedtls_sha256_starts(mbedtls_sha256_context_t *ctx)
{
  ctx->total[0] = 0;
  ctx->total[1] = 0;

  /* SHA-256 */
  ctx->state[0] = 0x6A09E667;
  ctx->state[1] = 0xBB67AE85;
  ctx->state[2] = 0x3C6EF372;
  ctx->state[3] = 0xA54FF53A;
  ctx->state[4] = 0x510E527F;
  ctx->state[5] = 0x9B05688C;
  ctx->state[6] = 0x1F83D9AB;
  ctx->state[7] = 0x5BE0CD19;

  return 0;
}

static const uint32_t K[] =
{
  0x428A2F98, 0x71374491, 0xB5C0FBCF, 0xE9B5DBA5,
  0x3956C25B, 0x59F111F1, 0x923F82A4, 0xAB1C5ED5,
  0xD807AA98, 0x12835B01, 0x243185BE, 0x550C7DC3,
  0x72BE5D74, 0x80DEB1FE, 0x9BDC06A7, 0xC19BF174,
  0xE49B69C1, 0xEFBE4786, 0x0FC19DC6, 0x240CA1CC,
  0x2DE92C6F, 0x4A7484AA, 0x5CB0A9DC, 0x76F988DA,
  0x983E5152, 0xA831C66D, 0xB00327C8, 0xBF597FC7,
  0xC6E00BF3, 0xD5A79147, 0x06CA6351, 0x14292967,
  0x27B70A85, 0x2E1B2138, 0x4D2C6DFC, 0x53380D13,
  0x650A7354, 0x766A0ABB, 0x81C2C92E, 0x92722C85,
  0xA2BFE8A1, 0xA81A664B, 0xC24B8B70, 0xC76C51A3,
  0xD192E819, 0xD6990624, 0xF40E3585, 0x106AA070,
  0x19A4C116, 0x1E376C08, 0x2748774C, 0x34B0BCB5,
  0x391C0CB3, 0x4ED8AA4A, 0x5B9CCA4F, 0x682E6FF3,
  0x748F82EE, 0x78A5636F, 0x84C87814, 0x8CC70208,
  0x90BEFFFA, 0xA4506CEB, 0xBEF9A3F7, 0xC67178F2,
};

#define  SHR(x, n) (((x) & 0xFFFFFFFF) >> (n))
#define ROTR(x, n) (SHR(x, n) | ((x) << (32 - (n))))

#define S0(x) (ROTR(x, 7) ^ ROTR(x, 18) ^  SHR(x, 3))
#define S1(x) (ROTR(x, 17) ^ ROTR(x, 19) ^  SHR(x, 10))

#define S2(x) (ROTR(x, 2) ^ ROTR(x, 13) ^ ROTR(x, 22))
#define S3(x) (ROTR(x, 6) ^ ROTR(x, 11) ^ ROTR(x, 25))

#define F0(x, y, z) (((x) & (y)) | ((z) & ((x) | (y))))
#define F1(x, y, z) ((z) ^ ((x) & ((y) ^ (z))))

#define R(t) \
  ( \
    local.W[t] = S1(local.W[(t) -  2]) + local.W[(t) -  7] +  \
                 S0(local.W[(t) - 15]) + local.W[(t) - 16]    \
  )

#define P(a, b, c, d, e, f, g, h, x, K)                         \
  do                                                            \
  {                                                             \
    local.temp1 = (h) + S3(e) + F1((e), (f), (g)) + (K) + (x);  \
    local.temp2 = S2(a) + F0((a), (b), (c));                    \
    (d) += local.temp1; (h) = local.temp1 + local.temp2;        \
  } while (0)

int32_t mbedtls_internal_sha256_process(mbedtls_sha256_context_t *ctx,
                                        const unsigned char data[SHA256_BLOCK_SIZE])
{
  struct
  {
    uint32_t temp1;
    uint32_t temp2;
    uint32_t W[64];
    uint32_t A[8];
  } local;

  uint32_t i;

  for (i = 0; i < 8; i++)
  {
    local.A[i] = ctx->state[i];
  }

  for (i = 0; i < 16; i++)
  {
    local.W[i] = GET_UINT32_BE(data, 4 * i);
  }

  for (i = 0; i < 16; i += 8)
  {
    P(local.A[0], local.A[1], local.A[2], local.A[3], local.A[4],
      local.A[5], local.A[6], local.A[7], local.W[i + 0], K[i + 0]);
    P(local.A[7], local.A[0], local.A[1], local.A[2], local.A[3],
      local.A[4], local.A[5], local.A[6], local.W[i + 1], K[i + 1]);
    P(local.A[6], local.A[7], local.A[0], local.A[1], local.A[2],
      local.A[3], local.A[4], local.A[5], local.W[i + 2], K[i + 2]);
    P(local.A[5], local.A[6], local.A[7], local.A[0], local.A[1],
      local.A[2], local.A[3], local.A[4], local.W[i + 3], K[i + 3]);
    P(local.A[4], local.A[5], local.A[6], local.A[7], local.A[0],
      local.A[1], local.A[2], local.A[3], local.W[i + 4], K[i + 4]);
    P(local.A[3], local.A[4], local.A[5], local.A[6], local.A[7],
      local.A[0], local.A[1], local.A[2], local.W[i + 5], K[i + 5]);
    P(local.A[2], local.A[3], local.A[4], local.A[5], local.A[6],
      local.A[7], local.A[0], local.A[1], local.W[i + 6], K[i + 6]);
    P(local.A[1], local.A[2], local.A[3], local.A[4], local.A[5],
      local.A[6], local.A[7], local.A[0], local.W[i + 7], K[i + 7]);
  }

  for (i = 16; i < 64; i += 8)
  {
    P(local.A[0], local.A[1], local.A[2], local.A[3], local.A[4],
      local.A[5], local.A[6], local.A[7], R(i + 0), K[i + 0]);
    P(local.A[7], local.A[0], local.A[1], local.A[2], local.A[3],
      local.A[4], local.A[5], local.A[6], R(i + 1), K[i + 1]);
    P(local.A[6], local.A[7], local.A[0], local.A[1], local.A[2],
      local.A[3], local.A[4], local.A[5], R(i + 2), K[i + 2]);
    P(local.A[5], local.A[6], local.A[7], local.A[0], local.A[1],
      local.A[2], local.A[3], local.A[4], R(i + 3), K[i + 3]);
    P(local.A[4], local.A[5], local.A[6], local.A[7], local.A[0],
      local.A[1], local.A[2], local.A[3], R(i + 4), K[i + 4]);
    P(local.A[3], local.A[4], local.A[5], local.A[6], local.A[7],
      local.A[0], local.A[1], local.A[2], R(i + 5), K[i + 5]);
    P(local.A[2], local.A[3], local.A[4], local.A[5], local.A[6],
      local.A[7], local.A[0], local.A[1], R(i + 6), K[i + 6]);
    P(local.A[1], local.A[2], local.A[3], local.A[4], local.A[5],
      local.A[6], local.A[7], local.A[0], R(i + 7), K[i + 7]);
  }

  for (i = 0; i < 8; i++)
  {
    ctx->state[i] += local.A[i];
  }

  /* Zeroise buffers and variables to clear sensitive data from memory. */
  mbedtls_platform_zeroize(&local, sizeof(local));

  return 0;
}

size_t mbedtls_internal_sha256_process_many(mbedtls_sha256_context_t *ctx, const uint8_t *data, size_t len)
{
  size_t processed = 0;

  while (len >= SHA256_BLOCK_SIZE)
  {
    if (mbedtls_internal_sha256_process(ctx, data) != 0)
    {
      return 0;
    }

    data += SHA256_BLOCK_SIZE;
    len  -= SHA256_BLOCK_SIZE;

    processed += SHA256_BLOCK_SIZE;
  }

  return processed;
}

/*
 * SHA-256 process buffer
 */
int32_t mbedtls_sha256_update(mbedtls_sha256_context_t *ctx, const unsigned char *input, size_t ilen)
{
  int32_t ret = -1;
  size_t fill;
  uint32_t left;

  if (ilen == 0)
  {
    return 0;
  }

  left = ctx->total[0] & 0x3F;
  fill = SHA256_BLOCK_SIZE - left;

  ctx->total[0] += (uint32_t) ilen;
  ctx->total[0] &= 0xFFFFFFFF;

  if (ctx->total[0] < (uint32_t) ilen)
  {
    ctx->total[1]++;
  }

  if (left && ilen >= fill)
  {
    memcpy((void *)(ctx->buffer + left), input, fill);

    if ((ret = mbedtls_internal_sha256_process(ctx, ctx->buffer)) != 0)
    {
      return ret;
    }

    input += fill;
    ilen  -= fill;
    left = 0;
  }

  while (ilen >= SHA256_BLOCK_SIZE)
  {
    size_t processed =
      mbedtls_internal_sha256_process_many(ctx, input, ilen);
    if (processed < SHA256_BLOCK_SIZE)
    {
      return -1;
    }

    input += processed;
    ilen  -= processed;
  }

  if (ilen > 0)
  {
    memcpy((void *)(ctx->buffer + left), input, ilen);
  }

  return 0;
}

/*
 * SHA-256 final digest
 */
int32_t mbedtls_sha256_finish(mbedtls_sha256_context_t *ctx, unsigned char *output)
{
  int32_t ret = -1;
  uint32_t used;
  uint32_t high;
  uint32_t low;

  /*
   * Add padding: 0x80 then 0x00 until 8 bytes remain for the length
   */
  used = ctx->total[0] & 0x3F;

  ctx->buffer[used++] = 0x80;

  if (used <= 56)
  {
    /* Enough room for padding + length in current block */
    memset(ctx->buffer + used, 0, 56 - used);
  }
  else
  {
    /* We'll need an extra block */
    if (used < SHA256_BLOCK_SIZE)
    {
      memset(ctx->buffer + used, 0, SHA256_BLOCK_SIZE - used);
    }

    if ((ret = mbedtls_internal_sha256_process(ctx, ctx->buffer)) != 0)
    {
      goto exit;
    }

    memset(ctx->buffer, 0, 56);
  }

  /*
   * Add message length
   */
  high = (ctx->total[0] >> 29)
         | (ctx->total[1] <<  3);
  low  = (ctx->total[0] <<  3);

  PUT_UINT32_BE(high, ctx->buffer, 56);
  PUT_UINT32_BE(low,  ctx->buffer, 60);

  if ((ret = mbedtls_internal_sha256_process(ctx, ctx->buffer)) != 0)
  {
    goto exit;
  }

  /*
   * Output final state
   */
  PUT_UINT32_BE(ctx->state[0], output,  0);
  PUT_UINT32_BE(ctx->state[1], output,  4);
  PUT_UINT32_BE(ctx->state[2], output,  8);
  PUT_UINT32_BE(ctx->state[3], output, 12);
  PUT_UINT32_BE(ctx->state[4], output, 16);
  PUT_UINT32_BE(ctx->state[5], output, 20);
  PUT_UINT32_BE(ctx->state[6], output, 24);
  PUT_UINT32_BE(ctx->state[7], output, 28);

  ret = 0;

exit:
  mbedtls_sha256_free(ctx);
  return ret;
}
