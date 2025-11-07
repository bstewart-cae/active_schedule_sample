/**
 ******************************************************************************
 * @file    flashctl.c
 * @author
 * @brief   flash control driver file
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
#include "flashctl.h"
#include "sysctrl.h"
#include "project_config.h"
/***********************************************************************************************************************
 *    GLOBAL FUNCTIONS
 **********************************************************************************************************************/
/**
* @brief flash contrl check address
*   check input flash address and length is correct or not.
*/
uint32_t Flash_Check_Address(uint32_t flash_address, uint32_t length)
{

    uint16_t  flash_size_id;

    //get flash size id
    flash_size_id = ( (flash_get_deviceinfo() >> FLASH_SIZE_ID_SHIFT) & 0xFF );

    if ( (flash_address < BOOT_LOADER_END_PROTECT_ADDR) || ( (flash_address + (length - 1) ) >= FLASH_END_ADDR(flash_size_id)) )
    {
        return STATUS_INVALID_PARAM;
    }

    return STATUS_SUCCESS;
}

/**
* @brief Flash_Get_Deviceinfo
*   Get the flash identifies for check flash size.
*/
uint32_t Flash_Get_Deviceinfo(void)
{
    return (FLASH->FLASH_INFO & 0x00FFFFFF);
}
/**
* @brief Flash_Get_flash size
*
*/
flash_size_t Flash_Size(void)
{
    uint32_t  flash_size_id;

    flash_size_id = ( (flash_get_deviceinfo() >> FLASH_SIZE_ID_SHIFT) & 0xFF );

    if (flash_size_id == FLASH_512K)
    {
        return FLASH_512K;
    }
    else if ( flash_size_id == FLASH_1024K)
    {
        return FLASH_1024K;
    }
    else if ( flash_size_id == FLASH_2048K)
    {
        return FLASH_2048K;
    }
    else
    {
        return FLASH_NOT_SUPPORT;
    }
}

/**
* @brief flash_read_page :
*   read one page. One page is 256 bytes, so buf_addr should have 256 bytes available.
*   this is non_block mode... so user should wait flash finish read outside this function.
*/
uint32_t Flash_Read_Page_Ext(uint32_t buf_addr, uint32_t read_page_addr)
{
    if (Flash_Check_Busy())
    {
        return STATUS_EBUSY;
    }

    Enter_Critical_Section();

    FLASH->COMMAND =  CMD_READPAGE;
    FLASH->FLASH_ADDR = read_page_addr;
    FLASH->MEM_ADDR  = buf_addr;
    FLASH->PATTERN = FLASH_UNLOCK_PATTER;
    FLASH->START = STARTBIT;

    Leave_Critical_Section();

    while (Flash_Check_Busy()) {;}

    return STATUS_SUCCESS;          /*remember to wait flash to finish read outside the caller*/
}

/**
* @brief flash_read_page :
*   read one page. One page is 256 bytes, so buf_addr should have 256 bytes available.
*   this is non_block mode... so user should wait flash finish read outside this function.
*/

