// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file TestZW_Security_Scheme0.c
 * @copyright 2022 Silicon Laboratories Inc.
 */
#include "mock_control.h"
#include "ZW_Security_Scheme0.h"
#include "ZW_typedefs.h"
#include "ZW_ctimer.h"

void setUpSuite(void) {

}

void tearDownSuite(void) {

}

void setUp(void) {

}

void tearDown(void) {

}

void test_s0_get_version(void)
{
  uint8_t network_key_s0[] = {
    0xE7, 0x86, 0xA5, 0x73,
    0x19, 0xA1, 0xD4, 0x76,
    0x50, 0xCF, 0xDC, 0x08,
    0x77, 0x92, 0xB2, 0x1D
  };
  uint8_t my_nonce[] = {
    0xE0, 0x76, 0x33, 0x1F, 
    0x17, 0x22, 0xBE, 0x7F
  };
  uint8_t enc_data[] = {
    /* 0xE3, 0x9C, 0xB9, 0xC3, 
    0x01, 0x41, 0x09, 0x20, 
    0x4E, */ 
    0x98, 0x81, 0x11, 0x48, 
    0x1C, 0x51, 0xA2, 0x17, 
    0x12, 0x32, 0x36, 0x3E, 
    0xD0, 0xE0, 0xC2, 0x55, 
    0xB3, 0xF4, 0xC5, 0x8F, 
    0x7F, 0x20
    /*, 0xB8 */
  };
  uint8_t send_data[] = {
    134, 18, 3, 7,
    17, 10, 17, 1,
    0
  };  
  uint8_t nonce_2[] = {
    0x5D, 0xF7, 0x79, 0x44, 0xD6, 0x21, 0xC8, 0x42
  };

  node_t controller_id = 1;
  node_t end_device_id = 78;
  STransmitCallback pCallback = { NULL, NULL };

  uint8_t dec_message[S0_MAX_ENCRYPTED_MSG_SIZE];
  uint8_t len;
  mock_t *pMock;

  mock_calls_clear();
  
  mock_call_use_as_stub(TO_STR(zpal_pm_cancel));
  mock_call_use_as_stub(TO_STR(ctimer_stop));
  mock_call_use_as_stub(TO_STR(ctimer_set));

  mock_call_expect(TO_STR(keystore_network_key_read), &pMock);
  pMock->expect_arg[ARG0].value   = 0x80; // KEY_CLASS_S0
  pMock->output_arg[ARG1].pointer = network_key_s0;
  pMock->return_code.v = true;

  uint8_t pm_handle1;
  mock_call_expect(TO_STR(zpal_pm_register), &pMock);
  pMock->expect_arg[ARG0].value = 0; // ZPAL_PM_TYPE_USE_RADIO
  pMock->return_code.p = &pm_handle1;
  
  uint8_t pm_handle2;
  mock_call_expect(TO_STR(zpal_pm_register), &pMock);
  pMock->expect_arg[ARG0].value = 0; // ZPAL_PM_TYPE_USE_RADIO  
  pMock->return_code.p = &pm_handle2;

  sec0_register_power_locks();

  mock_call_expect(TO_STR(zpal_pm_cancel), &pMock);
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;

  mock_call_expect(TO_STR(zpal_pm_cancel), &pMock);
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;

  sec0_init();

  mock_call_expect(TO_STR(zpal_get_random_data), &pMock);
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;  
  pMock->output_arg[ARG0].p = my_nonce;
  pMock->expect_arg[ARG1].value = 8;

  mock_call_expect(TO_STR(ZW_SendDataEx), &pMock);
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = 10; // sizeof(ZW_SECURITY_NONCE_REPORT_FRAME)
  pMock->compare_rule_arg[ARG2] = COMPARE_NOT_NULL;
  pMock->compare_rule_arg[ARG3] = COMPARE_NOT_NULL;
  pMock->return_code.v = 1; // ZW_TX_IN_PROGRESS

  mock_call_expect(TO_STR(zpal_pm_stay_awake), &pMock);
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = 500; // NONCE_REPORT_TIMEOUT

  sec0_send_nonce(controller_id, end_device_id, 33);

  mock_call_expect(TO_STR(xTaskGetTickCount), &pMock);
  pMock->return_code.v = 0xFFFFFF00;

  mock_call_expect(TO_STR(zpal_pm_cancel), &pMock);
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;

  len = sec0_decrypt_message(controller_id, end_device_id, enc_data, sizeof(enc_data), dec_message);

  TEST_ASSERT_EQUAL_INT8_MESSAGE(2, len, "Wrong message size");

  mock_call_expect(TO_STR(ZW_SendDataEx), &pMock);
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = 2; // sizeof(nonce_get)
  pMock->compare_rule_arg[ARG2] = COMPARE_NOT_NULL;
  pMock->compare_rule_arg[ARG3] = COMPARE_NOT_NULL;
  pMock->return_code.v = 1; // ZW_TX_IN_PROGRESS

  mock_call_expect(TO_STR(zpal_pm_stay_awake), &pMock);
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = 10000; // NONCE_REQUEST_TIMEOUT

  sec0_send_data(end_device_id, controller_id, send_data, sizeof(send_data), 37, &pCallback);

  mock_call_expect(TO_STR(zpal_get_random_data), &pMock);
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;  
  pMock->output_arg[ARG0].p = nonce_2;
  pMock->expect_arg[ARG1].value = 8;

  mock_call_expect(TO_STR(ZW_SendDataEx), &pMock);
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = 29; // encrypt_msg(0)
  pMock->compare_rule_arg[ARG2] = COMPARE_NOT_NULL;
  pMock->compare_rule_arg[ARG3] = COMPARE_NOT_NULL;
  pMock->return_code.v = 1; // ZW_TX_IN_PROGRESS

  mock_call_expect(TO_STR(zpal_pm_cancel), &pMock);
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;

  mock_call_expect(TO_STR(ZW_TransmitCallbackInvoke), &pMock);
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = 0x00; // TRANSMIT_COMPLETE_OK
  pMock->compare_rule_arg[ARG2] = COMPARE_NOT_NULL;

  sec0_register_nonce(controller_id, end_device_id, nonce_2);
}

