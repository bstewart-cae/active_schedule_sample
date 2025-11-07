/**
 ******************************************************************************
 * @file    retarget.c
 * @author
 * @brief   printf is usually used for ouput debug message.
* In most case, we only output debug message in non-secure world.
* So this retarget_drv.c should only runs in non-secure world.
*
* But in early develop stage, maybe we need to printf debug message
* in secure world, too. For this reason, we add one "secure version" retarget_drv_sec.c
* That is output function handle in secure world.
* For non-secure world, the output message will call secure world API to
* output the non-secure message. All input message (getchar) also send to non-secure world.
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



/***********************************************************************************************************************
 *    INCLUDES
 **********************************************************************************************************************/
#include "retarget.h"

/***********************************************************************************************************************
 *    CONSTANTS AND DEFINES
 **********************************************************************************************************************/
#define txbufsize              512
#define rxbufsize              128
#define txbuf_mask            (txbufsize -1)
#define rxbuf_mask            (rxbufsize -1)

#define  STATE_SEND_IDLE            0
#define  STATE_SEND_BUSY            1

ring_buf_t         sendbuf;
static volatile uint8_t   send_state;
static uint16_t           send_num;

ring_buf_t         recvbuf;
static uint8_t            tempbuffer[4];
/***********************************************************************************************************************
 *    GLOBAL FUNCTIONS
 **********************************************************************************************************************/
/**
* @brief printf callback function
*/
static void printf_callback(uint32_t event, void *p_context)
{
    uint16_t   wr_index, rd_index;
    uint16_t   data_in_fifo;

    /*Except UART_EVENT_TX_DONE, this callback function does NOT care any other event*/
    /*This is ISR, so it is already in priority mode*/

    if (event & UART_EVENT_TX_DONE)
    {

        sendbuf.rd_idx += send_num;
        send_num = 0;

        /*check is there data in buffer need to be send?*/

        rd_index = sendbuf.rd_idx & txbuf_mask;
        wr_index = sendbuf.wr_idx & txbuf_mask;

        if (sendbuf.rd_idx == sendbuf.wr_idx)
        {
            /* buffer empty ---
               except ISR printf, sendbuf.wr_idx will not changed */
            send_state = STATE_SEND_IDLE;
        }
        else
        {
            if (rd_index <= wr_index)
            {
                /* if fact, rd_index will be less than wr_index
                 * equal is empty
                 */
                send_num = wr_index - rd_index;
            }
            else
            {
                send_num = txbufsize - rd_index;
            }

            Uart_Tx(0, &(sendbuf.ring_buf[rd_index]), (uint32_t) send_num);
        }
    }

    if (event & UART_EVENT_RX_DONE)
    {

        data_in_fifo = (recvbuf.wr_idx - recvbuf.rd_idx) & recvbuf.bufsize_mask;

        if (data_in_fifo < rxbuf_mask)
        {
            recvbuf.ring_buf[(recvbuf.wr_idx & recvbuf.bufsize_mask)] = tempbuffer[0];
            recvbuf.wr_idx++;      /*so now wr_idex will pointer to next free bytes*/
        }
        else
        {
            /*drop this input buffer... because no space for input*/
            /*recv buffer full.. so we need to drop data ---
              if RT570 care this data, it should get data from recvbuffer ASAP
              or more buffer for received buffer. default is 128 bytes.*/
        }

        Uart_Rx(0, tempbuffer, 1);
    }

    return ;
}

