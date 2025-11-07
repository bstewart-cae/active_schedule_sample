
/**************************************************************************//**
 * @file     rf_mcu_ahb.c
 * @version
 * @brief    Communication subsystem with AHB

 ******************************************************************************/


#include "cm33.h"
#include "rf_mcu_ahb.h"
#include "stdio.h"


#define RF_MCU_USING_REG_FIELD     (TRUE)

void RfMcu_DmaIsrDefaultCb(void);

RF_MCU_DMA_ISR_CB rf_mcu_dma_isr_cb = RfMcu_DmaIsrDefaultCb;

void RfMcu_DmaIsrDefaultCb(void)
{
    return;
}

void RfMcu_MemorySetAhb(uint16_t sys_addr, const uint8_t *p_data, uint16_t data_length)
{
    data_length = (uint16_t)AHB_ALIGN_4(data_length);

    Enter_Critical_Section();
    RfMcu_DmaBusyCheck();
#if (RF_MCU_USING_REG_FIELD)
    RF_MCU_AHB_DMA_ADDR_REG                = (uint32_t)p_data;
    RF_MCU_AHB_DMA1_PTR->DMA_MCU_ADDR      = (sys_addr >> 2);
    RF_MCU_AHB_DMA1_PTR->DMA_LENGTH        = data_length;
    RF_MCU_AHB_DMA2_PTR->DMA_TYPE          = COMM_SUBSYSTEM_DMA_TYPE_MEM_WRITE;
    RF_MCU_AHB_HOST_CTRL_PTR->DMA_EN       = 1;
#else
    COMM_SUBSYSTEM_AHB->COMM_SUBSYSTEM_AHB_ADDR = (uint32_t)p_data;
    COMM_SUBSYSTEM_AHB->COMM_SUBSYSTEM_DMA1 = ((data_length << 16) | ((sys_addr >> 2) & 0x3FFF));
    COMM_SUBSYSTEM_AHB->COMM_SUBSYSTEM_DMA_TYPE = COMM_SUBSYSTEM_DMA_TYPE_MEM_WRITE;
    COMM_SUBSYSTEM_AHB->COMM_SUBSYSTEM_HOST = COMM_SUBSYSTEM_HOST_CTRL_DMA_ENABLE;
#endif
    Leave_Critical_Section();
    RfMcu_DmaBusyCheck();
}


void RfMcu_MemoryGetAhb(uint16_t sys_addr, uint8_t *p_data, uint16_t data_length)
{
    data_length = (uint16_t)AHB_ALIGN_4(data_length);

    Enter_Critical_Section();
    RfMcu_DmaBusyCheck();
#if (RF_MCU_USING_REG_FIELD)
    RF_MCU_AHB_DMA_ADDR_REG                = (uint32_t)p_data;
    RF_MCU_AHB_DMA1_PTR->DMA_MCU_ADDR      = (sys_addr >> 2);
    RF_MCU_AHB_DMA1_PTR->DMA_LENGTH        = data_length;
    RF_MCU_AHB_DMA2_PTR->DMA_TYPE          = COMM_SUBSYSTEM_DMA_TYPE_MEM_READ;
    RF_MCU_AHB_HOST_CTRL_PTR->DMA_EN       = 1;
#else
    COMM_SUBSYSTEM_AHB->COMM_SUBSYSTEM_AHB_ADDR = (uint32_t)p_data;
    COMM_SUBSYSTEM_AHB->COMM_SUBSYSTEM_DMA1 = ((data_length << 16) | ((sys_addr >> 2) & 0x3FFF));
    COMM_SUBSYSTEM_AHB->COMM_SUBSYSTEM_DMA_TYPE = COMM_SUBSYSTEM_DMA_TYPE_MEM_READ;
    COMM_SUBSYSTEM_AHB->COMM_SUBSYSTEM_HOST = COMM_SUBSYSTEM_HOST_CTRL_DMA_ENABLE;
#endif
    Leave_Critical_Section();
    RfMcu_DmaBusyCheck();
}