uint32_t Flash_Read_Page(uint32_t buf_addr, uint32_t read_page_addr)
{
    if (Flash_Check_Address(read_page_addr, LENGTH_PAGE) == STATUS_INVALID_PARAM)
    {
        return STATUS_INVALID_PARAM; //invalid addres range
    }

    if (Flash_Check_Busy())
    {
        return STATUS_EBUSY;
    }

    Enter_Critical_Section();

    FLASH->COMMAND =  CMD_READPAGE;
    FLASH->FLASH_ADDR = read_page_addr;
    FLASH->MEM_ADDR  = buf_addr;
    FLASH->PATTERN = FLASH_UNLOCK_PATTER;
    FLASH->START = STARTBIT;

    Leave_Critical_Section();

    while (Flash_Check_Busy()) {;}

    return STATUS_SUCCESS;          /*remember to wait flash to finish read outside the caller*/
}
/**
* @brief
* flash_read_page_syncmode :
 *   read one page. One page is 256 bytes, so buf_addr should have 256 bytes available.
 *   This is block mode. when user call this function, system will wait all data in buf_addr
 *   the return.
*/
uint32_t Flash_Read_Page_Syncmode(uint32_t buf_addr, uint32_t read_page_addr)
{

    if (Flash_Check_Address(read_page_addr, LENGTH_PAGE) == STATUS_INVALID_PARAM)
    {
        return STATUS_INVALID_PARAM; //invalid addres range
    }

    if (flash_check_busy())
    {
        return STATUS_EBUSY;     /*flash busy.. please call this function again*/
    }

    Enter_Critical_Section();

    FLASH->COMMAND =  CMD_READPAGE;
    FLASH->FLASH_ADDR = read_page_addr;
    FLASH->MEM_ADDR  = buf_addr;
    FLASH->PATTERN = FLASH_UNLOCK_PATTER;
    FLASH->START = STARTBIT;

    Leave_Critical_Section();

    while (Flash_Check_Busy()) {;}

    return STATUS_SUCCESS;      /*all data in register buffer now*/
}
/**
 * @brief Flash_Read_Byte
 *        The API function to get one byte data form address
*/
uint8_t Flash_Read_Byte(uint32_t read_addr)
{

    /*this is not a good idea to block function here....*/
    while (Flash_Check_Busy()) {;}

    Enter_Critical_Section();

    FLASH->COMMAND =  CMD_READBYTE;
    FLASH->FLASH_ADDR = read_addr;
    FLASH->PATTERN = FLASH_UNLOCK_PATTER;
    FLASH->START = STARTBIT;

    Leave_Critical_Section();

    while (Flash_Check_Busy()) {;}

    return FLASH->FLASH_DATA >> 8;
}

uint32_t Flash_Read_Byte_Check_Addr(uint32_t *buf_addr, uint32_t read_addr)
{

    if (Flash_Check_Address(read_addr, LENGTH_PAGE) == STATUS_INVALID_PARAM)
    {
        return STATUS_INVALID_PARAM; //invalid addres range
    }

    if (Flash_Check_Busy())
    {
        return STATUS_EBUSY;     /*flash busy.. please call this function again*/
    }

    enter_critical_section();

    FLASH->COMMAND =  CMD_READBYTE;
    FLASH->FLASH_ADDR = read_addr;
    FLASH->PATTERN = FLASH_UNLOCK_PATTER;
    FLASH->START = STARTBIT;

    leave_critical_section();

    while (flash_check_busy()) {;}

    *buf_addr = (FLASH->FLASH_DATA >> 8) & 0xFF;

    return STATUS_SUCCESS;
}
/**
* @brief Flash_Erase: erase flash address data
*/
uint32_t Flash_Erase(flash_erase_mode_t mode, uint32_t flash_addr)
{

    if (mode > FLASH_ERASE_SECURE)
    {
        return  STATUS_INVALID_PARAM;
    }

    /* For Safety reason, we don't implement
     * erase chip command here. */
    switch (mode)
    {
    case FLASH_ERASE_PAGE:
    {
        if ( (flash_get_deviceinfo() & 0xFF) != PUYA_MANU_ID )
        {
            return STATUS_INVALID_PARAM; //invalid flash id
        }

        if (Flash_Check_Address(flash_addr, LENGTH_PAGE) == STATUS_INVALID_PARAM)
        {
            return STATUS_INVALID_PARAM; //invalid addres range
        }

        FLASH->COMMAND =  CMD_ERASEPAGE;
        break;
    }
    case FLASH_ERASE_SECTOR:
    {
        if (Flash_Check_Address(flash_addr, LENGTH_4KB) == STATUS_INVALID_PARAM)
        {
            return STATUS_INVALID_PARAM; //invalid addres range
        }

        FLASH->COMMAND =  CMD_ERASESECTOR;
        break;
    }
    case FLASH_ERASE_32K:
    {
        if (Flash_Check_Address(flash_addr, LENGTH_32KB) == STATUS_INVALID_PARAM)
        {
            return STATUS_INVALID_PARAM; //invalid addres range
        }

        FLASH->COMMAND =  CMD_ERASE_BL32K;
        break;
    }
    case FLASH_ERASE_64K:
    {
        if (Flash_Check_Address(flash_addr, LENGTH_64KB) == STATUS_INVALID_PARAM)
        {
            return STATUS_INVALID_PARAM; //invalid addres range
        }

        FLASH->COMMAND =  CMD_ERASE_BL64K;
        break;
    }
    case FLASH_ERASE_SECURE:
    {
        /*This is special command for erase secure register*/
        FLASH->COMMAND = CMD_ERASE_SEC_PAGE;
        break;
    }
    default:
        return STATUS_INVALID_PARAM;
    }


    /*2022/04/28 add, Device busy. try again.*/
    if (Flash_Check_Busy())
    {
        return  STATUS_EBUSY;
    }

    Enter_Critical_Section();

    FLASH->FLASH_ADDR = flash_addr;
    FLASH->PATTERN = FLASH_UNLOCK_PATTER;
    FLASH->START = STARTBIT;

    while (Flash_Check_Busy()) {;}

    Leave_Critical_Section();

    return STATUS_SUCCESS;
}
/**
* @brief flash write one page (256bytes) data
*/
uint32_t Flash_Write_Page(uint32_t buf_addr, uint32_t write_page_addr)
{

    if (Flash_Check_Address(write_page_addr, LENGTH_PAGE) == STATUS_INVALID_PARAM)
    {
        return STATUS_INVALID_PARAM; //invalid addres range
    }

    /*2022/04/28 add, Device busy. try again.*/
    if (flash_check_busy())
    {
        return  STATUS_EBUSY;
    }

    Enter_Critical_Section();

    FLASH->COMMAND =  CMD_WRITEPAGE;
    FLASH->FLASH_ADDR = write_page_addr;
    FLASH->MEM_ADDR  = buf_addr;
    FLASH->PATTERN = FLASH_UNLOCK_PATTER;
    FLASH->START = STARTBIT;

    while (Flash_Check_Busy()) {;}

    Leave_Critical_Section();

    return STATUS_SUCCESS;
}

