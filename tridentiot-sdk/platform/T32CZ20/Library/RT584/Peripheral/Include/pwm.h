/**
 ******************************************************************************
 * @file    pwm.h
 * @author
 * @brief   pwm driver header file
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

#ifndef _RT584_PWM_H__
#define _RT584_PWM_H__

#ifdef __cplusplus
extern "C"
{
#endif



/***********************************************************************************************************************
 *    INCLUDES
 **********************************************************************************************************************/
/***********************************************************************************************************************
 *    MACROS
 **********************************************************************************************************************/
/***********************************************************************************************************************
 *    TYPEDEFS
 **********************************************************************************************************************/
/**@brief PWM ID mapping.
 */
typedef enum
{
    PWM_ID_0,
    PWM_ID_1,
    PWM_ID_2,
    PWM_ID_3,
    PWM_ID_4,
    PWM_ID_MAX,
} pwm_id_t;

/**@brief PWM Clock division table.
 */
typedef enum
{
    PWM_CLK_DIV_1 = 0,
    PWM_CLK_DIV_2,
    PWM_CLK_DIV_4,
    PWM_CLK_DIV_8,
    PWM_CLK_DIV_16,
    PWM_CLK_DIV_32,
    PWM_CLK_DIV_64,
    PWM_CLK_DIV_128,
    PWM_CLK_DIV_256,
} pwm_clk_div_t;

/**@brief PWM Sequence order table.
 * Order_0: S0  /  Order_1: S1  /  Order_2: S0S1  /  Order_3: S1S0
 */
typedef enum
{
    PWM_SEQ_ORDER_R = 0,
    PWM_SEQ_ORDER_T,
    PWM_SEQ_ORDER_MAX,
}   pwm_seq_order_t;

/**@brief PWM sequence selection table.
 */
typedef enum
{
    PWM_SEQ_NUM_1,
    PWM_SEQ_NUM_2,
}   pwm_seq_num_t;

/**@brief PWM sequence playmode table.
 */
typedef enum
{
    PWM_SEQ_MODE_NONCONTINUOUS,
    PWM_SEQ_MODE_CONTINUOUS,
}   pwm_seq_mode_t;

/**@brief PWM trigger source table.
 */
typedef enum
{
    PWM_TRIGGER_SRC_PWM0 = 0,
    PWM_TRIGGER_SRC_PWM1,
    PWM_TRIGGER_SRC_PWM2,
    PWM_TRIGGER_SRC_PWM3,
    PWM_TRIGGER_SRC_PWM4,
    PWM_TRIGGER_SRC_SELF = 7,
}   pwm_trigger_src_t;

/**@brief PWM DMA sample format table.
 */
typedef enum
{
    PWM_DMA_SMP_FMT_0 = 0,
    PWM_DMA_SMP_FMT_1,
}   pwm_dma_smp_fmt_t;

/**@brief PWM counter mode table.
 * UP: Up mode / UD: Up-Down mode
 */
typedef enum
{
    PWM_COUNTER_MODE_UP = 0,
    PWM_COUNTER_MODE_UD,
} pwm_counter_mode_t;

/**@brief PWM counter mode table.
 * UP: Up mode / UD: Up-Down mode
 */
typedef enum
{
    PWM_TRIG_DISABLE = 0,
    PWM_TRIG_ENABLE,
} pwm_counter_trig_t;

/**@brief PWM register mode.
 * UP: Up mode / UD: Up-Down mode
 */
typedef enum
{
    PWM_REG_MODE_DISABLE = 0,
    PWM_REG_MODE_ENABLE,
} pwm_register_mode_t;

/**@brief PWM register mode.
 * UP: Up mode / UD: Up-Down mode
 */
typedef enum
{
    PWM_REGMODE_DATA1 = 0,
    PWM_REGMODE_DATA2,
} pwm_regmode_data_t;
/**@brief PWM DMA auto table.
 */
typedef enum
{
    PWM_DMA_AUTO_DISABLE = 0,
    PWM_DMA_AUTO_ENABLE,
} pwm_dma_auto_t;

/***********************************************************************************************************************
 *    CONSTANTS AND DEFINES
 **********************************************************************************************************************/
#define PWM_ENABLE_PWM                  (0x01UL << PWM_CFG0_PWM_ENA_SHFT)                        /*!< pwm control enable. */
#define PWM_ENABLE_CLK                  (0x01UL << PWM_CFG0_CK_ENA_SHFT)                         /*!< pwm control clock enable. */

#define PWM_DISABLE_PWM                 (0x01UL << PWM_CFG0_PWM_ENA_SHFT)                        /*!< pwm control disable.  */
#define PWM_DISABLE_CLK                 (0x01UL << PWM_CFG0_CK_ENA_SHFT)                         /*!< pwm control clock disable. */

