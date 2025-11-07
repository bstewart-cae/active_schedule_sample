
/**************************************************************************//**
 * @file     rf_mcu.c
 * @version
 * @brief    API layer of Communication subsystem

 ******************************************************************************/


#include "cm33.h"
#include "sysctrl.h"
#include "rf_mcu_chip.h"
#include "rf_mcu.h"
#if (CFG_RF_MCU_CTRL_TYPE == RF_MCU_CTRL_BY_AHB)
#include "rf_mcu_ahb.h"
#elif (CFG_RF_MCU_CTRL_TYPE == RF_MCU_CTRL_BY_SPI)
#include "rf_mcu_spi.h"
#else
#error "CFG_RF_MCU_CTRL_TYPE not supported!"
#endif
#if (RF_MCU_CHIP_MODEL == RF_MCU_CHIP_569S)
#include "rt569smp_init.h"
#elif (RF_MCU_CHIP_MODEL == RF_MCU_CHIP_569MP)
#include "rt569mp_init.h"
#elif (RF_MCU_CHIP_MODEL == RF_MCU_CHIP_569M0)
#if (RF_MCU_CHIP_VER == RF_MCU_CHIP_VER_A)
#include "rt569mxa_init.h"
#elif (RF_MCU_CHIP_VER == RF_MCU_CHIP_VER_B)
#include "rt569mxb_init.h"
#endif
#endif
#include "stdio.h"

#if defined(__GNUC__) && (__GNUC__ >= 12)
#pragma GCC push_options
#pragma GCC optimize ("-O1")
#endif

volatile COMM_SUBSYSTEM_ISR_CONFIG gRfMcuIsrCfg;
#if (RF_MCU_PATCH_SUPPORTED)
RF_MCU_INIT_STATUS RfMcu_DefaultEntry(const uint8_t *patch_addr, uint32_t patch_length);
const uint8_t *pPatchAddr = NULL;
uint32_t uiPatchLength = 0;
RF_MCU_PATCH_ENTRY vPatchEntry = RfMcu_DefaultEntry;

const patch_cfg_ctrl_t PATCH_CFG_LIST[] =
{
    {0x0098, {0x01, 0x00, 0xFF, 0x00}},
    {0x009C, {0x03, 0x00, 0x00, 0x00}},
    {0x00A0, {0xA1, 0x8B, 0x00, 0x00}},
    {0x00C0, {0x12, 0xF0, 0x2A, 0x00}},
};

#define PATCH_CFG_NO (sizeof(PATCH_CFG_LIST)/sizeof(PATCH_CFG_LIST[0]))
#endif

#if (RF_MCU_CONST_LOAD_SUPPORTED)
const uint8_t *pRfMcuConstAddr = NULL;
uint32_t uiRfMcuConstLength = 0;
#endif

bool bFwLoaded = false;
bool bFwInitialized = false;
uint32_t vCurrentFw = (uint32_t)NULL;


#if (CFG_RF_MCU_CTRL_TYPE == RF_MCU_CTRL_BY_AHB)

void RfMcu_MemorySet(uint16_t sys_addr, const uint8_t *p_data, uint16_t data_length)
{
    RfMcu_MemorySetAhb(sys_addr, p_data, data_length);
}


void RfMcu_MemoryGet(uint16_t sys_addr, uint8_t *p_data, uint16_t data_length)
{
    RfMcu_MemoryGetAhb(sys_addr, p_data, data_length);
}


void RfMcu_IoSet(uint8_t queue_id, const uint8_t *p_data, uint16_t data_length)
{
    RfMcu_IoSetAhb(queue_id, p_data, data_length);
}


void RfMcu_IoGet(uint16_t queue_id, uint8_t *p_data, uint16_t data_length)
{
    RfMcu_IoGetAhb(queue_id, p_data, data_length);
}


void RfMcu_HostCmdSet(uint8_t cmd)
{
    RfMcu_HostCmdSetAhb(cmd);
}


void RfMcu_HostWakeUpMcu(void)
{
    RfMcu_HostWakeUpMcuAhb();
}


void RfMcu_HostCtrl(uint32_t ctrl)
{
    RfMcu_HostCtrlAhb(ctrl);
}


void RfMcu_HostModeEnable(void)
{
    RfMcu_HostCtrl(COMM_SUBSYSTEM_HOST_CTRL_ENABLE_HOST_MODE);
}


void RfMcu_HostModeDisable(void)
{
    RfMcu_HostCtrl(COMM_SUBSYSTEM_HOST_CTRL_DISABLE_HOST_MODE);
}


void RfMcu_HostResetMcu(void)
{
    RfMcu_HostCtrl(COMM_SUBSYSTEM_HOST_CTRL_RESET);
}

void RfMcu_ChipIdCheck(void)
{
    /* In AHB interface, there's no chip ID information */
}

void RfMcu_InterruptDisableAll(void)
{
    RfMcu_InterruptDisableAhb();
}


void RfMcu_InterruptEnableAll(void)
{
    RfMcu_InterruptEnableAhb();
}


uint16_t RfMcu_InterruptEnGet(void)
{
    return RfMcu_InterruptEnGetAhb();
}


void RfMcu_InterruptEnSet(uint16_t int_enable)
{
    RfMcu_InterruptEnSetAhb(int_enable);
}


void RfMcu_InterruptClear(uint32_t value)
{
    RfMcu_InterruptClearAhb(value);
}


void RfMcu_HostBusySet(bool status)
{
    RfMcu_HostBusySetAhb(status);
}


bool RfMcu_RxQueueCheck(void)
{
    return RfMcu_RxQueueIsReadyAhb();
}


uint16_t RfMcu_RxQueueRead(uint8_t *rx_data, RF_MCU_RXQ_ERROR *rx_queue_error)
{
    return RfMcu_RxQueueReadAhb(rx_data, rx_queue_error);
}


bool RfMcu_EvtQueueCheck(void)
{
    return RfMcu_EvtQueueIsReadyAhb();
}


