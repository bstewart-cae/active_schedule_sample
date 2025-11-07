/*

This is an implementation of the AES algorithm, specifically ECB, CTR and CBC mode.


The implementation is verified against the test vectors in:
  National Institute of Standards and Technology Special Publication 800-38A 2001 ED

ECB-AES128
----------

  plain-text:
    6bc1bee22e409f96e93d7e117393172a
    ae2d8a571e03ac9c9eb76fac45af8e51
    30c81c46a35ce411e5fbc1191a0a52ef
    f69f2445df4f9b17ad2b417be66c3710

  key:
    2b7e151628aed2a6abf7158809cf4f3c

  resulting cipher
    3ad77bb40d7a3660a89ecaf32466ef97
    f5d3d58503b9699de785895a96fdbaaf
    43b1cd7f598ece23881b00e3ed030688
    7b0c785e27e8ad3f8223207104725dd4


NOTE:   String length must be evenly divisible by 16byte (str_len % 16 == 0)
        You should pad the end of the string with zeros if this is not the case.
        For AES192/256 the key size is proportionally larger.

*/


/*****************************************************************************/
/* Includes:                                                                 */
/*****************************************************************************/

#include "cm33.h"

#include <string.h> // CBC mode, for memset
#include "aes.h"

#include "crypto.h"
//#include "project_config.h"
extern uint32_t   crypto_firmware;

#ifdef CRYPTO_AES_ENABLE

/* These function is private function in crypto.c
 *
 */

extern void aes_fw_load(void);

extern void aes_encryption(uint8_t round, uint8_t *in_buf, uint8_t *out_buf);

extern void aes_decryption(uint8_t round, uint8_t *in_buf, uint8_t *out_buf);


//#if (MODULE_ENABLE(SUPPORT_MULTITASKING))

//extern void crypto_mutex_lock(void);
//extern void crypto_mutex_unlock(void);

//#else

//#define crypto_mutex_lock()          ((void)0)
//#define crypto_mutex_unlock()        ((void)0)

//#endif


extern void crypto_copy(uint32_t *p_dest_addr, uint32_t *p_src_addr, uint32_t size);
extern void crypto_start(uint8_t op_num, uint8_t sb_num);

/*****************************************************************************/
/* Private variables:                                                        */
/*****************************************************************************/

// The lookup-tables are marked const so they can be placed in read-only storage instead of RAM
// The numbers below can be computed dynamically trading ROM for RAM -
// This can be useful in (embedded) bootloader applications, where ROM is often limited.
static const uint8_t sbox[256] =
{
    //0     1    2      3     4    5     6     7      8    9     A      B    C     D     E     F
    0x63, 0x7c, 0x77, 0x7b, 0xf2, 0x6b, 0x6f, 0xc5, 0x30, 0x01, 0x67, 0x2b, 0xfe, 0xd7, 0xab, 0x76,
    0xca, 0x82, 0xc9, 0x7d, 0xfa, 0x59, 0x47, 0xf0, 0xad, 0xd4, 0xa2, 0xaf, 0x9c, 0xa4, 0x72, 0xc0,
    0xb7, 0xfd, 0x93, 0x26, 0x36, 0x3f, 0xf7, 0xcc, 0x34, 0xa5, 0xe5, 0xf1, 0x71, 0xd8, 0x31, 0x15,
    0x04, 0xc7, 0x23, 0xc3, 0x18, 0x96, 0x05, 0x9a, 0x07, 0x12, 0x80, 0xe2, 0xeb, 0x27, 0xb2, 0x75,
    0x09, 0x83, 0x2c, 0x1a, 0x1b, 0x6e, 0x5a, 0xa0, 0x52, 0x3b, 0xd6, 0xb3, 0x29, 0xe3, 0x2f, 0x84,
    0x53, 0xd1, 0x00, 0xed, 0x20, 0xfc, 0xb1, 0x5b, 0x6a, 0xcb, 0xbe, 0x39, 0x4a, 0x4c, 0x58, 0xcf,
    0xd0, 0xef, 0xaa, 0xfb, 0x43, 0x4d, 0x33, 0x85, 0x45, 0xf9, 0x02, 0x7f, 0x50, 0x3c, 0x9f, 0xa8,
    0x51, 0xa3, 0x40, 0x8f, 0x92, 0x9d, 0x38, 0xf5, 0xbc, 0xb6, 0xda, 0x21, 0x10, 0xff, 0xf3, 0xd2,
    0xcd, 0x0c, 0x13, 0xec, 0x5f, 0x97, 0x44, 0x17, 0xc4, 0xa7, 0x7e, 0x3d, 0x64, 0x5d, 0x19, 0x73,
    0x60, 0x81, 0x4f, 0xdc, 0x22, 0x2a, 0x90, 0x88, 0x46, 0xee, 0xb8, 0x14, 0xde, 0x5e, 0x0b, 0xdb,
    0xe0, 0x32, 0x3a, 0x0a, 0x49, 0x06, 0x24, 0x5c, 0xc2, 0xd3, 0xac, 0x62, 0x91, 0x95, 0xe4, 0x79,
    0xe7, 0xc8, 0x37, 0x6d, 0x8d, 0xd5, 0x4e, 0xa9, 0x6c, 0x56, 0xf4, 0xea, 0x65, 0x7a, 0xae, 0x08,
    0xba, 0x78, 0x25, 0x2e, 0x1c, 0xa6, 0xb4, 0xc6, 0xe8, 0xdd, 0x74, 0x1f, 0x4b, 0xbd, 0x8b, 0x8a,
    0x70, 0x3e, 0xb5, 0x66, 0x48, 0x03, 0xf6, 0x0e, 0x61, 0x35, 0x57, 0xb9, 0x86, 0xc1, 0x1d, 0x9e,
    0xe1, 0xf8, 0x98, 0x11, 0x69, 0xd9, 0x8e, 0x94, 0x9b, 0x1e, 0x87, 0xe9, 0xce, 0x55, 0x28, 0xdf,
    0x8c, 0xa1, 0x89, 0x0d, 0xbf, 0xe6, 0x42, 0x68, 0x41, 0x99, 0x2d, 0x0f, 0xb0, 0x54, 0xbb, 0x16
};

