/**
 ******************************************************************************
 * @file    flashctl.h
 * @author
 * @brief   flash control driver header file
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


#ifndef _RT584_FLASHCTL_H_
#define _RT584_FLASHCTL_H_

#ifdef __cplusplus
extern "C"
{
#endif





/**************************************************************************************************
 *    INCLUDES
 *************************************************************************************************/
#include "cm33.h"

/***********************************************************************************************************************
 *    CONSTANTS AND DEFINES
 **********************************************************************************************************************/
/**
* @brief  To maintain compatibility with 58x, the modification will be done by using the macro (#define) in the pre-processing stage.
*/
#define flash_suspend_check()                                 Flash_Suspend_Check()
#define flash_set_timing(timing_cfg)                          Flash_Set_Timing(timing_cfg)
#define flash_get_deviceinfo()                                Flash_Get_Deviceinfo()
#define flash_get_status_reg(status)                          Flash_Get_Status_Reg(status)
#define flash_set_status_reg(status)                          Flash_Set_Status_Reg(status)
#define flash_read_page_ext(buf_addr, read_page_addr)         Flash_Read_Page_Ext(buf_addr, read_page_addr)
#define flash_read_page(buf_addr, read_page_addr)             Flash_Read_Page(buf_addr, read_page_addr)
#define flash_read_page_syncmode(buf_addr, read_page_addr)    Flash_Read_Page_Syncmode(buf_addr, read_page_addr)
#define flash_read_byte(read_addr)                            Flash_Read_Byte(read_addr)
#define flash_read_byte_check_addr(buf_addr, read_addr)       Flash_Read_Byte_check_addr(buf_addr, read_addr)
#define flash_write_page(buf_addr, write_page_addr)           Flash_Write_Page(buf_addr, write_page_addr)
#define flash_write_sec_register(buf_addr, write_reg_addr)    Flash_Write_Sec_Register(buf_addr, write_reg_addr)
#define flash_read_sec_register(buf_addr, read_reg_addr)      Flash_Read_Sec_Register(buf_addr, read_reg_addr)
#define flash_read_otp_sec_register(buf_addr, read_reg_addr)  Flash_Read_Otp_Sec_Register(buf_addr, read_reg_addr)
#define flash_erase(mode, flash_addr)                         Flash_Erase(mode, flash_addr)
#define flash_write_byte(write_addr, singlebyte)              Flash_Write_Byte(write_addr, singlebyte)
#define flash_verify_page(read_page_addr)                     Flash_Verify_Page(read_page_addr)
#define flash_get_unique_id(flash_id_buf_addr, buf_length)    Flash_Get_Unique_Id(flash_id_buf_addr, buf_length)
#define flash_timing_init()                                   Flash_Timing_Init()
#define flash_size()                                          Flash_Size()
#define flash_read_otp_sec_page(buf_addr)                     Flash_Read_Otp_Sec_Page(buf_addr)
#define flash_check_busy()                                    Flash_Check_Busy()
#define flash_get_crc()                                       Flash_Get_Crc()
#define flash_enable_qe()                                     Flash_Enable_Qe()
#define flash_set_read_pagesize()                             Flash_Set_Read_Pagesize()
#define flush_cache()                                         Flush_Cache()
#define flash_enable_suspend()                                Flash_Enable_Suspend()
#define flash_disable_suspend()                               Flash_Disable_Suspend()
#define flash_get_control_reg()                               Flash_Get_Control_Reg()
#define flash_check_address(flash_address, length)            Flash_Check_Address(flash_address, length)
/**
* @brief  Status Register definitions
*/
#define FLASH_UNLOCK_PATTER   0x52414254
/**
 * @brief  Status Register definitions
 */

#define  FLASH_STATUS_RW1     1       /*!< Status Register 1  */
#define  FLASH_STATUS_RW2     2       /*!< Status Register 2  */
#define  FLASH_STATUS_RW3     4       /*!< Status Register 3  */

#define  FLASH_STATUS_RW1_2    (FLASH_STATUS_RW1 | FLASH_STATUS_RW2)         /*!< Status Register Byte 1 and Byte 2, this define for writing status Bytes2  */
/**
 * @brief Define Secure Register address.
 *
 * NOTICE: different flash has different secure page registers location and different secure page size!
 */