uint16_t RfMcu_EvtQueueRead(uint8_t *evt, RF_MCU_RX_CMDQ_ERROR *rx_evt_error)
{
    return RfMcu_EvtQueueReadAhb(evt, rx_evt_error);
}


bool RfMcu_CmdQueueFullCheck(void)
{
    return RfMcu_CmdQueueFullCheckAhb();
}


RF_MCU_TX_CMDQ_ERROR RfMcu_CmdQueueSend(const uint8_t *cmd, uint32_t cmd_length)
{
    return RfMcu_CmdQueueSendAhb(cmd, cmd_length);
}


bool RfMcu_TxQueueFullCheck(void)
{
    return RfMcu_TxQueueFullCheckAhb();
}


RF_MCU_TXQ_ERROR RfMcu_TxQueueSendById(uint8_t queue_id, const uint8_t *tx_data, uint32_t data_length)
{
    return RfMcu_TxQueueSendAhb(queue_id, tx_data, data_length);
}


RF_MCU_STATE RfMcu_McuStateRead(void)
{
    return RfMcu_McuStateReadAhb();
}


void RfMcu_SysRdySignalWait(void)
{
    RfMcu_SysRdySignalWaitAhb();
}


uint8_t RfMcu_PowerStateCheck(void)
{
    RF_MCU_PWR_STATE power_state = RfMcu_PowerStateGetAhb();

    return (uint8_t)(power_state);
}


void RfMcu_DmaInit(void)
{
    NVIC_EnableIRQ(CommSubsystem_IRQn);
    RfMcu_InterruptEnSetAhb(COMM_SUBSYSTEM_DMA_INT_ENABLE);
}


void RfMcu_SpiSfrSet(uint8_t sfrOffset, uint8_t value)
{
    return;
}


uint8_t RfMcu_SpiSfrGet(uint8_t sfrOffset)
{
    return 0;
}


void RfMcu_IsrHandler(void)
{
    RfMcu_AhbIsrHandler(gRfMcuIsrCfg.commsubsystem_isr);
}

void RfMcu_IceModeCtrl(bool enable)
{
    /* TO DO or not used*/
}

void RFMcu_LoadExtMemInit(void)
{
    /* In AHB Case, it should not implement now */
}

#elif (CFG_RF_MCU_CTRL_TYPE == RF_MCU_CTRL_BY_SPI)

#define CHECK_SIZE                            8

void RfMcu_MemorySet(uint16_t sys_addr, const uint8_t *p_data, uint16_t data_length)
{
    RfMcu_MemorySetSpi(sys_addr, p_data, data_length);
}


void RfMcu_MemoryGet(uint16_t sys_addr, uint8_t *p_data, uint16_t data_length)
{
    RfMcu_MemoryGetSpi(sys_addr, p_data, data_length);
}


void RfMcu_IoSet(uint8_t queue_id, const uint8_t *p_data, uint16_t data_length)
{
    RfMcu_IoSetSpi(queue_id, p_data, data_length);
}


void RfMcu_IoGet(uint16_t queue_id, uint8_t *p_data, uint16_t data_length)
{
    RfMcu_IoGetSpi(queue_id, p_data, data_length);
}


void RfMcu_HostCmdSet(uint8_t cmd)
{
    RfMcu_HostCmdSetSpi(cmd);
}


void RfMcu_HostWakeUpMcu(void)
{
    RfMcu_HostWakeUpMcuSpi();
}


void RfMcu_HostCtrl(uint32_t ctrl)
{
    /* No used */
}


void RfMcu_HostModeEnable(void)
{
    uint8_t value = RfMcu_SpiGetSFR(RF_MCU_SPI_SFR_ADDR_MODE_SELECT);
    RfMcu_SpiSetSFR(RF_MCU_SPI_SFR_ADDR_MODE_SELECT, (value | RF_MCU_SPI_HOST_MODE_ENABLE_BIT));
}


void RfMcu_HostModeDisable(void)
{
    uint8_t value = RfMcu_SpiGetSFR(RF_MCU_SPI_SFR_ADDR_MODE_SELECT);
    RfMcu_SpiSetSFR(RF_MCU_SPI_SFR_ADDR_MODE_SELECT, (value & ~RF_MCU_SPI_HOST_MODE_ENABLE_BIT));
}


void RfMcu_HostResetMcu(void)
{
    RfMcu_SpiSetSFR(RF_MCU_SPI_SFR_ADDR_HOST_CTRL, RF_MCU_SPI_HOST_CTRL_RESET_BIT);

}

void RfMcu_ChipIdCheck(void)
{
    while (RfMcu_SpiGetSFR(RF_MCU_SPI_SFR_ADDR_CHIP_ID) != RF_MCU_CHIP_ID);
}

void RfMcu_InterruptDisableAll(void)
{
    RfMcu_InterruptDisableSpi();
}


void RfMcu_InterruptEnableAll(void)
{
    RfMcu_InterruptEnableSpi();
}


uint16_t RfMcu_InterruptEnGet(void)
{
    return RfMcu_InterruptEnGetSpi();
}


void RfMcu_InterruptEnSet(uint16_t int_enable)
{
    RfMcu_InterruptEnSetSpi(int_enable);
}


void RfMcu_InterruptClear(uint32_t value)
{
    RfMcu_InterruptClearSpi(value);
}


void RfMcu_HostBusySet(bool status)
{
    UNUSED(status);
}


bool RfMcu_RxQueueCheck(void)
{
    return RfMcu_RxQueueIsReadySpi();
}


uint16_t RfMcu_RxQueueRead(uint8_t *rx_data, RF_MCU_RXQ_ERROR *rx_queue_error)
{
    return RfMcu_RxQueueReadSpi(rx_data, rx_queue_error);
}


bool RfMcu_EvtQueueCheck(void)
{
    return RfMcu_EvtQueueIsReadySpi();
}


uint16_t RfMcu_EvtQueueRead(uint8_t *evt, RF_MCU_RX_CMDQ_ERROR *rx_evt_error)
{
    return RfMcu_EvtQueueReadSpi(evt, rx_evt_error);
}