/**
* @brief write one byte data into flash.
*/
uint32_t Flash_Write_Byte(uint32_t write_addr, uint8_t singlebyte)
{

    if (flash_check_address(write_addr, LENGTH_PAGE) == STATUS_INVALID_PARAM)
    {
        return STATUS_INVALID_PARAM; //invalid addres range
    }

    /*2022/04/28 add, Device busy. try again.*/
    if (flash_check_busy())
    {
        return  STATUS_EBUSY;
    }

    Enter_Critical_Section();

    FLASH->COMMAND =  CMD_WRITEBYTE;
    FLASH->FLASH_ADDR = write_addr;
    FLASH->FLASH_DATA = singlebyte;
    FLASH->PATTERN = FLASH_UNLOCK_PATTER;
    FLASH->START = STARTBIT;

    Leave_Critical_Section();

    return STATUS_SUCCESS;
}

/**
* @brief Flash_Verify_Page: CRC Check function
*/
uint32_t Flash_Verify_Page(uint32_t read_page_addr)
{


    if (Flash_Check_Address(read_page_addr, LENGTH_PAGE) == STATUS_INVALID_PARAM)
    {
        return STATUS_INVALID_PARAM; //invalid addres range
    }

    /*2022/04/28 add, Device busy. try again.*/
    if (Flash_Check_Busy())
    {
        return  STATUS_EBUSY;
    }

    Enter_Critical_Section();

    FLASH->COMMAND =  CMD_READVERIFY;
    FLASH->FLASH_ADDR = read_page_addr;
    FLASH->PATTERN = FLASH_UNLOCK_PATTER;
    FLASH->START = STARTBIT;

    Leave_Critical_Section();

    while (Flash_Check_Busy()) {;}

    return STATUS_SUCCESS;
}
/**
* @brief get Flash status register
*/
uint32_t Flash_Get_Status_Reg(flash_status_t *status)
{

    if (Flash_Check_Busy())
    {
        return  STATUS_EBUSY;
    }

    if ((status->require_mode)&FLASH_STATUS_RW1)
    {
        Enter_Critical_Section();

        FLASH->COMMAND =  CMD_READ_STATUS1;
        FLASH->PATTERN = FLASH_UNLOCK_PATTER;
        FLASH->START = STARTBIT;

        Leave_Critical_Section();

        /*this check_busy is very short... it just send command then to receive data*/
        while (Flash_Check_Busy()) {;}
        status->status1 = (uint8_t)((FLASH->FLASH_DATA) >> 8);
    }

    if (status->require_mode & FLASH_STATUS_RW2)
    {
        Enter_Critical_Section();

        FLASH->COMMAND =  CMD_READ_STATUS2;
        FLASH->PATTERN = FLASH_UNLOCK_PATTER;
        FLASH->START = STARTBIT;

        Leave_Critical_Section();

        while (Flash_Check_Busy()) {;}
        status->status2 = (uint8_t)((FLASH->FLASH_DATA) >> 8);
    }

    /*2022/01/18: GD does NOT have status bytes3.*/

    return STATUS_SUCCESS;
}
/**
* @brief set Flash status register
*/

