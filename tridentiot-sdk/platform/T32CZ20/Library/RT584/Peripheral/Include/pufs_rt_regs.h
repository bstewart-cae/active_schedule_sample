/**
 ******************************************************************************
 * @file    pufs_rf_reg.h
 * @author
 * @brief   physical unclonable function rf definition header file
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

#ifndef _RT584_PUFS_RT_REGS_H__
#define _RT584_PUFS_RT_REGS_H__

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

//----------------------REGISTER STRUCT-------------------------------------//
typedef struct pufs_pif_regs
{
    volatile  uint32_t  pif0;
    volatile  uint32_t  option1;
    volatile  uint32_t  option2;
    volatile  uint32_t  _pad0;
    volatile  uint32_t  cde_lock[4];           /*0x10~0x1C  cde_lckwd_xxx (cde_seg_xxx)*/
    volatile  uint32_t  secrp[2];              /*0x20~0x24 */
    volatile  uint32_t  _pad1[2];
    volatile  uint32_t  zeroized_puf;          /*0x30*/
    volatile  uint32_t  _pad2;
    volatile  uint32_t  zeroized_otp[2];       /*0x38 0x3C*/
    volatile  uint32_t  _pad3[12];             /*0x40 ~ 0x6C*/
    volatile  uint32_t  puf_lock[4];           /*0x70 ~ 0x7C*/
    volatile  uint32_t  otp_lock[32];          /*0x80 ~ 0xFC*/
} PUF_PIF_T;

typedef struct pufs_rng_regs
{
    volatile  uint32_t  version;                /*0x00*/
    volatile  uint32_t  _pad0;
    volatile  uint32_t  feature;                /*0x08*/
    volatile  uint32_t  _pad1;
    volatile  uint32_t  status;                 /*0x10*/
    volatile  uint32_t  _pad2[3];
    volatile  uint32_t  enable;                 /*0x20*/
    volatile  uint32_t  pathsel;                /*0x24*/
    volatile  uint32_t  _pad3[2];
    volatile  uint32_t  cfg0;                   /*0x30 */
    volatile  uint32_t  cfg1;                   /*0x34  ?*/
    volatile  uint32_t  cfg2;                   /*0x38 */
    volatile  uint32_t  cfg3;                   /*0x3C */
    volatile  uint32_t  wtrmark0;               /*0x40 */
    volatile  uint32_t  wtrmark1;               /*0x44 */
    volatile  uint32_t  _pad4[2];
    volatile  uint32_t  tfc0;                   /*0x50 RO */
    volatile  uint32_t  tfc1;
    volatile  uint32_t  tfc2;
    volatile  uint32_t  tfc3;
    volatile  uint32_t  fcnt0;                  /*0x60 RO */
    volatile  uint32_t  fcnt1;                  /*0x64 RO */
    volatile  uint32_t  _pad5;
    volatile  uint32_t  htclr;                  /*0x6C */
    volatile  uint32_t  data;                   /*0x70 */
    volatile  uint32_t  _pad6[3];
} PUF_RNG_T;

typedef struct pufs_cfg_regs
{
    volatile  uint32_t  version;
    volatile  uint32_t  features0;
    volatile  uint32_t  features1;
    volatile  uint32_t  features2;
    volatile  uint32_t  interrupt;              /*0x10 */
    volatile  uint32_t  _pad1[3];
    volatile  uint32_t  status;                 /*0x20 */
    volatile  uint32_t  pdstb;                  /*0x24 */
    volatile  uint32_t  cfg1;                   /*0x28 */
    volatile  uint32_t  cfg2;
    volatile  uint32_t  _pad2[12];              /*0x30~0x5C*/
    volatile  uint32_t  cde_msk[2];             /*0x60 */
    volatile  uint32_t  otp_msk[2];             /*0x68 */
    volatile  uint32_t  puf_msk;                /*0x70 */
    volatile  uint32_t  _pad3;
    volatile  uint32_t  sec_lock;               /*0x78 */
    volatile  uint32_t  reg_lock;               /*0x7C */
} PUF_CFG_T;