#define  FLASH_SECREG_R0_P0   ((0<<12))            /*!< Secure Page zero, page0 256 bytes. */
#define  FLASH_SECREG_R0_P1   ((0<<12) | 0x100)    /*!< Secure Page zero, page1 256 bytes. */
#define  FLASH_SECREG_R0_P2   ((0<<12) | 0x200)    /*!< Secure Page zero, page2 256 bytes. */
#define  FLASH_SECREG_R0_P3   ((0<<12) | 0x300)    /*!< Secure Page zero, page3 256 bytes. */

#define  FLASH_SECREG_R1_P0   ((1<<12))            /*!< Secure Page one, page0 256 bytes. */
#define  FLASH_SECREG_R1_P1   ((1<<12) | 0x100)    /*!< Secure Page one, page1 256 bytes. */
#define  FLASH_SECREG_R1_P2   ((1<<12) | 0x200)    /*!< Secure Page one, page2 256 bytes. */
#define  FLASH_SECREG_R1_P3   ((1<<12) | 0x300)    /*!< Secure Page one, page3 256 bytes. */

#define  FLASH_SECREG_R2_P0   ((2<<12))            /*!< Secure Page two, page0 256 bytes. */
#define  FLASH_SECREG_R2_P1   ((2<<12) | 0x100)    /*!< Secure Page two, page1 256 bytes. */
#define  FLASH_SECREG_R2_P2   ((2<<12) | 0x200)    /*!< Secure Page two, page2 256 bytes. */
#define  FLASH_SECREG_R2_P3   ((2<<12) | 0x300)    /*!< Secure Page two, page3 256 bytes. */

#define  FLASH_SECREG_R3_P0   ((3<<12))            /*!< Secure Page three, page0 256 bytes.*/
#define  FLASH_SECREG_R3_P1   ((3<<12) | 0x100)    /*!< Secure Page three, page1 256 bytes.*/
#define  FLASH_SECREG_R3_P2   ((3<<12) | 0x200)    /*!< Secure Page three, page2 256 bytes.*/
#define  FLASH_SECREG_R3_P3   ((3<<12) | 0x300)    /*!< Secure Page three, page3 256 bytes.*/


#define  FLASH_IDLE                     0          /*!< flash idle  status*/
#define  FLASH_BUSY                     1          /*!<  flash busy status*/

#define FLASH_SUSPEND_STATUS            0x80         /*!< flash suspend status  */
#define FLASH_BUSY_CHECK_COUNT          10000        /*!< flash busy check count*/

#define FLASH_TYPE_ID_MAKS      0xFFFF              /*!< flash id mask  */

#define FLASH_SIZE_ID_SHIFT     16                  /*!< flash id shift  */

//limit customer to use
#if FLASHCTRL_SECURE_EN==1
#define FLASH_SECURE_MODE_BASE_ADDR         0x10000000
#define BOOT_LOADER_END_PROTECT_ADDR        (FLASH_SECURE_MODE_BASE_ADDR+0x7000)
#else
#define BOOT_LOADER_END_PROTECT_ADDR        0x7000  /*!< Bootlader protect address  */ /*!< Bootlader protect address  */
#endif

#define FLASH_SIZE_ID_SHIFT     16                  /*!< flash idle  */

#define FLASH_SIZE_SECTOR_4KB       0x0C            /*!< flash sector size length  */
#define FLASH_SIZE_ID_512KB     0x13                /*!< 512KB flash id  */
#define FLASH_SIZE_ID_1MB       0x14                /*!< 1MB flash id  */
#define FLASH_SIZE_ID_2MB       0x15                /*!< 2MB flash id  */

#define LENGTH_BYTE     1
#define LENGTH_PAGE     256
#define LENGTH_4KB      (4*1024)
#define LENGTH_32KB     (32*1024)
#define LENGTH_64KB     (64*1024)

#define FLASH_END_PROTECT_SIZE               0x1000  /*!< Bootlader protect address  */

#define GDWQ_ID                 0x65C8              /*!< GD SPI Flash identifier id  */
#define GDLQ_ID                 0x60C8              /*!< GD SPI Flash identifier id  */
#define PUYA_ID                 0x6085              /*!< GD SPI Flash identifier id  */

