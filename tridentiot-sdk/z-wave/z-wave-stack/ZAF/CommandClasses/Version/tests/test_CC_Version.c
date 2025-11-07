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

void setUpSuite(void) {

}

void tearDownSuite(void) {

}

void setUp(void) {

}

void tearDown(void) {

}

static command_handler_input_t * version_command_class_get_frame_create(uint8_t commandClass);

void test_VERSION_CAPABILITIES_GET_V3(void)
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

  uint8_t expectedFrame[] = {
      0x86, // COMMAND_CLASS_VERSION_V3
      0x16, // VERSION_CAPABILITIES_REPORT_V3
      0x07
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
                                           .CommandClassVersions.SecurityVersion = 0,  // CommandClassVersions are setup run time
                                           .CommandClassVersions.Security2Version = 0,
                                           .CommandClassVersions.TransportServiceVersion = 0,
                                           .ProtocolVersion.Major = ZW_VERSION_MAJOR,
                                           .ProtocolVersion.Minor = ZW_VERSION_MINOR,
                                           .ProtocolVersion.Revision = ZW_VERSION_PATCH,
                                           .eProtocolType = EPROTOCOLTYPE_ZWAVE,
                                           .eLibraryType = 0                           // Library type is setup run time
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
      0x86, // COMMAND_CLASS_VERSION_V3
      0x18, // VERSION_ZWAVE_SOFTWARE_REPORT_V3

      SDK_VERSION_MAJOR, // SDK version major
      SDK_VERSION_MINOR, // SDK version minor
      SDK_VERSION_PATCH, // SDK version patch

      ZAF_VERSION_MAJOR, // ZAF version major
      ZAF_VERSION_MINOR, // ZAF version minor
      ZAF_VERSION_PATCH, // ZAF version patch
      (uint8_t)(ZAF_BUILD_NO >> 8), // ZAF build number MSB
      (uint8_t)ZAF_BUILD_NO, // ZAF build number LSB

      0x00, // Host interface version major
      0x00, // Host interface version minor
      0x00, // Host interface version patch
      0x00, // Host interface build number MSB
      0x00, // Host interface build number LSB

      ZW_VERSION_MAJOR, // ZW version major
      ZW_VERSION_MINOR, // ZW version minor
      ZW_VERSION_PATCH, // ZW version patch
      (uint8_t)(ZW_BUILD_NO >> 8), // ZW build number MSB
      (uint8_t)ZW_BUILD_NO, // ZW build number LSB

      0x13, // Application version major
      0x14, // Application version minor
      0x15, // Application version patch
      0x16, // Application build number MSB
      0x17, // Application build number LSB
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
                                           .CommandClassVersions.SecurityVersion = 0,  // CommandClassVersions are setup run time
                                           .CommandClassVersions.Security2Version = 0,
                                           .CommandClassVersions.TransportServiceVersion = 0,
                                           .ProtocolVersion.Major = ZW_VERSION_MAJOR,
                                           .ProtocolVersion.Minor = ZW_VERSION_MINOR,
                                           .ProtocolVersion.Revision = ZW_VERSION_PATCH,
                                           .eProtocolType = EPROTOCOLTYPE_ZWAVE,
                                           .eLibraryType = 0                           // Library type is setup run time
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
      0x86, // COMMAND_CLASS_VERSION_V3
      0x18, // VERSION_ZWAVE_SOFTWARE_REPORT_V3

      SDK_VERSION_MAJOR, // SDK version major
      SDK_VERSION_MINOR, // SDK version minor
      SDK_VERSION_PATCH, // SDK version patch

      ZAF_VERSION_MAJOR, // ZAF version major
      ZAF_VERSION_MINOR, // ZAF version minor
      ZAF_VERSION_PATCH, // ZAF version patch
      (uint8_t)(ZAF_BUILD_NO >> 8), // ZAF build number MSB
      (uint8_t) ZAF_BUILD_NO, // ZAF build number LSB

      0, // Host interface version major
      0, // Host interface version minor
      0, // Host interface version patch
      0, // Host interface build number MSB
      0, // Host interface build number LSB

      ZW_VERSION_MAJOR,
      ZW_VERSION_MINOR,
      ZW_VERSION_PATCH,
      (uint8_t)(ZW_BUILD_NO >> 8), // ZW build number MSB
      (uint8_t) ZW_BUILD_NO, // ZW build number LSB

      0, // Application version major
      0, // Application version minor
      0, // Application version patch
      0, // Application build number MSB
      0, // Application build number LSB
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


typedef struct
{
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
    {.cc = COMMAND_CLASS_VERSION,           .expected_version = 3, .isSpecialCase = false, .in_nif = false}, // Last value is "don't care"

    // Verifies correct versions in case the following CCs are
    // listed in the NIF.
    {.cc = COMMAND_CLASS_TRANSPORT_SERVICE, .expected_version = 4, .isSpecialCase = true,  .in_nif = true},
    {.cc = COMMAND_CLASS_SECURITY,          .expected_version = 5, .isSpecialCase = true,  .in_nif = true},
    {.cc = COMMAND_CLASS_SECURITY_2,        .expected_version = 6, .isSpecialCase = true,  .in_nif = true},

    // Verifies that zero is returned in case none of the following
    // CCs are listed in the NIF.
    {.cc = COMMAND_CLASS_TRANSPORT_SERVICE, .expected_version = 0, .isSpecialCase = true,  .in_nif = false},
    {.cc = COMMAND_CLASS_SECURITY,          .expected_version = 0, .isSpecialCase = true,  .in_nif = false},
    {.cc = COMMAND_CLASS_SECURITY_2,        .expected_version = 0, .isSpecialCase = true,  .in_nif = false},

    // Verifies that zero is returned in case of an invalid CC version.
    {.cc = 0xFF,                            .expected_version = 0, .isSpecialCase = true,  .in_nif = false} // Last value is "don't care"
  };


  SProtocolInfo protocolInfo;
  protocolInfo.CommandClassVersions.TransportServiceVersion = cc_versions[1].expected_version;
  protocolInfo.CommandClassVersions.SecurityVersion         = cc_versions[2].expected_version;
  protocolInfo.CommandClassVersions.Security2Version        = cc_versions[3].expected_version;

  SApplicationHandles appHandles;
  appHandles.pProtocolInfo = &protocolInfo;

  mock_call_use_as_stub(TO_STR(Check_not_legal_response_job));

  for (uint8_t i = 0; i < sizeof_array(cc_versions); i++)
  {
    command_handler_input_t * p_chi = version_command_class_get_frame_create(cc_versions[i].cc);

    mock_call_expect(TO_STR(ZAF_getAppHandle), &pMock);
    pMock->return_code.p = &appHandles;

    uint8_t nif;
    if (true == cc_versions[i].in_nif)
    {
      nif = cc_versions[i].cc;
    }
    else
    {
      nif = 0xFF; // Random CC
    }
    zaf_cc_list_t CommandClassList;
    CommandClassList.cc_list = &nif;
    CommandClassList.list_size = sizeof(nif);

    mock_call_expect(TO_STR(GetCommandClassList), &pMock);
    pMock->return_code.p = &CommandClassList;

    uint8_t expectedFrame[] = {
        0x86, // COMMAND_CLASS_VERSION_V3
        0x14, // VERSION_COMMAND_CLASS_REPORT_V2
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

  typedef struct
  {
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
                                           .CommandClassVersions.SecurityVersion = 0,  // CommandClassVersions are setup run time
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
  for (i = 0; i < NUMBER_OF_FIRMWARE_TARGETS; i++)
  {
    version_list[i].major = 0xA0 | i;
    version_list[i].minor = 0xB0 | i;

    if (0 == i)
    {
      mock_call_expect(TO_STR(zpal_get_app_version_major), &pMock);
      pMock->return_code.v = version_list[i].major;
      mock_call_expect(TO_STR(zpal_get_app_version_minor), &pMock);
      pMock->return_code.v = version_list[i].minor;
    }
    else
    {
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
      0x86, // COMMAND_CLASS_VERSION_V3
      0x12, // VERSION_REPORT_V2
      ZW_TYPE_LIBRARY,
      ZW_VERSION_MAJOR,
      ZW_VERSION_MINOR,
      version_list[0].major,
      version_list[0].minor,
      0x51, // Hardware version
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

static command_handler_input_t * version_command_class_get_frame_create(uint8_t commandClass)
{
  command_handler_input_t * p_chi = test_common_command_handler_input_allocate();
  p_chi->frame.as_byte_array[p_chi->frameLength++] = COMMAND_CLASS_VERSION;
  p_chi->frame.as_byte_array[p_chi->frameLength++] = VERSION_COMMAND_CLASS_GET;
  p_chi->frame.as_byte_array[p_chi->frameLength++] = commandClass;
  return p_chi;
}
