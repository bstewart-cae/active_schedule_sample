/***********************************************************************************************************************
 * @file     hmac_drbg.h
 * @version
 * @brief
 *
 * @copyright
 **********************************************************************************************************************/
#ifndef _RT584_SHA256_H_
#define _RT584_SHA256_H_


#define  SHA256_DIGEST_SIZE                32
#define  SHA256_BLOCK_SIZE                 64
#define  SHA256_CAL_MAX_SIZE               0x10000
#define  SHA224_DIGEST_SIZE                28

#ifdef __cplusplus
extern "C"
{
#endif
/***********************************************************************************************************************
 *    TYPEDEFS
 **********************************************************************************************************************/
typedef struct
{
    uint32_t total[2];
    uint8_t buffer[SHA256_BLOCK_SIZE];
} sha256_context;

typedef struct
{
    uint32_t total[2];
    uint8_t  buffer[SHA256_BLOCK_SIZE];
    uint8_t  opad[SHA256_BLOCK_SIZE];
} hmac_sha256_context;

/* According to RFC5869, L (length of output keying material in octets)
 * should be <= 255*HashLen
 * But in CM33 mcu system, 255*32 is too large. It is unrealistic.
 * Here we limit the HKDF block to 32. Even this setting 32*32= 1024
 * It is enough most case.
 */
#define HKDF_limit_block    32

typedef struct
{
    uint8_t *out_key;
    uint32_t out_len;

    const uint8_t *secret;
    uint32_t secret_len;

    const uint8_t *salt;
    uint32_t salt_len;

    const uint8_t *info;
    uint32_t info_len;

} hkdf_context;

typedef struct pbkdf2_struct
{
    uint8_t   *password;           /*!< pbkdf2 password */
    uint32_t  password_length;     /*!< pbkdf2 password  length*/

    uint8_t   *salt;               /*!< pbkdf2 salt*/
    uint32_t  salt_length;         /*!< pbkdf2 salt  length*/

    uint32_t  iteration;           /*!< pbkdf2 iteration  --- according matter, this value must between 1000~100000*/

    uint32_t  key_length;          /*!< pbkdf2 output key length*/
    uint8_t   *key_output;         /*!< pbkdf2 generted key output*/

} pbkdf2_st;
/***********************************************************************************************************************
 *    GLOBAL PROTOTYPES
 **********************************************************************************************************************/
/**
 * @brief Set Crypto accelerator to load SHA-256 hash algorithm.
 *
 * @param[in]  ctx    point to sha256_context
 *
 * @return   None
 *
 * @details  Crypto accelerator needs to load sha256 firmware before doing
 *           sha operation.
 *           This initialize function must be called every time before starting
 *           a new sha256 caculation.
 *           For multi-tasking environment, this sha256_init will auto
 *           acquire hardware accelerator resource semaphore. It suggests to
 *           avoid this case.
 *
 * @remark   hardware accelerator resource will be released in function
 *           sha256_finish, so it can NOT call any other secure function before
 *           finishing sha256 caculation.
 *
 */
void sha256_init(sha256_context *ctx);
/**
 * @brief Using Crypto accelerator to caculate SHA-256 hash algorithm for new add data.
 *
 * @param[in]  ctx     sha256_context
 * @param[in]  input   point to input buffer for sha256 caculation
 * @param[in]  length  number of input buffer
 *
 *
 * @retval
 *             return STATUS_ERROR    for length is zero or accelerator is not in
 *                                    SHA-256 hash mode
 *
 *             return STATUS_SUCCESS
 *
 * @details  Caculate SHA-256 hash value for new add data
 *
 *
 * @remark   hardware accelerator resource will be released in function
 *           sha256_finish, so it can NOT call any other secure function before
 *           finishing sha256 caculation.
 *
 */

uint32_t sha256_update(sha256_context *ctx, uint8_t *input, uint32_t length);
/**
 * @brief Get the SHA-256 final hash digest value.
 *
 * @param[in]   ctx     point to sha256_context
 * @param[out]  digest  point to the buffer to get the final hash value, the buffer
 *                      mute be 32 bytes.
 *
 *
 * @details  Crypto accelerator needs to load sha256 hash algorithm before
 *           doing crypto operation.
 *           For multi-tasking environment, this sha256_finish will auto
 *           release hardware accelerator resource semaphore.
 *
 *
 */

void sha256_finish(sha256_context *ctx, uint8_t *digest);
/**
 * @brief sh256 calcation function.
 *
 * @param[in]  input    point to input buffer for sha256 caculation
 * @param[in]  length   length  number of input buffer
 * @param[out]  digest  point to the buffer to get the final hash value, the buffer
 *                      mute be 32 bytes.
 *
 *
 * @details  Crypto accelerator needs to load sha256 hash algorithm before
 *           doing crypto operation.
 *           For multi-tasking environment, this sha256_finish will auto
 *           release hardware accelerator resource semaphore.
 *
 *
 */

void sha256(uint8_t *input, uint32_t length, uint8_t *digest);
/**
 * @brief Set Crypto accelerator to load SHA-224 hash algorithm.
 *
 * @param[in]  ctx    point to sha256_context
 *
 * @retval   None
 *
 * @details  Crypto accelerator needs to load sha224 firmware before doing
 *           sha operation.
 *           This initialize function must be called every time before starting
 *           a new sha224 caculation.
 *           For multi-tasking environment, this sha224_init will auto
 *           acquire hardware accelerator resource semaphore. It should
 *           avoid this case.
 *
 * @remark   hardware accelerator resource will be released in function
 *           sha256_finish, so it can NOT call any other secure function before
 *           finishing sha224 caculation.
 *           In fact, sha224 firmware is almost the same as sha256, the main
 *           difference is that sha224 only using 28 bytes digest.
 *           So system can still use sha256_update/sha256_update to caculate
 *           sha224 digest, just notice digest is 28 bytes in sha224.
 *
 */

void sha224_init(sha256_context *ctx);
/*
 * @brief HMAC Keyed-Hashing for Message Authentication API
 *        This function is raw version API for HMAC, that is, all message
 *        data already assigned in the function. If there are another
 *        append message buffer later, please use function "hmac_sha256_init_ex".
 *
 *
 * @param[in]  key          point to key buffer
 * @param[in]  key_length   number of bytes for key buffer data
 * @param[in]  msg          point to message buffer for authentication
 * @param[in]  msg_length   number of bytes for msg buffer data
 * @param[out] output       output buffer for digest, it should be 32 bytes.
 *
 *
 * @retval   STATUS_INVALID_PARAM  some input parameter is Invalid.
 *           STATUS_SUCCESS
 *
 * @details  The algorithm of this HMAC is based on FIPS 198-1.
 *           This function will create a sha256_context and two 64 bytes
 *           key (ipad key and opad key) inside the function stack,
 *           so this function will use more than 264 bytes.
 *           So please be careful for the stack size allocation.
 *
 *           This function is based SHA-256 Hash algorithm, so crypto accelerator
 *           resource should be available when calling this function. That is
 *           no other crypto function in progressing.
 *           If the input key is NULL but key length is not zero, the function
 *           will return STATUS_INVALID_PARAM. When key is null that key length
 *           must be zero. (Please avoid to using this setting! It is legal in
 *           algorithm, but not a good design.)
 *           The output digest buffer can NOT be NULL. It should be 32 bytes.
 *
 */

uint32_t hmac_sha256(const uint8_t *key,
                     uint32_t       key_length,
                     const uint8_t *msg,
                     uint32_t       msg_length,
                     uint8_t       *output);
/**
 * @brief HMAC Keyed-Hashing for Message Authentication API
 *        This function is "appended" version API for HMAC, major used in
 *        HMAC_drbg function. In HMAC_drbg design, system can add new extra
 *        entropy data or additional data to generate PRNG data.
 *        For this usage, we create this function hmac_sha256_init_ex to
 *        service the this request.
 *
 *
 * @param[in]  cntx         point to hmac_sha256_context
 * @param[in]  key          point to initial key buffer
 * @param[in]  key_length   number of bytes for key buffer data
 *
 *
 * @retval   STATUS_SUCCESS
 *
 * @details  This function is based SHA-256 Hash algorithm, so crypto accelerator
 *           resource should be available when calling this function. That is
 *           no other crypto function in progressing.
 *           If the input key is NULL but key length is not zero, the function
 *           will return STATUS_INVALID_PARAM.
 *           For key is null that key length must be zero.
 *           (Please avoid to using this setting! It is legal in algorithm,
 *           but not a good design.)
 *           The output digest buffer can NOT be NULL. It should be 32 bytes.
 *
 */
uint32_t hmac_sha256_init_ex(hmac_sha256_context  *cntx,
                             const uint8_t *key, uint32_t key_length);

/**
 * @brief RFC5869 HMAC-Based Extract-and-Expnd Key Derivation Function API
 *        (HKDF)
 *
 * @param[in]  hkdf_ctx     point to hkdf_context
 *
 *
 * @retval  STATUS_SUCCESS
 *           STATUS_INVALID_PARAM: the output buffer of hkdf is NULL or
 *                                 output length is too large.
 *                                 out_len "L" can NOT be more than 1KB in default.
 *
 * @details
 *          according to RFC 5869, section 2.2 salt is optional...
 *          if not provided, it is set to a string of HashLen zero!
 *
 *
 */

uint32_t hkdf_sha256(hkdf_context *hkdf_ctx);

/**
 * @brief     Generate pbkdf2_hmac
 *
 * @param     pbkdf_vector  pointer of struct pbkdf2_struct
 *                           password is defined in member "password", and it's length is password_length.
 *                           salt is defined in member "salt", and it's length is salt_length length.
 *                           key_output is output result buffer, and it's length is key_length
 *
 * @details
 *       pbkdf2_hmac is based the NIST 800-132 section 5.3
 *       Notice: This function is not necessary for matter..
 *
 */
uint32_t pbkdf2_hmac(pbkdf2_st  *pbkdf_vector);


#ifdef __cplusplus
}
#endif

#endif