typedef struct pufs_ptm_regs
{
    volatile  uint32_t  status;                 /*0x00*/
    volatile  uint32_t  rd_mode;                /*0x04*/
    volatile  uint32_t  ptc_page;               /*0x08*/
    volatile  uint32_t  healthcfg;              /*0x0C*/
    volatile  uint32_t  set_pin;                /*0x10*/
    volatile  uint32_t  set_flag;               /*0x14*/
    volatile  uint32_t  _pad0[2];
    volatile  uint32_t  puf_chk;                /*0x20*/
    volatile  uint32_t  puf_enroll;             /*0x24*/
    volatile  uint32_t  puf_zeroize;            /*0x28*/
    volatile  uint32_t  _pad1;
    volatile  uint32_t  off_chk;                /*0x30*/
    volatile  uint32_t  auto_repair;            /*0x34*/
    volatile  uint32_t  otp_zeroize;            /*0x38*/
    volatile  uint32_t  _pad2;
    volatile  uint32_t  repair_pgm;
    volatile  uint32_t  repair_reg;
    volatile  uint32_t  _pad[14];
} PUF_PTM_T;


/*2023/03/03: Because we don't include RT584 header file, we define a  OTP_SECURE_BASE here*/
#define OTP_SECURE_BASE      (0x50000000UL+0x44000UL)

#define OTP2_BS              (OTP_SECURE_BASE)

#define OTP1_BS              (OTP_SECURE_BASE + 0x400)

#define PUF_BS               (OTP_SECURE_BASE + 0x800)

#define OTP_PIF_S            ((PUF_PIF_T *) (OTP_SECURE_BASE + 0x900))

#define OTP_RNG_S            ((PUF_RNG_T *) (OTP_SECURE_BASE + 0xA00))

#define OTP_CFG_S            ((PUF_CFG_T *) (OTP_SECURE_BASE + 0xA80))

#define OTP_PTM_S            ((PUF_PTM_T *) (OTP_SECURE_BASE + 0xB80))


//----------------------REGISTER BIT MASKS----------------------------------//
#define RT_PIF_00_PUFLCK_MASK              0x000f0000
#define RT_PIF_00_OTPLCK_MASK              0x00f00000

//----------------------REGISTER POSITION BITS------------------------------//
#define RT_RNG_FUN_ENABLE_BITS             0
#define RT_RNG_OUT_ENABLE_BITS             2
#define RT_RNG_STATUS_RN_READY_BITS        10

#define RT_RNG_STATUS_CTRL_BITS        0
#define RT_RNG_STATUS_FIFO_BITS        1
#define RT_RNG_STATUS_ENTROPY_BITS     2
#define RT_RNG_STATUS_RNG_BITS         8
#define RT_RNG_STATUS_HEALTH_BITS      9
#define RT_RNG_STATUS_READY_BITS       10
#define RT_RNG_STATUS_ERROR_BITS       11

#define RT_RNG_FIFO_CLEAR_BITS              0
//----------------------REGISTER BIT MASKS----------------------------------//
#define RT_CFG_REGLCK_CDEMSK_MASK          0x000f0000
#define RT_CFG_REGLCK_KEYMSK_MASK          0x00f00000

//----------------------REGISTER BIT MASKS----------------------------------//
#define RT_PTM_STATUS_BUSY_MASK            0x00000001
#define RT_PTM_STATUS_ABNORMAL_MASK        0x0000001e


/** Setter bit manipulation macro */
#define SET_BIT(addr, shift) ((addr) |= (1u << (shift)))

/** Clearing bit manipulation macro */
#define CLR_BIT(addr, shift) ((addr) &= ~(1u << (shift)))

/** Getter bit manipulation macro */
#define GET_BIT(addr, shift) (bool)(((addr) & (1u << (shift))))

/** Clear-and-Set bit manipulation macro */
#define ASSIGN_BIT(addr, shift, value) \
    (addr = ((addr & ~(1u << (shift))) | (value << (shift))))