// The round constant word array, rcon[i], contains the values given by
// x to the power (i-1) being powers of x (x is denoted as {02}) in the field GF(2^8)

static const uint8_t rcon[10] =
{
    0x00000001, 0x00000002, 0x00000004, 0x00000008,
    0x00000010, 0x00000020, 0x00000040, 0x00000080,
    0x0000001B, 0x00000036
};

/*****************************************************************************/
/* Private functions:                                                        */
/*****************************************************************************/

#define getSBoxValue(num)      (sbox[(num)])

/*
 *   This function produces round keys. The round keys are used in each round
 * to encrypt/decrypt the states.
 */
static void keyexpansion(uint8_t *round_key, const uint8_t *key, uint8_t round)
{

    uint32_t *rk, i;

    rk =  (uint32_t *) round_key;

    for (i = 0; i < (round - 6); i++)
    {
        /*little endian only.*/
        rk[i] = *((uint32_t *) (key + (i << 2)));
    }

    switch (round)
    {

    case 10:

        for (i = 0; i < 10; i++, rk += 4)
        {

            rk[4] = rk[0] ^ rcon[i] ^
                    ((uint32_t) sbox[(rk[3] >>  8 ) & 0xFF]    )^
                    ((uint32_t) sbox[(rk[3] >> 16 ) & 0xFF] << 8)^
                    ((uint32_t) sbox[(rk[3] >> 24 ) & 0xFF] << 16)^
                    ((uint32_t) sbox[(rk[3]       ) & 0xFF] << 24);

            rk[5] = rk[1] ^ rk[4];
            rk[6] = rk[2] ^ rk[5];
            rk[7] = rk[3] ^ rk[6];
        }

        break;

    case 12:

        for (i = 0; i < 8; i++, rk += 6)
        {

            rk[6]  = rk[0] ^ rcon[i] ^
                     ((uint32_t) sbox[(rk[5] >>  8 ) & 0xFF ]     )^
                     ((uint32_t) sbox[(rk[5] >> 16 ) & 0xFF ] << 8 )^
                     ((uint32_t) sbox[(rk[5] >> 24 ) & 0xFF ] << 16 )^
                     ((uint32_t) sbox[(rk[5]       ) & 0xFF ] << 24 );

            rk[7]  = rk[1] ^ rk[6];
            rk[8]  = rk[2] ^ rk[7];
            rk[9]  = rk[3] ^ rk[8];
            rk[10] = rk[4] ^ rk[9];
            rk[11] = rk[5] ^ rk[10];
        }

        break;

    case 14:

        for (i = 0; i < 7; i++, rk += 8)
        {

            rk[8]  = rk[0] ^ rcon[i] ^
                     ((uint32_t) sbox[(rk[7] >>  8 ) & 0xFF ]     ) ^
                     ((uint32_t) sbox[(rk[7] >> 16 ) & 0xFF ] << 8 ) ^
                     ((uint32_t) sbox[(rk[7] >> 24 ) & 0xFF ] << 16 ) ^
                     ((uint32_t) sbox[(rk[7]       ) & 0xFF ] << 24 );

            rk[9]  = rk[1] ^ rk[8];
            rk[10] = rk[2] ^ rk[9];
            rk[11] = rk[3] ^ rk[10];

            rk[12] = rk[4] ^
                     ((uint32_t) sbox[(rk[11]       ) & 0xFF ]     ) ^
                     ((uint32_t) sbox[(rk[11] >>  8 ) & 0xFF ] << 8 ) ^
                     ((uint32_t) sbox[(rk[11] >> 16 ) & 0xFF ] << 16 ) ^
                     ((uint32_t) sbox[(rk[11] >> 24 ) & 0xFF ] << 24 );

            rk[13] = rk[5] ^ rk[12];
            rk[14] = rk[6] ^ rk[13];
            rk[15] = rk[7] ^ rk[14];
        }

        break;
    }

    return ;

}

