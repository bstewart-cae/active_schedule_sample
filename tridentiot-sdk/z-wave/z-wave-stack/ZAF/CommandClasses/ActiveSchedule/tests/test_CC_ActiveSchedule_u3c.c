/*
 * SPDX-License-Identifier: BSD-3-Clause
 * SPDX-FileCopyrightText: 2025 Z-Wave Alliance <https://z-wavealliance.org>
 * SPDX-FileCopyrightText: 2025 Card Access Engineering, LLC. <https://www.caengineering.com>
 */

#include <string.h>
#include <unity.h>
#include "test_common.h"
#include "ZW_classcmd.h"
#include "ZAF_CC_Invoker.h"
#include "ZAF_types.h"
#include "zaf_transport_tx_mock.h"
#include "ZAF_TSE_mock.h"
#include "cc_user_credential_config_api_mock.h"
#include "cc_active_schedule_config_api_mock.h"
#include "cc_active_schedule_io_mock.h"

// FIXME: Implement when integration with U3C Door Lock is in place
#if 0

#define test_cc1 COMMAND_CLASS_USER_CREDENTIAL_V2
#define test_cc2 COMMAND_CLASS_DOOR_LOCK_V4

/* Define callback constants */

#define MAX_NUM_SUPPORTED_YEAR_DAY 3
#define MAX_NUM_SUPPORTED_DAILY_REPEATING 57
#define NUM_SUPPORTED_CC 1

/* Define mock callbacks */
static uint16_t tstubs1_get_schedule_count(const ascc_type_t schedule_type)
{
  if(schedule_type == ASCC_TYPE_YEAR_DAY) {
    return MAX_NUM_SUPPORTED_YEAR_DAY;
  } else if (schedule_type == ASCC_TYPE_DAILY_REPEATING) {
    return MAX_NUM_SUPPORTED_DAILY_REPEATING;
  }
}

static uint16_t tstubs1_get_num_target_cc(void) {
  return 4;
}

static bool tstubs1_schedule_slot_validation(const uint16_t target_ID,
                                      const ascc_type_t type,
                                      const uint16_t slot)
{
  return false;
}

static ascc_op_result_t tstubs1_get_schedule_data(const ascc_type_t schedule_type,
                                                  const uint16_t slot,
                                                  const ascc_target_t * const target,
                                                  ascc_schedule_data_t * schedule,
                                                  uint16_t * next_slot)
{

  ascc_op_result_t result = {
    .result = ASCC_OPERATION_SUCCESS,
    .working_time = 0
  };
  return result;
}

static ascc_op_result_t tstubs1_set_schedule_data(const ascc_op_type_t operation,
                                                  const ascc_schedule_t * const schedule)
{
  ascc_op_result_t result = {
    .result = ASCC_OPERATION_WORKING,
    .working_time = 60
  };
  return result;
}

static ascc_op_result_t tstubs1_set_schedule_state(const ascc_target_t * const target,
                                                           const bool state)
{
  ascc_op_result_t result = {
    .result = ASCC_OPERATION_WORKING,
    .working_time = 60
  };
  return result;
}

static ascc_op_result_t tstubs1_get_schedule_state(const ascc_target_t * const target,
                                            bool * state)
{
  *state = false;
  ascc_op_result_t result = {
    .result = ASCC_OPERATION_WORKING,
    .working_time = 60
  };
  return result;
}

static ascc_op_result_t tstubs1_target_validation(const ascc_target_t * const target)
{
    ascc_op_result_t result = {
    .result = ASCC_OPERATION_WORKING,
    .working_time = 60
  };
  return result;
}

static bool tstubs1_schedule_data_validation(const ascc_schedule_t * const schedule)
{
  return false;
}

/* DEFINE ACTIVE SCHEDULE STUBS */
static ascc_target_stubs_t target_stubs1 = {
  .get_schedule_count = tstubs1_get_schedule_count,
  .get_target_count = tstubs1_get_num_target_cc,
  .get_schedule_data = tstubs1_get_schedule_data,
  .get_schedule_state = tstubs1_get_schedule_state,
  .set_schedule_data = tstubs1_set_schedule_data,
  .set_schedule_state = tstubs1_set_schedule_state,
  .validate_target = tstubs1_target_validation,
  .validate_schedule_slot = tstubs1_schedule_slot_validation,
  .validate_schedule_data = tstubs1_schedule_data_validation
 };
static ascc_target_stubs_t target_stubs2 = { 0 };
#endif /* FIXME */

void setUpSuite(void)
{
  // run initialization of test framework for this call
  cc_active_schedule_config_api_mock_Init();
  cc_active_schedule_io_mock_Init();
}

void tearDownSuite(void)
{
}

void setUp(void)
{
}

void tearDown(void)
{
}

void test_ACTIVE_SCHEDULE_test_callback_assignment(void)
{
}
