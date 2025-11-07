/**
 ******************************************************************************
 * @file    uart_drv.c
 * @author
 * @brief   uart driver file
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
#include "uart_drv.h"
#include "cm33.h"

/***********************************************************************************************************************
 *    CONSTANTS AND DEFINES
 **********************************************************************************************************************/
#define MAX_NUMBER_OF_UART        3

#define UART0_BASE_CLK            16


// UART flags
#define UART_FLAG_INITIALIZED        (1U << 0)
#define UART_FLAG_POWERED            (1U << 1)
#define UART_FLAG_CONFIGURED         (UART_FLAG_POWERED | UART_FLAG_INITIALIZED)

/*private structure for uart driver used.*/
/***********************************************************************************************************************
*    TYPEDEFS
 **********************************************************************************************************************/
// UART Transfer Information (Run-Time)
typedef struct _UART_TRANSFER_INFO
{
    uint8_t                *rx_buf;        // Pointer to in data buffer
    uint8_t                *tx_buf;        // Pointer to out data buffer
    uint32_t                rx_num;        // Total number of data to be received
    uint32_t                tx_num;        // Total number of data to be send
    uint32_t                rx_cnt;        // Number of data received
    uint32_t                tx_cnt;        // Number of data sent
} uart_transfer_info_t;

/* Remark: Please don't change uart_rx_status_t structure bit setting
 *
 */
typedef struct
{

    uint8_t resv1       : 3;               // Reserver bits.
    uint8_t rx_overflow : 1;               // Receive data overflow detected (cleared on start of next receive operation)
    uint8_t rx_break    : 1;               // Break detected on receive (cleared on start of next receive operation)
    uint8_t rx_framing_error : 1;          // Framing error detected on receive (cleared on start of next receive operation)
    uint8_t rx_parity_error  : 1;          // Parity error detected on receive (cleared on start of next receive operation)
    uint8_t resv2       : 1;               // Reserver bits.

} uart_rx_status_t;


typedef union
{
    uart_rx_status_t   uart_rx_status;
    uint32_t           init_byte;
} status_byte;

// UART Information (Run-Time)
typedef struct _UART_INFO
{

    uart_event_handler_t     cb_event;      // Event callback

    void                     *p_context;

    uart_transfer_info_t     xfer;          // Transfer information

    status_byte              status;        // UART　status flags

    /* "send_active" and "rx_busy" is single byte to avoid
       "race condition" issue */
    uint8_t                  send_active;   // TX Send active flag
    uint8_t                  rx_busy;       // Receiver busy flag

    uint8_t                  flags;         // UART driver flags
} uart_info_t;


typedef struct
{

    UART_T          *uart;       /*based hardware address*/

    uart_info_t     *info;       // Run-Time Information

    /*IRQ number*/
    IRQn_Type       irq_num;     // UART IRQ Number

    uint8_t         uart_id;     // uart id, 0 for uart0, 1 for uart1

    /*DMA information*/
    uint8_t         dma_tx;        // DMA channel for uart tx, if 0 doesn't use DMA for tx
    uint8_t         dma_rx;        // DMA channel for uart rx, if 0 doesn't use DMA for rx

    uint8_t         user_def_recv;    // user has itself recv function handler..

} uart_handle_t;


static uart_info_t uart0_info = {0};
static uart_info_t uart1_info = {0};
static uart_info_t uart2_info = {0};

#define USE_DMA     1

static const uart_handle_t  m_uart_handle[3] =
{
    {   /*UART0 instatnce*/
        UART0,
        &uart0_info,
        Uart0_IRQn,
        0,
#if USE_DMA
        //DMA
        1,
        1,
#else
        //PIO
        0,
        0,
#endif
        0
    },
    {   /*UART1 instatnce*/
        UART1,
        &uart1_info,
        Uart1_IRQn,
        1,
#if USE_DMA && TX_DMA_RX_PIO==0
        //DMA
        1,
        1,
        0
#elif TX_DMA_RX_PIO==1
        //DMA
        1,
        0,
        1
#else

        //PIO
        0,
        0,
        0
#endif



    },
    {   /*UART2 instatnce*/
        UART2,
        &uart2_info,
        Uart2_IRQn,
        2,

#if USE_DMA
        //DMA
        1,
        1,
#else
        //PIO
        0,
        0,
#endif

        0
    }
};

/***********************************************************************************************************************
 *    GLOBAL FUNCTIONS
 **********************************************************************************************************************/
