
#include <stdio.h>
#include <string.h>

#include "cm33.h"

#include "sha256.h"
#include "crypto.h"
//#include "project_config.h"

#ifdef CRYPTO_INT_ENABLE
extern volatile uint32_t  sha_finish;
#endif

//#if MODULE_ENABLE(SUPPORT_MULTITASKING)

//extern void crypto_mutex_lock(void);
//extern void crypto_mutex_unlock(void);

//#else

//#define crypto_mutex_lock()          ((void)0)
//#define crypto_mutex_unlock()        ((void)0)

//#endif

extern uint32_t   crypto_firmware;

extern void crypto_copy(uint32_t *p_dest_addr, uint32_t *p_src_addr, uint32_t size);


#define PUT_UINT32_BE(n,b,i) { *((uint32_t *)(b+i)) = __REV(n); }


const uint32_t sha256_init_digest[8] =
{
    0x6A09E667, 0xBB67AE85, 0x3C6EF372, 0xA54FF53A,
    0x510E527F, 0x9B05688C, 0x1F83D9AB, 0x5BE0CD19
};


const uint32_t sha224_init_digest[8] =
{
    0xC1059ED8, 0x367CD507, 0x3070DD17, 0xF70E5939,
    0xFFC00B31, 0x68581511, 0x64F98FA7, 0xBEFA4FA4
};

const uint32_t sha256_k[64] =
{
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
};


void sha256(uint8_t *input, uint32_t length, uint8_t *digest)
{
    sha256_context sha_cnxt;
    uint32_t sha256_verify_size;
    uint8_t   sha256_digest[SHA256_DIGEST_SIZE];

    sha256_init(&sha_cnxt);

    do
    {
        if (length >= SHA256_CAL_MAX_SIZE)
        {
            sha256_verify_size = SHA256_CAL_MAX_SIZE;
        }
        else
        {
            sha256_verify_size = length;
        }

        sha256_update(&sha_cnxt, input, sha256_verify_size);

        input = input + SHA256_CAL_MAX_SIZE;
        length = length - sha256_verify_size;

    } while (length > 0);

    sha256_finish(&sha_cnxt, sha256_digest);

    /*SHA256 result is big-endian, so we need to change it to little-endian*/
    buffer_endian_exchange((uint32_t *) digest,  (uint32_t *) sha256_digest, (32 >> 2));
}


void sha256_init(sha256_context *ctx)
{
    uint32_t reg;

    ctx->total[0] = 0;
    ctx->total[1] = 0;

    //crypto_mutex_lock();

    crypto_firmware = SHA_FIRMWARE;

    /*
     * check busy... almost impossible busy...
     * if busy, we should check why??
     */

    reg = CRYPTO->CRYPTO_CFG.reg;
    if (reg & (3 << 26))
    {
        /*why? check? almost impossible*/
        while (1);        /*debug...*/
    }

#ifdef  CRYPTO_INT_ENABLE
    sha_finish = 0;
#endif

    /*This initial is necessary setting.*/
    crypto_copy((uint32_t *) (CRYPTO_BASE + 0x1900), (uint32_t *) sha256_init_digest, 8);
    crypto_copy((uint32_t *) (CRYPTO_BASE + 0x1800), (uint32_t *) sha256_k, 64);

    CRYPTO->SHA_K_BASE  = 0x00;
    CRYPTO->SHA_DIGEST_BASE  = 0x100;
    CRYPTO->SHA_MISC_CFG  = 0x1;
}


void sha224_init(sha256_context *ctx)
{
    uint32_t reg;

    ctx->total[0] = 0;
    ctx->total[1] = 0;

    //crypto_mutex_lock();

    crypto_firmware = SHA_FIRMWARE;

    /*
     * check busy... almost impossible busy...
     * if busy, we should check why??
     */

    reg = CRYPTO->CRYPTO_CFG.reg;
    if (reg & (3 << 26))
    {
        /*why? check? almost impossible*/
        while (1);        /*debug...*/
    }

    /*This initial is necessary setting.*/
    crypto_copy((uint32_t *) (CRYPTO_BASE + 0x1900), (uint32_t *) sha224_init_digest, 8);
    crypto_copy((uint32_t *) (CRYPTO_BASE + 0x1800), (uint32_t *) sha256_k, 64);

    CRYPTO->SHA_K_BASE  = 0x00;
    CRYPTO->SHA_DIGEST_BASE  = 0x100;
    CRYPTO->SHA_MISC_CFG  = 0x1;      /*this setting should be remoed later.*/
}


