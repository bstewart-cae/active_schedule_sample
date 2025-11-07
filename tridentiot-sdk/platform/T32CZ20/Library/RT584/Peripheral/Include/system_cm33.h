/******************************************************************************
 * @file     system_cm33.h
 * @version
 * @brief    system initialization header file
 *
 * @copyright
 ******************************************************************************/


#ifndef _RT584_SYSTEM_CM33_H_
#define _RT584_SYSTEM_CM33_H_

#ifdef __cplusplus
extern "C" {
#endif

/**************************************************************************************************
 *    INCLUDES
 *************************************************************************************************/
#include <stdint.h>
#include "cm33.h"

/**************************************************************************************************
 *    CONSTANTS AND DEFINES
 *************************************************************************************************/
/**
 *  @Brief Exception / Interrupt Handler Function Prototype
 */
#define PMU_LDO_MODE     0
#define PMU_DCDC_MODE    1

#ifndef SET_PMU_MODE
#define SET_PMU_MODE    PMU_DCDC_MODE
#endif
/**************************************************************************************************
 *    TYPEDEFS
 *************************************************************************************************/
/**
 *  @Brief Exception / Interrupt Handler Function Prototype
 */
typedef void(*VECTOR_TABLE_Type)(void);

/**************************************************************************************************
 *    GLOBAL PROTOTYPES
 *************************************************************************************************/
/**
 *  @Brief Processor Clock Frequency
 */
extern uint32_t SystemCoreClock;
extern uint32_t SystemFrequency;  /* System Core Clock Frequency */

/**
 * Bootloader initialize the system
 *
 * @param  none
 * @return none
 *
 * @brief  Setup the microcontroller system.
 *         Initialize the System and update the SystemCoreClock variable.
 */
void SystemInit_Bootloader (void);

/**
 * Initialize the system
 *
 * @param  none
 * @return none
 *
 * @brief  Setup the microcontroller system.
 *         Initialize the System and update the SystemCoreClock variable.
 */
void SystemInit(void);

/**
 * Update SystemCoreClock variable
 *
 * @param  none
 * @return none
 *
 * @brief  Updates the SystemCoreClock with current core Clock
 *         retrieved from cpu registers.
 */
void SystemCoreClockUpdate(void);


#ifdef __cplusplus
}
#endif

#endif /* end of _SYSTEM_RT584_CM33_MCU_H_ */