uint32_t aes_key_init(struct aes_ctx *ctx, const uint8_t *key, uint32_t keybits)
{

    if (ctx == NULL)
    {
        return STATUS_INVALID_PARAM;
    }

    switch (keybits)
    {

    case AES_KEY128:
        ctx->round = 10;
        break;

    case AES_KEY192:
        ctx->round = 12;
        break;

    case AES_KEY256:
        ctx->round = 14;
        break;

    default:
        return STATUS_INVALID_PARAM;
    }

    /* this is software buffer, if we use this version,
     * we have RoundKey in ctx, so different ctx can
     * switch to use. However, ctx should reserved a 240 bytes buffer
     */

    keyexpansion((uint8_t *)(ctx->RoundKey), key,  ctx->round);

    return STATUS_SUCCESS;
}

void aes_load_round_key(struct aes_ctx *ctx)
{
    crypto_copy((uint32_t *)(CRYPTO_BASE + 0x1C00), (uint32_t *)(ctx->RoundKey), (ctx->round + 1) * 4);
}

uint32_t aes_ecb_encrypt(struct aes_ctx *ctx, uint8_t *in_buf, uint8_t *out_buf)
{
    if (ctx == NULL)
    {
        return STATUS_INVALID_PARAM;
    }

    aes_encryption(ctx->round, in_buf, out_buf);

    return STATUS_SUCCESS;
}


uint32_t aes_ecb_decrypt(struct aes_ctx *ctx, uint8_t *in_buf, uint8_t *out_buf)
{
    if (ctx == NULL)
    {
        return STATUS_INVALID_PARAM;
    }

    aes_decryption(ctx->round, in_buf, out_buf);

    return STATUS_SUCCESS;
}

uint32_t aes_release(struct aes_ctx *ctx)
{
    /*Not implement yet.. This function is reserved for Multitasking used*/

    if (crypto_firmware != AES_FIRMWARE)
    {
        /*hardware secure engine is not in AES mode*/
        return STATUS_ERROR;
    }

    //crypto_mutex_unlock();

    return STATUS_SUCCESS;
}

uint32_t aes_acquire(struct aes_ctx *ctx)
{
    /*Not implement yet.. This function is reserved for  Multitasking used*/
    //crypto_mutex_lock();

    if (crypto_firmware != AES_FIRMWARE)
    {
        aes_fw_load();   /*reload AES firmware */
        crypto_firmware = AES_FIRMWARE;
    }

    return STATUS_SUCCESS;
}

uint32_t aes_iv_set(struct aes_ctx *ctx,  const uint8_t *iv)
{
    if (ctx == NULL)
    {
        return STATUS_INVALID_PARAM;
    }

    memcpy (ctx->Iv, iv, AES_BLOCKLEN);
    return STATUS_SUCCESS;
}

uint32_t aes_iv_get(struct aes_ctx *ctx, uint8_t *iv)
{
    if (ctx == NULL)
    {
        return STATUS_INVALID_PARAM;
    }

    memcpy (iv, ctx->Iv, AES_BLOCKLEN);
    return STATUS_SUCCESS;
}