bool RfMcu_CmdQueueFullCheck(void)
{
    return RfMcu_CmdQueueFullCheckSpi();
}


RF_MCU_TX_CMDQ_ERROR RfMcu_CmdQueueSend(const uint8_t *cmd, uint32_t cmd_length)
{
    return RfMcu_CmdQueueSendSpi(cmd, cmd_length);
}


bool RfMcu_TxQueueFullCheck(void)
{
    return RfMcu_TxQueueFullCheckSpi();
}


RF_MCU_TXQ_ERROR RfMcu_TxQueueSendById(uint8_t queue_id, const uint8_t *tx_data, uint32_t data_length)
{
    return RfMcu_TxQueueSendSpi(queue_id, tx_data, data_length);
}


RF_MCU_STATE RfMcu_McuStateRead(void)
{
    return RfMcu_McuStateReadSpi();
}


void RfMcu_SysRdySignalWait(void)
{
    RfMcu_SysRdySignalWaitSpi();
}


uint8_t RfMcu_PowerStateCheck(void)
{
    RF_MCU_PWR_STATE power_state = RfMcu_PowerStateGetSpi();

    return (uint8_t)(power_state);
}

void RfMcu_GpioIsrHandler(uint32_t pin, void *isr_param)
{
    if (RF_MCU_SPI_GPIO_ISR_SELECT == pin)
    {
        RfMcu_SpiIsrHandler(gRfMcuIsrCfg.commsubsystem_isr);
    }
}

void RfMcu_DmaInit(void)
{
    RfMcu_SpiInit();

    pin_set_mode(RF_MCU_SPI_GPIO_ISR_SELECT, MODE_GPIO);
    gpio_cfg_input(RF_MCU_SPI_GPIO_ISR_SELECT, GPIO_PIN_INT_LEVEL_LOW);
    gpio_register_isr(RF_MCU_SPI_GPIO_ISR_SELECT, RfMcu_GpioIsrHandler, NULL);
    gpio_int_enable(RF_MCU_SPI_GPIO_ISR_SELECT);
}


void RfMcu_SpiSfrSet(uint8_t sfrOffset, uint8_t value)
{
    RfMcu_SpiSetSFR(sfrOffset, value);
}


uint8_t RfMcu_SpiSfrGet(uint8_t sfrOffset)
{
    return RfMcu_SpiGetSFR(sfrOffset);
}


void RfMcu_IsrHandler(void)
{
    /* RfMcu_SpiIsrHandler(gRfMcuIsrCfg.commsubsystem_isr); In SPI Case, it would not receive ISR from Communication_subsystem_ISR */
    BREAK();
}

void RfMcu_IceModeCtrl(bool enable)
{
    RfMcu_SpiIceModeCtrl(enable);
}

RF_MCU_INIT_STATUS RfMcu_CheckFwByByte(uint32_t start_addr, const uint8_t *fw_image, uint32_t image_size)
{
    uint32_t count;
    uint8_t check_count, buff[CHECK_SIZE];
    for (count = 0; count < (image_size / CHECK_SIZE) ; count++)
    {
        RfMcu_MemoryGet(COMM_SUBSYS_PROGRAM_START_ADDR + count * CHECK_SIZE, buff, CHECK_SIZE);
        for (check_count = 0 ; check_count < CHECK_SIZE ; check_count++)
        {
            if (buff[check_count] != fw_image[start_addr + count * CHECK_SIZE + check_count])
            {
                return RF_MCU_FIRMWARE_LOADING_FAIL;
            }
        }
    }
    return RF_MCU_INIT_NO_ERROR;
}


RF_MCU_INIT_STATUS RfMcu_CheckPatchByByte(const uint8_t *p_patch_image, uint32_t patch_image_size)
{
    uint32_t count;
    uint8_t check_count, buff[CHECK_SIZE];

    RfMcu_PmPageSelect(RF_MCU_PM_PAGE_SEL_1);

    for (count = 0; count < (patch_image_size / CHECK_SIZE) ; count++)
    {
        RfMcu_MemoryGet(RF_MCU_PATCH_START_ADDR + count * CHECK_SIZE, buff, CHECK_SIZE);
        for (check_count = 0 ; check_count < CHECK_SIZE ; check_count++)
        {
            if (buff[check_count] != p_patch_image[count * CHECK_SIZE + check_count])
            {
                RfMcu_PmPageSelect(RF_MCU_PM_PAGE_SEL_0);
                return RF_MCU_FIRMWARE_LOADING_FAIL;
            }
        }
    }

    RfMcu_PmPageSelect(RF_MCU_PM_PAGE_SEL_0);

    return RF_MCU_INIT_NO_ERROR;
}


RF_MCU_INIT_STATUS RfMcu_CheckFw(const uint8_t *fw_image, uint32_t image_size)
{
    RF_MCU_INIT_STATUS result;
    RfMcu_PmPageSelect(RF_MCU_PM_PAGE_SEL_0);

    if (image_size > COMM_SUBSYS_FW_PAGE_SIZE)
    {
        if ((result = RfMcu_CheckFwByByte(0, fw_image, COMM_SUBSYS_FW_PAGE_SIZE)) == RF_MCU_INIT_NO_ERROR)
        {
            RfMcu_PmPageSelect(RF_MCU_PM_PAGE_SEL_1);
            result = RfMcu_CheckFwByByte(COMM_SUBSYS_FW_PAGE_SIZE, fw_image, image_size - COMM_SUBSYS_FW_PAGE_SIZE);
        }
    }
    else
    {
        result = RfMcu_CheckFwByByte(0, fw_image, image_size);
    }

    return result;
}

void RFMcu_LoadExtMemInit(void)
{
    RfMcu_PhyExtMemInit();
}
#endif