static void sha256_process(uint8_t *data, uint32_t length)
{

#ifndef CRYPTO_INT_ENABLE
    /*if previous operation not finish... we should wait*/
    while (CRYPTO->CRYPTO_CFG.bit.sha_busy)
        ;

    if (CRYPTO->CRYPTO_CFG.bit.sha_done)
    {
        //clear the VLW_DEF register
        CRYPTO->CRYPTO_CFG.bit.clr_crypto_int = 1;
    }
#else

    if (CRYPTO->CRYPTO_CFG.bit.sha_busy)
    {
        while (sha_finish == 0)
            ;
    }

    sha_finish = 0;

#endif

    CRYPTO->SHA_DMA_BASE  = (uint32_t) data;
    CRYPTO->SHA_DMA_LENGTH  = length;

    /*enable sha256*/
    CRYPTO->CRYPTO_CFG.bit.enable_sha = 1;

    /*Notice: We don't wait SHA256 finish here.*/
}

uint32_t sha256_update(sha256_context *ctx, uint8_t *input, uint32_t length)
{
    uint32_t  left, fill;
    uint32_t  temp;

    if (crypto_firmware != SHA_FIRMWARE)
    {
        return STATUS_ERROR;
    }

    if (!length)
    {
        return STATUS_ERROR;
    }

    left = ctx->total[0] & 0x3F;
    fill = 64 - left;

    /* 2024/02/16 update overflow check...
     * it is almost impossible for sha data
     * over 4GB...malicious attack?
     */
    ctx->total[0] += length;

    if (ctx->total[0] < length)
    {
        ctx->total[1]++;    /*overflow for 32 bits... almost impossible*/
    }

    if (left && length >= fill)
    {

        memcpy((void *) (ctx->buffer + left), (void *) input, fill);

        sha256_process(ctx->buffer, 64);
        length -= fill;
        input += fill;
        left = 0;
    }

    /* in fact, length could not be more than 65536 bytes..
     * if length is more than 64KB.. separate the length several times...
     * but in real, using 64KB data for sha256 is not a good idea.
     */
    if (length >= 64)
    {
        temp = (length & ~63);       /*caculate block count.*/
        sha256_process(input, temp);
        length -= temp;
        input += temp;
    }

    if (length)
    {
        memcpy((void *) (ctx->buffer + left), (void *) input, length);
    }

    return STATUS_SUCCESS;
}