void test_s0_segmentation(void)
{
  uint8_t network_key_s0[] = {
    0xE7, 0x86, 0xA5, 0x73, 0x19, 0xA1, 0xD4, 0x76,
    0x50, 0xCF, 0xDC, 0x08, 0x77, 0x92, 0xB2, 0x1D
  };  
  uint8_t nonce_1[] = { 
    0xBC, 0xC2, 0x34, 0xB3, 
    0x74, 0xA8, 0x77, 0xB9
  };
  uint8_t enc_data_1[] = {
    /* 0xE3, 0x9C, 0xB9, 0xC3, 0x01, 0x41, 0x04, 0x38, 
    0x52, */
    0x98, 0xC1, 0xC1, 0xAD, 0x2D, 0x31, 0xE3, 0x2D, 
    0x14, 0x67, 0x1A, 0x63, 0x5C, 0xA5, 0x49, 0xD5, 
    0xED, 0xF7, 0x44, 0xF6, 0xB5, 0x4D, 0x09, 0x93, 
    0x08, 0xC7, 0x16, 0x24, 0xCC, 0x57, 0x1C, 0x3A, 
    0x47, 0x2C, 0x55, 0xE3, 0xAC, 0xBC, 0x4F, 0x4A, 
    0xC7, 0xCB, 0xB2, 0xFC, 0x64, 0xBC
    /*, 0x84 */
  };
  uint8_t nonce_2[] = { 
    0x74, 0x5A, 0x4A, 0x70, 0x70, 0x79, 0x99, 0x6F
  };
  uint8_t enc_data_2[] = {
    /* 0xE3, 0x9C, 0xB9, 0xC3, 0x01, 0x41, 0x05, 0x36, 
    0x52, */
    0x98, 0x81, 0x5B, 0xDB, 0xAD, 0xEB, 0x31, 0xFF, 
    0xA1, 0x13, 0x67, 0x2C, 0x8F, 0x97, 0xAE, 0x94, 
    0xF2, 0x61, 0x95, 0x3B, 0x16, 0x8C, 0x1E, 0x69, 
    0x42, 0x5D, 0xDB, 0x8B, 0x9D, 0x65, 0x72, 0xA7, 
    0x1A, 0x51, 0x23, 0x74, 0x21, 0x66, 0xE7, 0x3F, 
    0xB5, 0xDB, 0x51, 0xF9
    /*, 0xE6 */
  };
  node_t controller_id = 1;
  node_t end_device_id = 82;

  uint8_t dec_message[S0_MAX_ENCRYPTED_MSG_SIZE];
  uint8_t len;
  mock_t *pMock;

  mock_calls_clear();
  
  mock_call_use_as_stub(TO_STR(zpal_pm_cancel));
  mock_call_use_as_stub(TO_STR(ctimer_stop));
  mock_call_use_as_stub(TO_STR(ctimer_set));

  mock_call_expect(TO_STR(keystore_network_key_read), &pMock);
  pMock->expect_arg[ARG0].value   = 0x80; // KEY_CLASS_S0
  pMock->output_arg[ARG1].pointer = network_key_s0;
  pMock->return_code.v = true;

  sec0_init();  

  mock_call_expect(TO_STR(zpal_get_random_data), &pMock);
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;  
  pMock->output_arg[ARG0].p = nonce_1;
  pMock->expect_arg[ARG1].value = 8;

  mock_call_expect(TO_STR(ZW_SendDataEx), &pMock);
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = 10; // sizeof(ZW_SECURITY_NONCE_REPORT_FRAME)
  pMock->compare_rule_arg[ARG2] = COMPARE_NOT_NULL;
  pMock->compare_rule_arg[ARG3] = COMPARE_NOT_NULL;
  pMock->return_code.v = 1; // ZW_TX_IN_PROGRESS

  mock_call_expect(TO_STR(zpal_pm_stay_awake), &pMock);
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = 500; // NONCE_REPORT_TIMEOUT

  sec0_send_nonce(controller_id, end_device_id, 33);

  mock_call_expect(TO_STR(xTaskGetTickCount), &pMock);
  pMock->return_code.v = 0x00000000;

  mock_call_expect(TO_STR(zpal_pm_cancel), &pMock);
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;

  len = sec0_decrypt_message(controller_id, end_device_id, enc_data_1, sizeof(enc_data_1), dec_message);

  TEST_ASSERT_EQUAL_INT8_MESSAGE(0, len, "This is the first frame.");

  mock_call_expect(TO_STR(zpal_get_random_data), &pMock);
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;  
  pMock->output_arg[ARG0].p = nonce_2;
  pMock->expect_arg[ARG1].value = 8;

  mock_call_expect(TO_STR(ZW_SendDataEx), &pMock);
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = 10; // sizeof(ZW_SECURITY_NONCE_REPORT_FRAME)
  pMock->compare_rule_arg[ARG2] = COMPARE_NOT_NULL;
  pMock->compare_rule_arg[ARG3] = COMPARE_NOT_NULL;
  pMock->return_code.v = 1; // ZW_TX_IN_PROGRESS

  mock_call_expect(TO_STR(zpal_pm_stay_awake), &pMock);
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = 500; // NONCE_REPORT_TIMEOUT

  sec0_send_nonce(controller_id, end_device_id, 33);

  mock_call_expect(TO_STR(xTaskGetTickCount), &pMock);
  pMock->return_code.v = 0x00002000;

  mock_call_expect(TO_STR(zpal_pm_cancel), &pMock);
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;

  len = sec0_decrypt_message(controller_id, end_device_id, enc_data_2, sizeof(enc_data_2), dec_message);

  TEST_ASSERT_EQUAL_INT8_MESSAGE(50, len, "This is the second frame.");
}