RF_MCU_TXQ_ERROR RfMcu_TxQueueSend(uint8_t *tx_data, uint32_t data_length)
{
    uint32_t txEmptyQueue;
    uint8_t txQueueId;

    RfMcu_HostWakeUpMcu();

    RfMcu_MemoryGet(0x0048, (uint8_t *)&txEmptyQueue, 4);

    if ((txEmptyQueue & 0x7F) == 0x00)
    {
        return RF_MCU_TXQ_FULL;
    }

    for (txQueueId = 0; txQueueId < 7; txQueueId++)
    {
        if (txEmptyQueue & (1UL << txQueueId))
        {
            break;
        }
    }

    return RfMcu_TxQueueSendById(txQueueId, tx_data, data_length);
}


void RfMcu_RegSet(uint16_t reg_address, uint32_t value)
{
    uint8_t tx_data[4];
    tx_data[0] = value & 0xFF;
    tx_data[1] = (value & 0xFF00) >> 8;
    tx_data[2] = (value & 0xFF0000) >> 16;
    tx_data[3] = (value & 0xFF000000) >> 24;
    RfMcu_MemorySet(reg_address, tx_data, 4);
}


uint32_t RfMcu_RegGet(uint16_t reg_address)
{
    uint8_t rx_data[4] = {0};
    uint32_t reg_value;

    RfMcu_MemoryGet(reg_address, rx_data, 4) ;
    reg_value = (rx_data[0]) | (rx_data[1] << 8) | (rx_data[2] << 16) | (rx_data[3] << 24);

    return  reg_value;
}


void RfMcu_BmuReset(void)
{
    RfMcu_RegSet(0x0000, BIT0);
}


void RfMcu_EventQueueReset(void)
{
#define MAX_RF_MCU_EVT_LEN 64
    uint8_t state = 0;
    uint8_t clear_space[MAX_RF_MCU_EVT_LEN];
    RF_MCU_RX_CMDQ_ERROR rx_confirm_error;

    Delay_us(250);

    while (RfMcu_EvtQueueCheck())
    {
        do
        {
            state = (uint8_t)RfMcu_McuStateRead();
            state = state & RF_MCU_STATE_EVENT_DONE;
        } while (RF_MCU_STATE_EVENT_DONE != state);
        RfMcu_HostCmdSet(RF_MCU_STATE_EVENT_DONE);
        state = 0;

        do
        {
            state = (uint8_t)RfMcu_McuStateRead();
            state = state & RF_MCU_STATE_EVENT_DONE;
        } while (0 != state);

        RfMcu_EvtQueueRead(clear_space, &rx_confirm_error);

        Delay_us(50);
    }
}


void RfMcu_SysInitNotify(void)
{
    if (!bFwInitialized)
    {
        while (RfMcu_McuStateRead() != RF_MCU_STATE_INIT_SUCCEED);
        RfMcu_HostCmdSet(RF_MCU_STATE_INIT_SUCCEED);
        while (RfMcu_McuStateRead() == RF_MCU_STATE_INIT_SUCCEED);
        bFwInitialized = true;
    }
}


void RfMcu_PmPageSelect(RF_MCU_PM_PAGE_SEL page)
{
    uint32_t pm_sel_reg;

    pm_sel_reg = RfMcu_RegGet(RF_MCU_PM_SEL_REG_ADDR);
#if (RF_MCU_CHIP_MODEL == RF_MCU_CHIP_569M0)
    pm_sel_reg &= (~BIT0);
#else
    pm_sel_reg &= (~(BIT0 | BIT1));
#endif

    if (page == RF_MCU_PM_PAGE_SEL_0)
    {
        RfMcu_RegSet(RF_MCU_PM_SEL_REG_ADDR, pm_sel_reg);
    }
    else if (page == RF_MCU_PM_PAGE_SEL_1)
    {
        pm_sel_reg |= BIT0;
        RfMcu_RegSet(RF_MCU_PM_SEL_REG_ADDR, pm_sel_reg);
    }
#if (RF_MCU_CHIP_MODEL != RF_MCU_CHIP_569M0)
    else if (page == RF_MCU_PM_PAGE_SEL_2)
    {
        pm_sel_reg |= BIT1;
        RfMcu_RegSet(RF_MCU_PM_SEL_REG_ADDR, pm_sel_reg);
    }
#endif
    else
    {
        //BREAK();
    }
}


void RfMcu_PmToDmControl(bool Enable)
{
    uint32_t pm_sel_reg;

    if (Enable)
    {
        pm_sel_reg = RfMcu_RegGet(RF_MCU_PM_SEL_REG_ADDR);
        pm_sel_reg |= (BIT1);
        RfMcu_RegSet(RF_MCU_PM_SEL_REG_ADDR, pm_sel_reg);
    }
    else
    {
        pm_sel_reg = RfMcu_RegGet(RF_MCU_PM_SEL_REG_ADDR);
        pm_sel_reg &= (~BIT1);
        RfMcu_RegSet(RF_MCU_PM_SEL_REG_ADDR, pm_sel_reg);
    }
}


void RfMcu_ImageLoad(const uint8_t *fw_image, uint32_t image_size)
{
    RfMcu_PmPageSelect(RF_MCU_PM_PAGE_SEL_0);

    if (image_size > 2 * COMM_SUBSYS_FW_PAGE_SIZE)
    {
        RfMcu_MemorySet(COMM_SUBSYS_PROGRAM_START_ADDR, fw_image, COMM_SUBSYS_FW_PAGE_SIZE);

        RfMcu_PmPageSelect(RF_MCU_PM_PAGE_SEL_1);

        RfMcu_MemorySet(COMM_SUBSYS_PROGRAM_START_ADDR,
                        &fw_image[COMM_SUBSYS_FW_PAGE_SIZE],
                        COMM_SUBSYS_FW_PAGE_SIZE);

        RfMcu_PmPageSelect(RF_MCU_PM_PAGE_SEL_2);

        RfMcu_MemorySet(COMM_SUBSYS_PROGRAM_START_ADDR,
                        &fw_image[2 * COMM_SUBSYS_FW_PAGE_SIZE],
                        image_size - 2 * COMM_SUBSYS_FW_PAGE_SIZE);

        RfMcu_PmPageSelect(RF_MCU_PM_PAGE_SEL_0);
    }
    else if (image_size > COMM_SUBSYS_FW_PAGE_SIZE)
    {
        RfMcu_MemorySet(COMM_SUBSYS_PROGRAM_START_ADDR, fw_image, COMM_SUBSYS_FW_PAGE_SIZE);

        RfMcu_PmPageSelect(RF_MCU_PM_PAGE_SEL_1);

        RfMcu_MemorySet(COMM_SUBSYS_PROGRAM_START_ADDR,
                        &fw_image[COMM_SUBSYS_FW_PAGE_SIZE],
                        image_size - COMM_SUBSYS_FW_PAGE_SIZE);

        RfMcu_PmPageSelect(RF_MCU_PM_PAGE_SEL_0);
    }
    else
    {
        RfMcu_MemorySet(COMM_SUBSYS_PROGRAM_START_ADDR, fw_image, image_size);
    }
}