/**
* @brief uart initinal function
* @details config uart buadrate,stop bit, parity bit,interrupt, fifo size,dma mode
* @param[in] uart_id
* @param[in] p_config
* @param[in] uart_event_handler_t uart interrupt call back function
* @return None
*/
uint32_t Uart_Init(uint32_t uart_id,
                   uart_config_t const   *p_config,
                   uart_event_handler_t  event_handler)
{
    const uart_handle_t  *uart_dev;
    uart_info_t          *uart_info;
    UART_T               *uart;

    uint32_t             cval, ier_value, fcr_value;

    if ((uart_id >= MAX_NUMBER_OF_UART) || (p_config == NULL))
    {
        return STATUS_INVALID_PARAM;
    }

    uart_info = m_uart_handle[uart_id].info;

    if (uart_info->flags & UART_FLAG_INITIALIZED)
    {
        // Driver is already initialized
        // Please uninit the uart then init if APP wants to resetting
        return STATUS_INVALID_REQUEST;
    }

    uart_dev = &m_uart_handle[uart_id];
    uart = uart_dev->uart;

    // Initialize UART Run-time Resources

    uart_info->cb_event = event_handler;
    uart_info->p_context = p_config->p_context;

    /*Set rx_busy, rx_overflow, rx_break, rx_framing_error,
      and rx_parity_error as 0．　
      It also set tx_send_active as 0 */
    uart_info->status.init_byte = 0;
    uart_info->send_active = 0;
    uart_info->rx_busy = 0;

    /*set uart config*/

    // Clear UART IRQ
    NVIC_ClearPendingIRQ(uart_dev->irq_num);

#if 0
    // Enable UART peripheral clock
    enable_perclk((UART0_BASE_CLK + uart_dev->uart_id));
#endif

    /*RT584: Disable uart enable first */
    uart->UART_EN = UART_DISABLE;

    /*clear FIFO, REMAKR: FCR is write-only*/
    uart->FCR = FCR_DEFVAL;     /*reset FIFO*/

    // Disable interrupts
    uart->IER = 0;

    /*RT584 set baudrate has itself setting register.*/

    /*set baudrate*/
    uart->DLX = p_config->baudrate & 0xFFFF ;
    uart->FDL = (p_config->baudrate >> UART_BR_FRCT_SHIFT) & 0xFF;

    if (p_config->baudrate & ENABLE_LSM)
    {
        uart->LSM = 1;
    }
    else
    {
        uart->LSM = 0;
    }

    /*bits mode only use two bits.*/
    cval = p_config->databits & 0x03;

    /*set stop bits*/
    if (p_config->stopbit == UART_STOPBIT_TWO)
    {
        cval |= LCR_STOP;
    }

    /*set parity*/
    if (p_config->parity & PARENB)
    {
        cval |= LCR_PARITY;
    }
    if (!(p_config->parity & PARODD))
    {
        cval |= LCR_EPAR;
    }
    if (p_config->parity & CMSPAR)
    {
        cval |= LCR_SPAR;
    }

    uart->LCR = cval;

    uart->MCR = 0;         /*Initial default modem control register.*/

    /*init native DMA architecture setting*/

    uart->xDMA_RX_ENABLE = xDMA_Stop;
    uart->xDMA_TX_ENABLE = xDMA_Stop;

    uart->xDMA_TX_LEN = 0;
    uart->xDMA_RX_LEN = 0;

    /*clear interrupt flag if exists... write 1 clear*/
    uart->ISR = (ISR_RDA_INTR | ISR_THRE_INTR | ISR_RX_TIMEOUT | ISR_DMA_RX_INTR | ISR_DMA_TX_INTR);

    /* RT584 clear all LSR error if exists*/
    uart->LSR = UART_LSR_BRK_ERROR_BITS;

    ier_value = UART_IER_RLSI;      /* should we set this status error */

    fcr_value = 0;

    if (m_uart_handle[uart_id].dma_tx)
    {
        /* enable dma for tx*/
        ier_value |= UART_IER_DMA_TX_INTR;
        fcr_value = FCR_DMA_SELECT;         /*DMA must be enable for FCR*/
    }
    else
    {
        /* pio for tx interrupt enable in uart_tx*/

    }

    if ((m_uart_handle[uart_id].dma_rx) && (uart_dev->user_def_recv != 1))
    {
        /* enable dma for rx*/
        ier_value |= UART_IER_DMA_RX_INTR;
        fcr_value = FCR_DMA_SELECT;
    }
    else
    {
        /* pio for rx -- if user defined recv callback enable, we will use pio rx*/

    }

    if ((uart_id == 1 || uart_id == 2) && (p_config->hwfc))
    {
        /*only uart1 support HW flow control pin*/
        uart->MCR = MCR_HW_AUTOFLOW_EN;    /*Enable hardware CTS to block transmit data*/
        fcr_value |= FCR_RTS_LEVEL_14;     /*14 bytes will set pin RTSn=1*/
        /*when CTS change interrupt event generated */
        ier_value |= UART_IER_MSI ;        /*modem status change interrrupt*/
    }

    if (uart_dev->user_def_recv)
    {
        fcr_value |= FCR_TRIGGER_1;     /*FCR Trigger 1, so RX interrrupt can happen ASAP. */
    }
    else
    {
        fcr_value |=  FCR_TRIGGER_8;
    }

    uart->FCR = fcr_value;
    uart->IER = ier_value;

    NVIC_SetPriority(uart_dev->irq_num, p_config->interrupt_priority);
    NVIC_EnableIRQ(uart_dev->irq_num);

    /*this function auto turn-on the uart device power.*/
    uart_info->flags = (UART_FLAG_POWERED | UART_FLAG_INITIALIZED);

    /*RT584: uart enable after all setting is ready */
    uart->UART_EN = UART_ENABLE;

    return STATUS_SUCCESS;
}
/**
* @brief uart uninitinal function
* @details uninitinal UART
* @param[in] uart_id UART idiention
* @return STATUS_SUCCESS
*/
uint32_t Uart_Uninit(uint32_t uart_id)
{
    uart_info_t          *uart_info;
    UART_T               *uart;

    if (uart_id >= MAX_NUMBER_OF_UART)
    {
        return STATUS_INVALID_PARAM;
    }

    uart = m_uart_handle[uart_id].uart;
    uart->IER = 0;    /*disable all interrupt*/

    /*clear all interrupt if there pending.*/

    /*auto turn off the power*/
    uart_power(uart_id, UART_POWER_OFF);

    uart_info = m_uart_handle[uart_id].info;

    // Reset UART status flags
    uart_info->flags = 0;

    return STATUS_SUCCESS;
}
/**
* @brief uart uninitinal function
* @details uninitinal UART
* @param[in] uart_id UART idiention
* @return STATUS_SUCCESS
*/
uint32_t uart_power(uint32_t uart_id, uint32_t enable)
{
    const uart_handle_t  *uart_dev;
    uart_info_t          *uart_info;
    UART_T               *uart;

    if (uart_id >= MAX_NUMBER_OF_UART)
    {
        return STATUS_INVALID_PARAM;
    }

    uart_dev = &m_uart_handle[uart_id];
    uart_info = m_uart_handle[uart_id].info;

    /*
     * FIXME: this function is used in CMSIS uart architecture driver... but
     * in current desgin uart_power( id, UART_POWER_ON) has been replace
     * by uart_init(...), so system can forget call this function for UART_POWER_ON
     * call uart_init(...) directly.
     */

    uart = uart_dev->uart;

    if (enable)
    {
        /*power-on*/
        if (uart_info->flags & UART_FLAG_POWERED)
        {
            /*already power-on*/
            return STATUS_SUCCESS;
        }

#if 0
        // not implement yet
        // Enable UART peripheral clock
        enable_perclk((UART0_BASE_CLK + uart_dev->uart_id));
#endif

        /*clear FIFO, REMAKR: FCR is write-only*/
        uart->FCR |= FCR_DEFVAL;     /*reset FIFO*/

        /*Remark: we don't change IER setting here if initial*/

        // Clear driver variables
        uart_info->status.init_byte = 0;
        uart_info->send_active = 0;
        uart_info->rx_busy = 0;

        NVIC_ClearPendingIRQ(uart_dev->irq_num);

        if (uart_info->flags & UART_FLAG_INITIALIZED)
        {
            // Clear pending UART interrupts in NVIC
            NVIC_EnableIRQ(uart_dev->irq_num);
        }

        uart_info->flags |= UART_FLAG_POWERED;


    }
    else
    {
        /*power-off*/
        uart->UART_EN = UART_DISABLE;

        // Disable UART IRQ,
        NVIC_DisableIRQ(uart_dev->irq_num);

        uart->xDMA_RX_ENABLE = xDMA_Stop;
        uart->xDMA_TX_ENABLE = xDMA_Stop;

        /*reset FCR receive FIFO and transmit FIFO*/
        uart->FCR |=  FCR_CLEAR_XMIT | FCR_CLEAR_RCVR;

#if 0
        // Disable UART peripheral clock
        disable_perclk((UART0_BASE_CLK + uart_dev->uart_id));
#endif

        // Clear driver variables
        uart_info->status.init_byte = 0;
        uart_info->send_active = 0;
        uart_info->rx_busy = 0;

        uart_info->flags &= ~UART_FLAG_POWERED;

        // Clear pending UART interrupts in NVIC
        NVIC_ClearPendingIRQ(uart_dev->irq_num);
    }

    return STATUS_SUCCESS;
}