uint32_t Flash_Set_Status_Reg(const flash_status_t *status)
{


    /*2022/04/28 add, Device busy. try again.*/
    if (Flash_Check_Busy())
    {
        return  STATUS_EBUSY;
    }
    /*
     * 2022/01/18: GD only have status bytes1 and bytes2.
     * GD only support command 0x01. So if you want to write
     *
     */
    if (status->require_mode == FLASH_STATUS_RW1_2)
    {
        Enter_Critical_Section();
        /*GD write status2 must two bytes */
        FLASH->COMMAND =  CMD_WRITE_STATUS;
        FLASH->STATUS  = (uint32_t) (status->status1) | (uint32_t) ((status->status2) << 8);
        FLASH->PATTERN = FLASH_UNLOCK_PATTER;
        FLASH->START = STARTBIT;

        Leave_Critical_Section();

        while (Flash_Check_Busy()) {;}
    }
    else if (status->require_mode == FLASH_STATUS_RW1)
    {
        Enter_Critical_Section();

        FLASH->COMMAND =  CMD_WRITE_STATUS1;
        FLASH->STATUS  = (status->status1);
        FLASH->PATTERN = FLASH_UNLOCK_PATTER;
        FLASH->START = STARTBIT;

        Leave_Critical_Section();

        while (Flash_Check_Busy()) {;}
    }

    return STATUS_SUCCESS;

}
/**
* @brief  program secure page data
*             Note: write_page_addr must be alignment
*/

uint32_t Flash_Write_Sec_Register(uint32_t buf_addr, uint32_t write_reg_addr)
{
    uint32_t  addr;
    /*first we should check write_reg_addr*/
    addr = write_reg_addr >> 12;


    if ((addr > 3) || (write_reg_addr & 0xFF))
    {
        /*only support 3 secureity register.*/
        /*We need secure register write to be 256 bytes alignment*/
        return STATUS_INVALID_PARAM;
    }

    /*2022/04/28 add, Device busy. try again.*/
    if (Flash_Check_Busy())
    {
        return  STATUS_EBUSY;
    }

    Enter_Critical_Section();

    FLASH->COMMAND =  CMD_WRITE_SEC_PAGE;
    FLASH->FLASH_ADDR = write_reg_addr;
    FLASH->MEM_ADDR  = buf_addr;
    FLASH->PATTERN = FLASH_UNLOCK_PATTER;
    FLASH->START = STARTBIT;

    Leave_Critical_Section();

    return STATUS_SUCCESS;
}
/**
* @brief  read secure register data.
 * Note: read_page_addr must be alignment
 *
*/
uint32_t Flash_Read_Sec_Register(uint32_t buf_addr, uint32_t read_reg_addr)
{
    uint32_t  addr;
    /*first we should check read_reg_addr*/
    addr = read_reg_addr >> 12;

    /*2022/04/28 add, Device busy. try again.*/
    if (Flash_Check_Busy())
    {
        return  STATUS_EBUSY;
    }

    //if((addr>3)|| (read_reg_addr & 0xFF)) {
    if (addr > 3)
    {
        /*We need secure register read to be 256 bytes alignment*/
        return STATUS_INVALID_PARAM;
    }

    Enter_Critical_Section();

    FLASH->COMMAND =  CMD_READ_SEC_PAGE;
    FLASH->FLASH_ADDR = read_reg_addr;
    FLASH->MEM_ADDR  = buf_addr;
    FLASH->PATTERN = FLASH_UNLOCK_PATTER;
    FLASH->START = STARTBIT;

    Leave_Critical_Section();

    while (Flash_Check_Busy()) {;}

    return STATUS_SUCCESS;
}