static const uint8_t sha256_padding[64] =
{
    0x80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

void sha256_finish(sha256_context *ctx, uint8_t *digest)
{
    uint32_t last, padn;
    uint32_t high, low;
    uint8_t msglen[8];

    if (crypto_firmware != SHA_FIRMWARE)
    {
        return;    /*accelerator should be in hash mode*/
    }

    if (digest == NULL)       /*digest can NOT be NULL*/
    {
        return;
    }

    high = ((ctx->total[0] >> 29)
            | (ctx->total[1] << 3));
    low = (ctx->total[0] << 3);

    PUT_UINT32_BE(high, msglen, 0);
    PUT_UINT32_BE(low, msglen, 4);

    last = ctx->total[0] & 0x3F;
    padn = (last < 56) ? (56 - last) : (120 - last);

    sha256_update(ctx, (uint8_t *) sha256_padding, padn);
    sha256_update(ctx, msglen, 8);

#ifndef CRYPTO_INT_ENABLE
    /*wait last block finish*/
    while (!CRYPTO->CRYPTO_CFG.bit.sha_done)
        ;

    //clear the VLW_DEF register
    CRYPTO->CRYPTO_CFG.bit.clr_crypto_int = 1;
#else

    while (sha_finish == 0)
        ;

    sha_finish = 0;

#endif

    PUT_UINT32_BE(*((uint32_t *)(CRYPTO_BASE + 0x1900)), digest, 0);
    PUT_UINT32_BE(*((uint32_t *)(CRYPTO_BASE + 0x1904)), digest, 4);
    PUT_UINT32_BE(*((uint32_t *)(CRYPTO_BASE + 0x1908)), digest, 8);
    PUT_UINT32_BE(*((uint32_t *)(CRYPTO_BASE + 0x190C)), digest, 12);
    PUT_UINT32_BE(*((uint32_t *)(CRYPTO_BASE + 0x1910)), digest, 16);
    PUT_UINT32_BE(*((uint32_t *)(CRYPTO_BASE + 0x1914)), digest, 20);
    PUT_UINT32_BE(*((uint32_t *)(CRYPTO_BASE + 0x1918)), digest, 24);
    PUT_UINT32_BE(*((uint32_t *)(CRYPTO_BASE + 0x191C)), digest, 28);

    /*clean hash value in accelerator */
    crypto_copy((uint32_t *) (CRYPTO_BASE + 0x1900), (uint32_t *) sha256_init_digest, 8);

    //crypto_mutex_unlock();

}

#define  INNER_PAD          0x36                    /*FIPS 198-1 Inner pad*/
#define  OUTER_PAD          0x5C                    /*FIPS 198-1 Inner pad*/

static void rekey(uint8_t *ipad_key,  uint8_t *opad_key, const uint8_t *orig_key, unsigned int key_size)
{
    uint32_t i;

    for (i = 0; i < key_size; i++)
    {
        ipad_key[i] = INNER_PAD ^ orig_key[i];
        opad_key[i] = OUTER_PAD ^ orig_key[i];
    }

    for (; i < SHA256_BLOCK_SIZE; i++)
    {
        ipad_key[i] = INNER_PAD;
        opad_key[i] = OUTER_PAD;
    }
}

uint32_t hmac_sha256(const uint8_t  *key,
                     uint32_t        key_length,
                     const uint8_t   *msg,
                     uint32_t        msg_length,
                     uint8_t         *output)
{

    /*output buffer must be more than 32 bytes!!*/

    /*Please notice this function will need stack size over 200 bytes!*/
    sha256_context sha_cnxt;

    uint8_t   sha256_digest[SHA256_DIGEST_SIZE];

    /*ipad and opad, the first 64 bytes is ipad, the next 64 bytes is opad*/
    uint8_t   pad_key[SHA256_BLOCK_SIZE * 2];


    /*NOTICE: key_length can be zero!! But if key_length is not zero,,, then key can not BE NULL....*/
    if ((output == NULL) || ((key == NULL) && (key_length != 0)))
    {
        return STATUS_INVALID_PARAM  ;    /*not correct input*/
    }

    /*
     * NOTICE: EVEN key is NULL pointer, and key_length is 0, it still can generate a HMAC  value
     * The value is 0x19EF24A32C717B167F33A91D6F648BDF96596776AFDB6377AC434C1C293CCB04
     */

    /*check key size*/

    /* if (key_length <= SHA256_BLOCK_SIZE)
     * The next three calls are dummy calls just to avoid
     * certain timing attacks. Without these dummy calls,
     * adversaries would be able to learn whether the key_length is
     * greater than SHA256_BLOCK_SIZE by measuring the time
     * consumed in this process.
     */
    sha256_init(&sha_cnxt);
    sha256_update(&sha_cnxt, (uint8_t *) key, key_length);
    sha256_finish(&sha_cnxt, sha256_digest);

    if (key_length <= SHA256_BLOCK_SIZE)
    {
        /* Actual code for when key_size <= SHA256_BLOCK_SIZE:
         * so only use the original key  and padding zero
         */
        rekey(pad_key, pad_key + SHA256_BLOCK_SIZE, key, key_length);
    }
    else
    {
        /* because input key is more than 64 bytes,
         * so sha256 the input key first to become new key
         */
        rekey(pad_key, pad_key + SHA256_BLOCK_SIZE, sha256_digest, SHA256_DIGEST_SIZE);
    }

    /*FIPS 198-1 step 5.   H( (K_0^ ipad)||text) */
    sha256_init(&sha_cnxt);
    sha256_update(&sha_cnxt, pad_key, SHA256_BLOCK_SIZE);
    sha256_update(&sha_cnxt, (uint8_t *)msg, msg_length);
    sha256_finish(&sha_cnxt, sha256_digest);

    /*FIPS 198-1 step 9.  H((K_0^opad)||( H( (K_0^ ipad)||text)) */

    sha256_init(&sha_cnxt);
    sha256_update(&sha_cnxt, (pad_key + SHA256_BLOCK_SIZE), SHA256_BLOCK_SIZE);
    sha256_update(&sha_cnxt, sha256_digest, SHA256_DIGEST_SIZE);
    sha256_finish(&sha_cnxt, output);

    /*clean data in stack...*/
    memset(pad_key, 0, (SHA256_BLOCK_SIZE * 2));
    memset(sha256_digest, 0, SHA256_DIGEST_SIZE);
    memset(&sha_cnxt, 0, sizeof (sha256_context));

    return STATUS_SUCCESS;
}

uint32_t hmac_sha256_init_ex(
    hmac_sha256_context  *cntx,
    const uint8_t   *key,
    uint32_t        key_length)
{
    uint8_t   sha256_digest[SHA256_DIGEST_SIZE];

    /*ipad and opad, the first 64 bytes is ipad, the next 64 bytes is opad*/
    uint8_t   ipad_key[SHA256_BLOCK_SIZE];

    /*
     * NOTICE: EVEN key is NULL pointer, and key_length is 0, it still can generate a HMAC  value
     * The value is 0x19EF24A32C717B167F33A91D6F648BDF96596776AFDB6377AC434C1C293CCB04
     */

    if (key_length <= SHA256_BLOCK_SIZE)
    {
        /* Actual code for when key_size <= SHA256_BLOCK_SIZE: */
        rekey(ipad_key, cntx->opad, key, key_length);
    }
    else
    {
        sha256_init((sha256_context *) cntx);
        sha256_update((sha256_context *) cntx, (uint8_t *) key, key_length);
        sha256_finish((sha256_context *) cntx, sha256_digest);

        rekey(ipad_key, cntx->opad, sha256_digest, SHA256_DIGEST_SIZE);
    }

    sha256_init((sha256_context *) cntx);
    sha256_update((sha256_context *) cntx, ipad_key, SHA256_BLOCK_SIZE);

    return STATUS_SUCCESS;
}

uint32_t hkdf_sha256(hkdf_context *hkdf_ctx)
{
    /* algortithm based https://tools.ietf.org/html/rfc5869#section-2.2 */

    uint32_t  n;
    uint32_t  i, tlen, copy_length;

    uint8_t   prk_digest[SHA256_DIGEST_SIZE];
    uint8_t   temp_digest[SHA256_BLOCK_SIZE];

    uint8_t   *ptr;

    hmac_sha256_context hkdf_sha_cntx;

    uint8_t   ctr = 1 ;

    /* according to RFC 5869, section 2.2 salt is optional...
     * if not provided, it is set to a string of HashLen zero!
     */

    n = (hkdf_ctx->out_len + SHA256_DIGEST_SIZE - 1) >> 5;        /*because SHA256_DIGEST_SIZE is 32*/

    /* In RFC 5869 n is 255, but in our embed system 32 block is large
     * 32*32 = 1024. using 1k for HKDF is almost unrealistic.
     */
    if ((hkdf_ctx->out_key == NULL) || (n > HKDF_limit_block))
    {
        return STATUS_INVALID_PARAM;
    }

    /*hmac_sha256 must return SHA256_DIGEST_SIZE bytes. */
    /*RFC5869 2.2. Step 1: Extract*/
    hmac_sha256(hkdf_ctx->salt, hkdf_ctx->salt_len, hkdf_ctx->secret,
                hkdf_ctx->secret_len, prk_digest);


    /*RFC5869 2.2. Step 2:  Expand*/

    tlen = 0;

    ptr = hkdf_ctx->out_key;

    copy_length = hkdf_ctx->out_len;

    for (i = 0; i < n; i++)
    {

        hmac_sha256_init_ex(&hkdf_sha_cntx, prk_digest, SHA256_DIGEST_SIZE);
        sha256_update((sha256_context *) &hkdf_sha_cntx, temp_digest, tlen);
        sha256_update((sha256_context *) &hkdf_sha_cntx, (uint8_t *) hkdf_ctx->info, hkdf_ctx->info_len);
        sha256_update((sha256_context *) &hkdf_sha_cntx, (uint8_t *) &ctr, 1);
        sha256_finish((sha256_context *) &hkdf_sha_cntx, temp_digest);

        /*caculate outpad*/
        sha256_init((sha256_context *)  &hkdf_sha_cntx);
        sha256_update((sha256_context *)  &hkdf_sha_cntx,  (hkdf_sha_cntx.opad), SHA256_BLOCK_SIZE);
        sha256_update((sha256_context *)  &hkdf_sha_cntx, temp_digest, SHA256_DIGEST_SIZE);
        sha256_finish((sha256_context *)  &hkdf_sha_cntx, temp_digest);

        if (copy_length > SHA256_DIGEST_SIZE)
        {
            /*copy SHA256_DIGEST_SIZE bytes data */
            memcpy((void *)ptr, temp_digest, SHA256_DIGEST_SIZE);
            ptr += SHA256_DIGEST_SIZE;
            copy_length -= SHA256_DIGEST_SIZE;
            tlen = SHA256_DIGEST_SIZE;

        }
        else
        {
            /*copy the last bytes*/
            memcpy((void *)ptr, temp_digest, copy_length);
            ptr += copy_length;
            copy_length = 0;
        }

        ctr++;

    }

    /*clear stack.*/
    ctr = 0;
    memset( (void *)prk_digest, 0, SHA256_DIGEST_SIZE);
    memset( (void *)temp_digest, 0, SHA256_DIGEST_SIZE);


    return STATUS_SUCCESS;
}

/* the following two functions use for internal pbkdf2_hmac,
 * because hash pbkdf2_hmac repeat to caculate
 *   hmac_sha256_init_ex(&pbkdf2_sha_cntx, pb_vector->password, pb_vector->password_length);
 * so we get/restore the hash value direct without hashing again.
 *
 *
 */

static void get_temp_hash_value(uint32_t *buffer)
{
    crypto_copy( buffer, (uint32_t *) (CRYPTO_BASE + 0x1900), 8);
}

static void restore_temp_hash_value(uint32_t *buffer)
{
    crypto_copy( (uint32_t *) (CRYPTO_BASE + 0x1900), buffer, 8);
}

uint32_t pbkdf2_hmac(pbkdf2_st  *pb_vector)
{
    hmac_sha256_context   cntx_temp, cntx_temp_with_salt;
    uint8_t   cntx_temp_hash[SHA256_DIGEST_SIZE], cntx_temp_with_salt_hash[SHA256_DIGEST_SIZE];
    hmac_sha256_context   pbkdf2_sha_cntx;

    uint32_t     i, j, counter = 1, gen_key_length = 0, counter_be, remain_length;
    uint8_t      temp_digest[SHA256_DIGEST_SIZE], mdl[SHA256_DIGEST_SIZE], work_buf[SHA256_DIGEST_SIZE];

    /*the password and salt, output key could not be NULL or zero length.*/
    if ((pb_vector->password == NULL) || (pb_vector->salt == NULL) || (pb_vector->key_output == NULL))
    {
        return STATUS_ERROR;
    }

    if ((pb_vector->password_length == 0) || (pb_vector->salt_length == 0) || (pb_vector->key_length == 0))
    {
        return STATUS_ERROR;
    }

    if (pb_vector->iteration > 100000)
    {
        return  STATUS_ERROR;   /*over matter maximum number iterations*/
    }

    /*caculate ipad, and opad first*/
    hmac_sha256_init_ex(&pbkdf2_sha_cntx, pb_vector->password, pb_vector->password_length);
    /*backup pbkdf2_sha_cntx without salt --- it will reused later.*/
    memcpy( &cntx_temp, &pbkdf2_sha_cntx, sizeof(hmac_sha256_context));
    get_temp_hash_value((uint32_t *)cntx_temp_hash);


    sha256_update((sha256_context *)  &pbkdf2_sha_cntx, pb_vector->salt, pb_vector->salt_length);
    /*backup pbkdf2_sha_cntx --- it will reused later.*/
    memcpy( &cntx_temp_with_salt, &pbkdf2_sha_cntx, sizeof(hmac_sha256_context));
    get_temp_hash_value((uint32_t *)cntx_temp_with_salt_hash);

    remain_length = pb_vector->key_length - gen_key_length;

    while (gen_key_length < pb_vector->key_length)
    {
        counter_be = __REV(counter);    /*little endian to big endian*/
        sha256_update((sha256_context *)  &pbkdf2_sha_cntx, (uint8_t *) &counter_be, 4);

        sha256_finish((sha256_context *) &pbkdf2_sha_cntx, temp_digest);

        /*caculate outpad*/
        sha256_init((sha256_context *)  &pbkdf2_sha_cntx);
        sha256_update((sha256_context *)  &pbkdf2_sha_cntx,  (pbkdf2_sha_cntx.opad), SHA256_BLOCK_SIZE);

        sha256_update((sha256_context *)  &pbkdf2_sha_cntx, temp_digest, SHA256_DIGEST_SIZE);
        sha256_finish((sha256_context *)  &pbkdf2_sha_cntx, mdl);

        memcpy(work_buf, mdl, SHA256_DIGEST_SIZE);

        for (i = 1; i < pb_vector->iteration; i++)          /*Notice: i= 1*/
        {
            /*memcpy backup restore cntx_temp*/
            memcpy(&pbkdf2_sha_cntx, &cntx_temp, sizeof(hmac_sha256_context));
            restore_temp_hash_value((uint32_t *)cntx_temp_hash);

            sha256_update((sha256_context *)  &pbkdf2_sha_cntx, mdl, SHA256_DIGEST_SIZE);

            sha256_finish((sha256_context *) &pbkdf2_sha_cntx, temp_digest);

            /*caculate outpad*/
            sha256_init((sha256_context *)  &pbkdf2_sha_cntx);
            sha256_update((sha256_context *)  &pbkdf2_sha_cntx,  (pbkdf2_sha_cntx.opad), SHA256_BLOCK_SIZE);

            sha256_update((sha256_context *)  &pbkdf2_sha_cntx, temp_digest, SHA256_DIGEST_SIZE);
            sha256_finish((sha256_context *)  &pbkdf2_sha_cntx, mdl);

            for (j = 0; j < SHA256_DIGEST_SIZE; j++)
            {
                work_buf[j] ^= mdl[j];
            }

        }

        if (remain_length > SHA256_DIGEST_SIZE)
        {
            memcpy(pb_vector->key_output + gen_key_length, work_buf, SHA256_DIGEST_SIZE);
            gen_key_length += SHA256_DIGEST_SIZE;
            remain_length -= SHA256_DIGEST_SIZE;
            counter++;

            /*restore next pbkdf2_sha_cntx with salt*/
            memcpy( &pbkdf2_sha_cntx, &cntx_temp_with_salt, sizeof(hmac_sha256_context));
            restore_temp_hash_value((uint32_t *) cntx_temp_with_salt_hash);
        }
        else
        {
            memcpy(pb_vector->key_output + gen_key_length, work_buf, remain_length);
            gen_key_length += remain_length;
            remain_length = 0;
            break;
        }

    }

    return STATUS_SUCCESS;
}