#define PWM_RESET                       (0x01UL << PWM_CFG0_PWM_RST_SHFT)                        /*!< pwm control reset */
#define PWM_RDMA_ENABLE                 (0x01UL << PWM_CFG0_PWM_RDMA0_CTL0_SHFT)         /*!< pwm control rdma enable */
#define PWM_RDMA_RESET                  (0x01UL << PWM_CFG0_PWM_RDMA0_CTL1_SHFT)       /*!< pwm control rdma reset */

#define PWM_RDMA0_INT_CLR               (0x01UL << PWM_RDMA0_INT_CLR_SHFT)                   /*!< pwm rdma0 interrupt clear */
#define PWM_RDMA0_ERR_INT_CLR           (0x01UL << PWM_RDMA0_ERR_INT_CLR_SHFT)           /*!< pwm rdma0 error interrupt clear */
#define PWM_RDMA1_INT_CLR               (0x01UL << PWM_RDMA1_INT_CLR_SHFT)                   /*!< pwm rdma1 interrupt clear  */
#define PWM_RDMA1_ERR_INT_CLR           (0x01UL << PWM_RDMA1_ERR_INT_CLR_SHFT)           /*!< pwm rdma1 error interrupt clear */
#define PWM_RSEQ_DONE_INT_CLR           (0x01UL << PWM_RSEQ_DONE_INT_CLR_SHFT)           /*!< pwm rseq done  interrupt clear */
#define PWM_TSEQ_DONE_INT_CLR           (0x01UL << PWM_TSEQ_DONE_INT_CLR_SHFT)           /*!< pwm tseq done  interrupt clear. */
#define PWM_TRSEQ_DONE_INT_CLR          (0x01UL << PWM_TRSEQ_DONE_INT_CLR_SHFT)          /*!< pwm trseq done  interrupt clear. */
#define PWM_REG_MODE_INT_CLR            (0x01UL << PWM_REG_MODE_INT_CLR_SHFT)                /*!< pwm reg mode interrupt clear */

#define PWM_RDMA0_INT_MASK_ENABLE       (0x01UL << PWM_RDMA0_INT_MASK_SHFT)                  /*!< pwm rdma0 interrupt mask enable */
#define PWM_RDMA0_ERR_INT_MASK_ENABLE   (0x01UL << PWM_RDMA0_ERR_INT_MASK_SHFT)          /*!< pwm rdma0 error interrupt mask enable */
#define PWM_RDMA1_INT_MASK_ENABLE       (0x01UL << PWM_RDMA1_INT_MASK_SHFT)                  /*!< pwm rdma1 interrupt mask enable */
#define PWM_RDMA1_ERR_INT_MASK_ENABLE   (0x01UL << PWM_RDMA1_ERR_INT_MASK_SHFT)          /*!< pwm rdma1 error interrupt mask enable */
#define PWM_RSEQ_DONE_INT_MASK_ENABLE   (0x01UL << PWM_RSEQ_DONE_INT_MASK_SHFT)          /*!< pwm rseq done interrupt mask enable */
#define PWM_TSEQ_DONE_INT_MASK_ENABLE   (0x01UL << PWM_TSEQ_DONE_INT_MASK_SHFT)          /*!< pwm tseq done interrupt mask enable */
#define PWM_TRSEQ_DONE_INT_MASK_ENABLE  (0x01UL << PWM_TRSEQ_DONE_INT_MASK_SHFT)         /*!< pwm trseq done  interrupt mask enable */

/**@brief Convert THD_Value / End_Value / PHA_Value into a 32-bit data
 * Mode0: val1=THD1, val2=THD2
 * Mode1: val0=PHA, val1=THD, val2=end
 */
/***********************************************************************************************************************
 *    MACROS
 **********************************************************************************************************************/
#define PWM_FILL_SAMPLE_DATA_MODE0(val0,val1,val2)  ((val0 << 31) | (val2 << 16) | (val0 << 15) | (val1))
#define PWM_FILL_SAMPLE_DATA_MODE1(val0,val1,val2)  ((val2 << 16) | (val0 << 15) | (val1))

/**@brief Structure for each RDMA configurations
 */
typedef struct
{
    uint32_t    pwm_rdma_addr;                 /*!< xDMA start address configurations for PWM sequence controller. */
    uint16_t    pwm_element_num;               /*!< Element number configurations for PWM sequence controller. */
    uint16_t    pwm_repeat_num;                /*!< Repeat number configurations of each element for PWM sequence controller. */
    uint16_t    pwm_delay_num;                 /*!< Delay number configurations after PWM sequence is play finish for PWM sequence controller. */
} pwm_seq_para_t;