void RfMcu_ImageLoadBank(const uint8_t *bank_image, uint32_t bank_size)
{
    uint32_t imgBankOffset = (COMM_SUBSYS_FW_PAGE_SIZE - bank_size);

    if (bank_size)
    {
        RfMcu_PmPageSelect(RF_MCU_PM_PAGE_SEL_1);

        RfMcu_MemorySet(COMM_SUBSYS_PROGRAM_START_ADDR + imgBankOffset,
                        bank_image,
                        bank_size);

        RfMcu_PmPageSelect(RF_MCU_PM_PAGE_SEL_0);
    }
}


void RfMcu_IsrInit(COMM_SUBSYSTEM_ISR_t isr, uint32_t content)
{
    gRfMcuIsrCfg.commsubsystem_isr  = isr;
    gRfMcuIsrCfg.content            = content; // Legacy
}


#if (RF_MCU_PATCH_SUPPORTED)
void RfMcu_PatchLoad(const uint8_t *patch_addr, uint32_t patch_length)
{
    uint32_t cfg_idx = 1;

    /* Switch Program Segment from ROM to Patch RAM, shall be executed before SPI write Patch RAM */
    RfMcu_MemorySet(PATCH_CFG_LIST[0].cfg_sfr, (uint8_t *)PATCH_CFG_LIST[0].cfg_val, 4);
    RfMcu_PmPageSelect(RF_MCU_PM_PAGE_SEL_1);
    RfMcu_MemorySet(RF_MCU_PATCH_START_ADDR, patch_addr, patch_length);
    RfMcu_PmPageSelect(RF_MCU_PM_PAGE_SEL_0);

    for (; cfg_idx < PATCH_CFG_NO ; cfg_idx++)
    {
        RfMcu_MemorySet((uint16_t)PATCH_CFG_LIST[cfg_idx].cfg_sfr,
                        (uint8_t *)PATCH_CFG_LIST[cfg_idx].cfg_val,
                        (uint16_t)4);
    }
}


RF_MCU_INIT_STATUS RfMcu_PatchCheck(const uint8_t *patch_addr, uint32_t patch_length)
{
#if (CFG_RF_MCU_CTRL_TYPE == RF_MCU_CTRL_BY_SPI)
    RfMcu_CheckPatchByByte(patch_addr, patch_length);
#else
    UNUSED(patch_addr);
    UNUSED(patch_length);
#endif

    return RF_MCU_INIT_NO_ERROR;
}
#endif

extern const uint8_t firmware_const_m0a_16k[];
#if (RF_MCU_CONST_LOAD_SUPPORTED)
void RfMcu_ConstLoad(const uint8_t *p_const, uint32_t const_size)
{
#if (0)
#elif ((RF_MCU_CHIP_MODEL == RF_MCU_CHIP_569MP) || (RF_MCU_CHIP_MODEL == RF_MCU_CHIP_569S))
    /* Please Design RF_MCU_MP_CONST_START_ADDR if needed to allocate const separatedly */
#elif (RF_MCU_CHIP_MODEL == RF_MCU_CHIP_569M0)
    if (const_size)
    {
        RfMcu_MemorySet(RF_MCU_M0_CONST_START_ADDR, p_const, const_size);
    }
#endif
}
#endif


#if (RF_MCU_CHIP_MODEL == RF_MCU_CHIP_569M0)
void RfMcu_ImageLoadM0(const uint8_t *fw_image, uint32_t image_size)
{
#if 0
    uint16_t patch_cfg_reg0         = 0x0098;
    uint8_t patch_cfg_reg0_val[]    = {0x00, 0x00, 0xFF, 0x00};
    uint16_t patch_cfg_reg0_size    = 4;

    RfMcu_PmToDmControl(FALSE);

    /* Disable Patch Control & Enable Patch RAM */
    RfMcu_MemorySet(patch_cfg_reg0, patch_cfg_reg0_val, patch_cfg_reg0_size);

    RfMcu_PmPageSelect(RF_MCU_PM_PAGE_SEL_1);

    RfMcu_MemorySet(RF_MCU_PATCH_START_ADDR,
                    fw_image,
                    (image_size > RF_MCU_PATCH_MAX_SIZE) ? RF_MCU_PATCH_MAX_SIZE : image_size);

    RfMcu_PmPageSelect(RF_MCU_PM_PAGE_SEL_0);

#if (CFG_RF_MCU_CTRL_TYPE == RF_MCU_CTRL_BY_SPI)
    if (RF_MCU_INIT_NO_ERROR != RfMcu_CheckPatchByByte(fw_image,
            (image_size > RF_MCU_PATCH_MAX_SIZE) ? RF_MCU_PATCH_MAX_SIZE : image_size))
    {
        BREAK();
    }
#endif

    if (image_size > RF_MCU_PATCH_MAX_SIZE)
    {
        RfMcu_MemorySet(RF_MCU_M0_PROGRAM_START_ADDR,
                        &fw_image[RF_MCU_PATCH_MAX_SIZE],
                        image_size - RF_MCU_PATCH_MAX_SIZE);
    }

    RfMcu_PmToDmControl(TRUE);
#else
    RfMcu_PmToDmControl(TRUE);

    RfMcu_PmPageSelect(RF_MCU_PM_PAGE_SEL_0);

    RfMcu_MemorySet(COMM_SUBSYS_PROGRAM_START_ADDR,
                    fw_image,
                    image_size);
#endif
}


