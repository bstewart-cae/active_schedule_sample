/**
 ******************************************************************************
 * @file    trng.c
 * @author
 * @brief   trng driver file
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


/***********************************************************************************************************************
*    INCLUDES
**********************************************************************************************************************/
#include "trng.h"
#include "pufs_rt_regs.h"
#include "sysctrl.h"
#include "rf_mcu.h"
/***********************************************************************************************************************
 *    GLOBAL FUNCTIONS
 **********************************************************************************************************************/
/**
* @brief get random number, using PUF random number
* @return STATUS_SUCCESS
*/

uint32_t Get_Random_Number(uint32_t *p_buffer, uint32_t number)
{
    uint32_t  i, status = STATUS_SUCCESS;

    volatile uint32_t *ptr;

    enter_critical_section();

    Enable_Secure_Write_Protect();

    status = rt_write_rng_enable(true);

    if (status == STATUS_SUCCESS)
    {
        ptr = (volatile uint32_t *) p_buffer;

        for (i = 0; i < number ; ++i)
        {
            *ptr++ = OTP_RNG_S->data;
        }
    }

    rt_write_rng_enable(false);

    Disable_Secure_Write_Protect();

    leave_critical_section();

    return status;
}