void aes_xorwith(uint32_t *result, uint32_t *buf1, const uint32_t *buf2)
{
    /*
     *  1. This code is too simple, but we don't check
     *   result, buf1 or buf2 is NULL or not.
     *  2. This code will generated 26 bytes data in thumb mode.
     *  3. The block in AES is always 128bit no matter the key size
     */
    uint32_t  i;

    for (i = 0; i < (AES_BLOCKLEN >> 2); ++i)
    {
        result[i] = buf1[i] ^ buf2[i];
    }
}

uint32_t aes_cbc_buffer_encrypt(struct aes_ctx *ctx, uint8_t *in_buf, uint8_t *out_buf, uint32_t length)
{
    uint32_t  i;
    uint8_t  *Iv = ctx->Iv, temp[AES_BLOCKLEN];

    /*length must be 16N. if not 16N, please padding with zero or PKCS#7*/
    if ((ctx == NULL) || (length & (AES_BLOCKLEN - 1)))
    {
        return STATUS_INVALID_PARAM;
    }

    for (i = 0; i < length; i += AES_BLOCKLEN)
    {
        /*we don't want to corrupt in_buf*/
        aes_xorwith((uint32_t *) temp, (uint32_t *) in_buf, (uint32_t *) Iv);

        aes_ecb_encrypt(ctx, temp, out_buf);
        Iv = out_buf;

        in_buf += AES_BLOCKLEN;
        out_buf +=  AES_BLOCKLEN;
    }

    /* store Iv in ctx for next call */
    memcpy(ctx->Iv, Iv, AES_BLOCKLEN);

    return STATUS_SUCCESS;
}

uint32_t aes_cbc_buffer_decrypt(struct aes_ctx *ctx, uint8_t *in_buf, uint8_t *out_buf, uint32_t length)
{
    uint32_t i;
    uint8_t  store_next_iv[AES_BLOCKLEN];

    /*   Notice: it is possible that in_buf is out_buf, so
     * we should use another buffer to save the input buffer
     * (for IV)
     *
     *  If in_buf is different from out_buf, it is possible
     * to speed up this function without memcpy(...),
     * just use address pointer.
     *  (This required some design guarantee...)
     *
     */
    if ((ctx == NULL) || (length & (AES_BLOCKLEN - 1)))
    {
        return STATUS_INVALID_PARAM;
    }

    for (i = 0; i < length; i += AES_BLOCKLEN)
    {
        memcpy(store_next_iv, in_buf, AES_BLOCKLEN);

        aes_ecb_decrypt(ctx, in_buf, out_buf);

        aes_xorwith((uint32_t *) out_buf, (uint32_t *) out_buf, (uint32_t *) ctx->Iv);

        memcpy(ctx->Iv, store_next_iv, AES_BLOCKLEN);

        in_buf += AES_BLOCKLEN;
        out_buf += AES_BLOCKLEN;
    }

    return STATUS_SUCCESS;
}

uint32_t aes_ctr_buffer_crypt(struct aes_ctx *ctx,
                              uint8_t   *in_buf,
                              uint8_t   *out_buf,
                              uint8_t   *stream_block,
                              uint32_t  *nc_offset,
                              uint32_t  length )
{
    uint32_t i, n;

    /*if you want to save code, you can remove the boundary check...*/

    if (ctx == NULL)
    {
        return STATUS_INVALID_PARAM;
    }

    n = *nc_offset;

    while (length--)
    {

        if (n == 0)
        {

            /*generate new stream_block*/
            aes_ecb_encrypt(ctx, ctx->Iv, stream_block);

            for (i = 16; i > 0; i--)
            {
                /*Check the last byte count. only 0xFF need keep increasment.*/

                if ( ++(ctx->Iv[i - 1]) != 0)
                {
                    break;
                }
            }
        }

        *out_buf++ = *in_buf++ ^ stream_block[n];
        n = (n + 1) & 0xF;
    }

    *nc_offset = n;

    return STATUS_SUCCESS;
}