void RfMcu_PatchDisableM0(void)
{
    uint16_t patch_cfg_reg0         = 0x0098;
    uint8_t patch_cfg_reg0_val[]    = {0x00, 0x00, 0x00, 0x00};
    uint16_t patch_cfg_reg0_size    = 4;

    /* Disable Patch Control & Disable Patch RAM */
    RfMcu_MemorySet(patch_cfg_reg0, patch_cfg_reg0_val, patch_cfg_reg0_size);
}
#endif

#if (RF_MCU_FW_PRELOAD_SUPPORTED)
RF_MCU_INIT_STATUS RfMcu_SysInitFw(
    bool load_image,
    const uint8_t *p_sys_image,
    uint32_t image_size,
    COMM_SUBSYSTEM_ISR_CONFIG rf_mcu_isr_cfg,
    RF_MCU_INIT_STATUS rf_mcu_init_state
)
{
    RF_MCU_INIT_STATUS error = RF_MCU_INIT_NO_ERROR;
    bool bIsHostMode = FALSE;
    bool bFwChanged = false;

#if (RF_MCU_ICE_SUPPORTED)
    load_image = FALSE;
#endif

    if ((bFwLoaded) &&
            (image_size) &&
            (vCurrentFw != (uint32_t)p_sys_image))
    {
        bFwChanged = true;
    }
    else
    {
        bFwChanged = false;
    }

    RfMcu_ChipIdCheck();

    if (bFwChanged)
    {
        RfMcu_HostModeEnable();

        if (rf_mcu_init_state != RF_MCU_INIT_WITHOUT_RESET)
        {
            RfMcu_HostResetMcu();
        }

        RfMcu_SysRdySignalWait();
    }
    else
    {
        RfMcu_SysRdySignalWait();
    }

    if ((!bFwLoaded) || (bFwChanged))
    {
        bIsHostMode = true;
        bFwInitialized = false;
    }

#if (CFG_RF_MCU_CTRL_TYPE == RF_MCU_CTRL_BY_SPI)
    RfMcu_InterruptDisableAll();
#endif
#if (CFG_RF_MCU_CTRL_TYPE == RF_MCU_CTRL_BY_AHB)
    if (!bFwLoaded)
    {
        RfMcu_DmaInit();
    }
#endif

#if (CFG_RF_MCU_CTRL_TYPE == RF_MCU_CTRL_BY_SPI)
#if (RF_MCU_CHIP_TYPE == RF_MCU_TYPE_ASIC)
    RfMcu_IceModeCtrl(false);
#else
    if (load_image == true)
    {
        RfMcu_IceModeCtrl(false);
    }
    else
    {
        RfMcu_IceModeCtrl(true);
    }
#endif
#endif

    if (!bFwLoaded || bFwChanged)
    {
        if ((load_image == true) && (image_size))
        {
#if (RF_MCU_CONST_LOAD_SUPPORTED)
            if (pRfMcuConstAddr && uiRfMcuConstLength)
            {
                RfMcu_ConstLoad(pRfMcuConstAddr, uiRfMcuConstLength);
            }
#endif
#if ((RF_MCU_CHIP_MODEL == RF_MCU_CHIP_569M0) && (RF_MCU_CHIP_BASE == BASE_ROM_TYPE))
            RfMcu_ImageLoadM0(p_sys_image, image_size);
#else
            RfMcu_ImageLoad(p_sys_image, image_size);
#endif
#if (CFG_RF_MCU_CTRL_TYPE == RF_MCU_CTRL_BY_SPI)
            error = RfMcu_CheckFw(p_sys_image, image_size);
#endif

#if (RF_MCU_PATCH_SUPPORTED)
            RfMcu_HostModeDisable();
            RfMcu_SysInitNotify();
            RfMcu_HostModeEnable();
#endif
            vCurrentFw = (uint32_t)p_sys_image;
            bFwLoaded = true;
        }
    }

    RFMcu_LoadExtMemInit();

#if (RF_MCU_PATCH_SUPPORTED)
    if (pPatchAddr && uiPatchLength)
    {
        error = vPatchEntry(pPatchAddr, uiPatchLength);

        if (error != RF_MCU_INIT_NO_ERROR)
        {
            return error;
        }
    }
#elif (RF_MCU_CHIP_MODEL == RF_MCU_CHIP_569M0)
#if (RF_MCU_CHIP_BASE == BASE_ROM_TYPE)
    if (load_image == false)
#endif
    {
        RfMcu_PatchDisableM0();
    }
#endif

    RfMcu_IsrInit(rf_mcu_isr_cfg.commsubsystem_isr, rf_mcu_isr_cfg.content);
    RfMcu_InterruptEnableAll();
    if (bIsHostMode)
    {
        RfMcu_HostModeDisable();
    }

    return error;
}

RF_MCU_INIT_STATUS RfMcu_SysInit(
    bool load_image,
    const uint8_t *p_sys_image,
    uint32_t image_size,
    COMM_SUBSYSTEM_ISR_CONFIG rf_mcu_isr_cfg,
    RF_MCU_INIT_STATUS rf_mcu_init_state
)
{
    RF_MCU_INIT_STATUS error;

    error = RfMcu_SysInitFw(load_image, p_sys_image, image_size,
                            rf_mcu_isr_cfg, rf_mcu_init_state);

    RfMcu_SysInitNotify();

    return error;
}
#else
RF_MCU_INIT_STATUS RfMcu_SysInitFw(
    bool load_image,
    const uint8_t *p_sys_image,
    uint32_t image_size,
    COMM_SUBSYSTEM_ISR_CONFIG rf_mcu_isr_cfg,
    RF_MCU_INIT_STATUS rf_mcu_init_state
)
{
    UNUSED(load_image);
    UNUSED(p_sys_image);
    UNUSED(image_size);
    UNUSED(rf_mcu_isr_cfg);
    UNUSED(rf_mcu_init_state);
    return RF_MCU_INIT_NO_ERROR;
}

