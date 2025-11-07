// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file TestZW_lr_virtual_node_id_controller.c
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


void test_ZW_LR_EnableVirtualIDs(void)
{
  enableVirtualIDTestParams_t testCaseDefinitions[] =
  {
  // Testcase     LR virtual    Expected result 1          Expected result 2          Expected result 3             Expected result 4          Expected result 5           Expected result 5
  // name:        ID bitmask:   

    //Verify that LR virtual IDs are disabled by default.
    {"TC 01",    0x00,          LR_VIRTUAL_ID_INVALID,     LR_VIRTUAL_ID_DISABLED,    LR_VIRTUAL_ID_DISABLED,       LR_VIRTUAL_ID_DISABLED,     LR_VIRTUAL_ID_DISABLED,    LR_VIRTUAL_ID_INVALID},
    //Enable virtual node ID 4002 (bitmask 0x01)
    {"TC 02",    0x01,          LR_VIRTUAL_ID_INVALID,     LR_VIRTUAL_ID_ENABLED,     LR_VIRTUAL_ID_DISABLED ,      LR_VIRTUAL_ID_DISABLED,     LR_VIRTUAL_ID_DISABLED,    LR_VIRTUAL_ID_INVALID},
    //Enable virtual node ID 4002 and 4003 (bitmask 0x03)  
    {"TC 03",    0x03,          LR_VIRTUAL_ID_INVALID,     LR_VIRTUAL_ID_ENABLED,     LR_VIRTUAL_ID_ENABLED,        LR_VIRTUAL_ID_DISABLED,     LR_VIRTUAL_ID_DISABLED,    LR_VIRTUAL_ID_INVALID},
    //Enable virtual node ID 4002, 4003 and 4004 (bitmask 0x07)  
    {"TC 04",    0x07,          LR_VIRTUAL_ID_INVALID,     LR_VIRTUAL_ID_ENABLED,     LR_VIRTUAL_ID_ENABLED,        LR_VIRTUAL_ID_ENABLED,      LR_VIRTUAL_ID_DISABLED,    LR_VIRTUAL_ID_INVALID},
    //Enable virtual node ID 4002, 4003 ,4004 and 4005 (bitmask 0x0F)
    {"TC 05",    0x0F,          LR_VIRTUAL_ID_INVALID,     LR_VIRTUAL_ID_ENABLED,     LR_VIRTUAL_ID_ENABLED,        LR_VIRTUAL_ID_ENABLED,      LR_VIRTUAL_ID_ENABLED,     LR_VIRTUAL_ID_INVALID},
    //Disable all virtual node IDs (bitmask 0x00)  
    {"TC 06",    0x00,          LR_VIRTUAL_ID_INVALID,     LR_VIRTUAL_ID_DISABLED,    LR_VIRTUAL_ID_DISABLED,       LR_VIRTUAL_ID_DISABLED,     LR_VIRTUAL_ID_DISABLED,    LR_VIRTUAL_ID_INVALID},
    //Enable virtual node ID 4003 (bitmask 0x02)
    {"TC 07",    0x02,          LR_VIRTUAL_ID_INVALID,     LR_VIRTUAL_ID_DISABLED,    LR_VIRTUAL_ID_ENABLED,        LR_VIRTUAL_ID_DISABLED,     LR_VIRTUAL_ID_DISABLED,    LR_VIRTUAL_ID_INVALID},
    //Enable virtual node ID 4004 (bitmask 0x04)
    {"TC 08",    0x04,          LR_VIRTUAL_ID_INVALID,     LR_VIRTUAL_ID_DISABLED,    LR_VIRTUAL_ID_DISABLED,       LR_VIRTUAL_ID_ENABLED,       LR_VIRTUAL_ID_DISABLED,    LR_VIRTUAL_ID_INVALID},
    //Enable virtual node ID 4005 (bitmask 0x08)
    {"TC 09",    0x08,          LR_VIRTUAL_ID_INVALID,     LR_VIRTUAL_ID_DISABLED,    LR_VIRTUAL_ID_DISABLED,       LR_VIRTUAL_ID_DISABLED,      LR_VIRTUAL_ID_ENABLED,    LR_VIRTUAL_ID_INVALID}
  };

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

typedef struct  virtualIDFilterTestParams
{
  const char*     testCaseName;
  uint8_t         nodeIDsBitmask;
  uint8_t         cmdClass;
  uint16_t        nodeID;
  bool         expectedResult;
} virtualIDFilterTestParams_t;



/*Test LR virtual node id filtering in controllers and slaves*/
void test_ZW_LR_VirtualIDs_filters(void)
{
  virtualIDFilterTestParams_t testCaseDefinitions[] =
  {
   // Testcase     LR virtual    input cmd                   input node ID          Expected result
   // name:        ID bitmask:   class:
   // test a protocol frame with normal id
    {"TC 01", 0x00, ZWAVE_CMD_CLASS_PROTOCOL, 512, false},
    // test LR protocol frame with normal id
    {"TC 02", 0x00, ZWAVE_CMD_CLASS_PROTOCOL_LR, 720, false},
    // test a protocol frame with disabled LR virtual id
    {"TC 03", 0x00, ZWAVE_CMD_CLASS_PROTOCOL, 4002, true},
    // test a protocol frame with enabled LR virtual id
    {"TC 04", 0x04, ZWAVE_CMD_CLASS_PROTOCOL, 4004, true},
    // test a LR protocol frame with disabled LR virtual id
    {"TC 05", 0x00, ZWAVE_CMD_CLASS_PROTOCOL_LR, 4002, true},
    // test a LR protocol frame with enabled LR virtual id
    {"TC 06", 0x01, ZWAVE_CMD_CLASS_PROTOCOL_LR, 4002, true},
    // test an appication frame with normal destination id
    {"TC 07", 0x01, (ZWAVE_CMD_CLASS_APPL_MIN + 5), 256, false},
    // test an appication frame with disabled LR virtual id
    {"TC 08", 0x01, (ZWAVE_CMD_CLASS_APPL_MIN + 9), 4004, true},
    // test an appication frame with enabled LR virtual id
    {"TC 09", 0x01, (ZWAVE_CMD_CLASS_APPL_MIN + 9), 4002, false},
   };
  bool mDropFrame = false;
  for (uint8_t i = 0; i < sizeof(testCaseDefinitions) / sizeof(virtualIDFilterTestParams_t); i++)
  {
    ZW_LR_EnableVirtualIDs(testCaseDefinitions[i].nodeIDsBitmask);
    mDropFrame = ZW_Drop_frame_with_lr_virtual_id(testCaseDefinitions[i].cmdClass, testCaseDefinitions[i].nodeID);
    TEST_ASSERT_EQUAL_MESSAGE(testCaseDefinitions[i].expectedResult, mDropFrame, testCaseDefinitions[i].testCaseName);
  }
}