void RfMcu_IoSetAhb(uint8_t queue_id, const uint8_t *p_data, uint16_t data_length)
{
    Enter_Critical_Section();
    RfMcu_HostWakeUpMcuAhb();
    RfMcu_DmaBusyCheck();
#if (RF_MCU_USING_REG_FIELD)
    RF_MCU_AHB_DMA_ADDR_REG                = (uint32_t)p_data;
    RF_MCU_AHB_DMA1_PTR->DMA_MCU_ADDR      = queue_id;
    RF_MCU_AHB_DMA1_PTR->DMA_LENGTH        = data_length;
    RF_MCU_AHB_DMA2_PTR->DMA_TYPE          = COMM_SUBSYSTEM_DMA_TYPE_IO_WRITE;
    RF_MCU_AHB_HOST_CTRL_PTR->DMA_EN       = 1;
#else
    COMM_SUBSYSTEM_AHB->COMM_SUBSYSTEM_AHB_ADDR = (uint32_t)p_data;
    COMM_SUBSYSTEM_AHB->COMM_SUBSYSTEM_DMA1 = ((data_length << 16) | (queue_id));
    COMM_SUBSYSTEM_AHB->COMM_SUBSYSTEM_DMA_TYPE = COMM_SUBSYSTEM_DMA_TYPE_IO_WRITE;
    COMM_SUBSYSTEM_AHB->COMM_SUBSYSTEM_HOST = COMM_SUBSYSTEM_HOST_CTRL_DMA_ENABLE;

    while ((COMM_SUBSYSTEM_AHB->COMM_SUBSYSTEM_INTR_STATUS & COMM_SUBSYSTEM_DMA_INT_CLR) == 0);
    COMM_SUBSYSTEM_AHB->COMM_SUBSYSTEM_INTR_CLR = COMM_SUBSYSTEM_DMA_INT_CLR;
#endif
    Leave_Critical_Section();
    RfMcu_DmaBusyCheck();
}


void RfMcu_IoGetAhb(uint16_t queue_id, uint8_t *p_data, uint16_t data_length)
{
    Enter_Critical_Section();
    RfMcu_DmaBusyCheck();
#if (RF_MCU_USING_REG_FIELD)
    RF_MCU_AHB_DMA_ADDR_REG                = (uint32_t)p_data;
    RF_MCU_AHB_DMA1_PTR->DMA_MCU_ADDR      = queue_id;
    RF_MCU_AHB_DMA1_PTR->DMA_LENGTH        = data_length;
    RF_MCU_AHB_DMA2_PTR->DMA_TYPE          = COMM_SUBSYSTEM_DMA_TYPE_IO_READ;
    RF_MCU_AHB_HOST_CTRL_PTR->DMA_EN       = 1;
#else
    COMM_SUBSYSTEM_AHB->COMM_SUBSYSTEM_AHB_ADDR = (uint32_t)p_data;
    COMM_SUBSYSTEM_AHB->COMM_SUBSYSTEM_DMA1 = ((data_length << 16) | (queue_id));
    COMM_SUBSYSTEM_AHB->COMM_SUBSYSTEM_DMA_TYPE = COMM_SUBSYSTEM_DMA_TYPE_IO_READ;
    COMM_SUBSYSTEM_AHB->COMM_SUBSYSTEM_HOST = COMM_SUBSYSTEM_HOST_CTRL_DMA_ENABLE;

    while ((COMM_SUBSYSTEM_AHB->COMM_SUBSYSTEM_INTR_STATUS & COMM_SUBSYSTEM_DMA_INT_CLR) == 0);
    COMM_SUBSYSTEM_AHB->COMM_SUBSYSTEM_INTR_CLR = COMM_SUBSYSTEM_DMA_INT_CLR;
#endif

    Leave_Critical_Section();
    RfMcu_DmaBusyCheck();
}


void RfMcu_HostCmdSetAhb(uint8_t cmd)
{
#if (RF_MCU_USING_REG_FIELD)
    RF_MCU_AHB_HOST_CTRL_PTR->HOST_CMD = cmd;
#else
    COMM_SUBSYSTEM_AHB->COMM_SUBSYSTEM_HOST = (cmd & 0xFFUL);
#endif
}


void RfMcu_HostCtrlAhb(uint32_t ctrl)
{
#if (RF_MCU_USING_REG_FIELD)
    RF_MCU_AHB_HOST_CTRL_REG = (ctrl & ~0xFFUL);
#else
    COMM_SUBSYSTEM_AHB->COMM_SUBSYSTEM_HOST = (ctrl & ~0xFFUL);
#endif
}


