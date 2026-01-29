/*
 * SPDX-FileCopyrightText: 2022 Silicon Laboratories Inc. <https://www.silabs.com/>
 * SPDX-FileCopyrightText: 2025 Z-Wave Alliance <https://z-wavealliance.org/>
 * SPDX-FileCopyrightText: 2025 Card Access Engineering, LLC <https://www.caengineering.com/>
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

/**
 * @file test_CC_Version.c
 * @copyright 2022 Silicon Laboratories Inc.
 */
#include <mock_control.h>
#include <ZW_basis_api.h>
#include <string.h>
#include <ZW_application_transport_interface.h>
#include <test_common.h>
#include <SizeOf.h>
#include "ZAF_CC_Invoker.h"
#include "ZW_TransportEndpoint.h"
#include "cc_version_migration_types.h"
#include "cc_version_migration_config_api_mock.h"
#include "cc_version_migration_api_mock.h"

void setUpSuite(void)
{
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

static command_handler_input_t * version_command_class_get_frame_create(uint8_t commandClass);

void test_VERSION_CAPABILITIES_GET_V4(void)
{
  mock_calls_clear();

  RECEIVE_OPTIONS_TYPE_EX rxOptions;
  memset((uint8_t *)&rxOptions, 0x00, sizeof(RECEIVE_OPTIONS_TYPE_EX));
  ZW_APPLICATION_TX_BUFFER frame;
  uint8_t * pFrame = (uint8_t *)&frame;
  uint8_t commandLength = 0;
  uint8_t frameCount = 0;

  memset(pFrame, 0x00, sizeof(ZW_APPLICATION_TX_BUFFER));
  pFrame[frameCount++] = COMMAND_CLASS_VERSION_V3;
  pFrame[frameCount++] = VERSION_CAPABILITIES_GET_V3;
  commandLength = frameCount;

  mock_call_use_as_stub(TO_STR(Check_not_legal_response_job));
  cc_version_is_migration_supported_ExpectAndReturn(false);
  uint8_t expectedFrame[] = {
    0x86,   // COMMAND_CLASS_VERSION_V3
    0x16,   // VERSION_CAPABILITIES_REPORT_V3
    0x07    // Version Capabilites - Migration not supported
  };

  ZW_APPLICATION_TX_BUFFER frameOut;
  uint8_t frameOutLength;
  received_frame_status_t res = invoke_cc_handler_v2(&rxOptions, &frame,
                                                     commandLength, &frameOut,
                                                     &frameOutLength);

  TEST_ASSERT_EQUAL_UINT_MESSAGE(RECEIVED_FRAME_STATUS_SUCCESS,
                                 res,
                                 "Migration unsupported - return code from invoke_cc_handler_v2(...)");
  TEST_ASSERT_EQUAL_INT8_MESSAGE(frameOutLength,
                                 sizeof(expectedFrame),
                                 "Migration unsupported - Frame size does not match");
  TEST_ASSERT_EQUAL_INT8_ARRAY_MESSAGE(expectedFrame, &frameOut,
                                       sizeof(expectedFrame),
                                       "Migration Unsupported - Frame does not match");

  // Test again with Migration supported

  mock_call_use_as_stub(TO_STR(Check_not_legal_response_job));
  cc_version_is_migration_supported_ExpectAndReturn(true);
  expectedFrame[0] = 0x86;   // COMMAND_CLASS_VERSION_V3
  expectedFrame[1] = 0x16;   // VERSION_CAPABILITIES_REPORT_V3
  expectedFrame[2] = 0x0F;   // Version Capabilites - Migration *IS* supported


  res = invoke_cc_handler_v2(&rxOptions, &frame,
                                                     commandLength, &frameOut,
                                                     &frameOutLength);

  TEST_ASSERT_EQUAL_UINT_MESSAGE(RECEIVED_FRAME_STATUS_SUCCESS,
                                 res,
                                 "Supported Migration - return code from invoke_cc_handler_v2(...)");
  TEST_ASSERT_EQUAL_INT8_MESSAGE(frameOutLength,
                                 sizeof(expectedFrame),
                                 "Supported Migration - Frame size does not match");
  TEST_ASSERT_EQUAL_INT8_ARRAY_MESSAGE(expectedFrame, &frameOut,
                                       sizeof(expectedFrame),
                                       "Supported Migration - Frame does not match");

  mock_calls_verify();
}

void test_VERSION_ZWAVE_SOFTWARE_V3_GET_two_chip(void)
{
  mock_t* pMock = NULL;
  mock_calls_clear();

  RECEIVE_OPTIONS_TYPE_EX rxOptions;
  memset((uint8_t *)&rxOptions, 0x00, sizeof(RECEIVE_OPTIONS_TYPE_EX));
  ZW_APPLICATION_TX_BUFFER frame;
  uint8_t * pFrame = (uint8_t *)&frame;
  uint8_t commandLength = 0;
  uint8_t frameCount = 0;

  memset(pFrame, 0x00, sizeof(ZW_APPLICATION_TX_BUFFER));
  pFrame[frameCount++] = COMMAND_CLASS_VERSION_V3;
  pFrame[frameCount++] = VERSION_ZWAVE_SOFTWARE_GET_V3;
  commandLength = frameCount;

  mock_call_use_as_stub(TO_STR(Check_not_legal_response_job));

  mock_call_expect(TO_STR(zaf_config_get_build_no), &pMock);
  pMock->return_code.v = 0x1617;

  static SProtocolInfo m_ProtocolInfo = {
    .CommandClassVersions.SecurityVersion = 0,                                         // CommandClassVersions are setup run time
    .CommandClassVersions.Security2Version = 0,
    .CommandClassVersions.TransportServiceVersion = 0,
    .ProtocolVersion.Major = ZW_VERSION_MAJOR,
    .ProtocolVersion.Minor = ZW_VERSION_MINOR,
    .ProtocolVersion.Revision = ZW_VERSION_PATCH,
    .eProtocolType = EPROTOCOLTYPE_ZWAVE,
    .eLibraryType = 0                                                                  // Library type is setup run time
  };
  SApplicationHandles appHandles;
  appHandles.pProtocolInfo = &m_ProtocolInfo;

  mock_call_expect(TO_STR(ZAF_getAppHandle), &pMock);
  pMock->return_code.p = &appHandles;

  mock_call_expect(TO_STR(zpal_get_app_version_major), &pMock);
  pMock->return_code.v = 0x13;
  mock_call_expect(TO_STR(zpal_get_app_version_minor), &pMock);
  pMock->return_code.v = 0x14;
  mock_call_expect(TO_STR(zpal_get_app_version_patch), &pMock);
  pMock->return_code.v = 0x15;

  mock_call_expect(TO_STR(ZW_GetProtocolBuildNumber), &pMock);
  pMock->return_code.v = ZW_BUILD_NO;

  uint8_t expectedFrame[] = {
    0x86,   // COMMAND_CLASS_VERSION_V3
    0x18,   // VERSION_ZWAVE_SOFTWARE_REPORT_V3

    SDK_VERSION_MAJOR,   // SDK version major
    SDK_VERSION_MINOR,   // SDK version minor
    SDK_VERSION_PATCH,   // SDK version patch

    ZAF_VERSION_MAJOR,   // ZAF version major
    ZAF_VERSION_MINOR,   // ZAF version minor
    ZAF_VERSION_PATCH,   // ZAF version patch
    (uint8_t)(ZAF_BUILD_NO >> 8),   // ZAF build number MSB
    (uint8_t)ZAF_BUILD_NO,   // ZAF build number LSB

    0x00,   // Host interface version major
    0x00,   // Host interface version minor
    0x00,   // Host interface version patch
    0x00,   // Host interface build number MSB
    0x00,   // Host interface build number LSB

    ZW_VERSION_MAJOR,   // ZW version major
    ZW_VERSION_MINOR,   // ZW version minor
    ZW_VERSION_PATCH,   // ZW version patch
    (uint8_t)(ZW_BUILD_NO >> 8),   // ZW build number MSB
    (uint8_t)ZW_BUILD_NO,   // ZW build number LSB

    0x13,   // Application version major
    0x14,   // Application version minor
    0x15,   // Application version patch
    0x16,   // Application build number MSB
    0x17,   // Application build number LSB
  };

  ZW_APPLICATION_TX_BUFFER frameOut;
  uint8_t frameOutLength;
  received_frame_status_t res = invoke_cc_handler_v2(&rxOptions, &frame,
                                                     commandLength, &frameOut,
                                                     &frameOutLength);

  TEST_ASSERT_EQUAL_UINT_MESSAGE(RECEIVED_FRAME_STATUS_SUCCESS,
                                 res,
                                 "return code from invoke_cc_handler_v2(...)");
  TEST_ASSERT_EQUAL_INT8_MESSAGE(frameOutLength,
                                 sizeof(expectedFrame),
                                 "Frame size does not match");
  TEST_ASSERT_EQUAL_INT8_ARRAY_MESSAGE(expectedFrame, &frameOut,
                                       sizeof(expectedFrame),
                                       "Frame does not match");

  mock_calls_verify();
}

void test_VERSION_ZWAVE_SOFTWARE_V3_GET_no_host_nor_app_version(void)
{
  mock_t* pMock = NULL;
  mock_calls_clear();

  RECEIVE_OPTIONS_TYPE_EX rxOptions;
  memset((uint8_t *)&rxOptions, 0x00, sizeof(RECEIVE_OPTIONS_TYPE_EX));
  ZW_APPLICATION_TX_BUFFER frame;
  uint8_t * pFrame = (uint8_t *)&frame;
  uint8_t commandLength = 0;
  uint8_t frameCount = 0;

  memset(pFrame, 0x00, sizeof(ZW_APPLICATION_TX_BUFFER));
  pFrame[frameCount++] = COMMAND_CLASS_VERSION_V3;
  pFrame[frameCount++] = VERSION_ZWAVE_SOFTWARE_GET_V3;
  commandLength = frameCount;

  mock_call_use_as_stub(TO_STR(Check_not_legal_response_job));

  mock_call_expect(TO_STR(zaf_config_get_build_no), &pMock);
  pMock->return_code.v = 0;

  static SProtocolInfo m_ProtocolInfo = {
    .CommandClassVersions.SecurityVersion = 0,                                         // CommandClassVersions are setup run time
    .CommandClassVersions.Security2Version = 0,
    .CommandClassVersions.TransportServiceVersion = 0,
    .ProtocolVersion.Major = ZW_VERSION_MAJOR,
    .ProtocolVersion.Minor = ZW_VERSION_MINOR,
    .ProtocolVersion.Revision = ZW_VERSION_PATCH,
    .eProtocolType = EPROTOCOLTYPE_ZWAVE,
    .eLibraryType = 0                                                                  // Library type is setup run time
  };
  SApplicationHandles appHandles;
  appHandles.pProtocolInfo = &m_ProtocolInfo;

  mock_call_expect(TO_STR(ZAF_getAppHandle), &pMock);
  pMock->return_code.p = &appHandles;

  mock_call_expect(TO_STR(zpal_get_app_version_major), &pMock);
  mock_call_expect(TO_STR(zpal_get_app_version_minor), &pMock);
  mock_call_expect(TO_STR(zpal_get_app_version_patch), &pMock);
  mock_call_expect(TO_STR(ZW_GetProtocolBuildNumber), &pMock);
  pMock->return_code.v = ZW_BUILD_NO;

  uint8_t expectedFrame[] = {
    0x86,   // COMMAND_CLASS_VERSION_V3
    0x18,   // VERSION_ZWAVE_SOFTWARE_REPORT_V3

    SDK_VERSION_MAJOR,   // SDK version major
    SDK_VERSION_MINOR,   // SDK version minor
    SDK_VERSION_PATCH,   // SDK version patch

    ZAF_VERSION_MAJOR,   // ZAF version major
    ZAF_VERSION_MINOR,   // ZAF version minor
    ZAF_VERSION_PATCH,   // ZAF version patch
    (uint8_t)(ZAF_BUILD_NO >> 8),   // ZAF build number MSB
    (uint8_t) ZAF_BUILD_NO,   // ZAF build number LSB

    0,   // Host interface version major
    0,   // Host interface version minor
    0,   // Host interface version patch
    0,   // Host interface build number MSB
    0,   // Host interface build number LSB

    ZW_VERSION_MAJOR,
    ZW_VERSION_MINOR,
    ZW_VERSION_PATCH,
    (uint8_t)(ZW_BUILD_NO >> 8),   // ZW build number MSB
    (uint8_t) ZW_BUILD_NO,   // ZW build number LSB

    0,   // Application version major
    0,   // Application version minor
    0,   // Application version patch
    0,   // Application build number MSB
    0,   // Application build number LSB
  };

  ZW_APPLICATION_TX_BUFFER frameOut;
  uint8_t frameOutLength;
  received_frame_status_t res = invoke_cc_handler_v2(&rxOptions, &frame,
                                                     commandLength, &frameOut,
                                                     &frameOutLength);

  TEST_ASSERT_EQUAL_UINT_MESSAGE(RECEIVED_FRAME_STATUS_SUCCESS,
                                 res,
                                 "return code from invoke_cc_handler_v2(...)");
  TEST_ASSERT_EQUAL_INT8_MESSAGE(frameOutLength,
                                 sizeof(expectedFrame),
                                 "Frame size does not match");
  TEST_ASSERT_EQUAL_INT8_ARRAY_MESSAGE(expectedFrame, &frameOut,
                                       sizeof(expectedFrame),
                                       "Frame does not match");

  mock_calls_verify();
}

typedef struct {
  uint8_t cc;
  uint8_t expected_version;

  /*
   * A special case is when the CC is not implemented in the ZAF. CC Transport Service, S0 and S2
   * are all implemented in the protocol. Hence, the versions are stored in the app handle.
   * For the rest of the CCs, the version number is stored in the CC_handle defined by using the
   * REGISTER_CC() macro.
   */
  bool isSpecialCase;
  bool in_nif;
}
cc_version_t;

void test_VERSION_COMMAND_CLASS_GET_V2(void)
{
  mock_t* pMock = NULL;
  mock_calls_clear();

  const cc_version_t cc_versions[] = {
    // Verifies the version of CC Version.
    { .cc = COMMAND_CLASS_VERSION, .expected_version = 4, .isSpecialCase = false, .in_nif = false },         // Last value is "don't care"

    // Verifies correct versions in case the following CCs are
    // listed in the NIF.
    { .cc = COMMAND_CLASS_TRANSPORT_SERVICE, .expected_version = 4, .isSpecialCase = true, .in_nif = true },
    { .cc = COMMAND_CLASS_SECURITY, .expected_version = 5, .isSpecialCase = true, .in_nif = true },
    { .cc = COMMAND_CLASS_SECURITY_2, .expected_version = 6, .isSpecialCase = true, .in_nif = true },

    // Verifies that zero is returned in case none of the following
    // CCs are listed in the NIF.
    { .cc = COMMAND_CLASS_TRANSPORT_SERVICE, .expected_version = 0, .isSpecialCase = true, .in_nif = false },
    { .cc = COMMAND_CLASS_SECURITY, .expected_version = 0, .isSpecialCase = true, .in_nif = false },
    { .cc = COMMAND_CLASS_SECURITY_2, .expected_version = 0, .isSpecialCase = true, .in_nif = false },

    // Verifies that zero is returned in case of an invalid CC version.
    { .cc = 0xFF, .expected_version = 0, .isSpecialCase = true, .in_nif = false }                           // Last value is "don't care"
  };

  SProtocolInfo protocolInfo;
  protocolInfo.CommandClassVersions.TransportServiceVersion = cc_versions[1].expected_version;
  protocolInfo.CommandClassVersions.SecurityVersion         = cc_versions[2].expected_version;
  protocolInfo.CommandClassVersions.Security2Version        = cc_versions[3].expected_version;

  SApplicationHandles appHandles;
  appHandles.pProtocolInfo = &protocolInfo;

  mock_call_use_as_stub(TO_STR(Check_not_legal_response_job));

  for (uint8_t i = 0; i < sizeof_array(cc_versions); i++) {
    command_handler_input_t * p_chi = version_command_class_get_frame_create(cc_versions[i].cc);

    mock_call_expect(TO_STR(ZAF_getAppHandle), &pMock);
    pMock->return_code.p = &appHandles;

    uint8_t nif;
    if (true == cc_versions[i].in_nif) {
      nif = cc_versions[i].cc;
    } else {
      nif = 0xFF; // Random CC
    }
    zaf_cc_list_t CommandClassList;
    CommandClassList.cc_list = &nif;
    CommandClassList.list_size = sizeof(nif);

    mock_call_expect(TO_STR(GetCommandClassList), &pMock);
    pMock->return_code.p = &CommandClassList;

    uint8_t expectedFrame[] = {
      0x86,   // COMMAND_CLASS_VERSION_V3
      0x14,   // VERSION_COMMAND_CLASS_REPORT_V2
      cc_versions[i].cc,
      cc_versions[i].expected_version
    };

    ZW_APPLICATION_TX_BUFFER frameOut;
    uint8_t frameOutLength;
    received_frame_status_t res = invoke_cc_handler_v2(&p_chi->rxOptions, &p_chi->frame.as_zw_application_tx_buffer,
                                                       p_chi->frameLength, &frameOut,
                                                       &frameOutLength);

    TEST_ASSERT_EQUAL_UINT_MESSAGE(RECEIVED_FRAME_STATUS_SUCCESS,
                                   res,
                                   "return code from invoke_cc_handler_v2(...)");
    TEST_ASSERT_EQUAL_INT8_MESSAGE(frameOutLength,
                                   sizeof(expectedFrame),
                                   "Frame size does not match");
    TEST_ASSERT_EQUAL_INT8_ARRAY_MESSAGE(expectedFrame, &frameOut,
                                         sizeof(expectedFrame),
                                         "Frame does not match");

    test_common_command_handler_input_free(p_chi);
  }

  mock_calls_verify();
}

#define  NUMBER_OF_FIRMWARE_TARGETS  5
void test_VERSION_GET_V2(void)
{
  mock_t* pMock = NULL;
  mock_calls_clear();

  RECEIVE_OPTIONS_TYPE_EX rxOptions;
  memset((uint8_t *)&rxOptions, 0x00, sizeof(RECEIVE_OPTIONS_TYPE_EX));
  ZW_APPLICATION_TX_BUFFER frame;
  uint8_t * pFrame = (uint8_t *)&frame;
  uint8_t commandLength = 0;
  uint8_t frameCount = 0;

  const uint8_t ZW_TYPE_LIBRARY = ELIBRARYTYPE_DUT;

  typedef struct {
    uint8_t major;
    uint8_t minor;
  }
  version_t;

  version_t version_list[NUMBER_OF_FIRMWARE_TARGETS];

  memset(pFrame, 0x00, sizeof(ZW_APPLICATION_TX_BUFFER));
  pFrame[frameCount++] = COMMAND_CLASS_VERSION_V3;
  pFrame[frameCount++] = VERSION_GET_V2;
  commandLength = frameCount;

  mock_call_use_as_stub(TO_STR(Check_not_legal_response_job));

  static SProtocolInfo m_ProtocolInfo = {
    .CommandClassVersions.SecurityVersion = 0,                                         // CommandClassVersions are setup run time
    .CommandClassVersions.Security2Version = 0,
    .CommandClassVersions.TransportServiceVersion = 0,
    .ProtocolVersion.Major = ZW_VERSION_MAJOR,
    .ProtocolVersion.Minor = ZW_VERSION_MINOR,
    .ProtocolVersion.Revision = ZW_VERSION_PATCH,
    .eProtocolType = EPROTOCOLTYPE_ZWAVE,
    .eLibraryType = ELIBRARYTYPE_DUT
  };
  SApplicationHandles appHandles;
  appHandles.pProtocolInfo = &m_ProtocolInfo;

  mock_call_expect(TO_STR(ZAF_getAppHandle), &pMock);
  pMock->return_code.p = &appHandles;

  uint8_t i;
  VG_VERSION_REPORT_V2_VG firmwareVersion[NUMBER_OF_FIRMWARE_TARGETS];
  for (i = 0; i < NUMBER_OF_FIRMWARE_TARGETS; i++) {
    version_list[i].major = 0xA0 | i;
    version_list[i].minor = 0xB0 | i;

    if (0 == i) {
      mock_call_expect(TO_STR(zpal_get_app_version_major), &pMock);
      pMock->return_code.v = version_list[i].major;
      mock_call_expect(TO_STR(zpal_get_app_version_minor), &pMock);
      pMock->return_code.v = version_list[i].minor;
    } else {
      mock_call_expect(TO_STR(CC_Version_GetFirmwareVersion_handler), &pMock);
      pMock->expect_arg[0].v = i;
      pMock->compare_rule_arg[1] = COMPARE_NOT_NULL;
      firmwareVersion[i].firmwareVersion = version_list[i].major;
      firmwareVersion[i].firmwareSubVersion = version_list[i].minor;
      pMock->output_arg[1].p = &firmwareVersion[i];
    }
  }

  mock_call_expect(TO_STR(zaf_config_get_hardware_version), &pMock);
  pMock->return_code.v = 0x51; // Hardware version

  mock_call_expect(TO_STR(zaf_config_get_firmware_target_count), &pMock);
  pMock->return_code.v = NUMBER_OF_FIRMWARE_TARGETS;

  uint8_t expectedFrame[] = {
    0x86,   // COMMAND_CLASS_VERSION_V3
    0x12,   // VERSION_REPORT_V2
    ZW_TYPE_LIBRARY,
    ZW_VERSION_MAJOR,
    ZW_VERSION_MINOR,
    version_list[0].major,
    version_list[0].minor,
    0x51,   // Hardware version
    NUMBER_OF_FIRMWARE_TARGETS - 1,
    version_list[1].major,
    version_list[1].minor,
    version_list[2].major,
    version_list[2].minor,
    version_list[3].major,
    version_list[3].minor,
    version_list[4].major,
    version_list[4].minor,
  };

  ZW_APPLICATION_TX_BUFFER frameOut;
  uint8_t frameOutLength;
  received_frame_status_t res = invoke_cc_handler_v2(&rxOptions, &frame,
                                                     commandLength, &frameOut,
                                                     &frameOutLength);

  TEST_ASSERT_EQUAL_UINT_MESSAGE(RECEIVED_FRAME_STATUS_SUCCESS,
                                 res,
                                 "return code from invoke_cc_handler_v2(...)");
  TEST_ASSERT_EQUAL_INT8_MESSAGE(frameOutLength,
                                 sizeof(expectedFrame),
                                 "Frame size does not match");
  TEST_ASSERT_EQUAL_INT8_ARRAY_MESSAGE(expectedFrame, &frameOut,
                                       sizeof(expectedFrame),
                                       "Frame does not match");

  mock_calls_verify();
}

void test_VERSION_COMMAND_CLASS_MIGRATION_CAPABILITES_GET_migration_not_supported(void)
{
  mock_calls_clear();

  RECEIVE_OPTIONS_TYPE_EX rxOptions;
  memset((uint8_t *)&rxOptions, 0x00, sizeof(RECEIVE_OPTIONS_TYPE_EX));
  ZW_APPLICATION_TX_BUFFER frame;
  uint8_t * pFrame = (uint8_t *)&frame;
  uint8_t commandLength = 0;
  uint8_t frameCount = 0;

  memset(pFrame, 0x00, sizeof(ZW_APPLICATION_TX_BUFFER));
  pFrame[frameCount++] = COMMAND_CLASS_VERSION_V4;
  pFrame[frameCount++] = VERSION_MIGRATION_CAPABILITIES_GET_V4;
  commandLength = frameCount;

  mock_call_use_as_stub(TO_STR(Check_not_legal_response_job));
  cc_version_is_migration_supported_ExpectAndReturn(false);

  ZW_APPLICATION_TX_BUFFER frameOut;
  uint8_t frameOutLength;
  received_frame_status_t res = invoke_cc_handler_v2(&rxOptions, &frame,
                                                     commandLength, &frameOut,
                                                     &frameOutLength);

  TEST_ASSERT_EQUAL_UINT_MESSAGE(RECEIVED_FRAME_STATUS_NO_SUPPORT,
                                 res,
                                 "return code from invoke_cc_handler_v2(...)");
  mock_calls_verify();
}

/************************************************
 * Test Migration Capabilities handling functions
 ***********************************************/

void test_VERSION_COMMAND_CLASS_MIGRATION_CAPABILITES_GET_1_migration(void)
{
  mock_calls_clear();

  RECEIVE_OPTIONS_TYPE_EX rxOptions;
  memset((uint8_t *)&rxOptions, 0x00, sizeof(RECEIVE_OPTIONS_TYPE_EX));
  ZW_APPLICATION_TX_BUFFER frame;
  uint8_t * pFrame = (uint8_t *)&frame;
  uint8_t commandLength = 0;
  uint8_t frameCount = 0;

  memset(pFrame, 0x00, sizeof(ZW_APPLICATION_TX_BUFFER));
  pFrame[frameCount++] = COMMAND_CLASS_VERSION_V4;
  pFrame[frameCount++] = VERSION_MIGRATION_CAPABILITIES_GET_V4;
  commandLength = frameCount;

  mock_call_use_as_stub(TO_STR(Check_not_legal_response_job));

  /* Test 1 migration */
  uint8_t expectedFrame[] = {
    COMMAND_CLASS_VERSION_V4,                 // 0x86
    VERSION_MIGRATION_CAPABILITIES_REPORT_V4, // 0x1A
    0x01, // 1 migration supported
    0x01, // User Code to U3C Migration
  };

  /* Set up run 1 version mocks */
  cc_version_is_migration_supported_ExpectAndReturn(true);
  cc_version_is_migration_operation_supported_ExpectAndReturn(CC_VERSION_MIGRATION_OPERATION_USER_CODE_TO_U3C, true);
  cc_version_is_migration_operation_supported_ExpectAndReturn(CC_VERSION_MIGRATION_OPERATION_U3C_TO_USER_CODE, false);

  ZW_APPLICATION_TX_BUFFER frameOut;
  uint8_t frameOutLength;
  received_frame_status_t res = invoke_cc_handler_v2(&rxOptions, &frame,
                                                     commandLength, &frameOut,
                                                     &frameOutLength);

  TEST_ASSERT_EQUAL_UINT_MESSAGE(RECEIVED_FRAME_STATUS_SUCCESS,
                                 res,
                                 "return code from invoke_cc_handler_v2(...)");
  TEST_ASSERT_EQUAL_INT8_MESSAGE(frameOutLength,
                                 sizeof(expectedFrame),
                                 "Frame size does not match");
  TEST_ASSERT_EQUAL_INT8_ARRAY_MESSAGE(expectedFrame, &frameOut,
                                       sizeof(expectedFrame),
                                       "Frame does not match");
  mock_calls_verify();

  /* Run 2 - check other case */
  mock_calls_clear();
  mock_call_use_as_stub(TO_STR(Check_not_legal_response_job));

  /* Test 1 migration */
  expectedFrame[3] = 0x02; // Test and ensure that the code still works if only the other operation is supported

  /* Set up run 2 version mocks */
  cc_version_is_migration_supported_ExpectAndReturn(true);
  cc_version_is_migration_operation_supported_ExpectAndReturn(CC_VERSION_MIGRATION_OPERATION_USER_CODE_TO_U3C, false);
  cc_version_is_migration_operation_supported_ExpectAndReturn(CC_VERSION_MIGRATION_OPERATION_U3C_TO_USER_CODE, true);

  res = invoke_cc_handler_v2(&rxOptions, &frame,
                             commandLength, &frameOut,
                             &frameOutLength);

  TEST_ASSERT_EQUAL_UINT_MESSAGE(RECEIVED_FRAME_STATUS_SUCCESS,
                                 res,
                                 "return code from invoke_cc_handler_v2(...)");
  TEST_ASSERT_EQUAL_INT8_MESSAGE(frameOutLength,
                                 sizeof(expectedFrame),
                                 "Frame size does not match");
  TEST_ASSERT_EQUAL_INT8_ARRAY_MESSAGE(expectedFrame, &frameOut,
                                       sizeof(expectedFrame),
                                       "Frame does not match");

  mock_calls_verify();
}

void test_VERSION_COMMAND_CLASS_MIGRATION_CAPABILITES_GET_2_migration(void)
{
  mock_calls_clear();

  RECEIVE_OPTIONS_TYPE_EX rxOptions;
  memset((uint8_t *)&rxOptions, 0x00, sizeof(RECEIVE_OPTIONS_TYPE_EX));
  ZW_APPLICATION_TX_BUFFER frame;
  uint8_t * pFrame = (uint8_t *)&frame;
  uint8_t commandLength = 0;
  uint8_t frameCount = 0;

  memset(pFrame, 0x00, sizeof(ZW_APPLICATION_TX_BUFFER));
  pFrame[frameCount++] = COMMAND_CLASS_VERSION_V4;
  pFrame[frameCount++] = VERSION_MIGRATION_CAPABILITIES_GET_V4;
  commandLength = frameCount;

  mock_call_use_as_stub(TO_STR(Check_not_legal_response_job));

  /* Test 1 migration */
  uint8_t expectedFrame[] = {
    COMMAND_CLASS_VERSION_V4,                 // 0x86
    VERSION_MIGRATION_CAPABILITIES_REPORT_V4, // 0x1A
    0x02, // 2 migrations supported
    0x01, // User Code to U3C Migration
    0x02, // User Code to U3C Migration
  };

  cc_version_is_migration_supported_ExpectAndReturn(true);
  cc_version_is_migration_operation_supported_ExpectAndReturn(CC_VERSION_MIGRATION_OPERATION_USER_CODE_TO_U3C, true);
  cc_version_is_migration_operation_supported_ExpectAndReturn(CC_VERSION_MIGRATION_OPERATION_U3C_TO_USER_CODE, true);

  ZW_APPLICATION_TX_BUFFER frameOut;
  uint8_t frameOutLength;
  received_frame_status_t res = invoke_cc_handler_v2(&rxOptions, &frame,
                                                     commandLength, &frameOut,
                                                     &frameOutLength);

  TEST_ASSERT_EQUAL_UINT_MESSAGE(RECEIVED_FRAME_STATUS_SUCCESS,
                                 res,
                                 "return code from invoke_cc_handler_v2(...)");
  TEST_ASSERT_EQUAL_INT8_MESSAGE(frameOutLength,
                                 sizeof(expectedFrame),
                                 "Frame size does not match");
  TEST_ASSERT_EQUAL_INT8_ARRAY_MESSAGE(expectedFrame, &frameOut,
                                       sizeof(expectedFrame),
                                       "Frame does not match");

  mock_calls_verify();
}

/************************************
 * Test Migration handling functions
 ***********************************/
void test_VERSION_MIGRATION_SET_migration_not_supported(void)
{
  mock_calls_clear();

  RECEIVE_OPTIONS_TYPE_EX rxOptions;
  memset((uint8_t *)&rxOptions, 0x00, sizeof(RECEIVE_OPTIONS_TYPE_EX));
  ZW_APPLICATION_TX_BUFFER frame;
  uint8_t * pFrame = (uint8_t *)&frame;
  uint8_t commandLength = 0;
  uint8_t frameCount = 0;

  memset(pFrame, 0x00, sizeof(ZW_APPLICATION_TX_BUFFER));
  pFrame[frameCount++] = COMMAND_CLASS_VERSION_V4;
  pFrame[frameCount++] = VERSION_MIGRATION_SET_V4;
  pFrame[frameCount++] = CC_VERSION_MIGRATION_OPERATION_U3C_TO_USER_CODE;
  commandLength = frameCount;

  mock_call_use_as_stub(TO_STR(Check_not_legal_response_job));

  cc_version_is_migration_supported_ExpectAndReturn(false);

  ZW_APPLICATION_TX_BUFFER frameOut;
  uint8_t frameOutLength;
  received_frame_status_t res = invoke_cc_handler_v2(&rxOptions, &frame,
                                                     commandLength, &frameOut,
                                                     &frameOutLength);

  TEST_ASSERT_EQUAL_UINT_MESSAGE(RECEIVED_FRAME_STATUS_NO_SUPPORT,
                                 res,
                                 "return code from invoke_cc_handler_v2(...)");

  mock_calls_verify();
}

void test_VERSION_MIGRATION_SET_operation_not_supported(void)
{
  mock_calls_clear();

  RECEIVE_OPTIONS_TYPE_EX rxOptions;
  memset((uint8_t *)&rxOptions, 0x00, sizeof(RECEIVE_OPTIONS_TYPE_EX));
  ZW_APPLICATION_TX_BUFFER frame;
  uint8_t * pFrame = (uint8_t *)&frame;
  uint8_t commandLength = 0;
  uint8_t frameCount = 0;

  memset(pFrame, 0x00, sizeof(ZW_APPLICATION_TX_BUFFER));
  pFrame[frameCount++] = COMMAND_CLASS_VERSION_V4;
  pFrame[frameCount++] = VERSION_MIGRATION_SET_V4;
  pFrame[frameCount++] = CC_VERSION_MIGRATION_OPERATION_U3C_TO_USER_CODE;
  commandLength = frameCount;

  mock_call_use_as_stub(TO_STR(Check_not_legal_response_job));

  cc_version_is_migration_supported_ExpectAndReturn(true);
  cc_version_is_migration_operation_supported_ExpectAndReturn(CC_VERSION_MIGRATION_OPERATION_U3C_TO_USER_CODE, false);

  ZW_APPLICATION_TX_BUFFER frameOut;
  uint8_t frameOutLength;
  received_frame_status_t res = invoke_cc_handler_v2(&rxOptions, &frame,
                                                     commandLength, &frameOut,
                                                     &frameOutLength);

  TEST_ASSERT_EQUAL_UINT_MESSAGE(RECEIVED_FRAME_STATUS_FAIL,
                                 res,
                                 "return code from invoke_cc_handler_v2(...)");

  mock_calls_verify();
}

void test_VERSION_MIGRATION_SET_start_fail(void)
{
  mock_calls_clear();

  RECEIVE_OPTIONS_TYPE_EX rxOptions;
  memset((uint8_t *)&rxOptions, 0x00, sizeof(RECEIVE_OPTIONS_TYPE_EX));
  ZW_APPLICATION_TX_BUFFER frame;
  uint8_t * pFrame = (uint8_t *)&frame;
  uint8_t commandLength = 0;
  uint8_t frameCount = 0;

  memset(pFrame, 0x00, sizeof(ZW_APPLICATION_TX_BUFFER));
  pFrame[frameCount++] = COMMAND_CLASS_VERSION_V4;
  pFrame[frameCount++] = VERSION_MIGRATION_SET_V4;
  pFrame[frameCount++] = CC_VERSION_MIGRATION_OPERATION_U3C_TO_USER_CODE;
  commandLength = frameCount;

  mock_call_use_as_stub(TO_STR(Check_not_legal_response_job));

  /* Operation is otherwise valid but fails to start on the application level */
  cc_version_is_migration_supported_ExpectAndReturn(true);
  cc_version_is_migration_operation_supported_ExpectAndReturn(CC_VERSION_MIGRATION_OPERATION_U3C_TO_USER_CODE, true);
  CC_Version_Migration_Start_ExpectAndReturn(CC_VERSION_MIGRATION_OPERATION_U3C_TO_USER_CODE, false);

  ZW_APPLICATION_TX_BUFFER frameOut;
  uint8_t frameOutLength;
  received_frame_status_t res = invoke_cc_handler_v2(&rxOptions, &frame,
                                                     commandLength, &frameOut,
                                                     &frameOutLength);

  TEST_ASSERT_EQUAL_UINT_MESSAGE(RECEIVED_FRAME_STATUS_FAIL,
                                 res,
                                 "return code from invoke_cc_handler_v2(...)");

  mock_calls_verify();
}

void test_VERSION_MIGRATION_SET_valid(void)
{
  mock_calls_clear();

  RECEIVE_OPTIONS_TYPE_EX rxOptions;
  memset((uint8_t *)&rxOptions, 0x00, sizeof(RECEIVE_OPTIONS_TYPE_EX));
  ZW_APPLICATION_TX_BUFFER frame;
  uint8_t * pFrame = (uint8_t *)&frame;
  uint8_t commandLength = 0;
  uint8_t frameCount = 0;

  memset(pFrame, 0x00, sizeof(ZW_APPLICATION_TX_BUFFER));
  pFrame[frameCount++] = COMMAND_CLASS_VERSION_V4;
  pFrame[frameCount++] = VERSION_MIGRATION_SET_V4;
  pFrame[frameCount++] = CC_VERSION_MIGRATION_OPERATION_U3C_TO_USER_CODE;
  commandLength = frameCount;

  mock_call_use_as_stub(TO_STR(Check_not_legal_response_job));

  cc_version_is_migration_supported_ExpectAndReturn(true);
  cc_version_is_migration_operation_supported_ExpectAndReturn(CC_VERSION_MIGRATION_OPERATION_U3C_TO_USER_CODE, true);
  CC_Version_Migration_Start_ExpectAndReturn(CC_VERSION_MIGRATION_OPERATION_U3C_TO_USER_CODE, true);

  ZW_APPLICATION_TX_BUFFER frameOut;
  uint8_t frameOutLength;
  received_frame_status_t res = invoke_cc_handler_v2(&rxOptions, &frame,
                                                     commandLength, &frameOut,
                                                     &frameOutLength);

  TEST_ASSERT_EQUAL_UINT_MESSAGE(RECEIVED_FRAME_STATUS_SUCCESS,
                                 res,
                                 "return code from invoke_cc_handler_v2(...)");

  mock_calls_verify();
}

void test_VERSION_migration_get_migration_not_supported(void)
{
  mock_calls_clear();

  RECEIVE_OPTIONS_TYPE_EX rxOptions;
  memset((uint8_t *)&rxOptions, 0x00, sizeof(RECEIVE_OPTIONS_TYPE_EX));
  ZW_APPLICATION_TX_BUFFER frame;
  uint8_t * pFrame = (uint8_t *)&frame;
  uint8_t commandLength = 0;
  uint8_t frameCount = 0;

  memset(pFrame, 0x00, sizeof(ZW_APPLICATION_TX_BUFFER));
  pFrame[frameCount++] = COMMAND_CLASS_VERSION_V4;
  pFrame[frameCount++] = VERSION_MIGRATION_GET_V4;
  pFrame[frameCount++] = CC_VERSION_MIGRATION_OPERATION_U3C_TO_USER_CODE;
  commandLength = frameCount;

  mock_call_use_as_stub(TO_STR(Check_not_legal_response_job));

  cc_version_is_migration_supported_ExpectAndReturn(false);
  ZW_APPLICATION_TX_BUFFER frameOut;
  uint8_t frameOutLength;
  received_frame_status_t res = invoke_cc_handler_v2(&rxOptions, &frame,
                                                     commandLength, &frameOut,
                                                     &frameOutLength);

  TEST_ASSERT_EQUAL_UINT_MESSAGE(RECEIVED_FRAME_STATUS_NO_SUPPORT,
                                 res,
                                 "return code from invoke_cc_handler_v2(...)");

  mock_calls_verify();
}

void test_VERSION_migration_get_migration_operation_not_supported(void)
{
  mock_calls_clear();

  RECEIVE_OPTIONS_TYPE_EX rxOptions;
  memset((uint8_t *)&rxOptions, 0x00, sizeof(RECEIVE_OPTIONS_TYPE_EX));
  ZW_APPLICATION_TX_BUFFER frame;
  uint8_t * pFrame = (uint8_t *)&frame;
  uint8_t commandLength = 0;
  uint8_t frameCount = 0;

  memset(pFrame, 0x00, sizeof(ZW_APPLICATION_TX_BUFFER));
  pFrame[frameCount++] = COMMAND_CLASS_VERSION_V4;
  pFrame[frameCount++] = VERSION_MIGRATION_GET_V4;
  pFrame[frameCount++] = CC_VERSION_MIGRATION_OPERATION_U3C_TO_USER_CODE;
  commandLength = frameCount;

  mock_call_use_as_stub(TO_STR(Check_not_legal_response_job));

  cc_version_is_migration_supported_ExpectAndReturn(true);
  cc_version_is_migration_operation_supported_ExpectAndReturn(CC_VERSION_MIGRATION_OPERATION_U3C_TO_USER_CODE, false);
  ZW_APPLICATION_TX_BUFFER frameOut;
  uint8_t frameOutLength;
  received_frame_status_t res = invoke_cc_handler_v2(&rxOptions, &frame,
                                                     commandLength, &frameOut,
                                                     &frameOutLength);

  TEST_ASSERT_EQUAL_UINT_MESSAGE(RECEIVED_FRAME_STATUS_FAIL,
                                 res,
                                 "return code from invoke_cc_handler_v2(...)");

  mock_calls_verify();
}

void test_VERSION_migration_get_valid_but_failed(void)
{
  mock_calls_clear();

  RECEIVE_OPTIONS_TYPE_EX rxOptions;
  memset((uint8_t *)&rxOptions, 0x00, sizeof(RECEIVE_OPTIONS_TYPE_EX));
  ZW_APPLICATION_TX_BUFFER frame;
  uint8_t * pFrame = (uint8_t *)&frame;
  uint8_t commandLength = 0;
  uint8_t frameCount = 0;

  memset(pFrame, 0x00, sizeof(ZW_APPLICATION_TX_BUFFER));
  pFrame[frameCount++] = COMMAND_CLASS_VERSION_V4;
  pFrame[frameCount++] = VERSION_MIGRATION_GET_V4;
  pFrame[frameCount++] = CC_VERSION_MIGRATION_OPERATION_U3C_TO_USER_CODE;
  commandLength = frameCount;

  mock_call_use_as_stub(TO_STR(Check_not_legal_response_job));

  cc_version_is_migration_supported_ExpectAndReturn(true);
  cc_version_is_migration_operation_supported_ExpectAndReturn(CC_VERSION_MIGRATION_OPERATION_U3C_TO_USER_CODE, true);
  CC_Version_Migration_GetStatus_ExpectAnyArgsAndReturn(false);
  ZW_APPLICATION_TX_BUFFER frameOut;
  uint8_t frameOutLength;
  received_frame_status_t res = invoke_cc_handler_v2(&rxOptions, &frame,
                                                     commandLength, &frameOut,
                                                     &frameOutLength);

  TEST_ASSERT_EQUAL_UINT_MESSAGE(RECEIVED_FRAME_STATUS_FAIL,
                                 res,
                                 "return code from invoke_cc_handler_v2(...)");

  mock_calls_verify();
}

void test_VERSION_migration_get_valid(void)
{
  mock_calls_clear();

  RECEIVE_OPTIONS_TYPE_EX rxOptions;
  memset((uint8_t *)&rxOptions, 0x00, sizeof(RECEIVE_OPTIONS_TYPE_EX));
  ZW_APPLICATION_TX_BUFFER frame;
  uint8_t * pFrame = (uint8_t *)&frame;
  uint8_t commandLength = 0;
  uint8_t frameCount = 0;

  memset(pFrame, 0x00, sizeof(ZW_APPLICATION_TX_BUFFER));
  pFrame[frameCount++] = COMMAND_CLASS_VERSION_V4;
  pFrame[frameCount++] = VERSION_MIGRATION_GET_V4;
  pFrame[frameCount++] = CC_VERSION_MIGRATION_OPERATION_U3C_TO_USER_CODE;
  commandLength = frameCount;

  mock_call_use_as_stub(TO_STR(Check_not_legal_response_job));

  cc_version_migration_operation_state_t state = {
    .remaining_time = 0x30,
    .status = MIGRATION_STATUS_IN_PROGRESS,
  };

  cc_version_is_migration_supported_ExpectAndReturn(true);
  cc_version_is_migration_operation_supported_ExpectAndReturn(CC_VERSION_MIGRATION_OPERATION_U3C_TO_USER_CODE, true);
  CC_Version_Migration_GetStatus_ExpectAndReturn(CC_VERSION_MIGRATION_OPERATION_U3C_TO_USER_CODE, NULL, true);
  CC_Version_Migration_GetStatus_IgnoreArg_state();
  CC_Version_Migration_GetStatus_ReturnMemThruPtr_state(&state, sizeof(state));
  CC_Version_Migration_GetStatus_StopIgnore();

  uint8_t expectedFrame[] = {
    COMMAND_CLASS_VERSION_V4,
    VERSION_MIGRATION_REPORT_V4,
    CC_VERSION_MIGRATION_OPERATION_U3C_TO_USER_CODE,
    VERSION_MIGRATION_REPORT_IN_PROGRESS_V4,
    0x00,
    0x30
  };

  ZW_APPLICATION_TX_BUFFER frameOut;
  uint8_t frameOutLength;
  received_frame_status_t res = invoke_cc_handler_v2(&rxOptions, &frame,
                                                     commandLength, &frameOut,
                                                     &frameOutLength);

  TEST_ASSERT_EQUAL_UINT_MESSAGE(RECEIVED_FRAME_STATUS_SUCCESS,
                                 res,
                                 "return code from invoke_cc_handler_v2(...)");
  TEST_ASSERT_EQUAL_INT8_MESSAGE(frameOutLength,
                                 sizeof(expectedFrame),
                                 "Frame size does not match");
  TEST_ASSERT_EQUAL_INT8_ARRAY_MESSAGE(expectedFrame, &frameOut,
                                       sizeof(expectedFrame),
                                       "Frame does not match");

  mock_calls_verify();
}

static command_handler_input_t * version_command_class_get_frame_create(uint8_t commandClass)
{
  command_handler_input_t * p_chi = test_common_command_handler_input_allocate();
  p_chi->frame.as_byte_array[p_chi->frameLength++] = COMMAND_CLASS_VERSION;
  p_chi->frame.as_byte_array[p_chi->frameLength++] = VERSION_COMMAND_CLASS_GET;
  p_chi->frame.as_byte_array[p_chi->frameLength++] = commandClass;
  return p_chi;
}
