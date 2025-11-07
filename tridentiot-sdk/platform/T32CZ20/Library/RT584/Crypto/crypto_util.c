#include <stdio.h>
#include <string.h>

#include "cm33.h"
//#include "project_config.h"
//#if (MODULE_ENABLE(SUPPORT_MULTITASKING))

///*TODO: under construction.*/


//int32_t    crypto_count = 0;

//#endif

uint32_t   crypto_firmware = 0;

/*
 * Notice: This function,if origin_buf is little endian, then endian_result_buf will be big endian.
 * If origin_buf is big endian, endian_resut_buf will be little endian.
 * Please notice: endian_result_buf can not be origin_buf or overlap...
 *
 */
uint32_t buffer_endian_exchange(uint32_t *endian_result_buf, uint32_t *origin_buf, uint32_t buf_length)
{
    uint32_t  i;

    if ((endian_result_buf == NULL) || (origin_buf == NULL) || (buf_length == 0) || (endian_result_buf == origin_buf))
    {
        return STATUS_INVALID_PARAM;
    }

    /*TODO: we should check overlap error*/

    i = buf_length;

    for (i = 0; i < buf_length; i++)
    {
        /*Here we use arm-cortex special instruction to help to transfer endian*/
        endian_result_buf[i] = __REV(origin_buf[buf_length - 1 - i]);
    }

    return STATUS_SUCCESS;
}


uint32_t convert_endian(uint8_t *src, uint8_t *dest, uint32_t size)
{
    uint32_t i;

    if ((src == NULL) || (dest == NULL) || (size == 0) || (src == dest))
    {
        return STATUS_INVALID_PARAM;
    }

    /*TODO: we should check overlap error*/

    /*little_endian <-> big_endian is just  byte change, not include bit reverse.*/
    for (i = 0; i < size; i++)
    {
        dest[size - 1 - i] = src[i];
    }

    return STATUS_SUCCESS;

}

void crypto_parm_copy_p1(uint32_t *p_dest_addr, uint32_t *p_src_addr, uint32_t size)
{
    uint32_t i = 0;

    for (i = 0; i < size; i++ )
    {
        *(p_dest_addr + i) = *(p_src_addr + i);
    }
    *(p_dest_addr + i) = 0;
}

void crypto_copy(uint32_t *p_dest_addr, uint32_t *p_src_addr, uint32_t size)
{
    uint32_t i = 0;

    for (i = 0; i < size; i++ )
    {
        *(p_dest_addr + i) = *(p_src_addr + i);
    }
}

void crypto_lib_init(void)
{
    //#if (MODULE_ENABLE(SUPPORT_MULTITASKING))

    //    /*TODO: under construction for semaphore in secure mode*/

    //    crypto_count = 0;
    //    crypto_firmware = 0;         /*unknow firmware*/

    //#else

    crypto_firmware = 0;        /*unknow firmware*/

    //#endif

    return;
}