void test_s0_segmentation_overflow(void)
{
  uint8_t network_key_s0[] = {
    0xE7, 0x86, 0xA5, 0x73, 0x19, 0xA1, 0xD4, 0x76,
    0x50, 0xCF, 0xDC, 0x08, 0x77, 0x92, 0xB2, 0x1D
  };  
  uint8_t nonce_1[] = { 
    0xBC, 0xC2, 0x34, 0xB3, 
    0x74, 0xA8, 0x77, 0xB9
  };
  uint8_t enc_data_1[] = {
    /* 0xE3, 0x9C, 0xB9, 0xC3, 0x01, 0x41, 0x04, 0x38, 
    0x52, */
    0x98, 0xC1, 0xC1, 0xAD, 0x2D, 0x31, 0xE3, 0x2D, 
    0x14, 0x67, 0x1A, 0x63, 0x5C, 0xA5, 0x49, 0xD5, 
    0xED, 0xF7, 0x44, 0xF6, 0xB5, 0x4D, 0x09, 0x93, 
    0x08, 0xC7, 0x16, 0x24, 0xCC, 0x57, 0x1C, 0x3A, 
    0x47, 0x2C, 0x55, 0xE3, 0xAC, 0xBC, 0x4F, 0x4A, 
    0xC7, 0xCB, 0xB2, 0xFC, 0x64, 0xBC
    /*, 0x84 */
  };
  uint8_t nonce_2[] = { 
    0x74, 0x5A, 0x4A, 0x70, 0x70, 0x79, 0x99, 0x6F
  };
  uint8_t enc_data_2[] = {
    /* 0xE3, 0x9C, 0xB9, 0xC3, 0x01, 0x41, 0x05, 0x36, 
    0x52, */
    0x98, 0x81, 0x5B, 0xDB, 0xAD, 0xEB, 0x31, 0xFF, 
    0xA1, 0x13, 0x67, 0x2C, 0x8F, 0x97, 0xAE, 0x94, 
    0xF2, 0x61, 0x95, 0x3B, 0x16, 0x8C, 0x1E, 0x69, 
    0x42, 0x5D, 0xDB, 0x8B, 0x9D, 0x65, 0x72, 0xA7, 
    0x1A, 0x51, 0x23, 0x74, 0x21, 0x66, 0xE7, 0x3F, 
    0xB5, 0xDB, 0x51, 0xF9
    /*, 0xE6 */
  };
  node_t controller_id = 1;
  node_t end_device_id = 82;

  uint8_t dec_message[S0_MAX_ENCRYPTED_MSG_SIZE];
  uint8_t len;
  mock_t *pMock;

  mock_calls_clear();
  
  mock_call_use_as_stub(TO_STR(zpal_pm_cancel));
  mock_call_use_as_stub(TO_STR(ctimer_stop));
  mock_call_use_as_stub(TO_STR(ctimer_set));

  mock_call_expect(TO_STR(keystore_network_key_read), &pMock);
  pMock->expect_arg[ARG0].value   = 0x80; // KEY_CLASS_S0
  pMock->output_arg[ARG1].pointer = network_key_s0;
  pMock->return_code.v = true;

  sec0_init();  

  mock_call_expect(TO_STR(zpal_get_random_data), &pMock);
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;  
  pMock->output_arg[ARG0].p = nonce_1;
  pMock->expect_arg[ARG1].value = 8;

  mock_call_expect(TO_STR(ZW_SendDataEx), &pMock);
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = 10; // sizeof(ZW_SECURITY_NONCE_REPORT_FRAME)
  pMock->compare_rule_arg[ARG2] = COMPARE_NOT_NULL;
  pMock->compare_rule_arg[ARG3] = COMPARE_NOT_NULL;
  pMock->return_code.v = 1; // ZW_TX_IN_PROGRESS

  mock_call_expect(TO_STR(zpal_pm_stay_awake), &pMock);
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = 500; // NONCE_REPORT_TIMEOUT

  sec0_send_nonce(controller_id, end_device_id, 33);

  mock_call_expect(TO_STR(xTaskGetTickCount), &pMock);
  pMock->return_code.v = 0xFFFFFAFF;

  mock_call_expect(TO_STR(zpal_pm_cancel), &pMock);
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;

  len = sec0_decrypt_message(controller_id, end_device_id, enc_data_1, sizeof(enc_data_1), dec_message);

  TEST_ASSERT_EQUAL_INT8_MESSAGE(0, len, "This is the first frame.");

  mock_call_expect(TO_STR(zpal_get_random_data), &pMock);
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;  
  pMock->output_arg[ARG0].p = nonce_2;
  pMock->expect_arg[ARG1].value = 8;

  mock_call_expect(TO_STR(ZW_SendDataEx), &pMock);
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = 10; // sizeof(ZW_SECURITY_NONCE_REPORT_FRAME)
  pMock->compare_rule_arg[ARG2] = COMPARE_NOT_NULL;
  pMock->compare_rule_arg[ARG3] = COMPARE_NOT_NULL;
  pMock->return_code.v = 1; // ZW_TX_IN_PROGRESS

  mock_call_expect(TO_STR(zpal_pm_stay_awake), &pMock);
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = 500; // NONCE_REPORT_TIMEOUT

  sec0_send_nonce(controller_id, end_device_id, 33);

  mock_call_expect(TO_STR(xTaskGetTickCount), &pMock);
  pMock->return_code.v = 0xFFFFFBFF;

  mock_call_expect(TO_STR(zpal_pm_cancel), &pMock);
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;

  len = sec0_decrypt_message(controller_id, end_device_id, enc_data_2, sizeof(enc_data_2), dec_message);

  TEST_ASSERT_EQUAL_INT8_MESSAGE(50, len, "This is the second frame.");
}

