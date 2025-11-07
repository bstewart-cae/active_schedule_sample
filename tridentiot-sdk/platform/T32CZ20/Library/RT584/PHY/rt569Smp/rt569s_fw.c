/**************************************************************************//**
* @file     rt569mp_fw.c
* @version
* @brief    RF MCU FW initialization

******************************************************************************/

#include "cm33.h"
#include "chip_define.h"
#include "rf_mcu.h"
#include "rf_mcu_chip.h"
#include "rf_common_init.h"


/**************************************************************************************************
 *    GLOBAL VARIABLES
 *************************************************************************************************/
#if (RF_MCU_CHIP_MODEL == RF_MCU_CHIP_569S)
/* Use top level CHIP_VERSION */


#if (RF_MCU_CHIP_TYPE == RF_MCU_TYPE_ASIC)
#if (CHIP_VERSION == RT58X_SHUTTLE)
/* RADIO FW FOR SHUTTLE CHIP */

#if (RF_FW_INCLUDE_PCI == TRUE)
#include "prg_rt584z_shuttle_asic_pci_fw.h"
#endif

#if (RF_FW_INCLUDE_BLE == TRUE)
#error "rt584z does not support BLE"
#endif

#if (RF_FW_INCLUDE_MULTI_2P4G == TRUE)
#error "rt584z does not support 2.4G multi-protocol"
#endif

#elif (CHIP_VERSION == RT58X_MPA)
/* RADIO FW FOR MPA CHIP */

#if (RF_FW_INCLUDE_PCI == TRUE)
#include "prg_rt584z_mpa_asic_pci_fw.h"
#endif

#if (RF_FW_INCLUDE_BLE == TRUE)
#error "rt584z does not support BLE"
#endif

#if (RF_FW_INCLUDE_MULTI_2P4G == TRUE)
#error "rt584z does not support 2.4G multi-protocol"
#endif

#elif (CHIP_VERSION == RT58X_MPB)
/* RADIO FW FOR MPB CHIP */

#if (RF_FW_INCLUDE_PCI == TRUE)
#include "prg_rt584z_mpa_asic_pci_fw.h"
#endif

#if (RF_FW_INCLUDE_BLE == TRUE)
#error "rt584z does not support BLE"
#endif

#if (RF_FW_INCLUDE_MULTI_2P4G == TRUE)
#error "rt584z does not support 2.4G multi-protocol"
#endif

#else
#error "Selected CHIP_VERSION is not supported!"
#endif

#elif (RF_MCU_CHIP_TYPE == RF_MCU_TYPE_FPGA_RFT3)
/* RADIO FW FOR FPGA */

#if (RF_FW_INCLUDE_PCI == TRUE)
#include "prg_rt584z_mpa_fpga_t3_pci_fw.h"
#endif

#if (RF_FW_INCLUDE_BLE == TRUE)
#error "rt584z does not support BLE"
#endif

#if (RF_FW_INCLUDE_MULTI_2P4G == TRUE)
#error "rt584z does not support 2.4G multi-protocol"
#endif
#endif /* RF_MCU_CHIP_TYPE */

#if (RF_FW_INCLUDE_PCI == TRUE)
const uint32_t firmware_size_ruci = sizeof(firmware_program_ruci);
#else
const uint8_t firmware_program_ruci[] = {0};
const uint32_t firmware_size_ruci = 0;
#endif

#if (RF_FW_INCLUDE_BLE == TRUE)
const uint32_t firmware_size_ble = sizeof(firmware_program_ble);
#else
const uint8_t firmware_program_ble[] = {0};
const uint32_t firmware_size_ble = 0;
#endif

#if (RF_FW_INCLUDE_MULTI_2P4G == TRUE)
const uint32_t firmware_size_multi = sizeof(firmware_program_multi);
#else
const uint8_t firmware_program_multi[] = {0};
const uint32_t firmware_size_multi = 0;
#endif

/* COMMON RADIO FW FOR INTERNAL TEST */

#if (RF_CAL_TYPE & RF_CAL_PWR_ON_MODE)
#if (RF_FW_INCLUDE_PCI == TRUE)
const uint8_t *firmware_program_rfk = firmware_program_ruci;
const uint32_t firmware_size_rfk = sizeof(firmware_program_ruci);
#else
#error "RF_FW_INCLUDE_PCI is the prerequisite for RF_CAL_PWR_ON_MODE"
#endif
#else
const uint8_t firmware_program_rfk[] = {0};
const uint32_t firmware_size_rfk = 0;
#endif /* (RF_CAL_TYPE & RF_CAL_PWR_ON_MODE) */

#if (RF_FW_INCLUDE_INTERNAL_TEST == TRUE)
const uint8_t firmware_program_it[] = {0};
const uint32_t firmware_size_it = 0;
#if (RF_MCU_CONST_LOAD_SUPPORTED)
const uint8_t firmware_const_it[] = {0};
const uint32_t const_size_it = 0;
#endif
#endif /* (RF_FW_INCLUDE_INTERNAL_TEST == TRUE) */

#endif /* (RF_MCU_CHIP_MODEL == RF_MCU_CHIP_569S) */

