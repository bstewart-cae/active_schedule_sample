// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file TestZW_lr_virtual_node_id_slave.c
 * @copyright 2022 Silicon Laboratories Inc.
 */
#include <stdio.h>
#include "ZW_lib_defines.h"
#include "ZW_transport.h"
#include "ZW_lr_virtual_node_id.h"
#include "unity.h"

void setUpSuite(void) {

}

void tearDownSuite(void) {

}

void setUp(void) {

}

void tearDown(void) {

}

typedef struct  enableVirtualIDTestParams
{
  const char*     testCaseName;
  uint8_t         nodeIDsBitmask;
  LR_VIRTUAL_ID_TYPE         expectedResult1;
  LR_VIRTUAL_ID_TYPE         expectedResult2;
  LR_VIRTUAL_ID_TYPE         expectedResult3;
  LR_VIRTUAL_ID_TYPE         expectedResult4;
  LR_VIRTUAL_ID_TYPE         expectedResult5;
  LR_VIRTUAL_ID_TYPE         expectedResult6;
} enableVirtualIDTestParams_t;

enableVirtualIDTestParams_t testCaseDefinitions[] =
{
  // Testcase     LR virtual    Expected result 1          Expected result 2          Expected result 3             Expected result 4          Expected result 5           Expected result 5
  // name:        ID bitmask:   

  //Verify that LR virtual IDs are disabled by default.
  {"TC 01",    0x00,          LR_VIRTUAL_ID_INVALID,     LR_VIRTUAL_ID_ENABLED,    LR_VIRTUAL_ID_ENABLED,       LR_VIRTUAL_ID_ENABLED,     LR_VIRTUAL_ID_ENABLED,    LR_VIRTUAL_ID_INVALID},
  //Enable virtual node ID 4002 (bitmask 0x01)
  {"TC 02",    0x01,          LR_VIRTUAL_ID_INVALID,     LR_VIRTUAL_ID_ENABLED,    LR_VIRTUAL_ID_ENABLED,       LR_VIRTUAL_ID_ENABLED,     LR_VIRTUAL_ID_ENABLED,    LR_VIRTUAL_ID_INVALID},
  //Enable virtual node ID 4002 and 4003 (bitmask 0x03)  
  {"TC 03",    0x03,          LR_VIRTUAL_ID_INVALID,     LR_VIRTUAL_ID_ENABLED,    LR_VIRTUAL_ID_ENABLED,       LR_VIRTUAL_ID_ENABLED,     LR_VIRTUAL_ID_ENABLED,    LR_VIRTUAL_ID_INVALID},
  //Enable virtual node ID 4002, 4003 and 4004 (bitmask 0x07)  
  {"TC 04",    0x07,          LR_VIRTUAL_ID_INVALID,     LR_VIRTUAL_ID_ENABLED,    LR_VIRTUAL_ID_ENABLED,       LR_VIRTUAL_ID_ENABLED,     LR_VIRTUAL_ID_ENABLED,    LR_VIRTUAL_ID_INVALID},
  //Enable virtual node ID 4002, 4003 ,4004 and 4005 (bitmask 0x0F)
  {"TC 05",    0x07,          LR_VIRTUAL_ID_INVALID,     LR_VIRTUAL_ID_ENABLED,    LR_VIRTUAL_ID_ENABLED,       LR_VIRTUAL_ID_ENABLED,     LR_VIRTUAL_ID_ENABLED,    LR_VIRTUAL_ID_INVALID},
  //Disable all virtual node IDs (bitmask 0x00)  
  {"TC 06",    0x00,          LR_VIRTUAL_ID_INVALID,     LR_VIRTUAL_ID_ENABLED,    LR_VIRTUAL_ID_ENABLED,       LR_VIRTUAL_ID_ENABLED,     LR_VIRTUAL_ID_ENABLED,    LR_VIRTUAL_ID_INVALID},
  //Enable virtual node ID 4003 (bitmask 0x02)
  {"TC 07",    0x02,          LR_VIRTUAL_ID_INVALID,     LR_VIRTUAL_ID_ENABLED,    LR_VIRTUAL_ID_ENABLED,       LR_VIRTUAL_ID_ENABLED,     LR_VIRTUAL_ID_ENABLED,    LR_VIRTUAL_ID_INVALID},
  //Enable virtual node ID 4004 (bitmask 0x04)
  {"TC 08",    0x04,          LR_VIRTUAL_ID_INVALID,     LR_VIRTUAL_ID_ENABLED,    LR_VIRTUAL_ID_ENABLED,       LR_VIRTUAL_ID_ENABLED,     LR_VIRTUAL_ID_ENABLED,    LR_VIRTUAL_ID_INVALID},
  //Enable virtual node ID 4005 (bitmask 0x08)
  {"TC 09",    0x04,          LR_VIRTUAL_ID_INVALID,     LR_VIRTUAL_ID_ENABLED,    LR_VIRTUAL_ID_ENABLED,       LR_VIRTUAL_ID_ENABLED,     LR_VIRTUAL_ID_ENABLED,    LR_VIRTUAL_ID_INVALID},
};

void test_ZW_LR_EnableVirtualIDs(void)
{
  LR_VIRTUAL_ID_TYPE mLRIdType = LR_VIRTUAL_ID_INVALID;
  char str[20];
  //Verify that LR virtual ID is allways ENABLED in slaves
  for (uint8_t i = 0; i < sizeof(testCaseDefinitions)/sizeof(enableVirtualIDTestParams_t); i++)
  {
    ZW_LR_EnableVirtualIDs(testCaseDefinitions[i].nodeIDsBitmask);
    mLRIdType = ZW_Get_lr_virtual_id_type(4001);
    sprintf(str, "%s node: 4001", testCaseDefinitions[i].testCaseName);
    TEST_ASSERT_EQUAL_MESSAGE(testCaseDefinitions[i].expectedResult1, mLRIdType, str);
    mLRIdType = ZW_Get_lr_virtual_id_type(4002);
    sprintf(str, "%s node: 4002", testCaseDefinitions[i].testCaseName);
    TEST_ASSERT_EQUAL_MESSAGE(testCaseDefinitions[i].expectedResult2, mLRIdType, str);
    mLRIdType = ZW_Get_lr_virtual_id_type(4003);
    sprintf(str, "%s node: 4003", testCaseDefinitions[i].testCaseName);
    TEST_ASSERT_EQUAL_MESSAGE(testCaseDefinitions[i].expectedResult3, mLRIdType, str);
    mLRIdType = ZW_Get_lr_virtual_id_type(4004);
    sprintf(str, "%s node: 4004", testCaseDefinitions[i].testCaseName);
    TEST_ASSERT_EQUAL_MESSAGE(testCaseDefinitions[i].expectedResult4, mLRIdType, str);
    mLRIdType = ZW_Get_lr_virtual_id_type(4005);
    sprintf(str, "%s node: 4005", testCaseDefinitions[i].testCaseName);
    TEST_ASSERT_EQUAL_MESSAGE(testCaseDefinitions[i].expectedResult5, mLRIdType, str);
    mLRIdType = ZW_Get_lr_virtual_id_type(4006);
    sprintf(str, "%s node: 4006", testCaseDefinitions[i].testCaseName);
    TEST_ASSERT_EQUAL_MESSAGE(testCaseDefinitions[i].expectedResult6, mLRIdType, str);
  }
}