/**
* @brief Structure for each PWM configurations
*/
typedef struct
{
    pwm_seq_para_t        pwm_seq0;            /*!< Handle of PWM sequence controller configurations for R-SEQ. */
    pwm_seq_para_t        pwm_seq1;            /*!< Handle of PWM sequence controller configurations for T-SEQ. */
    uint16_t              pwm_play_cnt;        /*!< PWM play amount configuration. */
    uint16_t              pwm_count_end_val;   /*!< PWM counter end value configuration. */
    pwm_seq_order_t       pwm_seq_order;       /*!< PWM sequence play order configuration. */
    pwm_trigger_src_t     pwm_triggered_src;   /*!< PWM play trigger source configuration. */
    pwm_seq_num_t         pwm_seq_num;         /*!< PWM sequence number configuration. */
    pwm_id_t              pwm_id;              /*!< PWM ID designation. */
    pwm_clk_div_t         pwm_clk_div;         /*!< PWM gated clock divider value configuration. */
    pwm_counter_mode_t    pwm_counter_mode;    /*!< PWM counter mode configuration. */
    pwm_dma_smp_fmt_t     pwm_dma_smp_fmt;     /*!< PWM DMA sample format configuration. */
    pwm_seq_mode_t        pwm_seq_mode;        /*!< PWM sequence play mode configuration. */
} pwm_seq_para_head_t;

/** @addtogroup Peripheral_Driver RT584Z Periperhal Driver Function
  @{
*/


/** @addtogroup PWM_DRIVER PWM Driver Functions
  @{
*/

/***********************************************************************************************************************
 *    GLOBAL PROTOTYPES
 **********************************************************************************************************************/
/**
 * @brief Function to config pwm params
 * @param[in] pwm_para_config
 *            \arg pwm_seq0             Handle of PWM sequence controller configurations for R-SEQ
 *            \arg pwm_seq1             Handle of PWM sequence controller configurations for T-SEQ
 *            \arg pwm_play_cnt         PWM play amount configuration
 *            \arg pwm_count_end_val    PWM counter end value configuration
 *            \arg pwm_seq_order        PWM sequence play order configuration
 *            \arg pwm_triggered_src    PWM play trigger source configuration
 *            \arg pwm_seq_num          PWM sequence number configuration
 *            \arg pwm_id               PWM ID designation
 *            \arg pwm_clk_div          PWM gated clock divider value configuration
 *            \arg pwm_counter_mode     PWM counter mode configuration
 *            \arg pwm_dma_smp_fmt      PWM DMA sample format configuration
 *            \arg pwm_seq_mode         PWM sequence play mode configuration
 * @return None
 */
uint32_t Pwm_Init(pwm_seq_para_head_t *pwm_para_config);

/**
 * @brief Function to enable pwm irq,clock,rdam
 * @param[in] pwm_para_config
 *            \arg pwm_seq_order        PWM sequence play order configuration
 *            \arg pwm_seq_num          PWM sequence number configuration
 *            \arg pwm_id               PWM ID designation
 * @retval STATUS_SUCCESS config pwm irq,clock,rdma registers is vaild
 * @retval STATUS_INVALID_PARAM config pwm irq,clock,rdma registers is invaild
 */
uint32_t Pwm_Start(pwm_seq_para_head_t *pwm_para_config);
/**
 * @brief Function to enable pwm irq,clock,rdam
 * @param[in] pwm_para_config
 *            \arg pwm_seq_order        PWM sequence play order configuration
 *            \arg pwm_seq_num          PWM sequence number configuration
 *            \arg pwm_id               PWM ID designation
 * @retval STATUS_SUCCESS config pwm irq,clock,rdma registers is vaild
 * @retval STATUS_INVALID_PARAM config pwm irq,clock,rdma registers is invaild
 */
uint32_t Pwm_Stop(pwm_seq_para_head_t *pwm_para_config);

typedef void (*pwm_proc_cb)(PWM_T *pwm, uint32_t status, void *ctx);

/**
 * @brief Register user interrupt ISR callback function.
 *
 * @param[in]    pwm_id                 PWM ID designation
 * @param[in]    callback               Specifies user callback function when the PWM interrupt generated
 * @param[in]    ctx                    Paramter passed to user interrupt handler "callback"
 *
 * @retval      none
 */
void Pwm_Register_Callback(pwm_id_t pwm_id, pwm_proc_cb callback, void *ctx);

/**
 * @brief Unregister user interrupt ISR callback function.
 *
 * @param[in]    pwm_id                 PWM ID designation
 *
 * @retval      none
 */
void Pwm_UnRegister_Callback(pwm_id_t pwm_id);

/**@}*/ /* end of PWM_DRIVER group */

/**@}*/ /* end of PERIPHERAL_DRIVER group */

#ifdef __cplusplus
}
#endif

#endif /* End of _RT584_PWM_H_ */