/**
* @brief read flash unique ID
 *  flash ID is 128bits/16 bytes.
 *  if buf_length <16, it will return required length data only.
 *  if buf_length >16, it will return 16 bytes only.
 *  if buf_length = 0 , this function will return STATUS_INVALID_PARAM
*/
uint32_t Flash_Get_Unique_Id(uint32_t flash_id_buf_addr, uint32_t buf_length)
{
    uint32_t  i;
    uint8_t  temp[16], *ptr;

    if (Flash_Check_Busy())
    {
        return  STATUS_EBUSY;
    }

    /*
     * Notice: we don't check flash_id_buf_addr value here..
     * it should be correct address in SRAM!
     */
    if (buf_length == 0)
    {
        return STATUS_INVALID_PARAM;
    }
    else if (buf_length > 16)
    {
        buf_length = 16;
    }

    Enter_Critical_Section();

    FLASH->COMMAND =  CMD_READUID;
    FLASH->PAGE_READ_WORD = 0xF;
    FLASH->MEM_ADDR  = (uint32_t) temp;
    FLASH->PATTERN = FLASH_UNLOCK_PATTER;
    FLASH->START = STARTBIT;

    Leave_Critical_Section();

    ptr = (uint8_t *) flash_id_buf_addr;    /*set address*/

    while (Flash_Check_Busy()) {;}

    FLASH->PAGE_READ_WORD = 0xFF;   /*restore read one page length by default*/

    /*move unique number from stack to assign buffer*/
    for (i = 0; i < buf_length; i++)
    {
        ptr[i] = temp[i];
    }

    return STATUS_SUCCESS;
}
/**
* @brief  according mcu clock to set flash timing
*
*/
void Flash_Timing_Init(void)
{
    uint32_t   clk_mode, sys_clk;
    uint16_t   tdp, tres, tsus, trs, flash_type_id;

    flash_timing_mode_t  flash_timing;
    /*change AHB clock also need change flash timing.*/
    flash_type_id = Flash_Get_Deviceinfo() & FLASH_TYPE_ID_MAKS;
    sys_clk = 32;
    clk_mode = Get_Ahb_System_Clk();

    /*check flash type to adjust flash timing*/
    if (flash_type_id == GDWQ_ID)
    {
        tdp  = GDWQ_FLASH_TDP;
        tres = GDWQ_FLASH_TRES1;
        tsus = GDWQ_FLASH_TSUS;
        trs  = GDWQ_FLASH_TRS;
    }
    else if (flash_type_id == GDLQ_ID)
    {
        tdp  = GDLQ_FLASH_TDP;
        tres = GDLQ_FLASH_TRES1;
        tsus = GDLQ_FLASH_TSUS;
        trs  = GDLQ_FLASH_TRS;
    }
    else if (flash_type_id == PUYA_ID)
    {
        tdp  = PUYA_FLASH_TDP;
        tres = PUYA_FLASH_TRES1;
        tsus = PUYA_FLASH_TSUS;
        trs  = PUYA_FLASH_TRS;
    }
    else
    {
        tdp  = GDWQ_FLASH_TDP;
        tres = GDWQ_FLASH_TRES1;
        tsus = GDWQ_FLASH_TSUS;
        trs  = GDWQ_FLASH_TRS;
    }


    if (clk_mode == SYS_32MHZ_CLK)
    {
        sys_clk = 32;
    }
    else if (clk_mode == SYS_48MHZ_PLLCLK)
    {
        sys_clk = 48;
    }
    else if (clk_mode == SYS_64MHZ_PLLCLK)
    {
        sys_clk = 64;
    }
    else
    {
        sys_clk = 32;
    }

    flash_timing.deep_pd_timing = tdp * sys_clk + 2;
    flash_timing.deep_rpd_timing = tres * sys_clk + 2;
    flash_timing.suspend_timing = tsus * sys_clk + 2;
    flash_timing.resume_timing = trs * sys_clk + 2;

    //for FPGA Verify
    Flash_Set_Timing(&flash_timing);
}
/**
* @brief  settin flash deep power down, suspen, resume timing
*
*/
void Flash_Set_Timing(flash_timing_mode_t *timing_cfg)
{
    FLASH->DPD = timing_cfg->deep_pd_timing;
    FLASH->RDPD = timing_cfg->deep_rpd_timing;
    FLASH->SUSPEND = timing_cfg->suspend_timing;
    FLASH->RESUME  = timing_cfg->resume_timing;
    return;
}
/**
* @brief  read secure register data.
*                 Note: read_page_addr must be alignment
*/
uint32_t Flash_Read_Otp_Sec_Register(uint32_t buf_addr, uint32_t read_reg_addr)
{
    uint32_t  addr;


    /*first we should check read_reg_addr*/
    addr = read_reg_addr >> 12;


    /*2022/04/28 add, Device busy. try again.*/
    if (Flash_Check_Busy())
    {
        return  STATUS_EBUSY;
    }

    //if((addr>3)|| (read_reg_addr & 0xFF)) {
    if (addr > 3)
    {
        /*We need secure register read to be 256 bytes alignment*/
        return STATUS_INVALID_PARAM;
    }

    FLASH->COMMAND =  CMD_READ_SEC_PAGE;
    FLASH->FLASH_ADDR = read_reg_addr;
    FLASH->MEM_ADDR  = buf_addr;
    FLASH->PATTERN = FLASH_UNLOCK_PATTER;
    FLASH->START = STARTBIT;

    while (Flash_Check_Busy()) {;}

    return STATUS_SUCCESS;
}