void RfMcu_InterruptEnableAhb(void)
{
#if (RF_MCU_USING_REG_FIELD)
    RF_MCU_AHB_INTR_EN_REG = (RF_MCU_AHB_INTR_EN_REG | COMM_SUBSYSTEM_INT_ENABLE);
#else
    COMM_SUBSYSTEM_AHB->COMM_SUBSYSTEM_INTR_EN = (COMM_SUBSYSTEM_AHB->COMM_SUBSYSTEM_INTR_EN | COMM_SUBSYSTEM_INT_ENABLE);
#endif
}


void RfMcu_InterruptDisableAhb(void)
{
#if (RF_MCU_USING_REG_FIELD)
    RF_MCU_AHB_INTR_EN_REG = (RF_MCU_AHB_INTR_EN_REG & (~COMM_SUBSYSTEM_INT_ENABLE));
#else
    COMM_SUBSYSTEM_AHB->COMM_SUBSYSTEM_INTR_EN = (COMM_SUBSYSTEM_AHB->COMM_SUBSYSTEM_INTR_EN & (~COMM_SUBSYSTEM_INT_ENABLE));
    //COMM_SUBSYSTEM_AHB->COMM_SUBSYSTEM_INTR_EN = COMM_SUBSYSTEM_INT_ENABLE;
#endif
}

uint16_t RfMcu_InterruptEnGetAhb(void)
{
#if (RF_MCU_USING_REG_FIELD)
    return (uint16_t)(RF_MCU_AHB_INTR_EN_REG & COMM_SUBSYSTEM_INT_ALL_MASK);
#else
    return (uint16_t)(COMM_SUBSYSTEM_AHB->COMM_SUBSYSTEM_INTR_EN & COMM_SUBSYSTEM_INT_ALL_MASK);
#endif
}


void RfMcu_InterruptEnSetAhb(uint16_t int_enable)
{
#if (RF_MCU_USING_REG_FIELD)
    RF_MCU_AHB_INTR_EN_REG = (RF_MCU_AHB_INTR_EN_REG & (~COMM_SUBSYSTEM_INT_ALL_MASK)) | int_enable;
#else
    COMM_SUBSYSTEM_AHB->COMM_SUBSYSTEM_INTR_EN = int_enable;
#endif
}


void RfMcu_InterruptClearAhb(uint32_t value)
{
#if (RF_MCU_USING_REG_FIELD)
    RF_MCU_AHB_INTR_CLR_REG = value;
#else
    COMM_SUBSYSTEM_AHB->COMM_SUBSYSTEM_INTR_CLR = value;
#endif
}


void RfMcu_HostBusySetAhb(bool status)
{
    RF_MCU_AHB_INTR_EN_PTR->HOST_BUSY = (status) ? 1 : 0;
}


void RfMcu_DmaBusyCheck(void)
{
#if (RF_MCU_USING_REG_FIELD)
    while (RF_MCU_AHB_INTR_STATUS_PTR->DMA_IS_BUSY != 0);
#else
    volatile  uint32_t dma_is_busy;
    do
    {
        dma_is_busy = COMM_SUBSYSTEM_AHB->COMM_SUBSYSTEM_INTR_STATUS;
    } while ((dma_is_busy & 0x10000) != 0);
#endif
}


bool RfMcu_RxQueueIsReadyAhb(void)
{
#if (RF_MCU_USING_REG_FIELD)
    if (RF_MCU_RX_INFO_PTR->RX_Q_R_RDY & COMM_SUBSYS_RX_Q_AVAILABLE)
    {
        return TRUE;
    }
#else
    if (COMM_SUBSYSTEM_AHB->COMM_SUBSYSTEM_RX_INFO & COMM_SUBSYS_RX_Q_AVAILABLE)
    {
        return TRUE;
    }
#endif

    return FALSE;
}


uint16_t RfMcu_RxQueueReadAhb(uint8_t *rx_data, RF_MCU_RXQ_ERROR *rx_queue_error)
{
    volatile uint16_t data_length = 0;

    if (!RfMcu_RxQueueIsReadyAhb())
    {
        (*rx_queue_error) = RF_MCU_RXQ_NOT_AVAILABLE;
        return data_length;
    }

    (*rx_queue_error) = RF_MCU_RXQ_GET_SUCCESS;
#if (RF_MCU_USING_REG_FIELD)
    data_length = (RF_MCU_RX_INFO_PTR->RX_PKT_LEN & COMM_SUBSYS_MAX_RX_Q_LEN);
    data_length = (RF_MCU_RX_INFO_PTR->RX_PKT_LEN & COMM_SUBSYS_MAX_RX_Q_LEN);
#else
    data_length = ((COMM_SUBSYSTEM_AHB->COMM_SUBSYSTEM_RX_INFO >> 16) & COMM_SUBSYS_MAX_RX_Q_LEN);
    data_length = ((COMM_SUBSYSTEM_AHB->COMM_SUBSYSTEM_RX_INFO >> 16) & COMM_SUBSYS_MAX_RX_Q_LEN);
#endif
    RfMcu_IoGetAhb(COMM_SUBSYS_RX_QUEUE_ID, rx_data, data_length);

    return data_length;
}


