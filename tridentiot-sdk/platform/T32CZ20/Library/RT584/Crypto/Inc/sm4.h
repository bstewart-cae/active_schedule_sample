/***********************************************************************************************************************
 * @file     sm4.h
 * @version
 * @brief
 *
 * @copyright
 **********************************************************************************************************************/
#ifndef _RT584_SM4_H_
#define _RT584_SM4_H_
/***********************************************************************************************************************
 *    INCLUDES
 **********************************************************************************************************************/
#include <stdint.h>
#include "status.h"

#ifdef __cplusplus
extern "C"
{
#endif

/***********************************************************************************************************************
 *    CONSTANTS AND DEFINES
 **********************************************************************************************************************/
#define SM4_GenRK_inst_index      (10)
#define SM4_engine_inst_index     (40)
#define SM4_ECB_Enc_inst_index    (70)
#define SM4_ECB_Dec_inst_index    (80)
#define SM4_CBC_Enc_inst_index    (240)
#define SM4_CTR_inst_index        (280)
/***********************************************************************************************************************
 *    GLOBAL PROTOTYPES
 **********************************************************************************************************************/
/**
 * @brief This function acquire SM4 crypto engine
 *
 * @param[in]  void
 *
 * @return    STATUS_SUCCESS
 *
 * @details   This function only used in multi-tasking OS.
 *             Only one SM4 encryption/decryption request can be serviced at any time.
 *             To avoid more than two tasks request the hardware engine resource at the same time,
 *             there should be a semaphore protect to protect the hardware request run one by one.
 *             In single thread environment (FSM), sm4_acquire() function is always return STATUS_SUCCESS
 *             Notice: semaphore protect function depends on OS implemented.
 *
 */
uint32_t sm4_acquire();

/**
 * @brief This function release SM4 crypto engine
 *
 * @param[in]  void
 *
 * @return    STATUS_SUCCESS
 *            STATUS_ERROR: Hardware Engine is not in SM4 mode
 *
 * @details   This function only used in multi-tasking OS.
 *             If the task acquired the hardware engine before, it should call this function
 *             to release the resource.
 *             In single thread environment (FSM), aes_release function always return  STATUS_SUCCESS
 *             Notice: semaphore protect function depends on OS implemented.
 *
 */
uint32_t sm4_release();



void sm4_ecb_encode(uint8_t *out_packet, uint8_t *in_packet, uint8_t *mkey);

void sm4_ecb_decode(uint8_t *out_packet, uint8_t *in_packet, uint8_t  *mkey);

#ifdef __cplusplus
}
#endif

#endif //___SM4_H_

