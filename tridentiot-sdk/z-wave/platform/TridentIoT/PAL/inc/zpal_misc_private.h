/// ****************************************************************************
/// @file zpal_misc_private.h
///
/// @brief Platform specific zpal functions
///
///
/// SPDX-License-Identifier: LicenseRef-TridentMSLA
/// SPDX-FileCopyrightText: 2025 Trident IoT, LLC <https://www.tridentiot.com>
/// ***************************************************************************
#ifndef ZPAL_MISC_PRIVATE_H_
#define ZPAL_MISC_PRIVATE_H_

/**
 * @addtogroup zpal
 * @brief
 * Z-Wave miscellaneous private zpal functions
 * @{
 * @addtogroup zpal-misc
 * @brief
 * Platform specific misc API
 * @{
 */


#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define RESTART_COUNTER_SIZE        4

/**
 * @brief Get the persisted restart counter.
 * This function gets the restart counter from retention RAM.

 * @return The number of restarts since the last call to zpal_clear_restarts()
 *
 */
uint32_t zpal_get_restarts(void);

/**
 * @brief Clear the persisted restart counter.
 * This function clears the restart counter in retention RAM.
 * The restart counter is never zero initialized so it is necessary to clear
 * it manually to get the number of restarts performed over a period of time.
 */
void zpal_clear_restarts(void);

/**
 * @brief Increase the persisted restart counter.
 * This function increases the restart counter in retention RAM.
 */
void zpal_increase_restarts(void);

/**
 * @brief Erase a block of FLASH
 * This function erases a block of FLASH. Minumum size is 4KB.
 */
void zpal_block_flash_erase(uint32_t flash_addr, uint32_t image_size);


/**
 * @brief Perform a soft reset
 * This function will perform a soft reset and set a magic word in retention
 * RAM to signal that the soft reset was performed by the firmware
 */
void zpal_reset_soft(void);

/**
 * @brief Check if the magic work for internal soft reset is correct
 *
 * @return True - The reset was triggered by firmware
 */
bool zpal_was_internal_soft_reset(void);

#ifdef __cplusplus
}
#endif

#endif /* ZPAL_MISC_PRIVATE_H_ */