/**
* @brief read otp sec page data
*/
uint32_t Flash_Read_Otp_Sec_Page(uint32_t buf_addr)
{
    switch (Flash_Get_Deviceinfo()) //check flash device
    {
    case RT581_FLASH_TYPE:        //0x2000
    case FLASH_512K_TYPE:    //0x2000
        if (Flash_Read_Otp_Sec_Register((uint32_t)buf_addr, FLASH_SECREG_R2_P0))
        {
            return STATUS_INVALID_PARAM;
        }
        break;

    case FLASH_1MB_TYPE: //0x0000
    case FLASH_2MB_TYPE: //0x0000
        if (Flash_Read_Otp_Sec_Register((uint32_t)buf_addr, FLASH_SECREG_R0_P0))
        {
            return STATUS_INVALID_PARAM;
        }
    }

    return STATUS_SUCCESS;
}

/**
* @brief Enable flash Suspend fucntion
*/
void Flash_Enable_Suspend(void)
{
    FLASH->CONTROL_SET = FLASH->CONTROL_SET | 0x200;
}

/**
*@brief Disable flash Suspend fucntion
*/
void Flash_Disable_Suspend(void)
{
    FLASH->CONTROL_SET = FLASH->CONTROL_SET & ~(0x200);
}

/**
*@brief get flash control register value
*/
uint32_t Flash_Get_Control_Reg(void)
{
    return FLASH->CONTROL_SET;
}

/**
* @brief Flash_Erase: erase flash address data
*/
uint32_t Flash_Erase_Mpsector()
{

    /*2022/04/28 add, Device busy. try again.*/
    if (Flash_Check_Busy())
    {
        return  STATUS_EBUSY;
    }

    Enter_Critical_Section();
    FLASH->COMMAND =  CMD_ERASESECTOR;

#if FLASHCTRL_SECURE_EN==1

    if (Flash_Size() == FLASH_1024K)
    {
        FLASH->FLASH_ADDR = (0x000FF000 + FLASH_SECURE_MODE_BASE_ADDR);
    }
    else if (Flash_Size() == FLASH_2048K)
    {
        FLASH->FLASH_ADDR = (0x001FF000 + FLASH_SECURE_MODE_BASE_ADDR);
    }

#else
    if (Flash_Size() == FLASH_1024K)
    {
        FLASH->FLASH_ADDR = 0x000FF000;
    }
    else if (Flash_Size() == FLASH_2048K)
    {
        FLASH->FLASH_ADDR = 0x001FF000;
    }
#endif

    FLASH->PATTERN = FLASH_UNLOCK_PATTER;
    FLASH->START = STARTBIT;

    while (Flash_Check_Busy()) {;}

    Leave_Critical_Section();

    return STATUS_SUCCESS;
}
/**
* @brief flash write one page (256bytes) data
*/
uint32_t Flash_Write_Mp_Sector(uint32_t buf_addr, uint32_t write_page_addr)
{
    /*2022/04/28 add, Device busy. try again.*/
    if (flash_check_busy())
    {
        return  STATUS_EBUSY;
    }

    if (Flash_Size() == FLASH_1024K)
    {
#if FLASHCTRL_SECURE_EN==1

        if ((write_page_addr < 0x100FF000) || (write_page_addr > 0x10100000))
        {
            return STATUS_INVALID_PARAM;
        }
#else
        if ((write_page_addr < 0x000FF000) || (write_page_addr > 0x00100000))
        {
            return STATUS_INVALID_PARAM;
        }
#endif

    }
    else if (Flash_Size() == FLASH_2048K)
    {
#if FLASHCTRL_SECURE_EN==1

        if ((write_page_addr < 0x101FF000) || (write_page_addr > 0x10200000))
        {
            return STATUS_INVALID_PARAM;
        }
#else
        if ((write_page_addr < 0x001FF000) || (write_page_addr > 0x00200000))
        {
            return STATUS_INVALID_PARAM;
        }
#endif
    }

    Enter_Critical_Section();

    FLASH->COMMAND =  CMD_WRITEPAGE;
    FLASH->FLASH_ADDR = write_page_addr;
    FLASH->MEM_ADDR  = buf_addr;
    FLASH->PATTERN = FLASH_UNLOCK_PATTER;
    FLASH->START = STARTBIT;

    while (Flash_Check_Busy()) {;}

    Leave_Critical_Section();

    return STATUS_SUCCESS;
}