void test_s0_segmentation_with_expired_session(void)
{
  uint8_t network_key_s0[] = {
    0xE7, 0x86, 0xA5, 0x73, 0x19, 0xA1, 0xD4, 0x76,
    0x50, 0xCF, 0xDC, 0x08, 0x77, 0x92, 0xB2, 0x1D
  };  
  uint8_t nonce_1[] = { 
    0xBC, 0xC2, 0x34, 0xB3, 
    0x74, 0xA8, 0x77, 0xB9
  };
  uint8_t enc_data_1[] = {
    /* 0xE3, 0x9C, 0xB9, 0xC3, 0x01, 0x41, 0x04, 0x38, 
    0x52, */
    0x98, 0xC1, 0xC1, 0xAD, 0x2D, 0x31, 0xE3, 0x2D, 
    0x14, 0x67, 0x1A, 0x63, 0x5C, 0xA5, 0x49, 0xD5, 
    0xED, 0xF7, 0x44, 0xF6, 0xB5, 0x4D, 0x09, 0x93, 
    0x08, 0xC7, 0x16, 0x24, 0xCC, 0x57, 0x1C, 0x3A, 
    0x47, 0x2C, 0x55, 0xE3, 0xAC, 0xBC, 0x4F, 0x4A, 
    0xC7, 0xCB, 0xB2, 0xFC, 0x64, 0xBC
    /*, 0x84 */
  };
  uint8_t nonce_2[] = { 
    0x74, 0x5A, 0x4A, 0x70, 0x70, 0x79, 0x99, 0x6F
  };
  uint8_t enc_data_2[] = {
    /* 0xE3, 0x9C, 0xB9, 0xC3, 0x01, 0x41, 0x05, 0x36, 
    0x52, */
    0x98, 0x81, 0x5B, 0xDB, 0xAD, 0xEB, 0x31, 0xFF, 
    0xA1, 0x13, 0x67, 0x2C, 0x8F, 0x97, 0xAE, 0x94, 
    0xF2, 0x61, 0x95, 0x3B, 0x16, 0x8C, 0x1E, 0x69, 
    0x42, 0x5D, 0xDB, 0x8B, 0x9D, 0x65, 0x72, 0xA7, 
    0x1A, 0x51, 0x23, 0x74, 0x21, 0x66, 0xE7, 0x3F, 
    0xB5, 0xDB, 0x51, 0xF9
    /*, 0xE6 */
  };
  node_t controller_id = 1;
  node_t end_device_id = 82;

  uint8_t dec_message[S0_MAX_ENCRYPTED_MSG_SIZE];
  uint8_t len;
  mock_t *pMock;

  mock_calls_clear();
  
  mock_call_use_as_stub(TO_STR(zpal_pm_cancel));
  mock_call_use_as_stub(TO_STR(ctimer_stop));
  mock_call_use_as_stub(TO_STR(ctimer_set));

  mock_call_expect(TO_STR(keystore_network_key_read), &pMock);
  pMock->expect_arg[ARG0].value   = 0x80; // KEY_CLASS_S0
  pMock->output_arg[ARG1].pointer = network_key_s0;
  pMock->return_code.v = true;

  sec0_init();  

  mock_call_expect(TO_STR(zpal_get_random_data), &pMock);
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;  
  pMock->output_arg[ARG0].p = nonce_1;
  pMock->expect_arg[ARG1].value = 8;

  mock_call_expect(TO_STR(ZW_SendDataEx), &pMock);
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = 10; // sizeof(ZW_SECURITY_NONCE_REPORT_FRAME)
  pMock->compare_rule_arg[ARG2] = COMPARE_NOT_NULL;
  pMock->compare_rule_arg[ARG3] = COMPARE_NOT_NULL;
  pMock->return_code.v = 1; // ZW_TX_IN_PROGRESS

  mock_call_expect(TO_STR(zpal_pm_stay_awake), &pMock);
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = 500; // NONCE_REPORT_TIMEOUT

  sec0_send_nonce(controller_id, end_device_id, 33);

  mock_call_expect(TO_STR(xTaskGetTickCount), &pMock);
  pMock->return_code.v = 0x00000000;

  mock_call_expect(TO_STR(zpal_pm_cancel), &pMock);
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;

  len = sec0_decrypt_message(controller_id, end_device_id, enc_data_1, sizeof(enc_data_1), dec_message);

  TEST_ASSERT_EQUAL_INT8_MESSAGE(0, len, "This is the first frame.");

  mock_call_expect(TO_STR(zpal_get_random_data), &pMock);
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;  
  pMock->output_arg[ARG0].p = nonce_2;
  pMock->expect_arg[ARG1].value = 8;

  mock_call_expect(TO_STR(ZW_SendDataEx), &pMock);
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = 10; // sizeof(ZW_SECURITY_NONCE_REPORT_FRAME)
  pMock->compare_rule_arg[ARG2] = COMPARE_NOT_NULL;
  pMock->compare_rule_arg[ARG3] = COMPARE_NOT_NULL;
  pMock->return_code.v = 1; // ZW_TX_IN_PROGRESS

  mock_call_expect(TO_STR(zpal_pm_stay_awake), &pMock);
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = 500; // NONCE_REPORT_TIMEOUT

  sec0_send_nonce(controller_id, end_device_id, 33);

  mock_call_expect(TO_STR(xTaskGetTickCount), &pMock);
  pMock->return_code.v = 0x00003000;

  mock_call_expect(TO_STR(xTaskGetTickCount), &pMock);
  pMock->return_code.v = 0x00003050;

  mock_call_expect(TO_STR(xTaskGetTickCount), &pMock);
  pMock->return_code.v = 0x00003100;

  mock_call_expect(TO_STR(zpal_pm_cancel), &pMock);
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;

  len = sec0_decrypt_message(controller_id, end_device_id, enc_data_2, sizeof(enc_data_2), dec_message);

  TEST_ASSERT_EQUAL_INT8_MESSAGE(0, len, "This is the second frame, but the session expired.");
}

