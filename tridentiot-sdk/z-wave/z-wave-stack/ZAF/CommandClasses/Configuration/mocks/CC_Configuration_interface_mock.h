/***************************************************************************//**
 * @file CC_Configuration_interface_mock.h
 * @brief CC_Configuration_interface_mock.h
 * @copyright 2020 Silicon Laboratories Inc.
 *******************************************************************************
 * # License
 * <b>Copyright 2020 Silicon Laboratories Inc. www.silabs.com</b>
 *******************************************************************************
 *
 * The licensor of this software is Silicon Laboratories Inc. Your use of this
 * software is governed by the terms of Silicon Labs Master Software License
 * Agreement (MSLA) available at
 * www.silabs.com/about-us/legal/master-software-license-agreement. This
 * software is distributed to you in Source Code format and is governed by the
 * sections of the MSLA applicable to Source Code.
 *
 ******************************************************************************/
#ifndef CONFIGURATION_INTERFACE_H
#define CONFIGURATION_INTERFACE_H
// -----------------------------------------------------------------------------
//                                   Includes
// -----------------------------------------------------------------------------
#include <stddef.h>
#include <stdbool.h>
// -----------------------------------------------------------------------------
//                              Macros and Typedefs
// -----------------------------------------------------------------------------
typedef enum {
  configuration_index_dummy_limit_0 = 1,
  configuration_index_dummy_limit_1,
  configuration_index_count
} configuration_index;
// -----------------------------------------------------------------------------
//                                Global Variables
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
//                          Public Function Declarations
// -----------------------------------------------------------------------------

cc_configuration_t const*
configuration_get_default_config_table(void);

cc_configuration_io_interface_t const*
configuration_get_interface_reference(void);

void
configuration_mock_empty_fake_files(void);

void
cc_configuration_fill_fake_files_with_dummy_data(void);

#endif  // CONFIGURATION_INTERFACE_H
