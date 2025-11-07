// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file TestZW_home_id_generator.c
 * @copyright 2022 Silicon Laboratories Inc.
 */
#include "ZW_home_id_generator.h"
#include "mock_control.h"

void setUpSuite(void) {

}

void tearDownSuite(void) {

}

/*
  Test that if HomeIdGeneratorGetNewId input paramter is not null 
  the function will generatr a new home ID and the homeID will match return value
*/
void test_home_id_generator_valid_input(void)
{
  mock_t * pMock;
  uint8_t mHomeId[4];
  uint32_t u32HomeId;
  mock_calls_clear();
  mock_call_expect(TO_STR(zpal_get_random_data), &pMock);
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;  
  uint32_t fakeRandomHomeId = 0xc2234567;
  pMock->output_arg[ARG0].p = (uint8_t*)&fakeRandomHomeId;
  pMock->expect_arg[ARG1].value = 4;
  u32HomeId = HomeIdGeneratorGetNewId(mHomeId);
  /*Test the generated home id match the reutrned  home id arm uses little endian*/
  TEST_ASSERT_EQUAL_UINT8_MESSAGE(((uint8_t*)&u32HomeId)[3], mHomeId[0], "homeID byte 0 don't match" );
  TEST_ASSERT_EQUAL_UINT8_MESSAGE(((uint8_t*)&u32HomeId)[2], mHomeId[1], "homeID byte 1 don't match" ); 
  TEST_ASSERT_EQUAL_UINT8_MESSAGE(((uint8_t*)&u32HomeId)[1], mHomeId[2], "homeID byte 2 don't match" ); 
  TEST_ASSERT_EQUAL_UINT8_MESSAGE(((uint8_t*)&u32HomeId)[0], mHomeId[3], "homeID byte 3 don't match" );     


  TEST_ASSERT_EQUAL_UINT32_MESSAGE(fakeRandomHomeId, u32HomeId, "homeID does not match");

  mock_calls_verify();


}

/*
  Test that if HomeIdGeneratorGetNewId input paramter is null 
  No homeID will be generated and the return value is zero
*/

void test_home_id_generator_invalid_input(void)
{
  uint32_t u32HomeId;
  mock_calls_clear();
  u32HomeId = HomeIdGeneratorGetNewId(NULL);

  TEST_ASSERT_FALSE_MESSAGE(u32HomeId, "homeID is not zero");
  mock_calls_verify();


}