#define GD_MANU_ID              0xC8                /*!< GD SPI Flash identifier id  */
#define PUYA_MANU_ID            0x85                /*!< GD SPI Flash identifier id  */

#define  RT581_FLASH_TYPE          0x1460C8         /*!< GD SPI Flash identifier id  */
#define  RT582_FLASH_TYPE          0x1465C8         /*!< GD SPI Flash identifier id  */
#define  RT584_FLASH_TYPE          RT582_FLASH_TYPE /*!< GD SPI Flash identifier id  */
#define  RT582P512_FLASH_TYPE      0x136085         /*!< GD SPI Flash identifier id  */
#define  RT583_FLASH_TYPE          0x1565C8         /*!< GD SPI Flash identifier id  */
#define  FLASH_1MB_TYPE            RT582_FLASH_TYPE
#define  FLASH_2MB_TYPE            RT583_FLASH_TYPE
#define  FLASH_512K_TYPE           RT582P512_FLASH_TYPE

#if FLASHCTRL_SECURE_EN==1
#define FLASH_END_ADDR(x)               (x == FLASH_SIZE_ID_512KB ? (((1 << FLASH_SIZE_ID_512KB)+FLASH_SECURE_MODE_BASE_ADDR)-FLASH_END_PROTECT_SIZE) \
                                                                  : (x == FLASH_SIZE_ID_1MB ? (((1 << FLASH_SIZE_ID_1MB)+FLASH_SECURE_MODE_BASE_ADDR)-FLASH_END_PROTECT_SIZE) \
                                                                  : (((1 << FLASH_SIZE_ID_2MB)+FLASH_SECURE_MODE_BASE_ADDR)-FLASH_END_PROTECT_SIZE)))
#else
#define FLASH_END_ADDR(x)               (x == FLASH_SIZE_ID_512KB ? (((1 << FLASH_SIZE_ID_512KB)-FLASH_END_PROTECT_SIZE) \
                                                                  : (x == FLASH_SIZE_ID_1MB ? ((1 << FLASH_SIZE_ID_1MB)-FLASH_END_PROTECT_SIZE) \
                                                                  : ((1 << FLASH_SIZE_ID_2MB)-FLASH_END_PROTECT_SIZE)))
#endif
/**
 * @brief Define Flash timing
 *
 * NOTICE: different flash has different timing. We should set correct flash timing according to flash type.
 */

#define GDLQ_FLASH_TDP            3       /*!< LQ type flash deep power down (CS# High To Deep Power-Down Mode) and the unit is us */
#define GDLQ_FLASH_TRES1         20       /*!< LQ type flash release deep power down (CS# High To Standby Mode Without Electronic Signature) and the unit is us */
#define GDLQ_FLASH_TSUS          20       /*!< LQ type flash suspend time (CS# High To Next Command After Suspend) and the unit is us */
#define GDLQ_FLASH_TRS          100       /*!< LQ type flash resume time (Latency Between Resume And Next Suspend) and the unit is us */

#define GDWQ_FLASH_TDP            3       /*!< WQ type flash deep power down (CS# High To Deep Power-Down Mode) and the unit is us */
#define GDWQ_FLASH_TRES1         30       /*!< WQ type flash release deep power down (CS# High To Standby Mode Without Electronic Signature) and the unit is us */
#define GDWQ_FLASH_TSUS          40       /*!< WQ type flash suspend time (CS# High To Next Command After Suspend) and the unit is us */
#define GDWQ_FLASH_TRS          100       /*!< WQ type flash resume time (Latency Between Resume And Next Suspend) and the unit is us */

#define PUYA_FLASH_TDP            3       /*!< PUYA type flash deep power down (CS# High To Deep Power-Down Mode) and the unit is us */
#define PUYA_FLASH_TRES1         30       /*!< PUYA type flash release deep power down (CS# High To Standby Mode Without Electronic Signature) and the unit is us */
#define PUYA_FLASH_TSUS          40       /*!< PUYA type flash suspend time (CS# High To Next Command After Suspend) and the unit is us */
#define PUYA_FLASH_TRS          100       /*!< PUYA type flash resume time (Latency Between Resume And Next Suspend) and the unit is us */

