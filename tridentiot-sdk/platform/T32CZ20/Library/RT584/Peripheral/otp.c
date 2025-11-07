/**
 ******************************************************************************
 * @file    otp.c
 * @author
 * @brief   otp driver file
 *
 ******************************************************************************
 * @attention
 * Copyright (c) 2024 Rafael Micro.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE
 * AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * This file is part of library_name.
 */
/*****************************************************************************
 * Include
 ****************************************************************************/
#include "cm33.h"
#include "sysctrl.h"
#include "pufs_rt_regs.h"


/*****************************************************************************
 * RT Internal API functions
 ****************************************************************************/
/**
* @brief trng enable function
*/
uint32_t rt_write_rng_enable(bool fun_en)
{
    bool err_bit = FALSE;
    uint32_t en;
    uint16_t check_cnt = 100;
    uint16_t retry_cnt = 3;
    uint32_t status = STATUS_SUCCESS;

    if (OTP_RNG_S->version == 0x0) // unsupport
    {
        return STATUS_INVALID_REQUEST;
    }

    do
    {
        retry_cnt -= 1;
        err_bit = FALSE;
        en =  OTP_RNG_S->enable;

        if (fun_en)
        {
            SET_BIT(en, RT_RNG_FUN_ENABLE_BITS);
        }
        else
        {
            CLR_BIT(en, RT_RNG_FUN_ENABLE_BITS);
        }

        CLR_BIT(en, RT_RNG_OUT_ENABLE_BITS);

        OTP_RNG_S->enable = en;
        Delay_us(50);

        if (fun_en)
        {
            check_cnt = 100;
            while (!(GET_BIT(OTP_RNG_S->status, RT_RNG_STATUS_RN_READY_BITS)))
            {
                if ((GET_BIT(OTP_RNG_S->status, RT_RNG_STATUS_ERROR_BITS)))
                {
                    err_bit = TRUE;
                    status = STATUS_ERROR;
                    break;
                }

                check_cnt -= 1;
                if (check_cnt == 0)
                {
                    status = STATUS_EBUSY;
                    break;
                }
            }

            if (err_bit == TRUE) //error handling
            {
                OTP_RNG_S->enable = 0x0b;
                OTP_RNG_S->enable = 0x02;
                OTP_RNG_S->htclr = 0x01;
                check_cnt = 100;

                while (!(GET_BIT(OTP_RNG_S->status, RT_RNG_STATUS_FIFO_BITS)))
                {
                    check_cnt -= 1;
                    if (check_cnt == 0)
                    {
                        status = STATUS_TIMEOUT;
                        break;
                    }
                }
            }
            else
            {
                if (status == STATUS_SUCCESS)
                {
                    retry_cnt = 0;
                }
            }
        }
    } while ((err_bit == TRUE) && (retry_cnt > 0));

    return status;
}
/**
* @brief set otp lckwd readonly
*/
void set_otp2_lckwd_readonly(uint32_t number)
{
    uint32_t offset, mask;

    if (number > 255)
    {
        return;    /*otp2 only 1024 bytes. so cde255 is maximum*/
    }

    offset = (number >> 5);

    mask = (0xF << (offset << 2));

    /*lock 128 bytes*/
    OTP_PIF_S->cde_lock[0] |= mask;

}

/**
* @brief set opt lckwd readonly
*/
void set_otp_lckwd_readonly(uint32_t number)
{
    uint32_t offset, mask;

    offset = (number >> 3);

    mask = (0x3 << ((number & 0x7) << 2));

    OTP_PIF_S->otp_lock[offset] |= mask;
}
/**
* @brief set opt lckwd na
*/
void set_otp_lckwd_na(uint32_t number)
{
    uint32_t offset, mask;

    offset = (number >> 3);

    mask = (0xF << ((number & 0x7) << 2));

    OTP_PIF_S->otp_lock[offset] |= mask;
}

uint32_t get_otp_lckwd_state(uint32_t number)
{
    uint32_t offset, value;

    offset = (number >> 3);

    value = (OTP_PIF_S->otp_lock[offset] >> ((number & 0x7) << 2)) & 0xF;

    switch (value)
    {
    case 0:
    case 1:
    case 2:
    case 4:
    case 8:
        return OTP_LCK_RW;

    case 3:
    case 7:
    case 11:
        return OTP_LCK_RO;

    default:
        return OTP_LCK_NA;
    }

}

uint32_t get_otp2_lckwd_state(uint32_t number)
{
    uint32_t offset, value, mask, lock_status;

    if (number > 255)
    {
        return STATUS_ERROR;
    }

    lock_status = OTP_PIF_S->cde_lock[0];

    offset = (number >> 5);                 //number div 128

    mask = (0xF << (offset << 2));

    value = (lock_status & mask) >> (offset << 2);

    switch (value)
    {
    case 0:
    case 1:
    case 2:
    case 4:
    case 8:
        return OTP_LCK_RW;

    default:
        return OTP_LCK_RO;
    }
}


