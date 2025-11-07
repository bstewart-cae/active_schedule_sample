/******************************************************************************
*
* @File         ruci_pci_ook_cmd.h
* @Version
* $Revision: 7513
* $Date: 2025-01-03
* @Brief
* @Note
* Copyright (C) 2025 Rafael Microelectronics Inc. All rights reserved.
*
******************************************************************************/
#ifndef _RUCI_PCI_OOK_CMD_H
#define _RUCI_PCI_OOK_CMD_H

#include "ruci_head.h"

#if (RUCI_ENABLE_PCI)

/******************************************************************************
* DEFINES
*****************************************************************************/

#pragma pack(push)
#pragma pack(1)
#define RUCI_PCI_OOK_CMD_HEADER 0x19

// RUCI: initiate_ook ----------------------------------------------------------
#define RUCI_INITIATE_OOK                       RUCI_NUM_INITIATE_OOK, ruci_elmt_type_initiate_ook, ruci_elmt_num_initiate_ook
#define RUCI_CODE_INITIATE_OOK                  0x01
#define RUCI_LEN_INITIATE_OOK                   4
#define RUCI_NUM_INITIATE_OOK                   4
#define RUCI_PARA_LEN_INITIATE_OOK              1
#if (RUCI_ENDIAN_INVERSE)
extern const uint8_t ruci_elmt_type_initiate_ook[];
extern const uint8_t ruci_elmt_num_initiate_ook[];
#endif /* RUCI_ENDIAN_INVERSE */
typedef struct ruci_para_initiate_ook_s
{
    ruci_head_t     ruci_header;
    uint8_t         sub_header;
    uint8_t         length;
    uint8_t         band_type;
} ruci_para_initiate_ook_t;

/* User should provide msg buffer is greater than sizeof(ruci_para_initiate_ook_t) */
#define SET_RUCI_PARA_INITIATE_OOK(msg, band_type_in)        \
        do{                                                                                                            \
        ((ruci_para_initiate_ook_t *)msg)->ruci_header.u8                 = RUCI_PCI_OOK_CMD_HEADER;                \
        ((ruci_para_initiate_ook_t *)msg)->sub_header                     = RUCI_CODE_INITIATE_OOK;                 \
        ((ruci_para_initiate_ook_t *)msg)->length                         = RUCI_PARA_LEN_INITIATE_OOK;             \
        ((ruci_para_initiate_ook_t *)msg)->band_type                      = band_type_in;                           \
        }while(0)

// RUCI: set_ook_modem ---------------------------------------------------------
#define RUCI_SET_OOK_MODEM                      RUCI_NUM_SET_OOK_MODEM, ruci_elmt_type_set_ook_modem, ruci_elmt_num_set_ook_modem
#define RUCI_CODE_SET_OOK_MODEM                 0x02
#define RUCI_LEN_SET_OOK_MODEM                  7
#define RUCI_NUM_SET_OOK_MODEM                  4
#define RUCI_PARA_LEN_SET_OOK_MODEM             4
#if (RUCI_ENDIAN_INVERSE)
extern const uint8_t ruci_elmt_type_set_ook_modem[];
extern const uint8_t ruci_elmt_num_set_ook_modem[];
#endif /* RUCI_ENDIAN_INVERSE */
typedef struct ruci_para_set_ook_modem_s
{
    ruci_head_t     ruci_header;
    uint8_t         sub_header;
    uint8_t         length;
    uint32_t        bandwidth;
} ruci_para_set_ook_modem_t;

/* User should provide msg buffer is greater than sizeof(ruci_para_set_ook_modem_t) */
#define SET_RUCI_PARA_SET_OOK_MODEM(msg, bandwidth_in)        \
        do{                                                                                                            \
        ((ruci_para_set_ook_modem_t *)msg)->ruci_header.u8                 = RUCI_PCI_OOK_CMD_HEADER;                \
        ((ruci_para_set_ook_modem_t *)msg)->sub_header                     = RUCI_CODE_SET_OOK_MODEM;                \
        ((ruci_para_set_ook_modem_t *)msg)->length                         = RUCI_PARA_LEN_SET_OOK_MODEM;            \
        ((ruci_para_set_ook_modem_t *)msg)->bandwidth                      = bandwidth_in;                           \
        }while(0)

#pragma pack(pop)
#endif /* RUCI_ENABLE_PCI */
#endif /* _RUCI_PCI_OOK_CMD_H */