/***********************************************************************************************************************
 *    TYPEDEFS
 **********************************************************************************************************************/
/**
 * @brief Erase flash size definitions.
 */

typedef enum
{
    FLASH_ERASE_PAGE,       /*!< Erase Page size   */
    FLASH_ERASE_SECTOR,     /*!< Erase Sector size */
    FLASH_ERASE_32K,        /*!< Erase 32K block   */
    FLASH_ERASE_64K,        /*!< Erase 64K block   */
    FLASH_ERASE_SECURE,     /*!< Erase Secure page */
} flash_erase_mode_t;

/**
* @brief flash size definitions.
*/
typedef enum
{
    FLASH_NOT_SUPPORT = 0x00,
    FLASH_512K = 0x13,       /*!< 512K size   */
    FLASH_1024K = 0x14,      /*!< 1024K size   */
    FLASH_2048K = 0x15,      /*!< 2048K size   */
} flash_size_t;

/**
 * @brief Flash config structure holding flash timing configuration for flash
 *       suspend/resume/deep-powerdown.
 * @details
 *       Please read Flash Datasheet for suspend/resume/powerdown timing.
 *       The unit of this data member is in HCLK clock, for example,
 *       if CPU running in 32MHz, and flash deep power down
 *       is 3 us, then deep_pd_timing is 3*32, since 32 clock is 1 us.
 *
 */
typedef struct
{
    uint16_t   deep_pd_timing;        /*!< flash deep power down T_dp */
    uint16_t   deep_rpd_timing;       /*!< flash release deep power down T_rdp */
    uint16_t   suspend_timing;        /*!< flash suspend time T_sus */
    uint16_t   resume_timing;         /*!< flash resume time T_rs */
} flash_timing_mode_t;


/**
 * @brief Structure for status read/write
 *
 * @details
 *        require_mode is bitwise read/write operation.
 *        For example, if reqquired_mode = (FLASH_STATUS_RW1 | FLASH_STATUS_RW2)
 *        then the flash driver will read/write status1 register and status2
 *        register, depends on read/write API.
 *
 */

typedef struct
{
    uint8_t   require_mode;     /*!< bitwise mode to indicate read/write operation */
    uint8_t   status1;          /*!< flash status1 for read/write */
    uint8_t   status2;          /*!< flash status2 for read/write */
    uint8_t   status3;          /*!< flash status3 for read/write */
} flash_status_t;

/** @addtogroup Peripheral_Driver RT584Z Periperhal Driver Function
  @{
*/


/** @addtogroup FLASH_DRIVER FLASH Driver Functions
  @{
*/

/***********************************************************************************************************************
 *    GLOBAL PROTOTYPES
 **********************************************************************************************************************/
/**
* @brief Check Flash Suspend status
*
* @param None
*
* @details when the flash controller in suspend status,
*                  the flash controller must be reset otherwise flash program/erase function will be
*
*/
void Flash_Suspend_Check(void);

/**
* @brief flash contrl check address
*   check input flash address and length is correct or not.
*/
uint32_t Flash_Check_Address(uint32_t flash_address, uint32_t length);
/**
 * @brief Set Flash timing
 *
 * @param[in]  timing_cfg  Pointer to the flash timing structure flash_timing_mode_t
 *
 * @details Flash controller will use flash timing to control flash to enter
 *      deeply powerdown and release from deeply powerdown mode, and suspend/
 *      resume time.
 *
 */
void Flash_Set_Timing(flash_timing_mode_t *timing_cfg);


/**
 * @brief Get Flash model/type ID
 *
 * @param None
 *
 * @return
 * @retval    flash model identifier id.
 *
 */

uint32_t Flash_Get_Deviceinfo(void);


/**
 * @brief Get flash status register
 *
 * @param[in] status  The pointer of flash_status_t
 *
 * @return
 * @retval    STATUS_EBUSY
 * @retval    STATUS_SUCCESS  --- this operation has been finished
 *
 * @details  Read status will be save in status.status1, status.status2
 *       status.status3, depends on status.require_mode.
 *       require_mode is bitwise operation indicator, so it can read
 *       multiple status byte in one function call.
 *       Current flash does NOT have status3 byte, it reserved for future.
 */

