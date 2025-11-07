/***********************************************************************************************************************
 * @file     RT584_CRYPTO.h
 * @version
 * @brief    CRYPTO ACCELEATOR driver header file
 *
 * @copyright
 **********************************************************************************************************************/
#ifndef _RT584_CRYPTO_H_
#define _RT584_CRYPTO_H_
/***********************************************************************************************************************
 *    INCLUDES
 **********************************************************************************************************************/
#include <stdint.h>

#include "aes.h"

#include "sm4.h"

#include "ecc.h"

#include "sha256.h"

#include "hmac_drbg.h"

#include "crypto_util.h"

#include "sm3.h"

#ifdef CRYPTO_SECP256R1_ENABLE
#include "ecjpake.h"
#endif

#ifdef CRYPTO_INT_ENABLE
extern volatile uint32_t  crypto_finish;
#endif


#define   AES_FIRMWARE              0x000000AE      /*AES firmware*/
#define   ECC_FIRMWARE              0xECC00000      /*ECDH firmware*/
#define   ECC_FIRMWARE_VER          0xECC50000      /*ECDSA Verify firmware*/
#define   ECC_FIRMWARE_SIG          0xECC80000      /*ECDSA Signature firmware*/
#define   ECC_FIRMWARE_C25519       0xC2551900      /*Curve25519 ECDH firmware*/

#define   SM2_FIRMWARE              0xECCCA200      /*SM2*/
#define   SM2_FIRMWARE_SIG          0xECCCA500      /*SM2 Signature*/
#define   SM3_FIRMWARE              0x0000CA53      /*SM3*/
#define   SM4_FIRMWARE              0x0000CA54      /*SM4*/

#define   SHA_FIRMWARE              0x00005A00      /*SHA*/


#endif