static void ccm_cbc_mac(struct aes_ccm_encryption_packet  *ccm_packet, uint8_t *mac_buf)
{
    uint32_t   padding_bytes, cbc_num;
    uint8_t    *aes_buffer_ptr;
    uint8_t    B0_flags;

    aes_buffer_ptr = (uint8_t *) (CRYPTO_BASE + 0x1800);


    if (ccm_packet->hdr_len > 0)
    {
        B0_flags = 0x40 | ((ccm_packet->mlen - 2) << 2) | 1;  /*there is  addition cleartext */
    }
    else
    {
        B0_flags = ((ccm_packet->mlen - 2) << 2) | 1;
    }

    /*create B0*/
    *aes_buffer_ptr++ = B0_flags;
    memcpy( (void *)aes_buffer_ptr, (void *) (ccm_packet->nonce), NONCE_LEN);
    aes_buffer_ptr += NONCE_LEN;            /*Notice: We fixed length field is 2 bytes, so NONCE must be 13 bytes.*/

    *aes_buffer_ptr++ = (ccm_packet->data_len >> 8) & 0xFF;    /*B0[14]: plen high value*/
    *aes_buffer_ptr++ = ccm_packet->data_len & 0xFF;              /*B0[15]: plen low value*/

    if (ccm_packet->hdr_len > 0)
    {

        /*the first 2 bytes is length for additional data*/
        *aes_buffer_ptr++ = (ccm_packet->hdr_len >> 8) & 0xFF;
        *aes_buffer_ptr++ = ccm_packet->hdr_len & 0xFF;

        memcpy(aes_buffer_ptr, ccm_packet->hdr, ccm_packet->hdr_len);

        aes_buffer_ptr += ccm_packet->hdr_len;

        /*add padding 0 to 16N... if hlen is 16N+14 , no extra padding 0 required*/
        if ((ccm_packet->hdr_len & (AES_BLOCKLEN - 1)) != (AES_BLOCKLEN - 2))
        {

            padding_bytes = AES_BLOCKLEN - ((2 + ccm_packet->hdr_len) & (AES_BLOCKLEN - 1));

            memset(aes_buffer_ptr, 0, padding_bytes);

            aes_buffer_ptr += padding_bytes;

        }
        else
        {
            padding_bytes = 0;  /*no padding bytes*/
        }

    }

    /*data*/
    memcpy(aes_buffer_ptr, ccm_packet->data, ccm_packet->data_len);

    aes_buffer_ptr += ccm_packet->data_len;

    if ((ccm_packet->data_len & (AES_BLOCKLEN - 1)) != 0)
    {
        padding_bytes = AES_BLOCKLEN - (ccm_packet->data_len & (AES_BLOCKLEN - 1));

        memset(aes_buffer_ptr, 0, padding_bytes);

        aes_buffer_ptr += padding_bytes;

    }
    else
    {
        padding_bytes = 0;  /*no padding bytes*/
    }

    cbc_num =  (uint32_t) aes_buffer_ptr - (CRYPTO_BASE + 0x1800);

    /*set initial IV zero*/
    memset( (uint8_t *) (CRYPTO_BASE + 0x1D10), 0, AES_BLOCKLEN);

    *((volatile uint32_t *) (CRYPTO_BASE + 0x1D50))   = 0;             /*data offset. 0*/
    *((volatile uint32_t *) (CRYPTO_BASE + 0x1D40))   = 10 - 1;        /*it should be 10-1*/
    *((volatile uint32_t *) (CRYPTO_BASE + 0x1D48) )  = (cbc_num >> 4) - 1;  /*block number*/

    *((volatile uint32_t *) (CRYPTO_BASE + 0x1000)) = 0xa0000000 | AES_CBC_Enc_inst_index;


#ifndef CRYPTO_INT_ENABLE
    //Start the acceleator engine
    crypto_start(3, 0);

    //Waiting for calculation done
    while (!CRYPTO->CRYPTO_CFG.bit.crypto_done);

    //clear the VLW_DEF register
    CRYPTO->CRYPTO_CFG.bit.clr_crypto_int = 1;

#else
    //Start the acceleator engine
    crypto_start(3, 0);

    while (crypto_finish == 0)
        ;

    crypto_finish = 0;


#endif

    memcpy(mac_buf, (aes_buffer_ptr - AES_BLOCKLEN), ccm_packet->mlen); /*mac is in the last block*/

}