/**
* @brief Initialize stdout
*/
uint32_t Console_Drv_Init(uart_baudrate_t baudrate)
{

    uart_config_t   debug_console_drv_config;
    uint32_t        status;
    static uint8_t  xsend_buf[txbufsize];
    static uint8_t  xrecv_buf[rxbufsize];


    /*init ringbuf for TX*/
    sendbuf.ring_buf = xsend_buf;
    sendbuf.bufsize_mask = txbuf_mask;
    sendbuf.wr_idx = sendbuf.rd_idx = 0;
    send_state = STATE_SEND_IDLE;

    send_num = 0;

    recvbuf.ring_buf = xrecv_buf;
    recvbuf.bufsize_mask = rxbuf_mask;
    recvbuf.wr_idx = recvbuf.rd_idx = 0;

    /*init debug console uart0, 8bits 1 stopbit, none parity, no flow control.*/
    debug_console_drv_config.baudrate = baudrate;
    debug_console_drv_config.databits = UART_DATA_BITS_8;
    debug_console_drv_config.hwfc     = UART_HWFC_DISABLED;
    debug_console_drv_config.parity   = UART_PARITY_NONE;

    /* Important: p_contex will be the second parameter in uart callback.
     * In this example, we do NOT use p_context, (So we just use NULL)
     * but you can use it for whatever you want.
     */
    debug_console_drv_config.p_context = (void *) NULL;

    debug_console_drv_config.stopbit  = UART_STOPBIT_ONE;
    debug_console_drv_config.interrupt_priority = IRQ_PRIORITY_LOW;

    status = Uart_Init(0, &debug_console_drv_config, printf_callback);

    if (status != STATUS_SUCCESS)
    {
        /*almost impossible for this error...*/
        return  status;
    }

    /*uart device is auto power on in uart_init function */
    Uart_Rx(0, tempbuffer, 1);

    return (STATUS_SUCCESS);
}
/**
* @brief uart sned data function
*/
static void Uart_Send(int ch)
{
    uint16_t   data_in_fifo, free_space_in_fifo;

    uint16_t   rd_index;

    /* this uart_send has two version:
     * 1. for only single thread/FSM version: printf is only called by one task at any time.
     * 2. there are multiple tasks version: printf maybe called for any thread at the same time.
     */

wait_free_space:

    Enter_Critical_Section();

    data_in_fifo = (sendbuf.wr_idx - sendbuf.rd_idx) & txbuf_mask;
    free_space_in_fifo = txbuf_mask - data_in_fifo;

    if (free_space_in_fifo)
    {
        sendbuf.ring_buf[(sendbuf.wr_idx & txbuf_mask)] = (uint8_t) ch;
        sendbuf.wr_idx++;
    }
    else
    {
        /*no space for printf buffer, relese critical section first*/

        Leave_Critical_Section();

        if (SCB->ICSR & SCB_ICSR_VECTACTIVE_Msk)
        {
            /*running in interrupt mode*/
            return ;        /*no space left, so drop*/
        }
        else
        {
            /*For multi-tasking environment, it can release CPU here*/
            goto wait_free_space;
        }
    }

    /* check current TX state ---
     * in multiple task environment, this function is protected by disable interrupt.
     */
    if (send_state == STATE_SEND_IDLE)
    {
        /*check rd position, wr position*/

        rd_index = sendbuf.rd_idx & txbuf_mask;
        send_num = 1;

        send_state = STATE_SEND_BUSY;

        Leave_Critical_Section();

        Uart_Tx(0, &(sendbuf.ring_buf[rd_index]), (uint32_t) send_num);
    }
    else
    {
        /*uart0 is busy in transmit, so data will send next interrupt done.*/
        Leave_Critical_Section();
    }

    return;
}
/**
* @brief uart sleep function
*/
/*This function is for sleep function*/
void Console_Sleep(void)
{
    Enter_Critical_Section();

    Uart_Tx_Abort(0);
    /*reinit console.*/
    sendbuf.wr_idx = sendbuf.rd_idx = 0;
    send_state = STATE_SEND_IDLE;
    send_num = 0;

    Leave_Critical_Section();
}
/**
* @brief check uart0 rx buffder data number
*/
uint32_t Check_Uart0_Rx_Buffer_Data_Num(void)
{
    /*
     * return "current number of character" in rx buffer, it could be changed anytime
     * But in most time, application just want to know is there data in rx buffer?
     */
    return ((recvbuf.wr_idx - recvbuf.rd_idx) & recvbuf.bufsize_mask);
}



#if defined(__ARMCC_VERSION)

/**
* @brief Put a character to the stdout
* @param[in]  ch  Character to output
* @return The character written, or -1 on write error.
*/
int fputc(int ch, FILE *p_file)
{

    Uart_Send(ch);

    if (ch == 0x0A)
    {
        /*patch for "\n"*/
        Uart_Send(0x0D);
    }

    return (ch);
}

/**
* @brief get a character from the stdout
* @return receive the character data form ring buffer
*/
int fgetc(FILE *p_file)
{
    uint8_t input;

    /*
     * Remak: This function is designed for single thread
     * It does NOT consider multiple threads race condition issue.
     */

    while (recvbuf.wr_idx == recvbuf.rd_idx)
        ;       /*block to until host send data*/

    input = recvbuf.ring_buf[(recvbuf.rd_idx & recvbuf.bufsize_mask)];
    recvbuf.rd_idx++;

    return input;
}

#elif defined(__GNUC__)
/**
* @brief Put a character to the stdout
* @param[in]  ch  Character to output
* @return The character written, or -1 on write error.
*/
int _write(int fd, char *ptr, int len)
{
    int  i = len;

    while (i > 0)
    {

        Uart_Send(*ptr);

        if (*ptr == 0x0A)
        {
            Uart_Send(0x0D);
        }

        ptr++;
        i--;
    }

    return len;
}
/**
* @brief get a character from the stdout
* @return receive the character data form ring buffer
*/
int _read(int fd, char *ptr, int len)
{

    while (recvbuf.wr_idx == recvbuf.rd_idx)
        ;       /*block to until host send data*/

    *ptr = recvbuf.ring_buf[(recvbuf.rd_idx & recvbuf.bufsize_mask)];
    recvbuf.rd_idx++;

    return 1;
}

#endif