/**
* @brief uart Transfer function (DMA Architecture)
* @details uninitinal UART
* @param[in] uart_id UART idiention
* @param[in] p_data data buffer point
* @param[in] length data length
* @return STATUS_SUCCESS
*/
uint32_t Uart_Tx(uint32_t uart_id,
                 uint8_t const *p_data,
                 uint32_t length)
{

    const uart_handle_t  *uart_dev;
    uart_info_t          *uart_info;
    UART_T               *uart;
    int32_t              val;

    if (uart_id >= MAX_NUMBER_OF_UART)
    {
        return STATUS_INVALID_PARAM;
    }

    if ((p_data == NULL) || (length == 0) || (length > 65535))
    {
        // Invalid parameters
        return STATUS_INVALID_PARAM;
    }

    uart_dev = &m_uart_handle[uart_id];
    uart_info = uart_dev->info;
    uart = uart_dev->uart;

    if ((uart_info->flags & UART_FLAG_CONFIGURED) != UART_FLAG_CONFIGURED)
    {
        // UART is not configured (mode not selected)
        return STATUS_NO_INIT;
    }

    if (uart_info->send_active != 0)
    {
        // Send is not completed yet
        return STATUS_EBUSY;
    }

    // Set Send active flag
    uart_info->send_active = 1;

    // Save transmit buffer info
    uart_info->xfer.tx_buf = (uint8_t *)p_data;
    uart_info->xfer.tx_num = length;
    uart_info->xfer.tx_cnt = 0;

    if (uart_dev->dma_tx)
    {
        uart->xDMA_TX_ADDR = (uint32_t) p_data;
        uart->xDMA_TX_LEN  = length;
        uart->xDMA_TX_ENABLE = xDMA_Enable;
    }
    else
    {
        /*interrupt mode*/
        // Fill TX FIFO
        uart = uart_dev->uart;

        if (uart->LSR & UART_LSR_THRE)
        {
            val = 16;
            while ((val--) && (uart_info->xfer.tx_cnt != uart_info->xfer.tx_num))
            {
                uart->THR = uart_info->xfer.tx_buf[uart_info->xfer.tx_cnt++];
            }
        }

        /* It is POSSIBLE that send and receive are different task (in multitasking OS)
         * To avoid this situation, we add critical section protect.
         */
        Enter_Critical_Section();
        // Enable transmit holding register empty interrupt
        uart->IER |= UART_IER_THRI;
        Leave_Critical_Section();
    }

    if (uart_info->cb_event == NULL)
    {
        /*For block mode*/
        while (uart_info->send_active)
            ;
    }

    return STATUS_SUCCESS;

}
/**
* @brief uart Transfer Progress
* @details uninitinal UART
* @param[in] uart_id UART idiention
* @return Uart Transfer atcive flag
*/
bool Uart_Tx_In_Progress(uint32_t uart_id)
{
    return ((m_uart_handle[uart_id].info->send_active) ? true : false);
}