uint32_t aes_ccm_encryption(struct aes_ccm_encryption_packet  *ccm_packet)
{
    uint8_t    mac_buffer[16];

    uint32_t   cbc_num, output_length, mlen;
    uint8_t    *aes_buffer_ptr, *out_buf_ptr;

    /* 1. check return buffer.  output buffer should not null.
     * 2. check input data length should be less than 1024 bytes.
     * 3. check mlen?(valid value is 4, 6, 8, 10, 12, 14, 16)
     *
     */

    output_length = ccm_packet->hdr_len + ccm_packet->data_len + ccm_packet->mlen;

    /*This check can be removed for shrink code performance.*/
    mlen = (ccm_packet->mlen - 2) >> 1;

    /*Here we assume ccm max packet is less than 1024 bytes*/
    if ((ccm_packet->out_buf == NULL) || (*(ccm_packet->out_buf_len) < output_length) ||
            (output_length > 1024) || (mlen == 0) || (mlen > 7))
    {
        return STATUS_INVALID_PARAM;
    }

    ccm_cbc_mac(ccm_packet, mac_buffer);

    /*ctr mode --- Notice: because hardware using counter index start from 0.
      we should re-order  mac location to the first block, then move the block to end*/

    /*set ctr IV ==>  0x01 NONCE (13 bytes) 00 00*/
    aes_buffer_ptr = (uint8_t *) (CRYPTO_BASE + 0x1D10);

    *aes_buffer_ptr++ = 1;
    memcpy(aes_buffer_ptr, ccm_packet->nonce, NONCE_LEN);
    aes_buffer_ptr += NONCE_LEN;

    *aes_buffer_ptr++ = 0;
    *aes_buffer_ptr++ = 0;

    /*fill data to be */

    aes_buffer_ptr = (uint8_t *) (CRYPTO_BASE + 0x1800);

    memcpy(aes_buffer_ptr, mac_buffer, ccm_packet->mlen);   /*in fact, we don't care tail data of mlen ~ 16*/
    aes_buffer_ptr += 16;

    memcpy(aes_buffer_ptr, ccm_packet->data, ccm_packet->data_len); /*copy message data*/

    aes_buffer_ptr += ccm_packet->data_len;

    cbc_num = (uint32_t) aes_buffer_ptr - (CRYPTO_BASE + 0x1800) + (AES_BLOCKLEN - 1); /*for counter based*/


    *((volatile uint32_t *) (CRYPTO_BASE + 0x1D50))   = 0;             /*data offset. 0*/
    *((volatile uint32_t *) (CRYPTO_BASE + 0x1D40))   = 10 - 1;        /*it should be 10-1*/
    *((volatile uint32_t *) (CRYPTO_BASE + 0x1D48) )  = (cbc_num >> 4) - 1;

    *((volatile uint32_t *) (CRYPTO_BASE + 0x1D58) )  = 0x01000000;    /*increase byte[3] first*/
    *((volatile uint32_t *) (CRYPTO_BASE + 0x1D5C) )  = 0x00010000;    /*increate byte[2] if carry in*/

    *((volatile uint32_t *) (CRYPTO_BASE + 0x1000)) = 0xa0000000 | AES_CTR_inst_index;

#ifndef CRYPTO_INT_ENABLE
    crypto_start(3, 0);

    //Waiting for calculation done
    while (!CRYPTO->CRYPTO_CFG.bit.crypto_done);

    //clear the VLW_DEF register
    CRYPTO->CRYPTO_CFG.bit.clr_crypto_int = 1;
#else
    crypto_start(3, 0);

    while (crypto_finish == 0)
        ;

    crypto_finish = 0;

#endif

    /*ok now we should combine the output message*/
    out_buf_ptr = ccm_packet->out_buf;

    /*copy hdr*/
    memcpy(out_buf_ptr, ccm_packet->hdr,  ccm_packet->hdr_len);

    out_buf_ptr += ccm_packet->hdr_len;

    memcpy(out_buf_ptr, (uint8_t *) (CRYPTO_BASE + 0x1810),  ccm_packet->data_len);     //0xC10

    out_buf_ptr += ccm_packet->data_len;

    memcpy(out_buf_ptr, (uint8_t *) (CRYPTO_BASE + 0x1800),  ccm_packet->mlen);     //0xC00

    *(ccm_packet->out_buf_len) =  output_length;

    return STATUS_SUCCESS;

}