uint32_t Flash_Get_Status_Reg(flash_status_t *status);


/**
 * @brief Set flash status register
 *
 * @param[in] status  The pointer of flash_status_t
 *
 * @return
 * @retval    STATUS_EBUSY    --- previous flash command not finished, please call this function again after flash idle.
 * @retval    STATUS_SUCCESS  --- this operation has been finished
 *
 *
 * @details  Flash driver will write status of status.status1, status.status2
 *       status.status3 into flash, depends on status.require_mode.
 *       require_mode is bitwise operation indicator, so it can write
 *       multiple status byte in one function call.
 *       Current flash does NOT have status3 byte, it reserved for future.
 *
 *       Notice: PLEASE read flash datasheet for write status
 *       setting information before you write. Some status bitfield is
 *       OTP bit, it just can write once and can not erase again!!!
 *       This function is BLOCK function.
 *
 *         For current flash, writing status2 needs to write status1 byte, too.
 *       So please remember to read status1 byte first, then save this status1
 *       data in status.status1.
 */

uint32_t Flash_Set_Status_Reg(const flash_status_t *status);


/**
 * @brief Read flash page data in nonblocking mode
 *
 * @param[out] buf_addr  Specify the address for the read data of the page.
 *
 * @param[in]  read_page_addr Specify the page address in flash that required to read.
 *
 * @return
 * @retval    STATUS_SUCCESS  --- this operation has been executed... but read operation is still processing
 * @retval    STATUS_EBUSY    --- flash controller is busying, please call this function again when flash finish current operation.
 *
 * @details  buf_addr must be 4-bytes alignment. One page is 256 bytes, so
 *           buf_addr must have 256 bytes for read data, otherwise some data
 *           will be overwritten. Please notice that read_page_addr will be
 *           ignored the LSB 8 bits, that is LSB 8 bits of address is don't care.
 *           This function is NON_BLOCK function.
 *           That is user should wait the operation finish (using function, wait flash_check_busy() return 0,
 *           User can not call any flash function before this function finish.
 *           Can access all flash range
 *
 * @remark  Using this flash_read_page API DOES NOT through cache controller!
 *
 */

uint32_t Flash_Read_Page_Ext(uint32_t buf_addr, uint32_t read_page_addr);

/**
 * @brief Read flash page data in nonblocking mode
 *
 * @param[out] buf_addr  Specify the address for the read data of the page.
 *
 * @param[in]  read_page_addr Specify the page address in flash that required to read.
 *
 * @return
 * @retval    STATUS_SUCCESS  --- this operation has been executed... but read operation is still processing
 * @retval    STATUS_EBUSY    --- flash controller is busying, please call this function again when flash finish current operation.
 *
 * @details  buf_addr must be 4-bytes alignment. One page is 256 bytes, so
 *           buf_addr must have 256 bytes for read data, otherwise some data
 *           will be overwritten. Please notice that read_page_addr will be
 *           ignored the LSB 8 bits, that is LSB 8 bits of address is don't care.
 *           This function is NON_BLOCK function.
 *           That is user should wait the operation finish (using function, wait flash_check_busy() return 0,
 *           User can not call any flash function before this function finish.
 *
 * @remark  Using this flash_read_page API DOES NOT through cache controller!
 *
 */

uint32_t Flash_Read_Page(uint32_t buf_addr, uint32_t read_page_addr);

/**
 * @brief Read flash page data in blocking mode
 *
 * @param[out] buf_addr  Specify the address for the read data of the page.
 *
 * @param[in]  read_page_addr Specify the page address in flash that required to read.
 *
 * @return
 * @retval    STATUS_SUCCESS  --- this operation has been read finish
 * @retval    STATUS_EBUSY    --- flash controller is busying, please call this function again when flash finish current operation.
 *
 * @details  buf_addr must be 4-bytes alignment. One page is 256 bytes, so
 *           buf_addr must have 256 bytes for read data, otherwise some data
 *           will be overwritten. Please notice that read_page_addr will be
 *           ignored the LSB 8 bits, that is LSB 8 bits of address is don't care.
 *           This function is BLOCK function --- that is, it will waiting read page finish inside the function then return.
 *           That is user can get page data in buf_addr if this function returns STATUS_SUCCESS.
 *
 * @remark  Using this flash_read_page_block API DOES NOT through cache controller!
 */