/**
* @brief uart Received function
* @details
*          Please ONLY call this function when
*          (uart->LSR & UART_LSR_DR) is true.
* @param[in] uart_id UART idiention
* @return Uart Recevie data
*/
uint8_t Uart_Rx_GetBytes(uint32_t uart_id)
{
    const uart_handle_t  *uart_dev;
    UART_T               *uart;

    uart_dev = &m_uart_handle[uart_id];
    uart = uart_dev->uart;
    return  (uart->RBR & 0xFF);
}
/**
* @brief uart Transfer Progress
* @details uninitinal UART
* @param[in] uart_id UART idiention
* @return Uart Transfer atcive flag
*/


bool Uart_Trx_Complete(uint32_t uart_id)
{
    const uart_handle_t  *uart_dev;
    UART_T               *uart;
    bool trx_done = FALSE;

    uart_dev = &m_uart_handle[uart_id];
    uart = uart_dev->uart;

    if (((m_uart_handle[uart_id].info->send_active) == 0) && ((uart->LSR & UART_LSR_TEMT) != 0))
    {
        trx_done = TRUE;
    }

    return trx_done;
}
/**
* @brief uart Received function (DMA Architecture)
* @details
* @param[in] uart_id UART idiention
* @param[in] p_data Received data buffer
* @param[in] length Received data length
* @return
* @retval STATUS_INVALID_PARAM
* @retval STATUS_NO_INIT
* @retval STATUS_EBUSY
* @retval STATUS_SUCCESS
*/
uint32_t  Uart_Rx(uint32_t uart_id,
                  uint8_t  *p_data,
                  uint32_t  length)
{
    const uart_handle_t  *uart_dev;
    uart_info_t          *uart_info;
    UART_T               *uart;

    if (uart_id >= MAX_NUMBER_OF_UART)
    {
        return STATUS_INVALID_PARAM;
    }

    if ((p_data == NULL) || (length == 0U) || (length > 65535))
    {
        // Invalid parameters
        return STATUS_INVALID_PARAM;
    }

    uart_dev = &m_uart_handle[uart_id];
    uart_info = uart_dev->info;
    uart = uart_dev->uart;

    if ((uart_info->flags & UART_FLAG_CONFIGURED) != UART_FLAG_CONFIGURED)
    {
        // UART is not configured (mode not selected)
        return STATUS_NO_INIT;
    }

    // Check if receiver is busy
    if (uart_info->rx_busy == 1)
    {
        return STATUS_EBUSY;
    }

    // Set RX busy flag
    uart_info->rx_busy = 1;

    // Save number of data to be received
    uart_info->xfer.rx_num = length;

    /*  Clear RX statuses, clear break, framing_error
     * overflow and parity_error.. almost
     *  Almost IMPOSSIBLE to happen those error.
     */
    uart_info->status.init_byte = 0;

    // Save receive buffer info
    uart_info->xfer.rx_buf = (uint8_t *)p_data;
    uart_info->xfer.rx_cnt =  0;

    if ((m_uart_handle[uart_id].dma_rx) && (uart_dev->user_def_recv != 1))
    {
        // DMA mode
        uart->xDMA_RX_ADDR = (uint32_t) p_data;
        uart->xDMA_RX_LEN  = length;
        uart->xDMA_RX_ENABLE = xDMA_Enable;
    }
    else
    {
        // Enable receive data available interrupt
        /* It is POSSIBLE that send and receive are different task! */
        Enter_Critical_Section();
        uart->IER |= (UART_IER_RDI | UART_IER_RX_TIMEOUT);
        Leave_Critical_Section();
    }

    if (uart_info->cb_event == NULL)
    {
        /*For block mode*/
        while (uart_info->rx_busy)
            ;
    }

    return STATUS_SUCCESS;
}
/**
* @brief uart Received Abourt function
* @details
* @param[in] uart_id UART idiention
* @return None
*/
void Uart_Rx_Abort(uint32_t uart_id)
{
    const uart_handle_t  *uart_dev;
    uart_info_t          *uart_info;
    UART_T               *uart;

    if (uart_id >= MAX_NUMBER_OF_UART)
    {
        return;
    }

    uart_dev = &m_uart_handle[uart_id];
    uart_info = uart_dev->info;

    if ((uart_info->flags & UART_FLAG_CONFIGURED) != UART_FLAG_CONFIGURED)
    {
        // UART is not configured (mode not selected)
        return;
    }

    uart = uart_dev->uart;

    Enter_Critical_Section();
    // Disable receive data available interrupt
    uart->IER &= ~(UART_IER_RDI | UART_IER_RX_TIMEOUT);
    Leave_Critical_Section();

    // If DMA mode - disable DMA channel
    if ((uart_dev->dma_rx) && (uart_info->rx_busy))
    {
        uart->xDMA_RX_ENABLE = xDMA_Stop;
        uart->ISR = ISR_DMA_RX_INTR;
    }

    // Receive FIFO reset
    uart->FCR |= FCR_CLEAR_RCVR;

    // Clear RX busy status
    uart_info->rx_busy = 0;
}
/**
* @brief uart Transfer Abourt function
* @details
* @param[in] uart_id UART idiention
* @return None
*/
void Uart_Tx_Abort(uint32_t uart_id)
{
    const uart_handle_t  *uart_dev;
    uart_info_t          *uart_info;
    UART_T               *uart;

    if (uart_id >= MAX_NUMBER_OF_UART)
    {
        return;
    }

    uart_dev = &m_uart_handle[uart_id];
    uart_info = uart_dev->info;

    if ((uart_info->flags & UART_FLAG_CONFIGURED) != UART_FLAG_CONFIGURED)
    {
        // UART is not configured (mode not selected)
        return;
    }

    uart = uart_dev->uart;

    Enter_Critical_Section();
    // Disable transmit holding register empty interrupt
    uart->IER &= ~UART_IER_THRI;

    // If DMA mode - disable DMA channel
    if ((uart_dev->dma_tx) && (uart_info->send_active))
    {
        uart->xDMA_TX_ENABLE = xDMA_Stop;   /*stop DMA*/
        uart->ISR = ISR_DMA_TX_INTR;        /*clear DMA flag if exist*/
    }

    // Clear Send active flag
    uart_info->send_active = 0;

    /* RT584 FCR is RW, Transmit FIFO reset */
    uart->FCR |= FCR_CLEAR_XMIT;

    Leave_Critical_Section();

    /*if data in tranmist holding register, wait it empty.*/

    while ((uart->LSR & UART_LSR_TEMT) == 0)
        ;

}
/**
* @brief check uart receive data ready status function
* @details
* @param[in] uart_id UART idiention
* @return None
* @retval UART DATA Ready Status
*/

