/***********************************************************************************************************************
 * @file     crypto_util.h
 * @version
 * @brief
 *
 * @copyright
 **********************************************************************************************************************/
#ifndef _RT584_CRYPTO_UTIL_H__
#define _RT584_CRYPTO_UTIL_H__


#ifdef __cplusplus
extern "C"
{
#endif
/***********************************************************************************************************************
 *    GLOBAL PROTOTYPES
 **********************************************************************************************************************/
/**
 * @brief     This function performs endian transfer,
 *
 * @param[in]   src         The input buffer for data to be transferred.
 * @param[out]  dest        the ouput buffer to save data be transferred.
 * @param[out]  length      The size in bytes unit of data transfer.
 *
 * @return
 * @retval    STATUS_INVALID_PARAM
 * @retval    STATUS_SUCCESS
 *
 * @details
 *             This function will transfer data endian.
 *             If input buffer is big endian (network data default), it will be transferred to little endian.
 *             If input buffer is little endian, it will be transferred to big endian.
 *
 */
uint32_t convert_endian(uint8_t *src, uint8_t *dest, uint32_t size);


/**
 * @brief     This function performs endian transfer... this is fast version of convert_endian
 *
 * @param[out]  endian_result_buf    the ouput buffer to save data be transferred. pointer must be 4 bytes alignment.
 * @param[in]   origin_buf           The input buffer for data to be transferred. pointer must be 4 bytes alignment.
 *
 * @param[out]  length               The size in 4-bytes unit of data transfer.
 *
 * @return
 * @retval    STATUS_INVALID_PARAM
 * @retval    STATUS_SUCCESS
 *
 * @details
 *             This function will transfer data endian.
 *             If input buffer is big endian (network data default), it will be transferred to little endian.
 *             If input buffer is little endian, it will be transferred to big endian.
 *             Source buffer should NOT overlap destination buffer.
 *
 */
uint32_t buffer_endian_exchange(uint32_t *endian_result_buf, uint32_t *origin_buf, uint32_t buf_length);
/**
 * @brief
 * @param[in]
 *
 * @param[out]
 *
 * @return
 * @retval
 * @retval
 *
 * @details
 *
 *
 *
 *
 *
 */
void crypto_parm_copy_p1(uint32_t *p_dest_addr, uint32_t *p_src_addr, uint32_t size);
/**
 * @brief
 * @param[in]
 *
 * @param[out]
 *
 * @return
 * @retval
 * @retval
 *
 * @details
 *
 *
 *
 *
 *
 */
void crypto_copy(uint32_t *p_dest_addr, uint32_t *p_src_addr, uint32_t size);

/**
 * @brief     This function performs crypto function initial
 *
 * @param     none
 *
 * @return
 *             none
 *
 * @details
 *             This function initial crypto library. Because hardware secure engine
 *             only support one function between AES, SHA, ECC. Before executing secure
 *             hardware algorithm, hardware need to load micro-firmware instruction.
 *             Some secure algorithm need switch for more hardware function to
 *             finish the request. So system need to do "resource control".
 *             In multi-tasking environment, this function will create a semaphore to
 *             protect secure hardware resource, too. (This depends on OS)
 */
void crypto_lib_init(void);

#ifdef __cplusplus
extern "C"
}
#endif

#endif