uint32_t Flash_Read_Page_Syncmode(uint32_t buf_addr, uint32_t read_page_addr);

/**
 * @brief Read one byte from flash
 *
 *
 * @param[in]  read_addr Specify the byte address in flash that required to read.
 *
 * @retval    The data in read_addr of flash
 *
 * @details  read one byte from flash address "read_addr".
 *           This function is BLOCK function.
 */
uint8_t Flash_Read_Byte(uint32_t read_addr);

/**
 * @brief Write/Program flash page data
 *
 * @param[in] buf_addr  Specify the address of the write data.
 *
 * @param[in] write_page_addr Specify the page address in flash that to be written.
 *
 * @return
 * @retval    STATUS_SUCCESS  --- this operation has been executed... but write operation is still processing
 * @retval    STATUS_EBUSY    --- flash controller is busying, please call this function again when flash finish current operation.
 *
 * @details  buf_addr must be 4-bytes alignment. One page is 256 bytes, so
 *           buf_addr must have 256 bytes for write data, if the data you
 *           want to write is less than 256 bytes, please write "0xFF" for
 *           padding bytes. Please notice that write_page_addr will be
 *           ignored the LSB 8 bits, that is LSB 8 bits of address is don't care.
 *           This function is NON-BLOCKING function. Please call function flash_check_busy() to
 *           check write operation finish or not.
 *           Before writing finish, system SHOULD NOT call any other flash operation!
 *
 * @remark  Using this flash_write_page API DOES NOT through cache controller!
 *           So after write API finish, please call flush_cache() to refresh cache.
 *
 *
 */
uint32_t Flash_Write_Page(uint32_t buf_addr, uint32_t write_page_addr);

/*program secure register data*/

/**
 * @brief Write/Program flash secure register
 *
 * @param[in] buf_addr  Specify the address of the write data into the page.
 *
 * @param[in] write_reg_addr Specify the secure register address in flash to write.
 *
 * @return
 * @retval    STATUS_INVALID_PARAM  -- write_reg_addr is invalid value.
 * @retval    STATUS_SUCCESS --- write register opertation finish.
 * @retval    STATUS_EBUSY   --- flash controller is busying, please call this function again when flash finish current operation.
 *
 *
 * @details  buf_addr must be 4-bytes alignment. One page is 256 bytes, so
 *           buf_addr must have 256 bytes for write data, if the data you
 *           want to write is less than 256 bytes, please write "0xFF" for
 *           padding bytes. Please notice that write_page_addr must be
 *           256 bytes alignment, that is LSB 8 bits of address must be zero.
 *           This function is non-blocking function. Please call function flash_check_busy() to
 *           check write operation finish or not.
 *
 *           Before writing finish, system SHOULD NOT call any other flash operation!
 *
 * @remark  Using this flash_write_page API DOES NOT through cache controller!
 *           So after write API finish, please call flush_cache() to refresh cache.
 *
 */
uint32_t Flash_Write_Sec_Register(uint32_t buf_addr, uint32_t write_reg_addr);

/*read secure register data*/

/**
 * @brief Read flash secure register
 *
 * @param[out] buf_addr  Specify the address of the read data to return.
 *
 * @param[in] read_reg_addr Specify the secure register address in flash to read.
 *
 * @return
 * @retval    STATUS_INVALID_PARAM  read_reg_addr is invalid value.
 * @retval    STATUS_SUCCESS        read register opertation finish.
 * @retval    STATUS_EBUSY          lash controller is busying, please call this function again when flash finish current operation.
 *
 *
 * @details  buf_addr must be 4-bytes alignment. One page is 256 bytes, so
 *           buf_addr must have 256 bytes for read secure register data,
 *           otherwise some data will be overwritten.
 *           This function is block function. So if this function return STATUS_SUCCESS,
 *           it means the page buffer (256 bytes) has been read in buf_addr
 *
 */
uint32_t Flash_Read_Sec_Register(uint32_t buf_addr, uint32_t read_reg_addr);

