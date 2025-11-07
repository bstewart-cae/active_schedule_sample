
/*
 *  CTR_DRBG implementation based on AES-256 (NIST SP 800-90)
 *
 *  Copyright The Mbed TLS Contributors
 *  SPDX-License-Identifier: Apache-2.0
 *
 *  Licensed under the Apache License, Version 2.0 (the "License"); you may
 *  not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 *  WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */
/*
 *  The NIST SP 800-90 DRBGs are described in the following publication.
 *
 *  http://csrc.nist.gov/publications/nistpubs/800-90/SP800-90revised_March2007.pdf
 */

#ifndef __INCLUDE_CTR_DRBG_DEFINE__
#define __INCLUDE_CTR_DRBG_DEFINE__

#include "crypto.h"


#ifdef __cplusplus
extern "C"
{
#endif

#define CTR_DRBG_BLOCKSIZE          16      /**< Block size used by the cipher */


#ifndef  CTR_DRBG_USE_256_BIT_KEY

#define CTR_DRBG_KEYSIZE            16      /**< Key size used by the cipher  */
#define CTR_DRBG_ENTROPY_LEN        32      /**< Amount of entropy used per seed by default */
#define CTR_DRBG_KEYSIZE_BIT        128

#else

#define CTR_DRBG_KEYSIZE            32      /**< Key size used by the cipher  */
#define CTR_DRBG_ENTROPY_LEN        48      /**< Amount of entropy used per seed by default */
#define CTR_DRBG_KEYSIZE_BIT        256

#endif

#define CTR_DRBG_KEYBITS            ( CTR_DRBG_KEYSIZE * 8 )
#define CTR_DRBG_SEEDLEN            ( CTR_DRBG_KEYSIZE + CTR_DRBG_BLOCKSIZE )     /**< The seed length (counter + AES key)   */


#define CTR_DRBG_RESEED_INTERVAL    10000   /**< Interval before reseed is performed by default */

/*
 * Notice the following setting will allocate operation buffer in stack,
 * In mcu, it can NOT use a "large" number setting like PC-based system.
 * In fact, in real usage, the following setting is small number, too.
 *
 */
#define CTR_DRBG_MAX_INPUT          64      /**< Maximum number of additional input bytes */
#define CTR_DRBG_MAX_REQUEST        64      /**< Maximum number of requested bytes per call */
#define CTR_DRBG_MAX_SEED_INPUT     128     /**< Maximum size of (re)seed buffer */

#define CTR_DRBG_PR_OFF             0       /**< No prediction resistance       */
#define CTR_DRBG_PR_ON              1       /**< Prediction resistance enabled  */


/**
 * \brief          CTR_DRBG context structure
 */
typedef struct
{
    uint8_t  counter[16];       /*!<  counter (V)       */
    uint32_t reseed_counter;    /*!<  reseed counter    */

    uint32_t reseed_interval;   /*!<  reseed interval   */

    struct aes_ctx   aes_cntx;  /*!<  AES context,
                                 PLEASE NOTICE this structure will use large memory
                                 --it include AES extension key buffer  */
    /*
     * Callbacks (Entropy)
     */
    uint32_t (*f_entropy)(void *, uint8_t *, uint32_t);       /*!<  the entropy callback function.
                                                                NULL for using default TRNG to generate entropy data.
                                                                */

    void *p_entropy;            /*!<  context for the entropy function */

    uint8_t prediction_resistance;  /*!<  enable prediction resistance (Automatic
                                      reseed before every random generation)        */

    uint8_t entropy_len;         /*!<  amount of entropy grabbed on each (re)seed    */
} ctr_drbg_context;


/**
 * \brief               CTR_DRBG initialization
 *
 * \note                To achieve the nominal security strength permitted
 *                      by CTR_DRBG, the entropy length must be:
 *                      - at least 16 bytes for a 128-bit strength
 *                      (maximum achievable strength when using AES-128);
 *                      - at least 32 bytes for a 256-bit strength
 *                      (maximum achievable strength when using AES-256).
 *
 *                      In addition, if you do not pass a nonce in \p custom,
 *                      the sum of the entropy length
 *                      and the entropy nonce length must be:
 *                      - at least 32 bytes for a 128-bit strength
 *                      (maximum achievable strength when using AES-128);
 *                      - at least 48 bytes for a 256-bit strength
 *                      (maximum achievable strength when using AES-256).
 *
 *                      Please see NIST SP 800-90A 10.2.1
 *
 *
 * \param ctx           CTR_DRBG context to be initialized
 * \param f_entropy     Entropy callback (p_entropy, buffer to fill, buffer
 *                      length)
 * \param p_entropy     Entropy context
 * \param custom        Personalization data (Device specific identifiers)
 *                      (Can be NULL)
 * \param len           Length of personalization data
 *
 *
 * \return              STATUS_SUCCESS if successful, or
 *                       STATUS_INVALID_PARAM if input parameter error
 *
 */
uint32_t ctr_drbg_init( ctr_drbg_context *ctx,
                        uint32_t (*f_entropy)(void *, unsigned char *, size_t),
                        void *p_entropy,
                        const uint8_t *custom,
                        uint32_t len);

/**
 * \brief               CTR_DRBG reseeding (extracts data from entropy source)
 *
 * \param ctx           CTR_DRBG context
 * \param additional    Additional data to add to state (Can be NULL)
 * \param len           Length of additional data
 *
 * \return              STATUS_SUCCESS if successful, or
 *                      STATUS_INVALID_PARAM --- For input data is over system default size
 *                      STATUS_ERROR --- Entropy function return error.
 */

uint32_t ctr_drbg_reseed( ctr_drbg_context *ctx,
                          const uint8_t *additional, uint32_t len );

/**
 * \brief               Enable / disable prediction resistance (Default: Off)
 *
 * Note: If enabled, entropy is used for ctx->entropy_len before each call!
 *       Only use this if you have ample supply of good entropy!
 *
 * \param ctx           CTR_DRBG context
 * \param resistance    CTR_DRBG_PR_ON or CTR_DRBG_PR_OFF
 */
void ctr_drbg_set_prediction_resistance( ctr_drbg_context *ctx,
        uint8_t resistance );

/**
 * \brief               CTR_DRBG generate random with additional update input
 *
 * Note: Automatically reseeds if reseed_counter is reached.
 *
 * \param p_rng         CTR_DRBG context
 * \param output        Buffer to fill
 * \param output_len    Length of the buffer
 * \param additional    Additional data to update with (Can be NULL)
 * \param add_len       Length of additional data
 *
 * \return              STATUS_SUCCESS if successful, or
 *                      STATUS_INVALID_PARAM --- For input data is over system default size
 *                      STATUS_ERROR
 */
uint32_t ctr_drbg_random_with_add( void *p_rng,
                                   uint8_t *output, uint32_t output_len,
                                   const uint8_t *additional, uint32_t add_len );


/**
 * \brief   This function updates a CTR_DRBG instance with additional
 *          data and uses it to generate random data.
 *
 * This function automatically reseeds if the reseed counter is exceeded
 * or prediction resistance is enabled.
 *
 * \note                This function is not thread-safe. It is not safe
 *                      to call this function if another thread might be
 *                      concurrently obtaining random numbers from the same
 *                      context or updating or reseeding the same context.
 *
 * \param p_rng         The CTR_DRBG context. This must be a pointer to a
 *                      ctr_drbg_context structure.
 *
 * \param output        The buffer to fill.
 * \param output_len    The length of the buffer in bytes.
 * \param additional    Additional data to update. Can be \c NULL, in which
 *                      case the additional data is empty regardless of
 *                      the value of \p add_len.
 * \param add_len       The length of the additional data
 *                      if \p additional is not \c NULL.
 *                      This must be less than CTR_DRBG_MAX_INPUT
 *                      and less than
 *                      CTR_DRBG_MAX_SEED_INPUT - \c entropy_len
 *                      where \c entropy_len is the entropy length
 *                      configured for the context.
 *
 * \return             STATUS_SUCCESS if successful, or
 *                     STATUS_INVALID_PARAM --- For input data is over system default size
 *                     STATUS_ERROR, or
 *
 *
 *
 */
uint32_t  ctr_drbg_random( void *p_rng,
                           uint8_t *output, uint32_t output_len );




#ifdef __cplusplus
extern "C"
}
#endif

#endif