bool RfMcu_EvtQueueIsReadyAhb(void)
{
#if (RF_MCU_USING_REG_FIELD)
    if (RF_MCU_RX_INFO_PTR->RX_Q_R_RDY & COMM_SUBSYS_RX_CMD_Q_AVAILABLE)
    {
        return TRUE;
    }
#else
    if (COMM_SUBSYSTEM_AHB->COMM_SUBSYSTEM_RX_INFO & COMM_SUBSYS_RX_CMD_Q_AVAILABLE)
    {
        return TRUE;
    }
#endif

    return FALSE;
}


uint16_t RfMcu_EvtQueueReadAhb(uint8_t *evt, RF_MCU_RX_CMDQ_ERROR *rx_evt_error)
{
    volatile uint16_t evt_length = 0;

    if (!RfMcu_EvtQueueIsReadyAhb())
    {
        (*rx_evt_error) = RF_MCU_RX_CMDQ_NOT_AVAILABLE;
        return evt_length;
    }

    (*rx_evt_error) = RF_MCU_RX_CMDQ_GET_SUCCESS;
#if (RF_MCU_USING_REG_FIELD)
    evt_length = (RF_MCU_TX_INFO_PTR->CMDR_LEN & COMM_SUBSYS_MAX_RX_CMDQ_LEN);
    evt_length = (RF_MCU_TX_INFO_PTR->CMDR_LEN & COMM_SUBSYS_MAX_RX_CMDQ_LEN);
#else
    evt_length = ((COMM_SUBSYSTEM_AHB->COMM_SUBSYSTEM_TX_INFO >> 16) & COMM_SUBSYS_MAX_RX_CMDQ_LEN);
    evt_length = ((COMM_SUBSYSTEM_AHB->COMM_SUBSYSTEM_TX_INFO >> 16) & COMM_SUBSYS_MAX_RX_CMDQ_LEN);
#endif
    RfMcu_IoGetAhb(COMM_SUBSYS_RX_CMD_QUEUE_ID, evt, evt_length);

    return evt_length;
}


bool RfMcu_TxQueueIsOccupiedAhb(uint8_t queue_id)
{
#if (RF_MCU_USING_REG_FIELD)
    if (!(RF_MCU_TX_INFO_PTR->TX_Q_W_RDY & (1UL << queue_id)))
    {
        return TRUE;
    }
#else
    if (((COMM_SUBSYSTEM_AHB->COMM_SUBSYSTEM_TX_INFO >> queue_id) & 0x1) != 1)
    {
        return TRUE;
    }
#endif

    return FALSE;
}


bool RfMcu_TxQueueFullCheckAhb(void)
{
#if (RF_MCU_USING_REG_FIELD)
    if (!(RF_MCU_TX_INFO_PTR->TX_Q_W_RDY & 0x7F))
    {
        return TRUE;
    }
#else
    if (!(COMM_SUBSYSTEM_AHB->COMM_SUBSYSTEM_TX_INFO & 0x7F))
    {
        return TRUE;
    }
#endif

    return FALSE;
}


RF_MCU_TXQ_ERROR RfMcu_TxQueueSendAhb(uint8_t queue_id, const uint8_t *tx_data, uint32_t data_length)
{
    if (RfMcu_TxQueueIsOccupiedAhb(queue_id))
    {
        return RF_MCU_TXQ_FULL;
    }

    RfMcu_IoSetAhb(queue_id, tx_data, data_length);

    return RF_MCU_TXQ_SET_SUCCESS;
}


bool RfMcu_CmdQueueFullCheckAhb(void)
{
#if (RF_MCU_USING_REG_FIELD)
    if (!(RF_MCU_TX_INFO_PTR->TX_Q_W_RDY & COMM_SUBSYS_TX_CMD_Q_AVAILABLE))
    {
        return TRUE;
    }
#else
    if (!(COMM_SUBSYSTEM_AHB->COMM_SUBSYSTEM_TX_INFO & COMM_SUBSYS_TX_CMD_Q_AVAILABLE))
    {
        return TRUE;
    }
#endif

    return FALSE;
}