uint32_t aes_ccm_decryption_verification(struct aes_ccm_decryption_packet  *ccm_packet)
{
    uint8_t    mac_buffer[16], payload_mac[16];

    struct     aes_ccm_encryption_packet  ccm_cbc;

    uint32_t   cbc_num, mlen;
    uint8_t    *aes_buffer_ptr,  *pay_mac_ptr;


    /* 1. check return buffer.  output buffer should not null.
     * 2. check input data length should be less than 1024 bytes.
     * 3. check mlen?(valid value is 4, 6, 8, 10, 12, 14, 16)
     *
     */

    /*This check can be removed for shrink code performance.*/
    mlen = (ccm_packet->mlen - 2) >> 1;

    if ((ccm_packet->out_buf == NULL) || (mlen == 0) || (mlen > 7) ||
            (*(ccm_packet->out_buf_len) < ccm_packet->data_len) || (ccm_packet->payload_buf == NULL) )
    {
        return STATUS_INVALID_PARAM;
    }

    /*ctr mode --- Notice: because hardware using counter index start from 0.
      we should re-order  mac location to the first block, then move the block to end*/

    /*set ctr IV ==>  0x01 NONCE (14 bytes) 00 00*/
    aes_buffer_ptr = (uint8_t *) (CRYPTO_BASE + 0x1D10);

    *aes_buffer_ptr++ = 1;
    memcpy(aes_buffer_ptr, ccm_packet->nonce, NONCE_LEN);
    aes_buffer_ptr += NONCE_LEN;

    *aes_buffer_ptr++ = 0;
    *aes_buffer_ptr++ = 0;

    aes_buffer_ptr = (uint8_t *) (CRYPTO_BASE + 0x1800);

    pay_mac_ptr = ccm_packet->payload_buf + ccm_packet->hdr_len + ccm_packet->data_len;

    memcpy(aes_buffer_ptr, pay_mac_ptr, ccm_packet->mlen);   /*in fact, we don't care tail data of mlen ~ 16*/
    aes_buffer_ptr += 16;

    memcpy(aes_buffer_ptr,  (ccm_packet->payload_buf + ccm_packet->hdr_len), ccm_packet->data_len); /*copy message data*/

    aes_buffer_ptr += ccm_packet->data_len;

    cbc_num = (uint32_t) aes_buffer_ptr - (CRYPTO_BASE + 0x1800) + (AES_BLOCKLEN - 1); /*for counter based*/


    *((volatile uint32_t *) (CRYPTO_BASE + 0x1D50))   = 0;             /*data offset. 0*/
    *((volatile uint32_t *) (CRYPTO_BASE + 0x1D40))   = 10 - 1;        /*it should be 10-1*/
    *((volatile uint32_t *) (CRYPTO_BASE + 0x1D48) )  = (cbc_num >> 4) - 1;

    *((volatile uint32_t *) (CRYPTO_BASE + 0x1D58) )  = 0x01000000;    /*increase byte[3] first*/
    *((volatile uint32_t *) (CRYPTO_BASE + 0x1D5C) )  = 0x00010000;    /*increate byte[2] if carry in*/

    *((volatile uint32_t *) (CRYPTO_BASE + 0x1000)) = 0xa0000000 | AES_CTR_inst_index;

#ifndef CRYPTO_INT_ENABLE
    crypto_start(3, 0);

    //Waiting for calculation done
    while (!CRYPTO->CRYPTO_CFG.bit.crypto_done);

    //clear the VLW_DEF register
    CRYPTO->CRYPTO_CFG.bit.clr_crypto_int = 1;
#else
    crypto_start(3, 0);

    while (crypto_finish == 0)
        ;

    crypto_finish = 0;

#endif

    /*save payload caculate mac*/
    memcpy(payload_mac, (uint8_t *) (CRYPTO_BASE + 0x1800), (ccm_packet->mlen));

    /*copy ctr decryption result to out_buf*/
    memcpy(ccm_packet->out_buf, (uint8_t *) (CRYPTO_BASE + 0x1810), ccm_packet->data_len);

    ccm_cbc.nonce = ccm_packet->nonce;

    ccm_cbc.hdr = ccm_packet->payload_buf;
    ccm_cbc.hdr_len = ccm_packet->hdr_len;

    ccm_cbc.data = ccm_packet->out_buf;
    ccm_cbc.data_len = ccm_packet->data_len;

    ccm_cbc.mlen = ccm_packet->mlen;

    ccm_cbc_mac(&ccm_cbc, mac_buffer);

    /*compare decryption mac and cbc caculate mac*/
    if (memcmp(mac_buffer, payload_mac, ccm_packet->mlen))
    {
        *(ccm_packet->out_buf_len) = 0;
        return STATUS_ERROR;
    }

    *(ccm_packet->out_buf_len) = ccm_packet->data_len;

    return STATUS_SUCCESS;
}

#endif