RF_MCU_INIT_STATUS RfMcu_SysInit(
    bool load_image,
    const uint8_t *p_sys_image,
    uint32_t image_size,
    COMM_SUBSYSTEM_ISR_CONFIG rf_mcu_isr_cfg,
    RF_MCU_INIT_STATUS rf_mcu_init_state
)
{
    RF_MCU_INIT_STATUS error = RF_MCU_INIT_NO_ERROR;

#if (RF_MCU_ICE_SUPPORTED)
    load_image = FALSE;
#endif

    RfMcu_ChipIdCheck();

    RfMcu_HostModeEnable();

    if (rf_mcu_init_state != RF_MCU_INIT_WITHOUT_RESET)
    {
        RfMcu_HostResetMcu();
    }

    RfMcu_SysRdySignalWait();
#if (CFG_RF_MCU_CTRL_TYPE == RF_MCU_CTRL_BY_SPI)
    RfMcu_InterruptDisableAll();
#endif
#if (CFG_RF_MCU_CTRL_TYPE == RF_MCU_CTRL_BY_AHB)
    RfMcu_DmaInit();
#endif

#if (CFG_RF_MCU_CTRL_TYPE == RF_MCU_CTRL_BY_SPI)
#if (RF_MCU_CHIP_TYPE == RF_MCU_TYPE_ASIC)
    RfMcu_IceModeCtrl(false);
#else
    if (load_image == true)
    {
        RfMcu_IceModeCtrl(false);
    }
    else
    {
        RfMcu_IceModeCtrl(true);
    }
#endif
#endif

    if ((load_image == true) && (image_size))
    {
#if (RF_MCU_CONST_LOAD_SUPPORTED)
        if (pRfMcuConstAddr && uiRfMcuConstLength)
        {
            RfMcu_ConstLoad(pRfMcuConstAddr, uiRfMcuConstLength);
        }
#endif
#if ((RF_MCU_CHIP_MODEL == RF_MCU_CHIP_569M0) && (RF_MCU_CHIP_BASE == BASE_ROM_TYPE))
        RfMcu_ImageLoadM0(p_sys_image, image_size);
#else
        RfMcu_ImageLoad(p_sys_image, image_size);
#endif
#if (CFG_RF_MCU_CTRL_TYPE == RF_MCU_CTRL_BY_SPI)
        error = RfMcu_CheckFw(p_sys_image, image_size);
#endif

#if (RF_MCU_PATCH_SUPPORTED)
        RfMcu_HostModeDisable();
        RfMcu_SysInitNotify();
        RfMcu_HostModeEnable();
#endif
    }

    RFMcu_LoadExtMemInit();

#if (RF_MCU_PATCH_SUPPORTED)
    if (pPatchAddr && uiPatchLength)
    {
        error = vPatchEntry(pPatchAddr, uiPatchLength);

        if (error != RF_MCU_INIT_NO_ERROR)
        {
            return error;
        }
    }
#elif (RF_MCU_CHIP_MODEL == RF_MCU_CHIP_569M0)
#if (RF_MCU_CHIP_BASE == BASE_ROM_TYPE)
    if (load_image == false)
#endif
    {
        RfMcu_PatchDisableM0();
    }
#endif

    RfMcu_IsrInit(rf_mcu_isr_cfg.commsubsystem_isr, rf_mcu_isr_cfg.content);
    RfMcu_InterruptEnableAll();
    RfMcu_HostModeDisable();
    RfMcu_SysInitNotify();

    return error;
}

#endif


#if (RF_MCU_PATCH_SUPPORTED)
RF_MCU_INIT_STATUS RfMcu_DefaultEntry(const uint8_t *patch_addr, uint32_t patch_length)
{
    UNUSED(patch_addr);
    UNUSED(patch_length);
    return RF_MCU_INIT_NO_ERROR;
}


RF_MCU_INIT_STATUS RfMcu_PatchEntry(const uint8_t *patch_addr, uint32_t patch_length)
{
    RF_MCU_INIT_STATUS error = RF_MCU_INIT_NO_ERROR;

    RfMcu_PatchLoad(patch_addr, patch_length);

    error = RfMcu_PatchCheck(patch_addr, patch_length);

    return error;
}


RF_MCU_INIT_STATUS RfMcu_SysInitWithPatch(
    bool load_image,
    const uint8_t *p_sys_image,
    uint32_t sys_image_size,
    const uint8_t *p_patch_image,
    uint32_t patch_image_size,
    COMM_SUBSYSTEM_ISR_CONFIG rf_mcu_isr_cfg,
    RF_MCU_INIT_STATUS rf_mcu_init_state
)
{
    volatile RF_MCU_INIT_STATUS error = RF_MCU_INIT_NO_ERROR;

    if (p_patch_image && patch_image_size)
    {
        pPatchAddr = p_patch_image;
        uiPatchLength = patch_image_size;
        vPatchEntry = RfMcu_PatchEntry;
    }

    error = RfMcu_SysInit(load_image,
                          p_sys_image,
                          sys_image_size,
                          rf_mcu_isr_cfg,
                          rf_mcu_init_state);

    pPatchAddr = NULL;
    uiPatchLength = 0;
    vPatchEntry = RfMcu_DefaultEntry;

    return error;
}
#endif