bool Uart_Rx_Ready(uint32_t uart_id)
{
    const uart_handle_t  *uart_dev;
    uart_info_t          *uart_info;
    UART_T               *uart;

    if (uart_id >= MAX_NUMBER_OF_UART)
    {
        return false;   /*Invalid Id*/
    }

    uart_dev = &m_uart_handle[uart_id];
    uart_info = uart_dev->info;

    if ((uart_info->flags & UART_FLAG_CONFIGURED) != UART_FLAG_CONFIGURED)
    {
        // UART is not configured (mode not selected)
        return false;
    }

    uart = uart_dev->uart;

    return ((uart->LSR & UART_LSR_DR) ? true : false);

}
/**
* @brief Get Uart Warp status function
* @details
* @param[in] uart_id UART idiention
* @return None
* @retval
*/
__STATIC_INLINE void Uart_Get_Status_Warp(uint32_t uart_id, uart_status *state)
{
    uint32_t             temp;

    const uart_handle_t  *uart_dev;
    uart_info_t          *uart_info;
    UART_T               *uart;

    if (uart_id >= MAX_NUMBER_OF_UART)
    {
        temp = 0;      /*Invalid device id*/
    }
    else
    {

        uart_dev = &m_uart_handle[uart_id];
        uart_info = uart_dev->info;
        uart = uart_dev->uart;

        temp = (uint32_t) uart_info->status.init_byte;

        if (!(uart->LSR & UART_LSR_TEMT))
        {
            temp |= BIT0;
        }

        if (uart_info->rx_busy)
        {
            temp |= BIT1;
        }

        /* modem status for CTS, CTS is active low
         * so when CTS is low, data transfer,
         * when CTS is high, data stop to transfer
         * (auto by hardware if flow control enable)
         * this is only available for UART1
         */
        if ((uart_id == 1 || uart_id == 2) && (uart->MSR & BIT4))
        {
            temp |= BIT8;
        }
    }

    *((uint32_t *) state) = temp;

    return;
}
/**
* @brief Uart_Status_Get
* @details
* @param[in] uart_id UART idiention
* @return uart_status
* @retval
*/
uart_status Uart_Status_Get(uint32_t uart_id)
{
    uart_status          temp;

    /* It does not check uart device power state here.
     * this function should be called in power state
     */
    Uart_Get_Status_Warp(uart_id, &temp);

    return temp;
}
/**
* @brief Uart Set Break function
* @details
* @param[in] uart_id UART idiention
* @param[in] state UART idiention
* @return uart_status
* @retval STATUS_INVALID_PARAM
* @retval STATUS_EBUSY
* @retval STATUS_SUCCESS
*/