/*erase sector, block*/
/**
 * @brief Erase Sector/Blcok Function
 *
 * @param[in] mode  Specify the ease mode.
 *
 * @param[in] flash_addr Specify the start address in flash to be erased.
 *
 * @return
 * @retval   STATUS_INVALID_PARAM   mode is invalid value.
 * @retval   STATUS_SUCCESS         erase opertation start to processing.
 * @retval   STATUS_EBUSY           flash controller is busying, please call this function again when flash finish current operation.
 *
 *
 * @details  This function is asynchronous NON-BLOCKING mode. That is,
 *      when this function returns STATUS_SUCCESS, it does NOT mean the erase operation
 *      finish. It just means the erasing operation is in progressing.
 *      User should call function flash_check_busy() to check erase finish or
 *      not. No other erase or program instruction can be called before
 *      this erase function finish, otherwise those function will return STATUS_EBUSY.
 *
 */
uint32_t Flash_Erase(flash_erase_mode_t mode, uint32_t flash_addr);

/*write one byte to flash address "write_addr"*/

/**
 * @brief Write/Program flash one byte data
 *
 * @param[in] write_addr  Specify the address of the flash to be written.
 *
 * @param[in] singlebyte  Specify the byte that to be written into the flash.
 *
 * @return
 * @retval   STATUS_SUCCESS         erase opertation start to processing.
 * @retval   STATUS_EBUSY           flash controller is busying, please call this function again when flash finish current operation.
 *
 * @details  this function will write "singlebyte" data to flash address "write_addr".
 *           This function is non-block function. So user shoould check flash_check_busy() to become idle before any other flash API called.
 */

uint32_t Flash_Write_Byte(uint32_t write_addr, uint8_t singlebyte);

/*check the page  crc*/
/**
 * @brief Check the page crc
 *
 * @param[in] read_page_addr  Specify the address of the flash to be verified.
 *
 * @return
 * @retval   STATUS_SUCCESS         erase opertation start to processing.
 * @retval   STATUS_EBUSY           flash controller is busying, please call this function again when flash finish current operation.
 *
 * @details  This function is asynchronous NON-BLOCKING mode. That is,
 *      when this function returns, it does NOT mean the operation
 *      finish. It just means the counting crc8 operation is in progressing.
 *      User should call function flash_check_busy() to check verify finish or
 *      not. If the function counts CRC8 finish, user can call flash_get_crc()
 *      to get the CRC8 value. then user can compare the return CRC8 value
 *      with pre-caculate CRC8 value to verify write page data is correct or not.
 *
 */
uint32_t Flash_Verify_Page(uint32_t read_page_addr);

/*get flash unique ID. ID could be used for some random number generator. */

/**
 * @brief Get unique flash identifier.
 *
 * @param[out] flash_id_buf_addr  Specify the address for return unique id.
 *
 * @param[in] buf_length  Specify the length of unique id in byte it requirs to read.
 *
 * @return
 * @retval    STATUS_INVALID_PARAM  -- if buf_length is zero.
 * @retval    STATUS_SUCCESS
 *
 *
 * @details   flash_id_buf_addr must be 4-bytes alignment. Each flash has one unique
 *            identifier. The length of the uid depends on flash manufacture. Please
 *            check flash datasheet for this uid length.
 *
 */

uint32_t Flash_Get_Unique_Id(uint32_t flash_id_buf_addr, uint32_t buf_length);

/**
 * @brief Flash timing initinal
 *
 * @param[in] None
 * @details According to the flash type and system clock calcation the flash enter
 *          deeply powerdown timing and release from deeply powerdown mode timing and suspend timing, resume timing.
 *
 */
void Flash_Timing_Init(void);

/**
 * @brief Get Flash size
 *
 * @param None
 *
 * @return  return flash size.
 * @retval  flash_size_t
 *                  \arg FLASH_NOT_SUPPORT = 0x00,
 *                  \arg FLASH_512K = 0x13,
 *                  \arg FLASH_1024K = 0x14,
 *                  \arg FLASH_2048K = 0x15,
 *
 */
flash_size_t Flash_Size(void);

