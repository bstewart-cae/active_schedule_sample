/**
 ******************************************************************************
 * @file    pwm.c
 * @author
 * @brief   pwm driver file
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
#include "cm33.h"
#include "pwm.h"
/***********************************************************************************************************************
 *    CONSTANTS AND DEFINES
 **********************************************************************************************************************/

/***********************************************************************************************************************
*    TYPEDEFS
 **********************************************************************************************************************/

/***********************************************************************************************************************
 *    GLOBAL VARIABLES
 **********************************************************************************************************************/

/***********************************************************************************************************************
 *    GLOBAL FUNCTIONS
 **********************************************************************************************************************/
/**
* @brief Pwm initinal function
* @details config pwm paramas, sequence order,number, play count, clcok div,trigger source,nterrupt
* @param[in] pwm_para_config
* @return None
*/
uint32_t Pwm_Init(pwm_seq_para_head_t *pwm_para_config)
{
    PWM_T *pwm;
    pwm_seq_para_t *pwm_seq_set;

    switch (pwm_para_config->pwm_id)
    {
    case PWM_ID_0:
        pwm = PWM0;
        break;
    case PWM_ID_1:
        pwm = PWM1;
        break;
    case PWM_ID_2:
        pwm = PWM2;
        break;
    case PWM_ID_3:
        pwm = PWM3;
        break;
    case PWM_ID_4:
        pwm = PWM4;
        break;
    default:
        return STATUS_INVALID_PARAM;
    }

    pwm->PWM_INT_CLEAR = PWM_RDMA0_INT_CLR | PWM_RDMA1_INT_CLR | PWM_RDMA0_ERR_INT_CLR | PWM_RDMA1_ERR_INT_CLR | PWM_RSEQ_DONE_INT_CLR | PWM_TSEQ_DONE_INT_CLR | PWM_TRSEQ_DONE_INT_CLR | PWM_REG_MODE_INT_CLR;
    pwm->PWM_CTL1 |= PWM_RESET;
    pwm->PWM_RDMA0_CTL1 = PWM_RDMA_ENABLE;
    pwm->PWM_RDMA1_CTL1 = PWM_RDMA_RESET;

    pwm->PWM_SET0 = (pwm_para_config->pwm_seq_order     << PWM_CFG0_SEQ_ORDER_SHFT)    |
                    (pwm_para_config->pwm_seq_num       << PWM_CFG0_SEQ_NUM_SEL_SHFT)  |
                    (pwm_para_config->pwm_seq_mode      << PWM_CFG0_SEQ_MODE_SHFT)     |
                    (pwm_para_config->pwm_dma_smp_fmt   << PWM_CFG0_PWM_DMA_FMT_SHFT)  |
                    (pwm_para_config->pwm_counter_mode  << PWM_CFG0_PWM_CNT_MODE_SHFT) |
                    (PWM_DMA_AUTO_ENABLE                << PWM_CFG0_SEQ_DMA_AUTO_SHFT) |
                    (pwm_para_config->pwm_clk_div       << PWM_CFG0_CK_DIV_SHFT)       |
                    (pwm_para_config->pwm_triggered_src << PWM_CFG0_PWM_ENA_TRIG_SHFT);


    if (pwm_para_config->pwm_dma_smp_fmt == PWM_DMA_SMP_FMT_0)
    {
        pwm->PWM_SET1 = pwm_para_config->pwm_count_end_val;
    }

    pwm->PWM_SET2 = pwm_para_config->pwm_play_cnt;

    if (pwm_para_config->pwm_seq_num == PWM_SEQ_NUM_2)
    {
        pwm_seq_set = &(pwm_para_config->pwm_seq0);
        pwm->PWM_SET3 = pwm_seq_set->pwm_element_num;
        pwm->PWM_SET4 = pwm_seq_set->pwm_repeat_num;
        pwm->PWM_SET5 = pwm_seq_set->pwm_delay_num;
        pwm->PWM_RDMA0_SET0 = (pwm_para_config->pwm_dma_smp_fmt == PWM_DMA_SMP_FMT_0) ? ((pwm_seq_set->pwm_element_num >> 1) | ((pwm_seq_set->pwm_element_num >> 2) << 16))
                              : ((pwm_seq_set->pwm_element_num)    | ((pwm_seq_set->pwm_element_num >> 1) << 16));
        pwm->PWM_RDMA0_SET1 = pwm_seq_set->pwm_rdma_addr;

        pwm_seq_set = &(pwm_para_config->pwm_seq1);
        pwm->PWM_SET6 = pwm_seq_set->pwm_element_num;
        pwm->PWM_SET7 = pwm_seq_set->pwm_repeat_num;
        pwm->PWM_SET8 = pwm_seq_set->pwm_delay_num;
        pwm->PWM_RDMA1_SET0 = (pwm_para_config->pwm_dma_smp_fmt == PWM_DMA_SMP_FMT_0) ? ((pwm_seq_set->pwm_element_num >> 1) | ((pwm_seq_set->pwm_element_num >> 2) << 16))
                              : ((pwm_seq_set->pwm_element_num)    | ((pwm_seq_set->pwm_element_num >> 1) << 16));
        pwm->PWM_RDMA1_SET1 = pwm_seq_set->pwm_rdma_addr;
        pwm->PWM_INT_MASK = ~(PWM_RDMA0_ERR_INT_MASK_ENABLE | PWM_RDMA1_ERR_INT_MASK_ENABLE | PWM_RDMA0_INT_MASK_ENABLE  | PWM_RDMA1_INT_MASK_ENABLE  | PWM_RSEQ_DONE_INT_MASK_ENABLE | PWM_TSEQ_DONE_INT_MASK_ENABLE | PWM_TRSEQ_DONE_INT_MASK_ENABLE);



    }
    else if (pwm_para_config->pwm_seq_num == PWM_SEQ_NUM_1 )
    {
        if (pwm_para_config->pwm_seq_order == PWM_SEQ_ORDER_R)
        {
            pwm_seq_set = &(pwm_para_config->pwm_seq0);
            pwm->PWM_SET3 = pwm_seq_set->pwm_element_num;
            pwm->PWM_SET4 = pwm_seq_set->pwm_repeat_num;
            pwm->PWM_SET5 = pwm_seq_set->pwm_delay_num;
            pwm->PWM_RDMA0_SET0 = (pwm_para_config->pwm_dma_smp_fmt == PWM_DMA_SMP_FMT_0) ? ((pwm_seq_set->pwm_element_num >> 1) | ((pwm_seq_set->pwm_element_num >> 2) << 16))
                                  : ((pwm_seq_set->pwm_element_num)    | ((pwm_seq_set->pwm_element_num >> 1) << 16));
            pwm->PWM_RDMA0_SET1 = pwm_seq_set->pwm_rdma_addr;
            pwm->PWM_INT_MASK = ~(PWM_RDMA0_ERR_INT_MASK_ENABLE | PWM_RDMA0_INT_MASK_ENABLE | PWM_RSEQ_DONE_INT_MASK_ENABLE | PWM_TSEQ_DONE_INT_MASK_ENABLE | PWM_TRSEQ_DONE_INT_MASK_ENABLE);

        }
        else if (pwm_para_config->pwm_seq_order == PWM_SEQ_ORDER_T)
        {
            pwm_seq_set = &(pwm_para_config->pwm_seq1);
            pwm->PWM_SET6 = pwm_seq_set->pwm_element_num;
            pwm->PWM_SET7 = pwm_seq_set->pwm_repeat_num;
            pwm->PWM_SET8 = pwm_seq_set->pwm_delay_num;
            pwm->PWM_RDMA1_SET0 = (pwm_para_config->pwm_dma_smp_fmt == PWM_DMA_SMP_FMT_0) ? ((pwm_seq_set->pwm_element_num >> 1) | ((pwm_seq_set->pwm_element_num >> 2) << 16))
                                  : ((pwm_seq_set->pwm_element_num)    | ((pwm_seq_set->pwm_element_num >> 1) << 16));
            pwm->PWM_RDMA1_SET1 = pwm_seq_set->pwm_rdma_addr;
            pwm->PWM_INT_MASK = ~(PWM_RDMA1_ERR_INT_MASK_ENABLE | PWM_RDMA1_INT_MASK_ENABLE | PWM_RSEQ_DONE_INT_MASK_ENABLE | PWM_TSEQ_DONE_INT_MASK_ENABLE | PWM_TRSEQ_DONE_INT_MASK_ENABLE);
        }
    }

    return STATUS_SUCCESS;
}
/**
 * @brief Pwm Start function
 * @details config pwm wdma enable, sequenc nubmer and enable pwm clock source
 * @param[in] pwm_para_config
 * @return None
 */
