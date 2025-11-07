/**
 ******************************************************************************
 * @file    trng.h
 * @author
 * @brief   trng driver header file
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


#ifndef _RT584_TRNG_H_
#define _RT584_TRNG_H_

#ifdef __cplusplus
extern "C"
{
#endif



/**************************************************************************************************
 *    INCLUDES
 *************************************************************************************************/
//#include "cm33.h"
//#include "sysctrl.h"
//#include "pufs_rt_regs.h"

#include <stdint.h>

/**************************************************************************************************
 *    CONSTANTS AND DEFINES
 *************************************************************************************************/

#define get_random_number(p_buffer, number) Get_Random_Number(p_buffer, number)

/** @addtogroup Peripheral_Driver RT584Z Periperhal Driver Function
  @{
*/


/** @addtogroup TRNG_DRIVER TRNG Driver Functions
  @{
*/

/***********************************************************************************************************************
 *    GLOBAL PROTOTYPES
 **********************************************************************************************************************/
/**
* @brief get_random_numberk
* @details
* @param[in] p_buffer,
* @param[in] number,
* @retval    STATUS_SUCCESS           If uninitialization was successful.
*/
uint32_t Get_Random_Number(uint32_t *p_buffer, uint32_t number);


/**@}*/ /* end of TRNG_DRIVER group */

/**@}*/ /* end of PERIPHERAL_DRIVER group */

#ifdef __cplusplus
}
#endif

#endif


