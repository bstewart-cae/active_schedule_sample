/**
 ******************************************************************************
 * @file    ringbuffer.h
 * @author
 * @brief   ring buffer header file
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

#ifndef _RT584_RINGBUFFER_H__
#define _RT584_RINGBUFFER_H__

#ifdef __cplusplus
extern "C"
{
#endif
/***********************************************************************************************************************
 *    MACROS
 **********************************************************************************************************************/
#define bufsize_mask(X)    (X-1)
/***********************************************************************************************************************
 *    TYPEDEFS
 **********************************************************************************************************************/
/**
 * @brief ringbuffer typedef
 */
typedef struct
{
    uint8_t  *ring_buf;                 /* size should be 2^N, this value is buffersize-1 */
    uint16_t  bufsize_mask;             /* size should be 2^N, this value is buffersize-1 */
    volatile uint16_t  wr_idx;          /* write index must be volatile*/
    volatile uint16_t  rd_idx;          /* read index must be volatile*/
} ring_buf_t;



#ifdef __cplusplus
}
#endif

#endif      /* end of _RT584_RINGBUFFER_H__ */