uint32_t Pwm_Start(pwm_seq_para_head_t *pwm_para_config)
{
    PWM_T *pwm;

    switch (pwm_para_config->pwm_id)
    {
    case PWM_ID_0:
        pwm = PWM0;
        NVIC_EnableIRQ(Pwm0_IRQn);
        break;
    case PWM_ID_1:
        pwm = PWM1;
        NVIC_EnableIRQ(Pwm1_IRQn);
        break;
    case PWM_ID_2:
        pwm = PWM2;
        NVIC_EnableIRQ(Pwm2_IRQn);
        break;
    case PWM_ID_3:
        pwm = PWM3;
        NVIC_EnableIRQ(Pwm3_IRQn);
        break;
    case PWM_ID_4:
        pwm = PWM4;
        NVIC_EnableIRQ(Pwm4_IRQn);
        break;
    default:
        return STATUS_INVALID_PARAM;
    }

    if (pwm_para_config->pwm_seq_num == PWM_SEQ_NUM_2)
    {
        pwm->PWM_RDMA0_CTL0 = PWM_RDMA_ENABLE;
        pwm->PWM_RDMA1_CTL0 = PWM_RDMA_ENABLE;
    }
    else if (pwm_para_config->pwm_seq_num == PWM_SEQ_NUM_1 )
    {
        if (pwm_para_config->pwm_seq_order == PWM_SEQ_ORDER_R)
        {
            pwm->PWM_RDMA0_CTL0 = PWM_RDMA_ENABLE;
        }
        else if (pwm_para_config->pwm_seq_order == PWM_SEQ_ORDER_T)
        {
            pwm->PWM_RDMA1_CTL0 = PWM_RDMA_ENABLE;
        }
    }
    else
    {
        return STATUS_INVALID_PARAM;
    }

    pwm->PWM_CTL0 |= (PWM_ENABLE_PWM | PWM_ENABLE_CLK);

    return STATUS_SUCCESS;
}
/**
 * @brief Pwm Stop function
 * @details Disable for PWM interrupt, PWM xdma control
 * @param[in] id pwm identifie
 * @return None
 */