/** Set-or-Clear bit manipulation macro */
#define SET_OR_CLEAR_BIT(addr, shift, flag) \
    (flag)? (SET_BIT(addr, shift)): (CLR_BIT(addr, shift))



#define OTP_LCK_RW    0
#define OTP_LCK_RO    1
#define OTP_LCK_NA    2

#define OTP_NOT_ZEROIZED   0
#define OTP_ZEROIZED       1


typedef enum
{
    PUF_UID = 0,
    PUF_OTP1 = 1,
    PUF_OTP2 = 2,
    PUF_RAND = 3,
    PUF_MAX = 4,
} puf_id_t;

/**
 *@brief  Enable OTP TRNG function to generte random number
 *
 *@details  Call this API to enable OTP TRNG hardware control to
 *         prepare output random number.
 *@param fun_en : enable  hardware control prepare output random number
 *@return
 *          STATUS_SUCCESS  TRNG enabled and data ready
 *          STATUS_ERROR    Generator error occurred
 *          STATUS_TIMEOUT  Timeout waiting for FIFO to be cleared
 *          STATUS_EBUSY    Data ready bit not set within allowed retries
 */
uint32_t rt_write_rng_enable(bool fun_en);

/**
 * @brief Count 1's of input num.
 *
 * @detail  For hardware fault tolerant, some OTP memory will use
 *  "multiple bits" to express one bit 1. For example, it is very often
 *  to use 4 bits to express in bit, software will see
 *  1111b/1110b/1101b/1011b/0111b to be 1
 *  so we can use
 *        if (count_ones((value&0xF))>2)
 *              the target bit will be consider 1
 *
 */
uint32_t count_ones(uint32_t num);

uint32_t get_otp_zeroized_state(uint32_t number);

/**
 * @brief Set OTP2 memory cell to read only.
 *
 * @param number : The number in OTP2 memory to become read only.
 *          Notice: this number N will cause OTP2 [(N/32)*32] cell
 *          ~ OTP2[(N/32)*32+31] to become read only.
 *
 *
 * @detail  If system writes a OTP2 memory cell several times with
 *      different value, it will cause data saving in OTP2 cell
 *      become value undertermine/unpredicit. For important data saving
 *      in OTP2 cell, hardware provides lock write setting to
 *      prevent OTP2 cell data from being tampered/damaged.
 *
 *      For example: if system want to protect OTP2 memory cell OTP2_069
 *   then calling set_otp2_lckwd_readonly(69) will also cause
 *   OTP2_064 ~ OTP2_095 (each cell is 4 bytes, total 128 bytes)
 *   to become read only. Because of this setting limitation, we suggest
 *   related OTP2 cell data should be write correct before calling
 *   calling set_otp2_lckwd_readonly(...)
 */

void set_otp2_lckwd_readonly(uint32_t number);

/**
 * @brief Set OTP1 memory cell to read only.
 *
 * @param number : The number in OTP1 memory to become read only.
 *          Notice: this number N will cause OTP1 [N] cell
 *          to become read only. Each OTP1 cell is 4 bytes only.
 *
 * @detail  If system writes a OTP1 memory cell several times with
 *      different value, it will cause data saving in OTP1 cell
 *      become value undertermine/unpredicit. For important data saving
 *      in OTP1 cell, hardware provides lock write setting to
 *      prevent OTP1 cell data from being tampered/damaged.
 *
 *      For example: if system want to protect OTP1 memory cell OTP1_067
 *   then calling set_otp_lckwd_readonly(67). Notice: calling this lock
 *   function only influence the cell "number" (4 bytes).
 *
 */

void set_otp_lckwd_readonly(uint32_t number);

/**
 * @brief Set OTP1 memory cell to be Not access.
 *
 * @param number : The number in OTP1 memory to become not access.
 *          Notice: this number N will cause OTP1 [N] cell
 *          to become not accessed . Each OTP1 cell is 4 bytes only.
 *
 * @detail  If system want to destory a OTP1 memory cell data
 *      from leaking the original value, system can call this function.
 *
 *      For example: if system want to destory OTP1 memory cell OTP1_033
 *   then calling set_otp_lckwd_readonly(33). Notice: calling this
 *   function only influence the cell "number" (4 bytes).
 *   If the cell has been set read-only before by calling function
 *   set_otp_lckwd_readonly, it still can call this function to destory
 *   the data.
 */