RF_MCU_TX_CMDQ_ERROR RfMcu_CmdQueueSendAhb(const uint8_t *cmd, uint32_t cmd_length)
{
    //uint32_t i =0;
    if (RfMcu_CmdQueueFullCheckAhb())
    {
        return RF_MCU_TX_CMDQ_FULL;
    }
    /*
        info("\r\n cmd: ");
    for(i =0 ; i < cmd_length; i++)
    {
        info("%X ",cmd[i]);
    }
    info("\r\n");
        */
    RfMcu_IoSetAhb(COMM_SUBSYS_TX_CMD_QUEUE_ID, cmd, cmd_length);

    return RF_MCU_TX_CMDQ_SET_SUCCESS;
}

RF_MCU_STATE RfMcu_McuStateReadAhb(void)
{
#if (RF_MCU_USING_REG_FIELD)
    return (RF_MCU_STATE)RF_MCU_TX_INFO_PTR->MCU_STATE;
#else
    return (RF_MCU_STATE)((COMM_SUBSYSTEM_AHB->COMM_SUBSYSTEM_TX_INFO & 0xFF00) >> 8);
#endif
}


void RfMcu_SysRdySignalWaitAhb(void)
{
#if (RF_MCU_USING_REG_FIELD)
    while (RF_MCU_SYS_INFO_PTR->SYS_READY != 1);
#else
    uint32_t status;
    do
    {
        status = COMM_SUBSYSTEM_AHB->COMM_SUBSYSTEM_INFO;
    } while (((status & COMM_SUBSYSTEM_SYS_RDY) != 1));
#endif
}


RF_MCU_PWR_STATE RfMcu_PowerStateGetAhb(void)
{
#if (RF_MCU_USING_REG_FIELD)
    return (RF_MCU_PWR_STATE)(RF_MCU_SYS_INFO_PTR->PWR_STATE);
#else
    return (RF_MCU_PWR_STATE)((COMM_SUBSYSTEM_AHB->COMM_SUBSYSTEM_INFO & 0x6) >> 1);
#endif
}


void RfMcu_HostWakeUpMcuAhb (void)
{
    while (RfMcu_PowerStateGetAhb() != RF_MCU_PWR_STATE_NORMAL)
    {
        RfMcu_HostCtrlAhb(COMM_SUBSYSTEM_HOST_CTRL_WAKE_UP);
    }
}


void RfMcu_DmaIsrCbRegistration(RF_MCU_DMA_ISR_CB cb)
{
    if (cb)
    {
        rf_mcu_dma_isr_cb = cb;
    }
}

void RfMcu_AhbIsrHandler(COMM_SUBSYSTEM_ISR_t isr_cb)
{
#if (RF_MCU_USING_REG_FIELD)
    uint32_t status;
    status = RF_MCU_AHB_INTR_STATUS_REG;

    RfMcu_HostBusySetAhb(TRUE);

    RfMcu_HostWakeUpMcuAhb();

    if (status & RF_MCU_DMA_DONE_INTR)
    {
        /* Implement AHB DMA done callback if needed */
        rf_mcu_dma_isr_cb();
    }

    if (status & COMM_SUBSYSTEM_INT_STATUS_MASK)
    {
        Enter_Critical_Section();
        isr_cb(status & COMM_SUBSYSTEM_INT_STATUS_MASK);
        Leave_Critical_Section();
    }

    status &= (~COMM_SUBSYSTEM_INT_STATUS_MASK);

    RF_MCU_AHB_INTR_CLR_REG = status;

    RfMcu_HostBusySetAhb(FALSE);
#else
    uint32_t status;
    status = COMM_SUBSYSTEM_AHB->COMM_SUBSYSTEM_INTR_STATUS;

    if (status & COMM_SUBSYSTEM_DMA_INT_CLR)
    {
        COMM_SUBSYSTEM_AHB->COMM_SUBSYSTEM_INTR_CLR = COMM_SUBSYSTEM_DMA_INT_CLR;
    }

    if (status & COMM_SUBSYSTEM_INT_STATUS_MASK)
    {
        Enter_Critical_Section();
        isr_cb((status & 0xFF));
        Leave_Critical_Section();
    }
#endif
}