uint32_t Pwm_Stop(pwm_seq_para_head_t *pwm_para_config)
{
    PWM_T *pwm;

    switch (pwm_para_config->pwm_id)
    {
    case PWM_ID_0:
        pwm = PWM0;
        NVIC_DisableIRQ(Pwm0_IRQn);
        break;
    case PWM_ID_1:
        pwm = PWM1;
        NVIC_DisableIRQ(Pwm1_IRQn);
        break;
    case PWM_ID_2:
        pwm = PWM2;
        NVIC_DisableIRQ(Pwm2_IRQn);
        break;
    case PWM_ID_3:
        pwm = PWM3;
        NVIC_DisableIRQ(Pwm3_IRQn);
        break;
    case PWM_ID_4:
        pwm = PWM4;
        NVIC_DisableIRQ(Pwm4_IRQn);
        break;
    default:
        return STATUS_INVALID_PARAM;
    }


    if (pwm_para_config->pwm_seq_num == PWM_SEQ_NUM_2)
    {
        pwm->PWM_RDMA0_CTL1 |= PWM_RDMA_RESET;      //reset xdma
        pwm->PWM_RDMA1_CTL1 |= PWM_RDMA_RESET;      //reset xdma
    }
    else if (pwm_para_config->pwm_seq_num == PWM_SEQ_NUM_1 )
    {
        if (pwm_para_config->pwm_seq_order == PWM_SEQ_ORDER_R)
        {
            pwm->PWM_RDMA0_CTL1 |= PWM_RDMA_RESET;       //reset xdma
        }
        else if (pwm_para_config->pwm_seq_order == PWM_SEQ_ORDER_T)
        {
            pwm->PWM_RDMA1_CTL1 |= PWM_RDMA_RESET;        //reset xdma
        }
    }
    else
    {
        return STATUS_INVALID_PARAM;
    }

    pwm->PWM_CTL0 &= ~ (PWM_DISABLE_PWM | PWM_DISABLE_CLK);

    return STATUS_SUCCESS;
}