void set_otp_lckwd_na(uint32_t number);


/**
 * @brief Get OTP1 memory cell lock state.
 *
 * @param number : The number in OTP1 memory cell
 *
 * @return   OTP_LCK_RW: the cell is write available.
 *           OTP_LCK_RO: the cell is read only mode
 *           OTP_LCK_NA: the cell is not access.
 *
 */

uint32_t get_otp_lckwd_state(uint32_t number);
/**
 * @brief Get OTP2 memory cell lock state.
 *        The OTP2 lock is 128 bytes units
 * @param number : The number in OTP2 memory cell
 *
 * @return   OTP_LCK_RO: the cell is read only mode
 *           OTP_LCK_NA: the cell is not access.
 *
 */
uint32_t get_otp2_lckwd_state(uint32_t number);
/**
 * @brief Set OTP1 location OTP_(x/8) ~ OTP_(x/8+7) to be non-access
 *
 * @param lock_otp_number : The number in OTP1 memory to become NA
 *
 * @details If system want to hide some OTP1 information, it can set the
 *          area to be non-access by using function set_otp_postmasking(N)
 *     For example, if system want to hide OTP1_128 ~ OTP1_135 to be non-access
 *     area, system can call function set_otp_postmasking(128)
 *     then OTP1_128/OTP1_129/.../OTP135 will become 0xFFFFFFFF
 *     after setting set_otp_postmasking(128). This non-access will
 *     be clean after system reset.
 *
 */

void set_otp_postmasking(uint32_t lock_otp_number);

void set_otp_zeroized(uint32_t number);

/**
 * @brief Set register otp_msk[0] (OTP_CFG_S+0x68) and
 *             otp_msk[1] (OTP_CFG_S+0x68) register become Read-only
 *
 * @details set_otp_postmasking function can set OTP memory to be
 *   non-access, but clean these otp_msk registers setting will restore
 *   the related OTP memory value. To avoid this restore,
 *   call set_otp_postmasking_lock(), it will change otp_msk registers
 *   to become Read-only, it can not change the value of otp_msk until
 *   system reset. So the OTP memory that be set non-access by otp_msk
 *   registers can not restore back to original data until next reset.
 *
 */

void set_otp_postmasking_lock(void);

/**
 * @brief  read OTP1/OTP2 memory cell
 *         The each OTP1/OTP2 memory cell is 4 bytes only.
 *
 * @param id : select UID/OPT1/OTP2 address
 *          otp_number : uid/opt1/otp2 memory cell index
 *          *buf : buf point for get uid/opt1/otp2 memory data
 *          length : read uid/opt1/otp2 memory cell index
 * @return  STATUS_INVALID_REQUEST : invalid paramater
 *          STATUS_SUCCESS         : get OTP data success
 *
 */
uint32_t rt_otp_read_data(puf_id_t id, uint32_t otp_number, uint32_t *buf, uint32_t length);

/**
 * @brief write OTP1/OTP2 memory cell data. 4-bytes aligned
 *
 * @param id : select UID/OPT1/OTP2 address
 *        otp_number : opt1/otp2 memory cell index
 *        data : write otp1/otp2 memory celldata (4 bytes)
 * @return   STATUS_SUCCESS: the memory cell is write available.
 *           STATUS_INVALID_REQUEST : the memory cell is lock or number invailable.
 *
 */
uint32_t rt_otp_write_data(puf_id_t id, uint32_t otp_number, uint32_t data);

/**
 * @brief get otp version
 *
 * @param NONE
 *
 *
 * @return   otp version data
 *
 *
 */
uint32_t rt_otp_version(void);

#ifdef __cplusplus
} // closing brace for extern "C"
#endif

#endif /*_RT584_PUFS_RT_REGS_H__*/
