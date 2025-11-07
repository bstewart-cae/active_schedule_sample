/**
  ******************************************************************************
 * @file     flashctl_reg.h
  * @author
  * @brief   flash control register definition header file
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
  *
  */

#ifndef _RT584_FLASHCTL_REG_H_
#define _RT584_FLASHCTL_REG_H_

#if defined ( __CC_ARM   )
#pragma anon_unions
#endif

/** @addtogroup REGISTER RT584Z Peripheral Control Register

  @{

*/

/**
    @addtogroup FLASHCTL Flash Controller(Flash)
    Memory Mapped Structure for Flash Controller
  @{
*/

typedef struct
{
    __IO uint32_t COMMAND;        /*offset : 0x00*/
    __IO uint32_t FLASH_ADDR;     /*offset : 0x04*/
    __IO uint32_t START;          /*offset : 0x08*/
    __IO uint32_t STATUS;         /*offset : 0x0C*/
    __IO uint32_t FLASH_DATA;     /*offset : 0x10*/
    __IO uint32_t MEM_ADDR;       /*offset : 0x14*/
    __IO uint32_t CONTROL_SET;    /*offset : 0x18*/
    __IO uint32_t CRC;            /*offset : 0x1C*/
    __IO uint32_t DPD;            /*offset : 0x20*/
    __IO uint32_t RDPD;           /*offset : 0x24*/
    __IO uint32_t SUSPEND;        /*offset : 0x28*/
    __IO uint32_t RESUME;         /*offset : 0x2C*/
    __IO uint32_t FLASH_INSTR;    /*offset : 0x30*/
    __IO uint32_t PAGE_READ_WORD; /*offset : 0x34*/
    __IO uint32_t FLASH_INFO;     /*offset : 0x38*/
    __IO uint32_t RESV;           /*offset : 0x3C*/
    __IO uint32_t FLASH_INT;      /*offset : 0x40*/
    __IO uint32_t PATTERN;        /*offset : 0x44*/
} FLASHCTL_T;


#define  CMD_READBYTE       0x00

#define  CMD_READVERIFY     0x04
#define  CMD_READPAGE       0x08
#define  CMD_READUID        0x09

#define  CMD_READ_STATUS1      0x0D
#define  CMD_READ_STATUS2      0x0E
#define  CMD_READ_STATUS3      0x0F

#define  CMD_ERASEPAGE         0x20
#define  CMD_ERASESECTOR       0x21
#define  CMD_ERASE_BL32K       0x22
#define  CMD_ERASE_BL64K       0x24

#define  CMD_WRITEBYTE         0x10
#define  CMD_WRITEPAGE         0x18

#define  CMD_WRITE_STATUS      0x1C
#define  CMD_WRITE_STATUS1     0x1D
#define  CMD_WRITE_STATUS2     0x1E
#define  CMD_WRITE_STATUS3     0x1F


#define  CMD_READ_SEC_PAGE     ((1<<6) | CMD_READPAGE)
#define  CMD_WRITE_SEC_PAGE    ((1<<6) | CMD_WRITEPAGE)
#define  CMD_ERASE_SEC_PAGE    ((1<<6) | CMD_ERASESECTOR)

#define  STARTBIT          (1)
#define  BUSYBIT           (1<<8)

#define  CMD_FLASH_RESET_ENABLE     0x66
#define  CMD_FLASH_RESET            0x99
#define  CMD_MANUAL_MODE            0x30


/**@}*/ /* end of Flash Control group */

/**@}*/ /* end of REGISTER group */

#if defined ( __CC_ARM   )
#pragma no_anon_unions
#endif

#endif /* end of _RT584_FLASHCTL_REG_H_ */
