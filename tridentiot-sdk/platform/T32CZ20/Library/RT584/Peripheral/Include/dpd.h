/**
 ******************************************************************************
 * @file    dpd.h
 * @author
 * @brief   dpd driver header file
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

#ifndef __RT584_DPD_H__
#define __RT584_DPD_H__

#ifdef __cplusplus
extern "C"
{
#endif


/**************************************************************************************************
 *    INCLUDES
 *************************************************************************************************/
#include "cm33.h"

/**************************************************************************************************
 *    CONSTANTS AND DEFINES
 *************************************************************************************************/
#define  RESET_CAUSE_POR        (1<<0)
#define  RESET_CAUSE_EXT        (1<<1)
#define  RESET_CAUSE_DPD        (1<<2)
#define  RESET_CAUSE_DS         (1<<3)
#define  RESET_CAUSE_WDT        (1<<4)
#define  RESET_CAUSE_SOFT       (1<<5)
#define  RESET_CAUSE_LOCK       (1<<6)

#define CLEAR_RESET_CAUSE           (1)
#define DPD_GPIO_LATCH_ENABLE       (1<<16)
#define FLASH_DPD_ENABLE            (1<<31)

#define  DPD_RET3_SKIP_ISP                          (0x01)      /*<! Retention Register 3. SKIP ISP */
#define  DPD_RET3_DS_FAST_BOOT                      (0x04)      /*<! Retention Register 3. Deep sleep wakeup*/
#define  DPD_CMD_LATCH_ENABLE                       (1<<16)     /*<! Retention Register 3. latch enable*/
/*This is software define setting, in DeepSleep mode, RCO32K is ON or OFF*/
#define  DS_WAKEUP_MASK                             (1<<5)
#define  DS_WAKEUP_LOW                              (0)
#define  DS_WAKEUP_HIGH                             (1<<5)

#define  DS_RCO32K_OFF                              (0)
#define  DS_RCO32K_ON                               (1<<6)
#define  DS_RCO32K_MASK                             (1<<6)

/*This setting only used for Deep-power down mode.*/
#define  DPD_GPIO_LATCH_MASK                        (1)
#define  DPD_GPIO_NO_LATCH                          (0)

/**************************************************************************************************
 *    TYPEDEFS
 *************************************************************************************************/

/** @addtogroup Peripheral_Driver RT584Z Periperhal Driver Function
  @{
*/


/** @addtogroup DPD_DRIVER DPD Driver Functions
  @{
*/


/**************************************************************************************************
 *    GLOBAL PROTOTYPES
 *************************************************************************************************/
/**
 * @brief  Get reset all cause.
 *
 * @retval  get cause register value
 */
__STATIC_INLINE uint32_t Get_All_Reset_Cause(void)
{
    return (DPD_CTRL->DPD_RST_CAUSE.reg);
}

/**
 * @brief  Reset by Power on or not.
 *
 * @retval  0   reset not by Power on
 * @retval  1   reset by Power on
 */
__STATIC_INLINE bool Reset_By_Power_On(void)
{
    return (DPD_CTRL->DPD_RST_CAUSE.bit.RST_CAUSE_POR);
}

/**
 * @brief  Reset by External or not.
 *
 * @retval  0   reset not by External
 * @retval  1   reset by External
 */
__STATIC_INLINE bool Reset_By_External(void)
{
    return (DPD_CTRL->DPD_RST_CAUSE.bit.RST_CAUSE_EXT);
}

/**
 * @brief  Reset by Deep Power Down or not.
 *
 * @retval  0   reset not by Deep Power Down
 * @retval  1   reset by Deep Power Down
 */
__STATIC_INLINE bool Reset_By_Deep_Power_Down(void)
{
    return (DPD_CTRL->DPD_RST_CAUSE.bit.RST_CAUSE_DPD);
}

/**
 * @brief  Reset by Deep Sleep or not.
 *
 * @retval  0   reset not by Deep Sleep
 * @retval  1   reset by Deep Sleep
 */
__STATIC_INLINE bool Reset_By_Deep_Sleep(void)
{
    return (DPD_CTRL->DPD_RST_CAUSE.bit.RST_CAUSE_DS);
}