void test_s0_segmentation_with_expired_session_overflow(void)
{
  uint8_t network_key_s0[] = {
    0xE7, 0x86, 0xA5, 0x73, 0x19, 0xA1, 0xD4, 0x76,
    0x50, 0xCF, 0xDC, 0x08, 0x77, 0x92, 0xB2, 0x1D
  };  
  uint8_t nonce_1[] = { 
    0xBC, 0xC2, 0x34, 0xB3, 
    0x74, 0xA8, 0x77, 0xB9
  };
  uint8_t enc_data_1[] = {
    /* 0xE3, 0x9C, 0xB9, 0xC3, 0x01, 0x41, 0x04, 0x38, 
    0x52, */
    0x98, 0xC1, 0xC1, 0xAD, 0x2D, 0x31, 0xE3, 0x2D, 
    0x14, 0x67, 0x1A, 0x63, 0x5C, 0xA5, 0x49, 0xD5, 
    0xED, 0xF7, 0x44, 0xF6, 0xB5, 0x4D, 0x09, 0x93, 
    0x08, 0xC7, 0x16, 0x24, 0xCC, 0x57, 0x1C, 0x3A, 
    0x47, 0x2C, 0x55, 0xE3, 0xAC, 0xBC, 0x4F, 0x4A, 
    0xC7, 0xCB, 0xB2, 0xFC, 0x64, 0xBC
    /*, 0x84 */
  };
  uint8_t nonce_2[] = { 
    0x74, 0x5A, 0x4A, 0x70, 0x70, 0x79, 0x99, 0x6F
  };
  uint8_t enc_data_2[] = {
    /* 0xE3, 0x9C, 0xB9, 0xC3, 0x01, 0x41, 0x05, 0x36, 
    0x52, */
    0x98, 0x81, 0x5B, 0xDB, 0xAD, 0xEB, 0x31, 0xFF, 
    0xA1, 0x13, 0x67, 0x2C, 0x8F, 0x97, 0xAE, 0x94, 
    0xF2, 0x61, 0x95, 0x3B, 0x16, 0x8C, 0x1E, 0x69, 
    0x42, 0x5D, 0xDB, 0x8B, 0x9D, 0x65, 0x72, 0xA7, 
    0x1A, 0x51, 0x23, 0x74, 0x21, 0x66, 0xE7, 0x3F, 
    0xB5, 0xDB, 0x51, 0xF9
    /*, 0xE6 */
  };
  node_t controller_id = 1;
  node_t end_device_id = 82;

  uint8_t dec_message[S0_MAX_ENCRYPTED_MSG_SIZE];
  uint8_t len;
  mock_t *pMock;

  mock_calls_clear();
  
  mock_call_use_as_stub(TO_STR(zpal_pm_cancel));
  mock_call_use_as_stub(TO_STR(ctimer_stop));
  mock_call_use_as_stub(TO_STR(ctimer_set));

  mock_call_expect(TO_STR(keystore_network_key_read), &pMock);
  pMock->expect_arg[ARG0].value   = 0x80; // KEY_CLASS_S0
  pMock->output_arg[ARG1].pointer = network_key_s0;
  pMock->return_code.v = true;

  sec0_init();  

  mock_call_expect(TO_STR(zpal_get_random_data), &pMock);
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;  
  pMock->output_arg[ARG0].p = nonce_1;
  pMock->expect_arg[ARG1].value = 8;

  mock_call_expect(TO_STR(ZW_SendDataEx), &pMock);
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = 10; // sizeof(ZW_SECURITY_NONCE_REPORT_FRAME)
  pMock->compare_rule_arg[ARG2] = COMPARE_NOT_NULL;
  pMock->compare_rule_arg[ARG3] = COMPARE_NOT_NULL;
  pMock->return_code.v = 1; // ZW_TX_IN_PROGRESS

  mock_call_expect(TO_STR(zpal_pm_stay_awake), &pMock);
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = 500; // NONCE_REPORT_TIMEOUT

  sec0_send_nonce(controller_id, end_device_id, 33);

  mock_call_expect(TO_STR(xTaskGetTickCount), &pMock);
  pMock->return_code.v = 0xFFFFD7F0;

  mock_call_expect(TO_STR(zpal_pm_cancel), &pMock);
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;

  len = sec0_decrypt_message(controller_id, end_device_id, enc_data_1, sizeof(enc_data_1), dec_message);

  TEST_ASSERT_EQUAL_INT8_MESSAGE(0, len, "This is the first frame.");

  mock_call_expect(TO_STR(zpal_get_random_data), &pMock);
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;  
  pMock->output_arg[ARG0].p = nonce_2;
  pMock->expect_arg[ARG1].value = 8;

  mock_call_expect(TO_STR(ZW_SendDataEx), &pMock);
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = 10; // sizeof(ZW_SECURITY_NONCE_REPORT_FRAME)
  pMock->compare_rule_arg[ARG2] = COMPARE_NOT_NULL;
  pMock->compare_rule_arg[ARG3] = COMPARE_NOT_NULL;
  pMock->return_code.v = 1; // ZW_TX_IN_PROGRESS

  mock_call_expect(TO_STR(zpal_pm_stay_awake), &pMock);
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = 500; // NONCE_REPORT_TIMEOUT

  sec0_send_nonce(controller_id, end_device_id, 33);

  mock_call_expect(TO_STR(xTaskGetTickCount), &pMock);
  pMock->return_code.v = 0x00003000;

  mock_call_expect(TO_STR(xTaskGetTickCount), &pMock);
  pMock->return_code.v = 0x00003050;

  mock_call_expect(TO_STR(xTaskGetTickCount), &pMock);
  pMock->return_code.v = 0x00003100;

  mock_call_expect(TO_STR(zpal_pm_cancel), &pMock);
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;

  len = sec0_decrypt_message(controller_id, end_device_id, enc_data_2, sizeof(enc_data_2), dec_message);

  TEST_ASSERT_EQUAL_INT8_MESSAGE(0, len, "This is the second frame, but the session expired.");
}

/*
 * EOF
 */
