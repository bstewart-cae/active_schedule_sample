/***********************************************************************************************************************
 * @file     sm3.h
 * @version
 * @brief
 *
 * @copyright
 **********************************************************************************************************************/
#ifndef _RT584_SM3_H_
#define _RT584_SM3_H_

/***********************************************************************************************************************
 *    CONSTANTS AND DEFINES
 **********************************************************************************************************************/
#define  SM3_DIGEST_SIZE                32
#define  SM3_BLOCK_SIZE                 64


#ifdef SUPPORT_SM3

/**
 * \brief          SM3 context structure
 */
typedef struct
{
    uint32_t   total[2];       /*!< number of bytes processed  */
    uint8_t    buffer[64];     /*!< data block being processed */
} sm3_context;

#ifdef __cplusplus
extern "C" {
#endif
/**
 * \brief          SM3 context setup
 *
 * \param ctx      context to be initialized
 */
void sm3_init(sm3_context *ctx);
/**
 * \brief          SM3 process buffer
 *
 * \param   ctx      SM3 context
 * \param   input    buffer holding the  data
 * \param   length   length of the input data
 */
uint32_t sm3_update(sm3_context *ctx, uint8_t *input, uint32_t length);
/**
 * \brief          SM3 final digest
 *
 * \param   ctx         SM3 context
 * \param   digest      SM3 digest
 *
 */
void sm3_final(sm3_context *ctx, uint8_t *digest);


#ifdef __cplusplus
}
#endif

#endif   /*end for SUPPORT_SM3 */

#endif /* sm3.h */