/**
 * @brief  Reset by WDT or not.
 *
 * @retval  0   reset not by WDT
 * @retval  1   reset by WDT
 */
__STATIC_INLINE bool Reset_By_WDT(void)
{
    return (DPD_CTRL->DPD_RST_CAUSE.bit.RST_CAUSE_WDT);
}

/**
 * @brief  Reset by software or not.
 *
 * @retval  0   reset not by software
 * @retval  1   reset by software
 */
__STATIC_INLINE bool Reset_By_Software(void)
{
    return (DPD_CTRL->DPD_RST_CAUSE.bit.RST_CAUSE_SOFT);
}

/**
 * @brief  Reset by Lock or not.
 *
 * @retval  0   reset not by Lock
 * @retval  1   reset by Lock
 */
__STATIC_INLINE bool Reset_By_Lock(void)
{
    return (DPD_CTRL->DPD_RST_CAUSE.bit.RST_CAUSE_LOCK);
}

/**
 * @brief  Clear reset cause.
 *
 * @retval  None
 */
__STATIC_INLINE void Clear_Reset_Cause(void)
{
    DPD_CTRL->DPD_CMD.bit.CLR_RST_CAUSE = 1;
}

/**
 * @brief  Set retention reg 0.
 *
 * @param value: value save at retention reg 0
 *
 * @retval  None
 */
__STATIC_INLINE void Set_Retention_Reg0(uint32_t value)
{
    DPD_CTRL->DPD_RET0_REG = value;
}

/**
 * @brief  Get retention reg 0.
 *
 * @param value: the address for return value.
 *
 * @retval  Non
 */
__STATIC_INLINE void Get_Retention_Reg0(uint32_t *value)
{
    *value =  DPD_CTRL->DPD_RET0_REG;
}

/**
 * @brief  Set retention reg 1.
 *
 * @param value: value save at retention reg 1
 *
 * @retval  None
 */
__STATIC_INLINE void Set_Retention_Reg1(uint32_t value)
{
    DPD_CTRL->DPD_RET1_REG = value;
}

/**
 * @brief  Get retention reg 1.
 *
 * @param value: the address for return value.
 *
 * @retval  Non
 */
__STATIC_INLINE void Get_Retention_Reg1(uint32_t *value)
{
    *value =  DPD_CTRL->DPD_RET1_REG;
}

/**
 * @brief  Set retention reg 2.
 *
 * @param value: value save at retention reg 2
 *
 * @retval  None
 */
__STATIC_INLINE void Set_Retention_Reg2(uint32_t value)
{
    DPD_CTRL->DPD_RET2_REG = value;
}

/**
 * @brief  Get retention reg 2.
 *
 * @param value: the address for return value.
 *
 * @retval  Non
 */
__STATIC_INLINE void Get_Retention_Reg2(uint32_t *value)
{
    *value =  DPD_CTRL->DPD_RET2_REG;
}


/**
 * @brief  Set Deep slee wake up fast boot
 *
 * @param   NONE
 *
 * @retval  NONE
 */
__STATIC_INLINE void DPD_Set_DeepSleep_Wakeup_Fast_Boot(void)
{
    DPD_CTRL->DPD_RET3_REG |= (DPD_RET3_SKIP_ISP | DPD_RET3_DS_FAST_BOOT);
}

/**
 * @brief  clear dpd latch enable
 *
 * @param   NONE
 *
 * @retval  NONE
 */
__STATIC_INLINE void DPD_Clear_Latch(void)
{
    DPD_CTRL->DPD_CMD.reg &= ~(DPD_CMD_LATCH_ENABLE);
}

/**
 * @brief  clear dpd latch enable
 *
 * @param   NONE
 *
 * @retval  NONE
 */
__STATIC_INLINE void DPD_Flash_Enable(void)
{
    DPD_CTRL->DPD_CMD.reg |= (FLASH_DPD_ENABLE);
}

/**@}*/ /* end of DPD_DRIVER group */

/**@}*/ /* end of PERIPHERAL_DRIVER group */

#ifdef __cplusplus
}
#endif

#endif      /* end of __RT584_DPD_H__ */