uint32_t Uart_Set_Break(uint32_t uart_id, uint32_t state)
{
    const uart_handle_t  *uart_dev;
    uart_info_t          *uart_info;
    UART_T               *uart;

    if (uart_id >= MAX_NUMBER_OF_UART)
    {
        return STATUS_INVALID_PARAM;    /*Invalid device id*/
    }

    uart_dev = &m_uart_handle[uart_id];
    uart_info = uart_dev->info;


    if ((uart_info->flags & UART_FLAG_POWERED) == 0)
    {
        // UART not powered
        return STATUS_INVALID_REQUEST;
    }

    uart = uart_dev->uart;

    if (state)
    {
        /*Set Break*/
        if ((uart_info->send_active != 0) || !(uart->LSR & UART_LSR_TEMT))
        {
            return STATUS_EBUSY;
        }

        uart->LCR |= LCR_SBC;
        uart_info->send_active = 1;

    }
    else
    {
        /*Clear Break*/
        uart->LCR &= ~LCR_SBC;
        // Clear Send active flag
        uart_info->send_active = 0;
    }

    return STATUS_SUCCESS;

}
/**
* @brief Uart Set Break function
* @details
* @param[in] uart_id UART idiention
* @param[in] state UART idiention
* @return uart_status
* @retval STATUS_INVALID_PARAM
* @retval STATUS_EBUSY
* @retval STATUS_SUCCESS
*/
uint32_t Uart_Set_Modem_Status(uint32_t uart_id, uint32_t state)
{
    const uart_handle_t  *uart_dev;
    UART_T               *uart;


    if ( (uart_id == 0) ||  (uart_id > 2) )
    {
        return STATUS_INVALID_PARAM;    /*Only UART1 support modem RTS*/
    }

    if (( m_uart_handle[uart_id].info->flags & UART_FLAG_POWERED) == 0)
    {
        // UART not powered
        return STATUS_INVALID_REQUEST;
    }


    uart_dev = &m_uart_handle[uart_id];
    uart = uart_dev->uart;

    if (state)
    {
        /*
         * set RTS, nRTS will be low (nRTS is low active)
         * UART other  side can send data to us.
         */
        uart->MCR |= MCR_RTS;
    }
    else
    {
        /*
         * clear RTS, nRTS will be high (nRTS is low active)
         * UART other side SHOULD STOP to send data.
         */
        uart->MCR &= ~MCR_RTS;
    }

    return STATUS_SUCCESS;
}
/**
* @brief set uart clock source
* @details
* @param[in] uart_id UART idiention
* @param[in] uart_clock_src_t clk_src
* @return None
*/
void Uart_Clock_Source(uart_id_t id, uart_clock_src_t clk_src)
{
    if (clk_src == UART_CLK_SRC_RCO1M || clk_src == UART_CLK_SRC_RCO32K)
    {
        PMU_CTRL->SOC_PMU_RCO1M.bit.EN_RCO_1M = 1;
    }

    SYSCTRL->SYS_CLK_CTRL1.reg = (SYSCTRL->SYS_CLK_CTRL1.reg & ~(UART_CLOCK_SOURCE_MASK << (id << 1)));
    SYSCTRL->SYS_CLK_CTRL1.reg = (SYSCTRL->SYS_CLK_CTRL1.reg | (clk_src << (id << 1)));
}
/**
* @brief Receive line interrupt handler
* @details  uint32_t UART_RxLineIntHandler(uart_resources_t *uart_dev)
* @param[in] uart_resources_t     Pointer to uart_handle_t
* @return  Rx Line event mask
* @retval  Rx Line event mask
*/
static uint32_t Uart_Rx_Line_Int_Handler(const uart_handle_t *uart_dev)
{
    uint32_t     lsr, event;
    UART_T       *uart;
    uart_rx_status_t  *status;

    event = 0;

    uart = uart_dev->uart;
    lsr  = uart->LSR;
    uart->LSR = (lsr & UART_LSR_BRK_ERROR_BITS) ;        /*write 1 clear*/

    status = (uart_rx_status_t *) & (uart_dev->info->status);

    // OverRun error
    if (lsr & UART_LSR_OE)
    {
        status->rx_overflow = 1;
        event |= UART_EVENT_RX_OVERFLOW;
    }

    // Parity error
    if (lsr & UART_LSR_PE)
    {
        status->rx_parity_error = 1;
        event |= UART_EVENT_RX_PARITY_ERROR;
    }

    // Break detected
    if (lsr & UART_LSR_BI)
    {
        status->rx_break = 1;
        event |= UART_EVENT_RX_BREAK;
    }

    // Framing error
    if (lsr & UART_LSR_FE)
    {
        status->rx_framing_error = 1;
        event |= UART_EVENT_RX_FRAMING_ERROR;
    }

    return event;
}
/**
* @brief void uart_irqhandler
* @details
* @param[in] uart_dev     Pointer to uart_handle_t
* @return uart_status
*/
static void Uart_IrqHandler(const uart_handle_t  *uart_dev)
{
    uint32_t     isr_status, event, val, event1;
    UART_T       *uart;
    uart_info_t  *uart_info;

    event = 0;
    uart  = uart_dev->uart;

    isr_status = uart->ISR;

    isr_status = isr_status & uart->IER;        /*avoid pio flag in DMA mode.*/

    uart_info = uart_dev->info;

    /*check DMA complete or not, it can not use IIR to check.*/

    if ((isr_status & ISR_DMA_RX_INTR) == ISR_DMA_RX_INTR)
    {
        /*clear RX interrupt*/
        uart->ISR = ISR_DMA_RX_INTR;
        /*stop xDMA for RX*/
        uart->xDMA_RX_ENABLE = xDMA_Stop;
        // Clear RX busy flag and set receive transfer complete event
        uart_info->xfer.rx_cnt = uart_info->xfer.rx_num;
        uart_info->rx_busy = 0;
        event |= UART_EVENT_RX_DONE;
    }

    if ((isr_status & ISR_DMA_TX_INTR) == ISR_DMA_TX_INTR)
    {
        /*clear TX interrupt*/
        uart->ISR = ISR_DMA_TX_INTR;
        /*stop xDMA for RX*/
        uart->xDMA_TX_ENABLE = xDMA_Stop;

        uart_info->xfer.tx_cnt = uart_info->xfer.tx_num;
        uart_info->send_active = 0;

        // Set send complete event
        event |= UART_EVENT_TX_DONE;
    }

    // Receive line status
    if ((isr_status & ISR_LSR_INTR) == ISR_LSR_INTR)
    {
        event |= Uart_Rx_Line_Int_Handler(uart_dev);
    }

    /*only check PIO RX status when IER enable*/
    if (isr_status & (ISR_RDA_INTR | ISR_RX_TIMEOUT))
    {
        /* PIO RX, Get available data from RX FIFO */
        /*clear RX interrupt first... because pio read will take long time.*/
        uart->ISR = (ISR_RDA_INTR | ISR_RX_TIMEOUT);

        while ((uart->LSR & UART_LSR_DR))
        {
            /*2021/11/22 add: patch for user isr to handle uart input message*/
            if (uart_dev->user_def_recv)
            {
                event1 = UART_EVENT_RX_RECV;    /*specail event notify to user AP ASAP*/
                uart_info->cb_event(event1, uart_info->p_context);
            }
            else
            {
                /* Read data from RX FIFO into receive buffer */
                uart_info->xfer.rx_buf[uart_info->xfer.rx_cnt++] = uart->RBR;

                // Check if requested amount of data is received
                if (uart_info->xfer.rx_cnt == uart_info->xfer.rx_num)
                {
                    // Disable RDA interrupt,
                    /* we don't Enter_Critical_Section here.
                       because here is in ISR */
                    uart->IER &= ~(UART_IER_RDI | UART_IER_RX_TIMEOUT);

                    // Clear RX busy flag and set receive transfer complete event
                    uart_info->rx_busy = 0;
                    event |= UART_EVENT_RX_DONE;
                    break;
                }

                // Character time-out indicator
                if ((isr_status & ISR_RX_TIMEOUT) == ISR_RX_TIMEOUT)
                {
                    // Signal RX Time-out event, if not all requested data received
                    if (uart_info->xfer.rx_cnt != uart_info->xfer.rx_num)
                    {
                        event |= UART_EVENT_RX_TIMEOUT;
                    }
                }
            }
        }
    }

    /*only check PIO TX status when IER enable*/
    if ((isr_status & ISR_THRE_INTR) == ISR_THRE_INTR)
    {
        /* PIO RX, Get available data from RX FIFO */
        uart->ISR = ISR_THRE_INTR;      /*clear interrupt source flag first.*/
        val = 16;

        while ((val--) && (uart_info->xfer.tx_num != uart_info->xfer.tx_cnt))
        {
            // Write data to Tx FIFO
            uart->THR = uart_info->xfer.tx_buf[uart_info->xfer.tx_cnt++];
        }

        // Check if all data is transmitted
        if (uart_info->xfer.tx_num == uart_info->xfer.tx_cnt)
        {
            // Disable THRE interrupt
            uart->IER &= ~UART_IER_THRI;
            // Clear TX busy flag
            uart_info->send_active = 0;
            // Set send complete event
            event |= UART_EVENT_TX_DONE;
        }
    }

    // Modem interrupt (UART1 only), and only CTS changed
    if ((uart_dev->uart_id == 1 || uart_dev->uart_id == 2) && (isr_status & ISR_MSR_INTR))
    {
        // Save modem status register
        val = uart->MSR;
        // Check CTS state changed
        if (val & UART_MSR_DCTS)
        {
            uart->MSR = UART_MSR_DCTS;      /*clear MSR_INTR by write 1 to MSR*/
            event |= UART_EVENT_CTS;
        }
    }

    if ((uart_info->cb_event != NULL) && (event != 0))
    {
        uart_info->cb_event(event, uart_info->p_context);
    }

}
/**
* @brief uart interrupt handler
* @details
* @param[in] uart_dev     Pointer to uart_handle_t
* @return uart_status
*/
void UART0_Handler(void)
{
    Uart_IrqHandler(&m_uart_handle[0]);
}

#ifdef BOOTLOADER
__WEAK void UART1_Handler(void)
#else
void UART1_Handler(void)
#endif
{
    Uart_IrqHandler(&m_uart_handle[1]);
}

#ifdef BOOTLOADER
__WEAK void UART2_Handler(void)
#else
void UART2_Handler(void)
#endif
{
    Uart_IrqHandler(&m_uart_handle[2]);
}



