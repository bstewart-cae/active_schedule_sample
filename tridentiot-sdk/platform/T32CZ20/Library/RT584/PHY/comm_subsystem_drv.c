
/**************************************************************************//**
 * @file     comm_subsystem_drv.c
 * @version
 * @brief    Use comm_subsystem driver to enable comm_subsystem SRAM auto deep sleep

 ******************************************************************************/


#include "rf_mcu_types.h"

#define COMM_SUBSYSTEN_PMU_MEM_CTRL_ADDR (0x42C)
#define REG_LENGTH        (4)

void Commsubsystem_Reg_Ctrl_Init(void)
{
    /* Enable HOST mode */
    COMM_SUBSYSTEM_AHB->COMM_SUBSYSTEM_HOST = COMM_SUBSYSTEM_HOST_CTRL_ENABLE_HOST_MODE;

    /* System reset */
    COMM_SUBSYSTEM_AHB->COMM_SUBSYSTEM_HOST = COMM_SUBSYSTEM_HOST_CTRL_RESET;

    /* DMA init */
    COMM_SUBSYSTEM_AHB->COMM_SUBSYSTEM_HOST = COMM_SUBSYSTEM_HOST_CTRL_WAKE_UP;
    COMM_SUBSYSTEM_AHB->COMM_SUBSYSTEM_HOST = COMM_SUBSYSTEM_HOST_CTRL_RESET;
}
void Commsubsystem_Reg_Ctrl(uint8_t reg_rw, uint8_t *p_data)
{
    uint16_t reg_address = COMM_SUBSYSTEN_PMU_MEM_CTRL_ADDR;
    uint16_t data_length = REG_LENGTH;
    uint32_t status = 0;

    /* DMA transfer set and init */
    COMM_SUBSYSTEM_AHB->COMM_SUBSYSTEM_AHB_ADDR = (uint32_t)p_data;
    COMM_SUBSYSTEM_AHB->COMM_SUBSYSTEM_DMA1 = ((data_length << 16) | ((reg_address >> 2) & 0x3FFF));
    COMM_SUBSYSTEM_AHB->COMM_SUBSYSTEM_DMA_TYPE = reg_rw;
    COMM_SUBSYSTEM_AHB->COMM_SUBSYSTEM_HOST = COMM_SUBSYSTEM_HOST_CTRL_DMA_ENABLE;

    /* Polling DMA interrupt status until it is not pending */
    do
    {
        status = COMM_SUBSYSTEM_AHB->COMM_SUBSYSTEM_INTR_STATUS;
    } while ((status & COMM_SUBSYSTEM_DMA_INT_CLR) == 0);

    /* DMA interrupt status clear */
    COMM_SUBSYSTEM_AHB->COMM_SUBSYSTEM_INTR_CLR = COMM_SUBSYSTEM_DMA_INT_CLR;
}

void Comm_Subsystem_Sram_Deep_Sleep_Init(void)
{
    uint32_t status;
    uint8_t reg_val[4];

    /* Enable dma and system reset */
    Commsubsystem_Reg_Ctrl_Init();

    do
    {
        status = COMM_SUBSYSTEM_AHB->COMM_SUBSYSTEM_INFO;
    } while (((status & COMM_SUBSYSTEM_SYS_RDY) != 1));

    /* Enable communication subsystem SRAM Shutdown control */
    Commsubsystem_Reg_Ctrl(COMM_SUBSYSTEM_DMA_TYPE_MEM_READ, reg_val);
    reg_val[0] = (0xFF);
    reg_val[1] = (0xFF);
    reg_val[2] = (0xFF);
    Commsubsystem_Reg_Ctrl(COMM_SUBSYSTEM_DMA_TYPE_MEM_WRITE, reg_val);
}

uint32_t Get_Comm_Subsystem_Rtc_Cnt_Slow(void)
{
    COMM_SUBSYSTEM_AHB->COMM_SUBSYSTEM_INTR_EN |= 0x20000;

    return COMM_SUBSYSTEM_AHB->COMM_SUBSYSTEM_RTC_SLOW;
}


