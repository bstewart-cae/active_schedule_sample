/******************************************************************************
*
* @File         ruci_cmn_sys_cmd.h
* @Version
* $Revision: 7513
* $Date: 2025-01-03
* @Brief
* @Note
* Copyright (C) 2025 Rafael Microelectronics Inc. All rights reserved.
*
******************************************************************************/
#ifndef _RUCI_CMN_SYS_CMD_H
#define _RUCI_CMN_SYS_CMD_H

#include "ruci_head.h"

#if (RUCI_ENABLE_CMN)

/******************************************************************************
* DEFINES
*****************************************************************************/

#pragma pack(push)
#pragma pack(1)
#define RUCI_CMN_SYS_CMD_HEADER 0x30

// RUCI: get_fw_ver ------------------------------------------------------------
#define RUCI_GET_FW_VER                         RUCI_NUM_GET_FW_VER, ruci_elmt_type_get_fw_ver, ruci_elmt_num_get_fw_ver
#define RUCI_CODE_GET_FW_VER                    0x01
#define RUCI_LEN_GET_FW_VER                     3
#define RUCI_NUM_GET_FW_VER                     3
#define RUCI_PARA_LEN_GET_FW_VER                0
#if (RUCI_ENDIAN_INVERSE)
extern const uint8_t ruci_elmt_type_get_fw_ver[];
extern const uint8_t ruci_elmt_num_get_fw_ver[];
#endif /* RUCI_ENDIAN_INVERSE */
typedef struct ruci_para_get_fw_ver_s
{
    ruci_head_t     ruci_header;
    uint8_t         sub_header;
    uint8_t         length;
} ruci_para_get_fw_ver_t;

/* User should provide msg buffer is greater than sizeof(ruci_para_get_fw_ver_t) */
#define SET_RUCI_PARA_GET_FW_VER(msg)        \
        do{                                                                                                            \
        ((ruci_para_get_fw_ver_t *)msg)->ruci_header.u8                 = RUCI_CMN_SYS_CMD_HEADER;                \
        ((ruci_para_get_fw_ver_t *)msg)->sub_header                     = RUCI_CODE_GET_FW_VER;                   \
        ((ruci_para_get_fw_ver_t *)msg)->length                         = RUCI_PARA_LEN_GET_FW_VER;               \
        }while(0)

// RUCI: set_pta_default -------------------------------------------------------
#define RUCI_SET_PTA_DEFAULT                    RUCI_NUM_SET_PTA_DEFAULT, ruci_elmt_type_set_pta_default, ruci_elmt_num_set_pta_default
#define RUCI_CODE_SET_PTA_DEFAULT               0x02
#define RUCI_LEN_SET_PTA_DEFAULT                5
#define RUCI_NUM_SET_PTA_DEFAULT                5
#define RUCI_PARA_LEN_SET_PTA_DEFAULT           2
#if (RUCI_ENDIAN_INVERSE)
extern const uint8_t ruci_elmt_type_set_pta_default[];
extern const uint8_t ruci_elmt_num_set_pta_default[];
#endif /* RUCI_ENDIAN_INVERSE */
typedef struct ruci_para_set_pta_default_s
{
    ruci_head_t     ruci_header;
    uint8_t         sub_header;
    uint8_t         length;
    uint8_t         enable_flag;
    uint8_t         inverse_ctrl;
} ruci_para_set_pta_default_t;

/* User should provide msg buffer is greater than sizeof(ruci_para_set_pta_default_t) */
#define SET_RUCI_PARA_SET_PTA_DEFAULT(msg, enable_flag_in, inverse_ctrl_in)        \
        do{                                                                                                            \
        ((ruci_para_set_pta_default_t *)msg)->ruci_header.u8                 = RUCI_CMN_SYS_CMD_HEADER;                \
        ((ruci_para_set_pta_default_t *)msg)->sub_header                     = RUCI_CODE_SET_PTA_DEFAULT;              \
        ((ruci_para_set_pta_default_t *)msg)->length                         = RUCI_PARA_LEN_SET_PTA_DEFAULT;          \
        ((ruci_para_set_pta_default_t *)msg)->enable_flag                    = enable_flag_in;                         \
        ((ruci_para_set_pta_default_t *)msg)->inverse_ctrl                   = inverse_ctrl_in;                        \
        }while(0)

#pragma pack(pop)
#endif /* RUCI_ENABLE_CMN */
#endif /* _RUCI_CMN_SYS_CMD_H */
