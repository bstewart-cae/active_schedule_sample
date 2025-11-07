/***********************************************************************************************************************
 * @file     aes.h
 * @version
 * @brief    AES driver header file
 *
 * @copyright
 **********************************************************************************************************************/
#ifndef _RT584_AES_H_
#define _RT584_AES_H_
/***********************************************************************************************************************
 *    INCLUDES
 **********************************************************************************************************************/
#include <stdint.h>
#include "status.h"

#ifdef __cplusplus
extern "C"
{
#endif


/**
 *  @Brief AES BLOCK LENGTH Constant Definitions
 */

#define AES_BLOCKLEN              16     /*!< Block length in bytes, AES is 128bytes per block  \hideinitializer */

/**
 *  @Brief AES_KEY SIZE Constant Definitions
 */
#define  AES_KEY128               128    /*!< AES key size is 128bits  \hideinitializer */
#define  AES_KEY192               192    /*!< AES key size is 192bits  \hideinitializer */
#define  AES_KEY256               256    /*!< AES key size is 256bits  \hideinitializer */


#define  NONCE_LEN                13


#define AES_Enc_inst_index        (16)
#define AES_Dec_inst_index        (64)
#define AES_CBC_Enc_inst_index    (120)
#define AES_CTR_inst_index        (160)


#define AES_Round_Key_Size        240    /*!< for AES256, the maximum round key size is 240. \hideinitializer */

#define AES_keyExpSize            AES_Round_Key_Size

/**
 *@brief  AES ENCRYPTION DECRYPTION operation definitions.
 */

typedef  enum
{
    AES_CRYPTO_DECRYPT = 0,     /*!< AES operation for decryption  \hideinitializer */
    AES_CRYPTO_ENCRYPT = 1,     /*!< AES operation for encryption  \hideinitializer */
} crypto_operation_t;


/**
 *@brief  Structure for the AES Context definition.
 *
 */

struct aes_ctx
{
    uint8_t RoundKey[AES_keyExpSize];       /*!< AES roud key */

    uint8_t Iv[AES_BLOCKLEN];               /*!< AES initialization vecotr buffer*/

    uint8_t round;                          /*!< The number of rounds */
};


/**
 *@brief  Structure for the AES CCM packet structure definition.
 *
 *@details  This aes_ccm_encryption_packet only supports length field is 2 bytes
 *
 *           Please notice: hdr_len+data_len must be less than 480 bytes.
 *
 */

struct aes_ccm_encryption_packet
{
    uint8_t   *nonce;      /*!< Nonce N -- Here it must be 13 bytes.*/

    uint8_t   *hdr;        /*!< Additional data authenticated but not encrypted*/
    uint32_t  hdr_len;     /*!< data length for authenticated but not encrypted*/

    uint8_t   *data;       /*!< message to be encrypted*/
    uint32_t  data_len;    /*!< data length for message to be encrypted */

    uint32_t  mlen;        /*!< mac length, valid value are 4,6,8,10,12,14,16 */

    uint8_t   *out_buf;       /*!< ccm output message buffer*/
    uint32_t  *out_buf_len;   /*!< number bytes of ccm output message buffer.*/

};


/**
 *@brief  Structure for the AES CCM decryption packet structure definition.
 *
 *@details  This aes_ccm_decryption_packet only supports length field is 2 bytes
 *
 *           Please notice: hdr_len+data_len must be less than 480 bytes.
 *
 */

struct aes_ccm_decryption_packet
{
    uint8_t   *payload_buf;      /*!< ccm payload buffer for decryption and verification*/
    uint32_t  payload_length;    /*!< ccm payload buffer bytes length */

    uint8_t   *nonce;            /*!< Nonce N -- Here it must be 13 bytes.*/

    uint32_t  hdr_len;           /*!< data length for authenticated but not encrypted*/

    uint32_t  data_len;          /*!< data length for message to be encrypted */

    uint32_t  mlen;              /*!< mac length, valid value are 4,6,8,10,12,14,16 */

    uint8_t   *out_buf;          /*!< ccm output message buffer for decryption */
    uint32_t  *out_buf_len;      /*!< number bytes of ccm output message buffer of decryption.*/

};


/**
 * @brief Set Crypto Engine to load AES encryption/decryption algorithm.
 *
 * @param[in]  None
 *
 * @details   Crypto Engine needs to load crypto algorithm before doing crypto operation.
 *           Only initialize the algorithm once before aes encryption/decryption.
 *           Crypto Engine supports encryption and decryption switch without initial again.
 *           But when system uses crypto engine to do other algorithm, like ECC, the
 *           system should load the crypto algorithm to do that specified operation.
 *           After that, if system wants to use crypto engine to do aes operation again, then
 *           it should call aes_fw_init() function to load AES algorithm again.
 *
 */

void aes_fw_init(void);

/**
 * @brief Init AES  key expansion
 *
 * @param[in]  ctx        The AES context to initialize.
 * @param[in]  key        key for encryption or decryption
 * @param[in]  keybits    must be AES_KEY128, AES_KEY192, or AES_KEY256
 *
 * @details
 *       This is software function to expansion aes key to aes round key.
 *       Please be notice expansion key size is 240 bytes in CTX.
 *       Save expansion key in CTX, so system can switch different CTX for different endpoint.
 *
 */
uint32_t aes_key_init(struct aes_ctx *ctx, const uint8_t *key, uint32_t keybits);

/**
 * @brief Load AES expansion round key to engine
 *
 * @param[in]  ctx        The AES context
 *
 * @details
 *     Load expansion round key to engine.
 *
 */

void aes_load_round_key(struct aes_ctx *ctx);

/**
 * @brief This function performs an AES single-block encryption operation
 *
 * @param[in]  ctx        The AES context to use for encryption
 * @param[in]  in_buf     The 16-byte buffer holding the input data.
 * @param[out] out_buf    The 16-byte buffer holding the output data
 *
 * @details
 *       It performs the encryption operation on input data buffer defined in the in_buf
 *       This function is block function. that is, this function will return when
 *       crypto engine finish the encryption.
 *
 */
uint32_t aes_ecb_encrypt(struct aes_ctx *ctx, uint8_t *in_buf, uint8_t *out_buf);


/**
 * @brief This function performs an AES single-block decryption operation
 *
 * @param[in]  ctx        The AES context to use for decryption
 * @param[in]  in_buf     The 16-byte buffer holding the input data.
 * @param[out] out_buf    The 16-byte buffer holding the output data
 *
 * @details
 *       It performs the decryption operation on input data buffer defined in the in_buf
 *       This function is block function. that is, this function will return when
 *       crypto engine finish the decryption.
 *
 */

uint32_t aes_ecb_decrypt(struct aes_ctx *ctx, uint8_t *in_buf, uint8_t *out_buf);

/**
 * @brief This function release AES crypto engine
 *
 * @param[in]  ctx    AES context
 *
 * @return    STATUS_SUCCESS
 *
 * @details   This function only used in multi-tasking OS.
 *             If the task acquired the hardware engine before, it should call this function
 *             to release the resource.
 *             In single thread environment (FSM), aes_release function always return  STATUS_SUCCESS
 *             Notice: semaphore protect function depends on OS implemented.
 *
 */
uint32_t aes_release(struct aes_ctx *ctx);


/**
 * @brief This function acquire AES crypto engine
 *
 * @param[in]  ctx    AES context
 *
 * @return    STATUS_SUCCESS
 *
 * @details   This function only used in multi-tasking OS.
 *             Only one AES encryption/decryption request can be serviced at any time.
 *             To avoid more than two tasks request the hardware engine resource at the same time,
 *             there should be a semaphore protect to protect the hardware request run one by one.
 *             In single thread environment (FSM), aes_acquire() function is always return STATUS_SUCCESS
 *             Notice: semaphore protect function depends on OS implemented.
 *
 *
 */
uint32_t aes_acquire(struct aes_ctx *ctx);

/**
 * @brief     xorwith buffer1 and buffer2
 *
 * @param[out]  result  the buffer to hold the result
 * @param[in]   buf1
 * @param[in]   buf2
 *
 * @details
 *             result[i] = buf1[i] ^ buf2[i];
 *
 */
void aes_xorwith(uint32_t *result, uint32_t *buf1, const uint32_t *buf2);

/**
 * @brief     Set initialization vector for encryption or decryption.
 *
 * @param[in]  ctx  The AES context to use for encryption or decryption.
 * @param[in]  iv   The initialization vector.
 *
 * @details
 *             Some AES mode, like CBC or CTR, need to set IV before encryption.
 *             This function provide user to update IV before operation.
 *
 */

uint32_t aes_iv_set(struct aes_ctx *ctx, const uint8_t *iv);


/**
 * @brief     Get initialization vector for encryption or decryption.
 *
 * @param[in]  ctx  The AES context to use for encryption or decryption.
 * @param[out] iv   Buffer for saving initialization vector result.
 *
 * @details
 *             Many AES mode, like CTR or CBC mode, need to set IV before encryption.
 *             This function provide user to get IV result.
 *
 */
uint32_t aes_iv_get(struct aes_ctx *ctx, uint8_t  *iv);

/**
 * @brief     This function performs an AES-CBC encryption operation on full blocks.
 *
 * @param[in]  ctx        The AES context to use for encryption.
 * @param[in]  in_buf     The buffer holding the input data.
 * @param[out] out_buf    The buffer holding the output data.
 * @param[in]  length     The length of the input data in Bytes. This must be a multiple for the block size (16 Bytes).
 *
 * @return
 *             STATUS_INVALID_PARAM    if length is 0 or not 16*N bytes
 *             STATUS_SUCCESS
 * @details
 *             This function perfroms AES CBC mode. It is block function, that is,
 *             this function will return when all data has been encrypted.
 *
 *
 */

uint32_t aes_cbc_buffer_encrypt(struct aes_ctx *ctx, uint8_t *in_buf, uint8_t *out_buf, uint32_t length);

/**
 * @brief     This function performs an AES-CBC decryption operation on full blocks.
 *
 * @param[in]  ctx        The AES context to use for decryption.
 * @param[in]  in_buf     The buffer holding the input data.
 * @param[out] out_buf    The buffer holding the output data.
 * @param[in]  length     The length of the input data in Bytes. This must be a multiple for the block size (16 Bytes).
 *
 * @return
 *             STATUS_INVALID_PARAM    if length is 0 or not 16*N bytes
 *             STATUS_SUCCESS
 * @details
 *             This function perfroms AES CBC mode. It is block function, that is,
 *             this function will return when all data has been decrypted.
 *
 *
 */

uint32_t aes_cbc_buffer_decrypt(struct aes_ctx *ctx, uint8_t *in_buf, uint8_t *out_buf, uint32_t length);

/**
 * @brief     This function performs an AES-CRT cryption operation
 *
 * @param[in]   ctx           The AES context to use for cryption.
 * @param[in]   in_buf        The buffer holding the input data.
 * @param[out]  out_buf       The buffer holding the output data.
 * @param[out]  stream_block  The saved stream block fir resuming. This is overwritten by the function.
 * @param[in]   nc_offset     The offset in the current stream_block, for resuming within the current cipher steam.
 *                            The offset pointer should be 0 at the start if a stream.
 * @param[in]   length        The length of the input data
 *
 * @return
 *             STATUS_INVALID_PARAM
 *             STATUS_SUCCESS
 * @details
 *             This function perfroms AES CBC mode. It is block function, that is,
 *             this function will return when all data has been decrypted.
 *
 *
 */


uint32_t aes_ctr_buffer_crypt(struct aes_ctx *ctx,
                              uint8_t   *in_buf,
                              uint8_t   *out_buf,
                              uint8_t   *stream_block,
                              uint32_t  *nc_offset,
                              uint32_t  length );

/**
 * @brief
 *
 * @param[in]
 *
 * @details
 *
 *
 */
uint32_t aes_ccm_encryption(struct aes_ccm_encryption_packet  *ccm_packet);
/**
* @brief
*
* @param[in]
*
* @details
*
*
*/
uint32_t aes_ccm_decryption_verification(struct aes_ccm_decryption_packet  *ccm_packet);




#ifdef __cplusplus
}
#endif

#endif //___AES_H_