uint32_t rt_otp_write_data(puf_id_t id, uint32_t otp_number, uint32_t data)
{
    volatile uint32_t *otp_addr_ptr;
    uint32_t offset;

    //
    if ((id >= PUF_MAX) || (id == PUF_UID) || (otp_number >= 256))
    {
        return STATUS_INVALID_REQUEST;
    }

    offset = otp_number * 4;

    if (id == PUF_OTP1)
    {

        if (get_otp_lckwd_state(otp_number) != OTP_LCK_RW)
        {
            return STATUS_INVALID_REQUEST;
        }

        otp_addr_ptr = (volatile uint32_t *)(OTP1_BS + offset);
        *otp_addr_ptr = data;
    }
    else if (id == PUF_OTP2)
    {

        if (get_otp2_lckwd_state(otp_number) != OTP_LCK_RW)
        {
            return STATUS_INVALID_REQUEST;
        }

        otp_addr_ptr = (volatile uint32_t *)(OTP2_BS + offset);
        *otp_addr_ptr = data;
    }
    else
    {
        return STATUS_INVALID_REQUEST;
    }

    return STATUS_SUCCESS;

}



uint32_t rt_otp_read_data(puf_id_t id, uint32_t otp_number, uint32_t *buf, uint32_t length)
{
    volatile uint32_t *puf_base_addr = NULL;
    volatile uint32_t *puf_max_addr  = NULL;
    uint32_t i;

    //
    if ((id >= PUF_MAX) || (otp_number >= 256) || ((otp_number + length) > 256))
    {
        return STATUS_INVALID_REQUEST;
    }

    switch (id)
    {
    case PUF_UID:
        puf_base_addr = (volatile uint32_t *)(PUF_BS + (otp_number * 4));
        puf_max_addr  = (volatile uint32_t *)(PUF_BS + 0x400);   // 256  * 4 bytes
        break;

    case PUF_OTP1:
        puf_base_addr = (volatile uint32_t *)(OTP1_BS + (otp_number * 4));
        puf_max_addr = (volatile uint32_t *)(OTP1_BS + 0x400);
        break;

    case PUF_OTP2:
        puf_base_addr = (volatile uint32_t *)(OTP2_BS + (otp_number * 4));
        puf_max_addr = (volatile uint32_t *)(OTP2_BS + 0x400);
        break;

    default:
        return STATUS_INVALID_REQUEST;
    }

    //
    for (i = 0; i < length; i++)
    {
        volatile uint32_t *cur_addr = puf_base_addr + i;
        if (cur_addr < puf_max_addr)
        {
            buf[i] = *cur_addr;
        }
        else
        {
            return STATUS_INVALID_REQUEST; // ??
        }
    }

    return STATUS_SUCCESS;

}
/**
* @brief set otp zeroized
*/
void set_otp_zeroized(uint32_t number)
{
    uint32_t offset;

    if (number >= 256)
    {
        return;
    }

    /*2025/03/03 bug fixed */
    offset = number >> 5;  /*OTP_0 ~OTP7 is one zeroized unit */

    OTP_PTM_S->otp_zeroize = 0x80 + offset;

    /*first wait PTM busy state to 0.*/
    while (OTP_PTM_S->status & BIT0)
        ;

}

/**
* @brief get otp zerized statue
*/
uint32_t get_otp_zeroized_state(uint32_t number)
{
    uint32_t offset, value;
    //uint32_t  *addr ,test_value,mask;

    if (number >= 256)
    {
        return OTP_NOT_ZEROIZED;    /*in fact, it is error*/
    }

    offset = number >> 7;  /*otp_128 in one 4-bytes register */

    number &= 0x7F;

    //mask = 3<<((number>>3)*2);

    value = (OTP_PIF_S->zeroized_otp[offset] >> ((number >> 3) * 2)) & 0x3;

    if (value == 3)
    {
        return OTP_ZEROIZED;
    }
    else
    {
        return OTP_NOT_ZEROIZED;
    }

}

void set_otp_postmasking(uint32_t lock_otp_number)
{
    uint32_t  bit_mask_shift;// lock_otp_reg_index;

    bit_mask_shift = (lock_otp_number >> 3) << 1; /*this is otp_n index */

    if (bit_mask_shift < 32)
    {
        /*postmask in otp_psmsk_0 register offset 0x68*/
        /*set OTP postmsk*/
        OTP_CFG_S->otp_msk[0] |= (0x3 << bit_mask_shift);
    }
    else
    {
        /*postmask in otp_psmsk_1 register offset 0x6C*/
        bit_mask_shift -= 32;
        OTP_CFG_S->otp_msk[1] |= (0x3 << bit_mask_shift);
    }

    return;
}

/*
 * Let OTP_CFG_S register 0x68 and 0x6c to be Read-only.
 * This setting will remain until next POR.
 */
void set_otp_postmasking_lock(void)
{
    OTP_CFG_S->reg_lock = (0xF) << 20;
    return;
}

/**
 * @brief Count 1's
 */
uint32_t count_ones(uint32_t num)
{
    uint32_t ret = 0;

    while (num != 0)
    {
        ++ret;
        num &= (num - 1);
    }

    return ret;
}



uint32_t rt_otp_version(void)
{

    return OTP_CFG_S->version;
}