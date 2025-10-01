/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    sha256.h
  * @author  GPM Application Team
  * @brief   This file is part of mbed TLS and used for sha256 computation.
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

/**
  * Portions of this file are based on mbed TLS, which is licensed under the Apache-2.0 license as indicated below.
  * It was taken from the mbed TLS version 3.6.2 and modified for use in this project.
  *
  * Reference source:
  * https://github.com/Mbed-TLS/mbedtls/blob/v3.6.2/include/mbedtls/sha256.h
  */

/**
  * The Secure Hash Algorithms 224 and 256 (SHA-224 and SHA-256) cryptographic
  * hash functions are defined in <em>FIPS 180-4: Secure Hash Standard (SHS)</em>.
  */
/*
 *  Copyright The Mbed TLS Contributors
 *  SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later
 */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef MBEDTLS_SHA256_H
#define MBEDTLS_SHA256_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Includes ------------------------------------------------------------------*/
#include <stddef.h>
#include <stdint.h>

/**
  * \brief          The SHA-256 context structure.
  *
  *                 The structure is used both for SHA-256 and for SHA-224
  *                 checksum calculations. The choice between these two is
  *                 made in the call to mbedtls_sha256_starts().
  */
typedef struct
{
  unsigned char buffer[64];   /*!< The data block being processed. */
  uint32_t total[2];          /*!< The number of Bytes processed.  */
  uint32_t state[8];          /*!< The intermediate digest state.  */
} mbedtls_sha256_context_t;

/**
  * \brief          This function initializes a SHA-256 context.
  *
  * \param ctx      The SHA-256 context to initialize. This must not be \c NULL.
  */
void mbedtls_sha256_init(mbedtls_sha256_context_t *ctx);

/**
  * \brief          This function clears a SHA-256 context.
  *
  * \param ctx      The SHA-256 context to clear. This may be \c NULL, in which
  *                 case this function returns immediately. If it is not \c NULL,
  *                 it must point to an initialized SHA-256 context.
  */
void mbedtls_sha256_free(mbedtls_sha256_context_t *ctx);

/**
  * \brief          This function clones the state of a SHA-256 context.
  *
  * \param dst      The destination context. This must be initialized.
  * \param src      The context to clone. This must be initialized.
  */
void mbedtls_sha256_clone(mbedtls_sha256_context_t *dst,
                          const mbedtls_sha256_context_t *src);

/**
  * \brief          This function starts a SHA-224 or SHA-256 checksum
  *                 calculation.
  *
  * \param ctx      The context to use. This must be initialized.
  *
  * \return         \c 0 on success.
  * \return         A negative error code on failure.
  */
int32_t mbedtls_sha256_starts(mbedtls_sha256_context_t *ctx);

/**
  * \brief          This function feeds an input buffer into an ongoing
  *                 SHA-256 checksum calculation.
  *
  * \param ctx      The SHA-256 context. This must be initialized
  *                 and have a hash operation started.
  * \param input    The buffer holding the data. This must be a readable
  *                 buffer of length \p ilen Bytes.
  * \param ilen     The length of the input data in Bytes.
  *
  * \return         \c 0 on success.
  * \return         A negative error code on failure.
  */
int32_t mbedtls_sha256_update(mbedtls_sha256_context_t *ctx,
                              const unsigned char *input,
                              size_t ilen);

/**
  * \brief          This function finishes the SHA-256 operation, and writes
  *                 the result to the output buffer.
  *
  * \param ctx      The SHA-256 context. This must be initialized
  *                 and have a hash operation started.
  * \param output   The SHA-224 or SHA-256 checksum result.
  *                 This must be a writable buffer of length \c 32 bytes
  *                 for SHA-256, \c 28 bytes for SHA-224.
  *
  * \return         \c 0 on success.
  * \return         A negative error code on failure.
  */
int32_t mbedtls_sha256_finish(mbedtls_sha256_context_t *ctx,
                              unsigned char *output);

/**
  * \brief          This function processes a single data block within
  *                 the ongoing SHA-256 computation. This function is for
  *                 internal use only.
  *
  * \param ctx      The SHA-256 context. This must be initialized.
  * \param data     The buffer holding one block of data. This must
  *                 be a readable buffer of length \c 64 Bytes.
  *
  * \return         \c 0 on success.
  * \return         A negative error code on failure.
  */
int32_t mbedtls_internal_sha256_process(mbedtls_sha256_context_t *ctx,
                                        const unsigned char data[64]);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* mbedtls_sha256.h */