typedef struct __pwm_cb_s
{
    pwm_proc_cb callback;
    void *ctx;
} __pwm_cb_t;

static __pwm_cb_t __pwm_user_isr[PWM_ID_MAX];

void Pwm_Register_Callback(pwm_id_t pwm_id, pwm_proc_cb callback, void *ctx)
{
    if (pwm_id >= PWM_ID_MAX)
    {
        return;
    }
    __pwm_user_isr[pwm_id].callback = callback;
    __pwm_user_isr[pwm_id].ctx = ctx;
}

void Pwm_UnRegister_Callback(pwm_id_t pwm_id)
{
    if (pwm_id >= PWM_ID_MAX)
    {
        return;
    }
    __pwm_user_isr[pwm_id].callback = NULL;
    return;
}

static void __Pwm_Handler(PWM_T *pwm, const __pwm_cb_t *pwm_cb)
{
    uint32_t status;

    status = pwm->PWM_INT_STATUS;
    pwm->PWM_INT_CLEAR = status;
    if (status & PWM_RDMA0_STATUS_ERR_INT_MASK)
    {
        pwm->PWM_INT_MASK |= PWM_RDMA0_ERR_INT_MASK_ENABLE;
    }
    if (status & PWM_RDMA1_STATUS_ERR_INT_MASK)
    {
        pwm->PWM_INT_MASK |= PWM_RDMA1_ERR_INT_MASK_ENABLE;
    }

    {
        pwm_proc_cb callback;

        if ((callback = pwm_cb->callback) != NULL)
        {
            callback(pwm, status, pwm_cb->ctx);
        }
    }
}

/**
 * @brief PWM0 interrupt handerl
 * @return None
 */
void Pwm0_Handler(void)
{
    __Pwm_Handler(PWM0, &__pwm_user_isr[PWM_ID_0]);
}

/**
 * @brief PWM1 interrupt handerl
 * @return None
 */
void Pwm1_Handler(void)
{
    __Pwm_Handler(PWM1, &__pwm_user_isr[PWM_ID_1]);
}

/**
 * @brief PWM2 interrupt handerl
 * @return None
 */
void Pwm2_Handler(void)
{
    __Pwm_Handler(PWM2, &__pwm_user_isr[PWM_ID_2]);
}

/**
 * @brief PWM3 interrupt handerl
 * @return None
 */
void Pwm3_Handler(void)
{
    __Pwm_Handler(PWM3, &__pwm_user_isr[PWM_ID_3]);
}

/**
 * @brief PWM4 interrupt handerl
 * @return None
 */
void Pwm4_Handler(void)
{
    __Pwm_Handler(PWM4, &__pwm_user_isr[PWM_ID_4]);
}