#if (RF_MCU_CONST_LOAD_SUPPORTED)
RF_MCU_INIT_STATUS RfMcu_SysInitWithConst(
    bool load_image,
    const uint8_t *p_sys_image,
    uint32_t sys_image_size,
    const uint8_t *p_const,
    uint32_t const_size,
    COMM_SUBSYSTEM_ISR_CONFIG rf_mcu_isr_cfg,
    RF_MCU_INIT_STATUS rf_mcu_init_state
)
{
    volatile RF_MCU_INIT_STATUS error = RF_MCU_INIT_NO_ERROR;

    if (p_const && const_size)
    {
        pRfMcuConstAddr = p_const;
        uiRfMcuConstLength = const_size;
    }

    error = RfMcu_SysInit(load_image,
                          p_sys_image,
                          sys_image_size,
                          rf_mcu_isr_cfg,
                          rf_mcu_init_state);

    pRfMcuConstAddr = NULL;
    uiRfMcuConstLength = 0;

    return error;
}
#endif


bool RfMcu_CpuInit(const uint8_t *image_path, uint32_t image_size, RF_MCU_SYSTEM_CONFIG_PTR_t sys_config)
{
    bool status_success;
    COMM_SUBSYSTEM_ISR_CONFIG isr_cfg;

    isr_cfg.commsubsystem_isr = sys_config->rf_mcu_isr;
    isr_cfg.content = 0;

    status_success =
        (RfMcu_SysInit(true, image_path, image_size, isr_cfg, RF_MCU_INIT_NO_ERROR)
         == RF_MCU_INIT_NO_ERROR);

    return status_success;
}


bool RfMcu_CpuStart(RF_MCU_SYSTEM_CONFIG_PTR_t sys_config)
{
    bool status_success;
    COMM_SUBSYSTEM_ISR_CONFIG isr_cfg;

    isr_cfg.commsubsystem_isr = sys_config->rf_mcu_isr;
    isr_cfg.content = 0;

    status_success =
        (RfMcu_SysInit(false, NULL, 0, isr_cfg, RF_MCU_INIT_WITHOUT_RESET)
         == RF_MCU_INIT_NO_ERROR);

    return status_success;
}


void CommSubsys_Handler(void)
{
    RfMcu_IsrHandler();
}


void MemManage_Handler (void)
{
    uint32_t cfsr = SCB->CFSR;

    if (cfsr & SCB_CFSR_MMARVALID_Msk)
    {
        __BKPT(0);
    }

    if (cfsr & SCB_CFSR_MLSPERR_Msk)
    {
        __BKPT(0);
    }

    if (cfsr & SCB_CFSR_MSTKERR_Msk)
    {
        __BKPT(0);
    }

    if (cfsr & SCB_CFSR_MUNSTKERR_Msk)
    {
        __BKPT(0);
    }

    if (cfsr & SCB_CFSR_DACCVIOL_Msk)
    {
        __BKPT(0);
    }

    if (cfsr & SCB_CFSR_IACCVIOL_Msk)
    {
        __BKPT(0);
    }
}


void BusFault_Handler (void)
{
    uint32_t cfsr = SCB->CFSR;

    if (cfsr & SCB_CFSR_BFARVALID_Msk)
    {
        __BKPT(0);
    }

    if (cfsr & SCB_CFSR_LSPERR_Msk)
    {
        __BKPT(0);
    }

    if (cfsr & SCB_CFSR_UNSTKERR_Msk)
    {
        __BKPT(0);
    }

    if (cfsr & SCB_CFSR_IMPRECISERR_Msk)
    {
        __BKPT(0);
    }

    if (cfsr & SCB_CFSR_PRECISERR_Msk)
    {
        __BKPT(0);
    }

    if (cfsr & SCB_CFSR_IBUSERR_Msk)
    {
        __BKPT(0);
    }
}


void UsageFault_Handler(void)
{
    uint32_t cfsr = SCB->CFSR;

    if (cfsr & SCB_CFSR_UNDEFINSTR_Msk)
    {
        __BKPT(0);
    }

    if (cfsr & SCB_CFSR_INVSTATE_Msk)
    {
        __BKPT(0);
    }

    if (cfsr & SCB_CFSR_INVPC_Msk)
    {
        __BKPT(0);
    }

    if (cfsr & SCB_CFSR_NOCP_Msk)
    {
        __BKPT(0);
    }

    if (cfsr & SCB_CFSR_STKOF_Msk)
    {
        __BKPT(0);
    }

    if (cfsr & SCB_CFSR_UNALIGNED_Msk)
    {
        __BKPT(0);
    }

    if (cfsr & SCB_CFSR_DIVBYZERO_Msk)
    {
        __BKPT(0);
    }
}


void HardFault_Handler(void)
{
    __asm volatile (
        "TST lr, #4 \n"
        "ITE EQ \n"
        "MRSEQ r0, MSP \n"
        "MRSNE r0, PSP \n"
        "B HardFault_Handler_C \n"
    );
}


void HardFault_Handler_C(uint32_t *hardfault_args)
{
    uint32_t stacked_r0 = hardfault_args[0];
    uint32_t stacked_r1 = hardfault_args[1];
    uint32_t stacked_r2 = hardfault_args[2];
    uint32_t stacked_r3 = hardfault_args[3];
    uint32_t stacked_r12 = hardfault_args[4];
    uint32_t stacked_lr = hardfault_args[5];
    uint32_t stacked_pc = hardfault_args[6];
    uint32_t stacked_psr = hardfault_args[7];

    uint32_t volatile hfsr = SCB->HFSR;
    uint32_t volatile cfsr = SCB->CFSR;
    uint32_t volatile bfar = SCB->BFAR;
    uint32_t volatile mmfar = SCB->MMFAR;

    // Implement logging for faults here
    UNUSED(stacked_r0);
    UNUSED(stacked_r1);
    UNUSED(stacked_r2);
    UNUSED(stacked_r3);
    UNUSED(stacked_r12);
    UNUSED(stacked_lr);
    UNUSED(stacked_pc);
    UNUSED(stacked_psr);
    UNUSED(hfsr);
    UNUSED(cfsr);
    UNUSED(bfar);
    UNUSED(mmfar);

    __BKPT(0);
}

#if defined(__GNUC__) && (__GNUC__ >= 12)
#pragma GCC pop_options
#endif
