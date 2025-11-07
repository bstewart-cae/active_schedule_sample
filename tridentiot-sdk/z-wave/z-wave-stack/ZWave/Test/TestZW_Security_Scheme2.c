// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file TestZW_Security_Scheme2.c
 * @copyright 2022 Silicon Laboratories Inc.
 */
#include "mock_control.h"
#include "ZW_Security_Scheme2.h"
#include "ZW_typedefs.h"

void setUpSuite(void) {

}

void tearDownSuite(void) {

}

void setUp(void) {

}

void tearDown(void) {

}

#define BUF_LENGTH 10

void test_S2_send_frame(void)
{
  mock_calls_clear();

  struct S2 ctxt;
  s2_connection_t conn;
  uint8_t buf[BUF_LENGTH];

  mock_t * pMock;

  memset(&conn, 0, sizeof(conn));

  mock_call_expect(TO_STR(ZW_SendDataEx), &pMock);
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = BUF_LENGTH;
  pMock->compare_rule_arg[ARG2] = COMPARE_NOT_NULL;
  pMock->compare_rule_arg[ARG3] = COMPARE_NOT_NULL;

  mock_call_expect(TO_STR(S2_send_frame_done_notify), &pMock);
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = 0;
  pMock->expect_arg[ARG2].value = 500;

  S2_send_frame(&ctxt,
                &conn,
                buf,
                BUF_LENGTH);

  mock_calls_verify();
}


void test_S2_send_frame_no_cb(void)
{
  mock_calls_clear();

  struct S2 ctxt;
  s2_connection_t conn;
  uint8_t buf[BUF_LENGTH];

  mock_t * pMock;

  memset(&conn, 0, sizeof(conn));

  mock_call_expect(TO_STR(ZW_SendDataEx), &pMock);
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = BUF_LENGTH;
  pMock->compare_rule_arg[ARG2] = COMPARE_NOT_NULL;
  pMock->compare_rule_arg[ARG3] = COMPARE_NOT_NULL;

  S2_send_frame_no_cb(&ctxt,
                      &conn,
                      buf,
                      BUF_LENGTH);

  mock_calls_verify();
}


void test_S2_send_frame_multi(void)
{
  mock_calls_clear();

  struct S2 ctxt;
  s2_connection_t conn;
  uint8_t buf[BUF_LENGTH];

  mock_t * pMock;

  memset(&conn, 0, sizeof(conn));

  mock_call_expect(TO_STR(ZW_SendDataEx), &pMock);
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = BUF_LENGTH;
  pMock->compare_rule_arg[ARG2] = COMPARE_NOT_NULL;
  pMock->compare_rule_arg[ARG3] = COMPARE_NOT_NULL;

  mock_call_expect(TO_STR(S2_send_frame_done_notify), &pMock);
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = 0;
  pMock->expect_arg[ARG2].value = 500;

  S2_send_frame_multi(&ctxt,
                      &conn,
                      buf,
                      BUF_LENGTH);

  mock_calls_verify();
}