/**
 * @brief Read otp sec page
 *
 * @param[in] buf_addr buffer address
 *
 * @retval    STATUS_INVALID_PARAM
 * @retval    STATUS_SUCCESS
 *
 */
uint32_t Flash_Read_Otp_Sec_Page(uint32_t buf_addr);

/**
 * @brief Check Flash in busy state or idle state
 *
 * @param  None
 *
 * @return
 * @retval   0 --- flash is not busy.
 * @retval   1 --- flash is in busy state.
 *
 * @details   Because some flash functions are non-blocking functions,
 *        flash controller should provide this polling fucntion to help
 *        user to know the flash request finished or not.
 *
 */

/*check Flash command Finish*/
__STATIC_INLINE uint32_t Flash_Check_Busy()
{
    return ((FLASH->START)&BUSYBIT) ? 1 : 0 ;
}

/**
 * @brief Get flash read page verification CRC8 value
 *
 * @param     NONE
 *
 * @retval    CRC8
 *
 *
 * @details   This function return CRC8 of the page that specify in previous
 *        funciton flash_verify_page(...).
 *
 */

/*check Flash CRC*/
__STATIC_INLINE uint32_t Flash_Get_Crc()
{
    return (FLASH->CRC);
}

/*set Flash to 4bit SPI mode*/

/**
 * @brief Set flash opeation in 4bits SPI mode
 *
 * @param     NONE
 *
 * @retval    NONE
 *
 * @details   When MCU reboots, the flash controller will operate in 1-bit SPI
 *       mode by default. To speed up the read, user can call function flash_enable_qe()
 *       to enable controller to operate in 4-bits mode. Because most flash operations are
 *       read operations, this 4-bits mode is very important for code XIP execution and read.
 */

__STATIC_INLINE void Flash_Enable_Qe(void)
{
    FLASH->CONTROL_SET = 0x1EF;
}



/**
 * @brief Set flash read page size
 *
 * @param    NONE
 *
 * @retval   NONE
 *
 * @details  This function set read page length be 256 bytes by default.
 *                   Set readpage size. Here we fix to read 256 bytes.
 *                   do not change it
 *
 */

__STATIC_INLINE void Flash_Set_Read_Pagesize(void)
{
    FLASH->PAGE_READ_WORD = 0xFF;
}

/**
 * @brief flush cache
 *
 * @param    NONE
 *
 * @retval   NONE
 *
 * @details  Remark: This behavior will flush all data in cache...
 *                So after this instruction, when CPU want to execute next instruction,
 *                CPU need to re-access data from flash to cache... the cost is very high....
 *                Please ONLY call this function when erase/write behavior happen, you want
 *                to update cache too.
 *
 */
__STATIC_INLINE void Flush_Cache(void)
{
    SYSCTRL->CACHE_CTRL.reg |= (3 << 8);
}

/**
 * @brief This function enable internal flash suspend.
 *
 * @param    NONE
 *
 * @retval   NONE
 *
 *
 */
void Flash_Enable_Suspend(void);

/**
 * @brief This function disable internal flash suspend.
 *
 * @param    NONE
 *
 * @retval   NONE
 *
 *
 */
void Flash_Disable_Suspend(void);

/**
 * @brief get flash control register
 *
 * @param    NONE
 *
 * @retval   control register value
 *
 * @details  This function get flash control register value.
 *
 */
uint32_t Flash_Get_Control_Reg(void);


/**
* @brief Flash_Erase: erase flash address data
*/
uint32_t Flash_Erase_Mpsector(void);
/**
* @brief flash write one page (256bytes) data
*/
uint32_t Flash_Write_Mp_Sector(uint32_t buf_addr, uint32_t write_page_addr);
/**
* @brief flash write one byte data
*/
uint32_t Flash_Write_MpSector_TxPwrCfgByte(uint32_t write_addr, uint8_t singlebyte);
/**
* @brief flash write one byte data
*/
uint32_t Flash_Write_MpSector_RfTrimByte(uint32_t write_addr, uint8_t singlebyte);
/**@}*/ /* end of FLASH_DRIVER group */

/**@}*/ /* end of PERIPHERAL_DRIVER group */

#ifdef __cplusplus
}
#endif

#endif /* end of _RT584_FLASHCTL_H__ */