/**
* @brief write one byte data into flash.
*/
uint32_t Flash_Write_MpSector_TxPwrCfgByte(uint32_t write_addr, uint8_t singlebyte)
{

    /*2022/04/28 add, Device busy. try again.*/
    if (flash_check_busy())
    {
        return  STATUS_EBUSY;
    }

    if (Flash_Size() == FLASH_1024K)
    {
#if FLASHCTRL_SECURE_EN==1

        if ((write_addr < 0x100FFFD8) || (write_addr > 0x100FFFDF))
        {
            return STATUS_INVALID_PARAM;
        }
#else
        if ((write_addr < 0x000FFFD8) || (write_addr > 0x000FFFDF))
        {
            return STATUS_INVALID_PARAM;
        }
#endif

    }
    else if (Flash_Size() == FLASH_2048K)
    {
#if FLASHCTRL_SECURE_EN==1

        if ((write_addr < 0x101FFFD8) || (write_addr > 0x101FFFDF))
        {
            return STATUS_INVALID_PARAM;
        }
#else
        if ((write_addr < 0x001FFFD8) || (write_addr > 0x001FFFDF))
        {
            return STATUS_INVALID_PARAM;
        }
#endif
    }

    Enter_Critical_Section();

    FLASH->COMMAND =  CMD_WRITEBYTE;
    FLASH->FLASH_ADDR = write_addr;
    FLASH->FLASH_DATA = singlebyte;
    FLASH->PATTERN = FLASH_UNLOCK_PATTER;
    FLASH->START = STARTBIT;
    while (Flash_Check_Busy()) {;}
    Leave_Critical_Section();

    return STATUS_SUCCESS;
}


/**
* @brief write one byte data into flash.
*/
uint32_t Flash_Write_MpSector_RfTrimByte(uint32_t write_addr, uint8_t singlebyte)
{

    /*2022/04/28 add, Device busy. try again.*/
    if (flash_check_busy())
    {
        return  STATUS_EBUSY;
    }

    if (Flash_Size() == FLASH_1024K)
    {
#if FLASHCTRL_SECURE_EN==1

        if ((write_addr < 0x100FF219) || (write_addr > 0x100FF22A))
        {
            return STATUS_INVALID_PARAM;
        }
#else
        if ((write_addr < 0x000FFFD8) || (write_addr > 0x000FFFDF))
        {
            return STATUS_INVALID_PARAM;
        }
#endif

    }
    else if (Flash_Size() == FLASH_2048K)
    {
#if FLASHCTRL_SECURE_EN==1

        if ((write_addr < 0x101FF219) || (write_addr > 0x101FF22A))
        {
            return STATUS_INVALID_PARAM;
        }
#else
        if ((write_addr < 0x001FFFD8) || (write_addr > 0x001FFFDF))
        {
            return STATUS_INVALID_PARAM;
        }
#endif
    }

    Enter_Critical_Section();

    FLASH->COMMAND =  CMD_WRITEBYTE;
    FLASH->FLASH_ADDR = write_addr;
    FLASH->FLASH_DATA = singlebyte;
    FLASH->PATTERN = FLASH_UNLOCK_PATTER;
    FLASH->START = STARTBIT;
    while (Flash_Check_Busy()) {;}
    Leave_Critical_Section();

    return STATUS_SUCCESS;
}


