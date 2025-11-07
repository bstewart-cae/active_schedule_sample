/**
 * @file test_CC_FirmwareUdate.c
 * @copyright 2022 Silicon Laboratories Inc.
 */
#include <mock_control.h>
#include <ZW_TransportEndpoint.h>
#include <string.h>
#include "ota_util.h"
#include <test_common.h>
#include <ZW_application_transport_interface.h>
#include <SizeOf.h>
#include <SwTimer.h>
#include <CRC.h>
#include <ZAF_file_ids.h>
#include <zpal_status.h>
#include <zpal_bootloader.h>
#include <zpal_nvm.h>
#include <ZAF_Common_helper_mock.h>
#include "zaf_transport_tx_mock.h"
//#define DEBUGPRINT
#ifdef DEBUGPRINT
#include <CRC.h>
#include <stdio.h>
#endif

void setUpSuite(void) {

}

void tearDownSuite(void) {

}

void tearDown(void) {

}


static void (*g_request_get_cb)(transmission_result_t * pTxResult);
static bool zaf_transport_tx_callback(const uint8_t* frame, uint8_t frame_length,
                                zaf_tx_callback_t callback,
                                zaf_tx_options_t* zaf_tx_options,
                                int cmock_num_calls)
{
  (void) frame;
  (void) frame_length;
  (void) zaf_tx_options;
  (void) cmock_num_calls;

  g_request_get_cb = callback;

  return true;
}

void setUp(void)
{
  mock_calls_clear();
  mock_call_use_as_stub(TO_STR(zpal_pm_register));
  mock_call_use_as_stub(TO_STR(zpal_pm_stay_awake));
  zaf_transport_tx_AddCallback(zaf_transport_tx_callback);
}

#include "ZAF_CC_Invoker.h"
#define handleCommandClassFWUpdate(a,b,c) invoke_cc_handler_v2(a,b,c,NULL,NULL)

#define TIMER_START_FWUPDATE_FRAME_GET 10000 // 10 sec
// Number of retries receiving the same fragment before aborting
#define FIRMWARE_UPDATE_MAX_RETRY 10

#define OTA_CACHE_SIZE 200

#define ZAF_FILE_SIZE_CC_FIRMWARE_UPDATE 16

/// Callback from zaf_transport_tx() from FW Update Request Get
static void (*request_get_cb)(transmission_result_t * pTxResult);


/*****************************************************************************
 *                            STATIC FUNCTIONS
 ****************************************************************************/

static command_handler_input_t * firmware_update_activation_set_frame_create(
    uint16_t manufacturerID,
    uint16_t firmwareID,
    uint16_t checksum,
    uint8_t firmware_target,
    uint8_t hardware_version)
{
  command_handler_input_t * p_chi = test_common_command_handler_input_allocate();
  p_chi->frame.as_byte_array[p_chi->frameLength++] = COMMAND_CLASS_FIRMWARE_UPDATE_MD_V3;
  p_chi->frame.as_byte_array[p_chi->frameLength++] = FIRMWARE_UPDATE_ACTIVATION_SET_V5;
  p_chi->frame.as_byte_array[p_chi->frameLength++] = (uint8_t)(manufacturerID >> 8);
  p_chi->frame.as_byte_array[p_chi->frameLength++] = (uint8_t)manufacturerID;
  p_chi->frame.as_byte_array[p_chi->frameLength++] = (uint8_t)(firmwareID >> 8);
  p_chi->frame.as_byte_array[p_chi->frameLength++] = (uint8_t)firmwareID;
  p_chi->frame.as_byte_array[p_chi->frameLength++] = (uint8_t)(checksum >> 8);
  p_chi->frame.as_byte_array[p_chi->frameLength++] = (uint8_t)checksum;
  p_chi->frame.as_byte_array[p_chi->frameLength++] = firmware_target;
  p_chi->frame.as_byte_array[p_chi->frameLength++] = hardware_version;
  return p_chi;
}

static command_handler_input_t * firmware_update_meta_data_report_frame_create(
    bool last_fragment,
    uint16_t report_number,
    uint16_t fragment_size)
{
  command_handler_input_t * p_chi = test_common_command_handler_input_allocate();
  p_chi->frame.as_byte_array[p_chi->frameLength++] = COMMAND_CLASS_FIRMWARE_UPDATE_MD_V5;
  p_chi->frame.as_byte_array[p_chi->frameLength++] = FIRMWARE_UPDATE_MD_REPORT_V5;
  p_chi->frame.as_byte_array[p_chi->frameLength] = (uint8_t)(report_number >> 8);
  p_chi->frame.as_byte_array[p_chi->frameLength++] |= (last_fragment) ? 0x80 : 0;
  p_chi->frame.as_byte_array[p_chi->frameLength++] = (uint8_t)report_number;
  for (uint8_t i = 0; i < fragment_size; i++)
  {
    p_chi->frame.as_byte_array[p_chi->frameLength++] = i;
  }
  uint16_t crc_frame = CRC_CheckCrc16(CRC_INITAL_VALUE, p_chi->frame.as_byte_array, 4 + fragment_size);
  p_chi->frame.as_byte_array[p_chi->frameLength++] = (uint8_t)(crc_frame >> 8);
  p_chi->frame.as_byte_array[p_chi->frameLength++] = (uint8_t)crc_frame;
#ifdef DEBUGPRINT
  printf("\n================ FW MD REPORT FRAME CREATE ====================\n");
  printf("Frame: ");
  for (uint8_t j = 0; j < p_chi->frameLength; j++)
  {
    printf(" 0x%02X", p_chi->frame.as_byte_array[j]);
  }
  printf(" (Length: %u)", p_chi->frameLength);
  printf(" (CRC frame data: 0x%04X)", crc_frame);
  printf(" (CRC total: 0x%04X)\n", CRC_CheckCrc16(CRC_INITAL_VALUE, p_chi->frame.as_byte_array, 6 + fragment_size));
  printf(" (CRC fragment: 0x%04X)\n", CRC_CheckCrc16(CRC_INITAL_VALUE, p_chi->frame.as_byte_array + 4, fragment_size));
  printf("==============================================================\n");
#endif
  return p_chi;
}

static command_handler_input_t * firmware_update_meta_data_get_frame_create(void)
{
  command_handler_input_t * p_chi = test_common_command_handler_input_allocate();
  p_chi->frame.as_byte_array[p_chi->frameLength++] = COMMAND_CLASS_FIRMWARE_UPDATE_MD_V5;
  p_chi->frame.as_byte_array[p_chi->frameLength++] = FIRMWARE_MD_GET_V5;
  return p_chi;
}

static command_handler_input_t * firmware_update_md_request_get_v3_frame_create(
    uint16_t manufacturerID,
    uint16_t firmwareID,
    uint16_t checksum,
    uint8_t firmware_target,
    uint16_t fragment_size)
{
  command_handler_input_t * p_chi = test_common_command_handler_input_allocate();
  p_chi->frame.as_byte_array[p_chi->frameLength++] = COMMAND_CLASS_FIRMWARE_UPDATE_MD_V3;
  p_chi->frame.as_byte_array[p_chi->frameLength++] = FIRMWARE_UPDATE_MD_REQUEST_GET_V3;
  p_chi->frame.as_byte_array[p_chi->frameLength++] = (uint8_t)(manufacturerID >> 8);
  p_chi->frame.as_byte_array[p_chi->frameLength++] = (uint8_t)manufacturerID;
  p_chi->frame.as_byte_array[p_chi->frameLength++] = (uint8_t)(firmwareID >> 8);
  p_chi->frame.as_byte_array[p_chi->frameLength++] = (uint8_t)firmwareID;
  p_chi->frame.as_byte_array[p_chi->frameLength++] = (uint8_t)(checksum >> 8);
  p_chi->frame.as_byte_array[p_chi->frameLength++] = (uint8_t)checksum;
  p_chi->frame.as_byte_array[p_chi->frameLength++] = firmware_target;
  p_chi->frame.as_byte_array[p_chi->frameLength++] = (uint8_t)(fragment_size >> 8);
  p_chi->frame.as_byte_array[p_chi->frameLength++] = (uint8_t)fragment_size;
  return p_chi;
}

static command_handler_input_t * firmware_update_md_request_get_v4_frame_create(
    uint16_t manufacturerID,
    uint16_t firmwareID,
    uint16_t checksum,
    uint8_t firmware_target,
    uint16_t fragment_size,
    bool activation)
{
  command_handler_input_t * p_chi = test_common_command_handler_input_allocate();
  p_chi->frame.as_byte_array[p_chi->frameLength++] = COMMAND_CLASS_FIRMWARE_UPDATE_MD_V4;
  p_chi->frame.as_byte_array[p_chi->frameLength++] = FIRMWARE_UPDATE_MD_REQUEST_GET_V4;
  p_chi->frame.as_byte_array[p_chi->frameLength++] = (uint8_t)(manufacturerID >> 8);
  p_chi->frame.as_byte_array[p_chi->frameLength++] = (uint8_t)manufacturerID;
  p_chi->frame.as_byte_array[p_chi->frameLength++] = (uint8_t)(firmwareID >> 8);
  p_chi->frame.as_byte_array[p_chi->frameLength++] = (uint8_t)firmwareID;
  p_chi->frame.as_byte_array[p_chi->frameLength++] = (uint8_t)(checksum >> 8);
  p_chi->frame.as_byte_array[p_chi->frameLength++] = (uint8_t)checksum;
  p_chi->frame.as_byte_array[p_chi->frameLength++] = firmware_target;
  p_chi->frame.as_byte_array[p_chi->frameLength++] = (uint8_t)(fragment_size >> 8);
  p_chi->frame.as_byte_array[p_chi->frameLength++] = (uint8_t)fragment_size;
  p_chi->frame.as_byte_array[p_chi->frameLength++] = (uint8_t)activation & 0x01;
  return p_chi;
}

static command_handler_input_t * firmware_update_md_request_get_v5_frame_create(
    uint16_t manufacturerID,
    uint16_t firmwareID,
    uint16_t checksum,
    uint8_t firmware_target,
    uint16_t fragment_size,
    bool activation,
    uint8_t hardwareVersion)
{
  command_handler_input_t * p_chi = test_common_command_handler_input_allocate();
  p_chi->frame.as_byte_array[p_chi->frameLength++] = COMMAND_CLASS_FIRMWARE_UPDATE_MD_V5;
  p_chi->frame.as_byte_array[p_chi->frameLength++] = FIRMWARE_UPDATE_MD_REQUEST_GET_V5;
  p_chi->frame.as_byte_array[p_chi->frameLength++] = (uint8_t)(manufacturerID >> 8);
  p_chi->frame.as_byte_array[p_chi->frameLength++] = (uint8_t)manufacturerID;
  p_chi->frame.as_byte_array[p_chi->frameLength++] = (uint8_t)(firmwareID >> 8);
  p_chi->frame.as_byte_array[p_chi->frameLength++] = (uint8_t)firmwareID;
  p_chi->frame.as_byte_array[p_chi->frameLength++] = (uint8_t)(checksum >> 8);
  p_chi->frame.as_byte_array[p_chi->frameLength++] = (uint8_t)checksum;
  p_chi->frame.as_byte_array[p_chi->frameLength++] = firmware_target;
  p_chi->frame.as_byte_array[p_chi->frameLength++] = (uint8_t)(fragment_size >> 8);
  p_chi->frame.as_byte_array[p_chi->frameLength++] = (uint8_t)fragment_size;
  p_chi->frame.as_byte_array[p_chi->frameLength++] = (uint8_t)activation & 0x01;
  p_chi->frame.as_byte_array[p_chi->frameLength++] = hardwareVersion;
  return p_chi;
}

static command_handler_input_t * meta_data_prepare_get_frame_create(
    uint16_t manufacturerID,
    uint16_t firmwareID,
    uint8_t firmware_target,
    uint16_t fragment_size,
    uint8_t hardware_version)
{
  command_handler_input_t * p_chi = test_common_command_handler_input_allocate();
  p_chi->frame.as_byte_array[p_chi->frameLength++] = COMMAND_CLASS_FIRMWARE_UPDATE_MD_V5;
  p_chi->frame.as_byte_array[p_chi->frameLength++] = FIRMWARE_UPDATE_MD_PREPARE_GET_V5;
  p_chi->frame.as_byte_array[p_chi->frameLength++] = (uint8_t)(manufacturerID >> 8);
  p_chi->frame.as_byte_array[p_chi->frameLength++] = (uint8_t)manufacturerID;
  p_chi->frame.as_byte_array[p_chi->frameLength++] = (uint8_t)(firmwareID >> 8);
  p_chi->frame.as_byte_array[p_chi->frameLength++] = (uint8_t)firmwareID;
  p_chi->frame.as_byte_array[p_chi->frameLength++] = firmware_target;
  p_chi->frame.as_byte_array[p_chi->frameLength++] = (uint8_t)(fragment_size >> 8);
  p_chi->frame.as_byte_array[p_chi->frameLength++] = (uint8_t)fragment_size;
  p_chi->frame.as_byte_array[p_chi->frameLength++] = hardware_version;
  return p_chi;
}

/// Helper function for calling and verifying FW Update Request Get,
/// when test purpose is something else.
static void call_FW_update_Request_Get(mock_t *pMock,
                                        uint16_t fragment_size,
                                        bool timers_needed
                                        )
{
  uint16_t MANUFACTURER_ID = 0xAABB;
  uint16_t PRODUCT_TYPE_ID = 0x00CC;
  uint16_t PRODUCT_ID = 0x00DD;
  const uint16_t FIRMWARE_ID = 0xCCDD;
  const uint16_t CHECKSUM = 0xEEFF;
  const uint8_t FIRMWARE_TARGET = 0;
  const bool ACTIVATION = true;
  const uint8_t HARDWARE_VERSION = 1;

  mock_call_use_as_stub(TO_STR(zaf_config_get_firmware_target_count));

  mock_call_expect(TO_STR(ZAF_getAppHandle), &pMock);
  SApplicationHandles AppHandles;
  SNetworkInfo NetworkInfo;
  AppHandles.pNetworkInfo = &NetworkInfo;
  NetworkInfo.MaxPayloadSize = 58; //We have tests where FRAGMENT_SIZE is up to 50 so we need this much.
  pMock->return_code.p = &AppHandles;

  mock_call_expect(TO_STR(zaf_config_get_hardware_version), &pMock);
  pMock->return_code.v = HARDWARE_VERSION;

  mock_call_expect(TO_STR(zaf_config_get_manufacturer_id), &pMock);
  pMock->return_code.v = MANUFACTURER_ID;

  mock_call_expect(TO_STR(zaf_config_get_product_type_id), &pMock);
  pMock->return_code.v = PRODUCT_TYPE_ID;

  mock_call_expect(TO_STR(zaf_config_get_product_id), &pMock);
  pMock->return_code.v = PRODUCT_ID;

  static uint8_t expectedFwMdRequestReport[] = {
                             COMMAND_CLASS_FIRMWARE_UPDATE_MD,
                             FIRMWARE_UPDATE_MD_REQUEST_REPORT_V5,
                             FIRMWARE_UPDATE_MD_REQUEST_REPORT_VALID_COMBINATION_V5
  };

  if(timers_needed)
  {
    mock_call_expect(TO_STR(TimerIsActive), &pMock);
    pMock->compare_rule_arg[0] = COMPARE_NOT_NULL;
    pMock->return_code.v = false;

    mock_call_expect(TO_STR(TimerStart), &pMock);
    pMock->compare_rule_arg[0] = COMPARE_NOT_NULL;
    pMock->expect_arg[1].v = TIMER_START_FWUPDATE_FRAME_GET;
    pMock->return_code.v = ESWTIMER_STATUS_SUCCESS;
  }

  zaf_transport_rx_to_tx_options_Ignore();
  zaf_transport_tx_ExpectAndReturn(expectedFwMdRequestReport, sizeof(expectedFwMdRequestReport), NULL, NULL, true);
  zaf_transport_tx_IgnoreArg_callback();
  zaf_transport_tx_IgnoreArg_zaf_tx_options();

  command_handler_input_t * p_chi =
      firmware_update_md_request_get_v5_frame_create(MANUFACTURER_ID,
                                                     FIRMWARE_ID,
                                                     CHECKSUM,
                                                     FIRMWARE_TARGET,
                                                     fragment_size,
                                                     ACTIVATION,
                                                     HARDWARE_VERSION);

  received_frame_status_t status;
  ZW_APPLICATION_TX_BUFFER frameOut;
  uint8_t frameOutLength;

  status = invoke_cc_handler_v2(&p_chi->rxOptions,
                              &p_chi->frame.as_zw_application_tx_buffer,
                              p_chi->frameLength, &frameOut, &frameOutLength);
  TEST_ASSERT_MESSAGE(status == RECEIVED_FRAME_STATUS_SUCCESS, "Wrong frame status :(");

  test_common_command_handler_input_free(p_chi);
}

/*****************************************************************************
 *                             API FUNCTIONS
 ****************************************************************************/

void test_FIRMWARE_MD_GET_one_firmware_target(void)
{
  mock_t* pMock = NULL;

  const uint8_t HARDWARE_VERSION = 1;

  command_handler_input_t * p_chi = firmware_update_meta_data_get_frame_create();

  mock_call_use_as_stub(TO_STR(Check_not_legal_response_job));

  mock_call_expect(TO_STR(zaf_config_get_manufacturer_id), &pMock);
  pMock->return_code.v = APP_MANUFACTURER_ID;

  const uint16_t PRODUCT_TYPE_ID = 0x3456;
  const uint16_t PRODUCT_ID = 0xAA78;

  mock_call_expect(TO_STR(zaf_config_get_product_type_id), &pMock);
  pMock->return_code.v = PRODUCT_TYPE_ID;

  mock_call_expect(TO_STR(zaf_config_get_product_id), &pMock);
  pMock->return_code.v = PRODUCT_ID;

  mock_call_expect(TO_STR(zaf_config_get_product_type_id), &pMock);
  pMock->return_code.v = PRODUCT_TYPE_ID;

  mock_call_expect(TO_STR(zaf_config_get_product_id), &pMock);
  pMock->return_code.v = PRODUCT_ID;

  mock_call_expect(TO_STR(ZAF_getAppHandle), &pMock);
  SApplicationHandles AppHandles;
  SNetworkInfo NetworkInfo;
  AppHandles.pNetworkInfo = &NetworkInfo;
  NetworkInfo.MaxPayloadSize = 10;
  pMock->return_code.p = &AppHandles;

  mock_call_expect(TO_STR(zaf_config_get_hardware_version), &pMock);
  pMock->return_code.v = HARDWARE_VERSION;

  mock_call_use_as_stub(TO_STR(zaf_config_get_firmware_target_count));

  uint8_t expectedFrame[] = {
      0x7A, // COMMAND_CLASS_FIRMWARE_UPDATE_MD
      0x02, // FIRMWARE_MD_REPORT_V4
      (uint8_t)(APP_MANUFACTURER_ID >> 8),
      (uint8_t)APP_MANUFACTURER_ID,
      (uint8_t)(APP_FIRMWARE_ID >> 8),
      (uint8_t)APP_FIRMWARE_ID,
      0, // CRC
      0, // CRC
      0xFF, // Hardcoded to upgradable.
      0, // Number of firmware targets following this value. The one before is NOT included.
      0,
      4, // Subtract the size of FW UPDATE MD frame size.
      HARDWARE_VERSION
  };

  received_frame_status_t status;
  ZW_APPLICATION_TX_BUFFER frameOut;
  uint8_t frameOutLength;

  status = invoke_cc_handler_v2(&p_chi->rxOptions,
                              &p_chi->frame.as_zw_application_tx_buffer,
                              p_chi->frameLength, &frameOut, &frameOutLength);

  TEST_ASSERT_MESSAGE(status == RECEIVED_FRAME_STATUS_SUCCESS, "Wrong frame status :(");
  TEST_ASSERT_EQUAL_INT8_MESSAGE(frameOutLength,
                                  sizeof(expectedFrame),
                                  "Frame size does not match");
  TEST_ASSERT_EQUAL_INT8_ARRAY_MESSAGE(expectedFrame, &frameOut,
                                        sizeof(expectedFrame),
                                        "Frame does not match");

  test_common_command_handler_input_free(p_chi);

  mock_calls_verify();
}

void test_FIRMWARE_MD_GET_five_firmware_targets(void)
{
  mock_t* pMock = NULL;

  const uint8_t NUMBER_OF_FIRMWARE_TARGETS = 5;
  const uint8_t HARDWARE_VERSION = 1;

  command_handler_input_t * p_chi = firmware_update_meta_data_get_frame_create();

  mock_call_use_as_stub(TO_STR(Check_not_legal_response_job));

  mock_call_expect(TO_STR(zaf_config_get_manufacturer_id), &pMock);
  pMock->return_code.v = APP_MANUFACTURER_ID;

  const uint16_t PRODUCT_TYPE_ID = 0x3456;
  const uint16_t PRODUCT_ID = 0xAA78;

  mock_call_expect(TO_STR(zaf_config_get_product_type_id), &pMock);
  pMock->return_code.v = PRODUCT_TYPE_ID;

  mock_call_expect(TO_STR(zaf_config_get_product_id), &pMock);
  pMock->return_code.v = PRODUCT_ID;

  mock_call_expect(TO_STR(zaf_config_get_product_type_id), &pMock);
  pMock->return_code.v = PRODUCT_TYPE_ID;

  mock_call_expect(TO_STR(zaf_config_get_product_id), &pMock);
  pMock->return_code.v = PRODUCT_ID;

  mock_call_expect(TO_STR(zaf_config_get_firmware_target_count), &pMock);
  pMock->return_code.v = NUMBER_OF_FIRMWARE_TARGETS;

  mock_call_expect(TO_STR(ZAF_getAppHandle), &pMock);
  SApplicationHandles AppHandles;
  SNetworkInfo NetworkInfo;
  AppHandles.pNetworkInfo = &NetworkInfo;
  NetworkInfo.MaxPayloadSize = 10;
  pMock->return_code.p = &AppHandles;

  mock_call_expect(TO_STR(zaf_config_get_hardware_version), &pMock);
  pMock->return_code.v = HARDWARE_VERSION;

  uint8_t expectedFrame[] = {
      0x7A, // COMMAND_CLASS_FIRMWARE_UPDATE_MD
      0x02, // FIRMWARE_MD_REPORT_V4
      (uint8_t)(APP_MANUFACTURER_ID >> 8),
      (uint8_t)APP_MANUFACTURER_ID,
      (uint8_t)(APP_FIRMWARE_ID >> 8),
      (uint8_t)APP_FIRMWARE_ID,
      0, // CRC
      0, // CRC
      0xFF, // Hardcoded to upgradable.
      4, // Number of firmware targets following this value. The one before is NOT included.
      0, // Fragment size MSB - hardcoded to zero.
      4, // Fragment size LSB
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      HARDWARE_VERSION
  };

  received_frame_status_t status;
  ZW_APPLICATION_TX_BUFFER frameOut;
  uint8_t frameOutLength;

  status = invoke_cc_handler_v2(&p_chi->rxOptions,
                              &p_chi->frame.as_zw_application_tx_buffer,
                              p_chi->frameLength, &frameOut, &frameOutLength);

  TEST_ASSERT_MESSAGE(status == RECEIVED_FRAME_STATUS_SUCCESS, "Wrong frame status :(");
  TEST_ASSERT_EQUAL_INT8_MESSAGE(frameOutLength,
                                  sizeof(expectedFrame),
                                  "Frame size does not match");
  TEST_ASSERT_EQUAL_INT8_ARRAY_MESSAGE(expectedFrame, &frameOut,
                                        sizeof(expectedFrame),
                                        "Frame does not match");

  test_common_command_handler_input_free(p_chi);

  mock_calls_verify();
}

/*
 * Test that the Request Report contains a status of invalid fragment size for Request Get commands
 * with an invalid fragment size.
 */
void test_FIRMWARE_UPDATE_MD_REQUEST_GET_V3_fragment_size_invalid(void)
{
  command_handler_input_t * p_chi;
  mock_t* pMock = NULL;

  // See function handleCommandClassFirmwareUpdateMaxFragmentSize in ota_util.c.
  const uint8_t test_fragment_size[] = {0, 100};

  const uint16_t MANUFACTURER_ID = 0xAABB;
  const uint16_t FIRMWARE_ID = 0xCCDD;
  const uint16_t CHECKSUM = 0xEEFF;
  const uint8_t FIRMWARE_TARGET = 0;

  uint8_t i;
  for (i = 0; i < sizeof(test_fragment_size); i++)
  {
    p_chi = firmware_update_md_request_get_v3_frame_create(
        MANUFACTURER_ID,
        FIRMWARE_ID,
        CHECKSUM,
        FIRMWARE_TARGET,
        test_fragment_size[i]);

    mock_call_use_as_stub(TO_STR(Check_not_legal_response_job));
    mock_call_use_as_stub(TO_STR(zaf_config_get_firmware_target_count));
    mock_call_use_as_stub(TO_STR(zaf_config_get_hardware_version));

    mock_call_expect(TO_STR(zaf_config_get_manufacturer_id), &pMock);
    pMock->return_code.v = MANUFACTURER_ID;

    mock_call_expect(TO_STR(ZAF_getAppHandle), &pMock);
    SApplicationHandles AppHandle;
    SNetworkInfo NetworkInfo;

    /*
     * Increase by the size of CC frame and subtract 1 to get a max fragment size one byte smaller
     * than the given fragment size in Request Get.
     */
    NetworkInfo.MaxPayloadSize = test_fragment_size[i] + 6 - 1;

    AppHandle.pNetworkInfo = &NetworkInfo;
    pMock->return_code.p = &AppHandle;

    uint8_t expectedFrame[] = {
        0x7A, // COMMAND_CLASS_FIRMWARE_UPDATE_MD
        0x04, // FIRMWARE_UPDATE_MD_REQUEST_REPORT_V3
        0x02 // FIRMWARE_UPDATE_MD_REQUEST_REPORT_INVALID_FRAGMENT_SIZE_v3
    };

    zaf_transport_rx_to_tx_options_Ignore();
    zaf_transport_tx_ExpectAndReturn(expectedFrame, sizeof(expectedFrame), NULL, NULL, true);
    zaf_transport_tx_IgnoreArg_callback();
    zaf_transport_tx_IgnoreArg_zaf_tx_options();

    received_frame_status_t status;
    status = handleCommandClassFWUpdate(
        &p_chi->rxOptions,
        &p_chi->frame.as_zw_application_tx_buffer,
        p_chi->frameLength);

    TEST_ASSERT_MESSAGE(status == RECEIVED_FRAME_STATUS_SUCCESS, "Wrong frame status :(");

    test_common_command_handler_input_free(p_chi);
  }

  mock_calls_verify();
}



typedef struct
{
  uint16_t manufacturer_id;
  uint16_t firmware_id;
}
test_invalid_combination_t;

void test_FIRMWARE_UPDATE_MD_REQUEST_GET_V4_invalid_combination(void)
{
  command_handler_input_t * p_chi;
  mock_t* pMock = NULL;

  const uint16_t MANUFACTURER_ID = 0xAABB;
  const uint16_t PRODUCT_TYPE_ID = 0x00CC;
  const uint16_t PRODUCT_ID = 0x00DD;
  const uint16_t FIRMWARE_ID = 0xCCDD; // Firmware ID is a combination of product type ID and product ID.
  const uint16_t CHECKSUM = 0xEEFF;
  const uint8_t FIRMWARE_TARGET = 0;
  const bool ACTIVATION = true;
  const uint16_t FRAGMENT_SIZE = 10;

  const test_invalid_combination_t test_invalid_combinations[] = {
                                                                 {0xABCD, 0xABCD},
                                                                 {MANUFACTURER_ID, 0xABCD},
                                                                 {0xABCD, FIRMWARE_ID}
  };

  uint8_t i;
  for (i = 0; i < sizeof_array(test_invalid_combinations); i++)
  {
    p_chi = firmware_update_md_request_get_v4_frame_create(
        test_invalid_combinations[i].manufacturer_id, // Variable input
        test_invalid_combinations[i].firmware_id, // Variable input
        CHECKSUM,
        FIRMWARE_TARGET,
        FRAGMENT_SIZE,
        ACTIVATION);

    mock_call_use_as_stub(TO_STR(Check_not_legal_response_job));
    mock_call_use_as_stub(TO_STR(zaf_config_get_firmware_target_count));
    mock_call_use_as_stub(TO_STR(zaf_config_get_hardware_version));
    mock_call_use_as_stub(TO_STR(ZAF_getAppHandle));

    mock_call_expect(TO_STR(zaf_config_get_manufacturer_id), &pMock);
    pMock->return_code.v = MANUFACTURER_ID;

    mock_call_expect(TO_STR(zaf_config_get_product_type_id), &pMock);
    pMock->return_code.v = PRODUCT_TYPE_ID;

    mock_call_expect(TO_STR(zaf_config_get_product_id), &pMock);
    pMock->return_code.v = PRODUCT_ID;

    uint8_t expectedFrame[] = {
        0x7A, // COMMAND_CLASS_FIRMWARE_UPDATE_MD
        0x04, // FIRMWARE_UPDATE_MD_REQUEST_REPORT_V4
        0x00 // FIRMWARE_UPDATE_MD_REQUEST_REPORT_INVALID_COMBINATION_V4
    };

    zaf_transport_rx_to_tx_options_Ignore();
    zaf_transport_tx_ExpectAndReturn(expectedFrame, sizeof(expectedFrame), NULL, NULL, true);
    zaf_transport_tx_IgnoreArg_callback();
    zaf_transport_tx_IgnoreArg_zaf_tx_options();

    received_frame_status_t status;
    status = handleCommandClassFWUpdate(
        &p_chi->rxOptions,
        &p_chi->frame.as_zw_application_tx_buffer,
        p_chi->frameLength);

    TEST_ASSERT_MESSAGE(status == RECEIVED_FRAME_STATUS_SUCCESS, "Wrong frame status :(");

    test_common_command_handler_input_free(p_chi);
  }

  mock_calls_verify();
}

void test_FIRMWARE_UPDATE_MD_PREPARE_GET_V5(void)
{
  const uint16_t MANUFACTURER_ID = 0xAABB;
  const uint16_t FIRMWARE_ID = 0xCCDD;
  const uint8_t FIRMWARE_TARGET = 0;
  const uint16_t FRAGMENT_SIZE = 10; // Random number
  const uint8_t HARDWARE_VERSION = 3; // Random number

  command_handler_input_t * p_chi = meta_data_prepare_get_frame_create(MANUFACTURER_ID,
                                                                       FIRMWARE_ID,
                                                                       FIRMWARE_TARGET,
                                                                       FRAGMENT_SIZE,
                                                                       HARDWARE_VERSION);

  mock_call_use_as_stub(TO_STR(Check_not_legal_response_job));

  uint8_t expectedFrame[] = {
                             COMMAND_CLASS_FIRMWARE_UPDATE_MD_V5,
                             FIRMWARE_UPDATE_MD_PREPARE_REPORT_V5,

                             /*
                              * We expect the following status because we do not support firmware
                              * download (yet).
                              *
                              * ERROR. This firmware target is not downloadable. The receiving node
                              * MUST NOT initiate the firmware download.
                              *
                              * Complies with CC:007A.05.0B.11.001.
                              */
                             0x03,

                             /*
                              * 0x0000 is not in the spec but will be. It makes no sense to respond
                              * with the checksum if we're not planning to let anyone initiate a
                              * download.
                              */
                             0x00,
                             0x00

  };

  received_frame_status_t status;
  ZW_APPLICATION_TX_BUFFER frameOut;
  uint8_t frameOutLength;

  status = invoke_cc_handler_v2(&p_chi->rxOptions,
                              &p_chi->frame.as_zw_application_tx_buffer,
                              p_chi->frameLength, &frameOut, &frameOutLength);

  TEST_ASSERT_MESSAGE(status == RECEIVED_FRAME_STATUS_SUCCESS, "Wrong frame status :(");
  TEST_ASSERT_EQUAL_INT8_MESSAGE(frameOutLength,
                                  sizeof(expectedFrame),
                                  "Frame size does not match");
  TEST_ASSERT_EQUAL_INT8_ARRAY_MESSAGE(expectedFrame, &frameOut,
                                        sizeof(expectedFrame),
                                        "Frame does not match");

  test_common_command_handler_input_free(p_chi);

  mock_calls_verify();
}

/* Test the FirmwareUpdate CC V5 Request Get Commands with invalid hardware version */
void test_FIRMWARE_UPDATE_MD_REQUEST_GET_V5_hardware_version_invalid(void)
{
  mock_t* pMock = NULL;

  const uint16_t MANUFACTURER_ID = 0xAABB;
  const uint16_t FIRMWARE_ID = 0xCCDD;
  const uint16_t CHECKSUM = 0xEEFF;
  const uint8_t FIRMWARE_TARGET = 0;
  const bool ACTIVATION = true;
  const uint8_t FRAGMENT_SIZE = 1;
  const uint8_t HARDWARE_VERSION = 1;
  const uint8_t TEST_HARDWARE_VERSION = 0xFF; // Invalid hardware version

  command_handler_input_t * p_chi = firmware_update_md_request_get_v5_frame_create(
        MANUFACTURER_ID,
        FIRMWARE_ID,
        CHECKSUM,
        FIRMWARE_TARGET,
        FRAGMENT_SIZE,
        ACTIVATION,
        TEST_HARDWARE_VERSION);

  mock_call_use_as_stub(TO_STR(Check_not_legal_response_job));
  mock_call_use_as_stub(TO_STR(zaf_config_get_firmware_target_count));

  mock_call_expect(TO_STR(zaf_config_get_hardware_version), &pMock);
  pMock->return_code.v = HARDWARE_VERSION;

  uint8_t expectedFrame[] = {
      0x7A, // COMMAND_CLASS_FIRMWARE_UPDATE_MD
      0x04, // FIRMWARE_UPDATE_MD_REQUEST_REPORT_V5
      0x04  // FIRMWARE_UPDATE_MD_REQUEST_REPORT_INVALID_HARDWARE_VERSION_V5
  };

  zaf_transport_rx_to_tx_options_Ignore();
  zaf_transport_tx_ExpectAndReturn(expectedFrame, sizeof(expectedFrame), NULL, NULL, true);
  zaf_transport_tx_IgnoreArg_callback();
  zaf_transport_tx_IgnoreArg_zaf_tx_options();

  received_frame_status_t status;
  status = handleCommandClassFWUpdate(
      &p_chi->rxOptions,
      &p_chi->frame.as_zw_application_tx_buffer,
      p_chi->frameLength);

  TEST_ASSERT_MESSAGE(status == RECEIVED_FRAME_STATUS_SUCCESS, "Wrong frame status :(");

  test_common_command_handler_input_free(p_chi);

  mock_calls_verify();
}

/*
 * Verifies the flow of a successful firmware update with instant reboot.
 */
void test_successful_firmware_update(void)
{
  mock_t * pMock = NULL;
  mock_t * pMock_AppTimerRegister_Timerotasuccess = NULL;
  mock_t * pMock_AppTimerRegister_FrameGet = NULL;

  const uint16_t MANUFACTURER_ID = APP_MANUFACTURER_ID;
  const uint16_t PRODUCT_TYPE_ID = 0x00CC;
  const uint16_t PRODUCT_ID = 0x00DD;
  const uint16_t FIRMWARE_ID = 0xCCDD; // Firmware ID is a combination of product type ID and product ID.
  const uint8_t FIRMWARE_TARGET = 0;
  const uint16_t FRAGMENT_SIZE = 20;

  mock_call_use_as_stub(TO_STR(ZAF_nvm_write)); // Invoked in Request Get handler
  mock_call_use_as_stub(TO_STR(ZAF_nvm_read));  // Invoked in CC_FirmwareUpdate_Init()
  mock_call_use_as_stub(TO_STR(zpal_bootloader_reset_page_counters));

  /*
   * Initialization
   */
  mock_call_expect(TO_STR(zpal_bootloader_init), &pMock);
  pMock->return_code.v = ZPAL_STATUS_OK;

  mock_call_expect(TO_STR(zpal_bootloader_get_info), &pMock);
  pMock->compare_rule_arg[0] = COMPARE_NOT_NULL;
  zpal_bootloader_info_t info = {
                                  .type = ZPAL_BOOTLOADER_PRESENT,
                                  .capabilities = ZPAL_BOOTLOADER_CAPABILITY_STORAGE
  };
  pMock->output_arg[0].p = &info;

  /*
   * Make sure that ZAF_nvm_get_object_size() returns ZPAL_STATUS_OK indicating that the file is OK.
   */
  mock_call_expect(TO_STR(ZAF_nvm_get_object_size), &pMock);
  pMock->compare_rule_arg[0] = COMPARE_ANY;
  pMock->compare_rule_arg[1] = COMPARE_NOT_NULL;
  size_t fileSize = ZAF_FILE_SIZE_CC_FIRMWARE_UPDATE;
  pMock->output_arg[1].p     = &fileSize;
  pMock->return_code.v       = ZPAL_STATUS_OK;

  mock_call_expect(TO_STR(TimerIsActive), &pMock);
  pMock->compare_rule_arg[0] = COMPARE_NOT_NULL;
  pMock->return_code.v  = false;

  mock_call_expect(TO_STR(AppTimerRegister), &pMock_AppTimerRegister_FrameGet);
  pMock_AppTimerRegister_FrameGet->compare_rule_arg[0] = COMPARE_NOT_NULL;
  pMock_AppTimerRegister_FrameGet->expect_arg[1].v = false;
  pMock_AppTimerRegister_FrameGet->compare_rule_arg[2] = COMPARE_NOT_NULL;
  pMock_AppTimerRegister_FrameGet->return_code.v = true;

  mock_call_expect(TO_STR(AppTimerRegister), &pMock_AppTimerRegister_Timerotasuccess);
  pMock_AppTimerRegister_Timerotasuccess->compare_rule_arg[0] = COMPARE_NOT_NULL;
  pMock_AppTimerRegister_Timerotasuccess->expect_arg[1].v = false;
  pMock_AppTimerRegister_Timerotasuccess->compare_rule_arg[2] = COMPARE_NOT_NULL;

  // Call function that would be normally called from ApplicationTask
  bool updated_successfully = false;
  mock_call_expect(TO_STR(zpal_bootloader_is_first_boot), &pMock);
  pMock->compare_rule_arg[0] = COMPARE_NOT_NULL;
  pMock->output_arg[0].p     = &updated_successfully;
  pMock->return_code.v = false;

  mock_call_expect(TO_STR(ZAF_nvm_get_object_size), &pMock);
  pMock->compare_rule_arg[0] = COMPARE_ANY;
  pMock->compare_rule_arg[1] = COMPARE_NOT_NULL;
  pMock->output_arg[1].p     = &fileSize;
  pMock->return_code.v       = ZPAL_STATUS_OK;

  CC_FirmwareUpdate_Init(NULL, NULL, false);

  /*
   * Firmware Update Meta Data Request Get + Request Report
   */
  command_handler_input_t * pRequestGet;

  pRequestGet = firmware_update_md_request_get_v5_frame_create(
      MANUFACTURER_ID,
      FIRMWARE_ID,
      0x3378,
      FIRMWARE_TARGET,
      FRAGMENT_SIZE,
      false,
      1);

  mock_call_use_as_stub(TO_STR(Check_not_legal_response_job));
  mock_call_use_as_stub(TO_STR(zaf_config_get_hardware_version));
  mock_call_use_as_stub(TO_STR(zaf_config_get_firmware_target_count));

  mock_call_expect(TO_STR(zaf_config_get_manufacturer_id), &pMock);
  pMock->return_code.v = MANUFACTURER_ID;

  mock_call_expect(TO_STR(zaf_config_get_product_type_id), &pMock);
  pMock->return_code.v = PRODUCT_TYPE_ID;

  mock_call_expect(TO_STR(zaf_config_get_product_id), &pMock);
  pMock->return_code.v = PRODUCT_ID;

  mock_call_expect(TO_STR(ZAF_getAppHandle), &pMock);
  SApplicationHandles AppHandles;
  SNetworkInfo NetworkInfo;
  AppHandles.pNetworkInfo = &NetworkInfo;
  NetworkInfo.MaxPayloadSize = 52;  // max payload size for S0
  pMock->return_code.p = &AppHandles;

  uint8_t expectedFrame[] = {
      0x7A, // COMMAND_CLASS_FIRMWARE_UPDATE_MD
      0x04, // FIRMWARE_UPDATE_MD_REQUEST_REPORT_V5
      0xFF  // FIRMWARE_UPDATE_MD_REQUEST_REPORT_VALID_COMBINATION_V5
  };

  mock_call_expect(TO_STR(TimerIsActive), &pMock_AppTimerRegister_FrameGet);
  pMock_AppTimerRegister_FrameGet->compare_rule_arg[0] = COMPARE_NOT_NULL;
  pMock_AppTimerRegister_FrameGet->return_code.v = false;

  mock_call_expect(TO_STR(TimerStart), &pMock_AppTimerRegister_FrameGet);
  pMock_AppTimerRegister_FrameGet->compare_rule_arg[0] = COMPARE_NOT_NULL;
  pMock_AppTimerRegister_FrameGet->expect_arg[1].v = TIMER_START_FWUPDATE_FRAME_GET;
  pMock_AppTimerRegister_FrameGet->return_code.v = ESWTIMER_STATUS_SUCCESS;

  zaf_transport_rx_to_tx_options_Ignore();
  zaf_transport_tx_ExpectAndReturn(expectedFrame, sizeof(expectedFrame), NULL, NULL, true);
  zaf_transport_tx_IgnoreArg_callback();
  zaf_transport_tx_IgnoreArg_zaf_tx_options();

  received_frame_status_t status;
  zaf_transport_resume_Expect();
  status = INVOKE_CC_HANDLER(handleCommandClassFWUpdate, pRequestGet);

  TEST_ASSERT_MESSAGE(status == RECEIVED_FRAME_STATUS_SUCCESS, "Wrong frame status :(");

  request_get_cb = g_request_get_cb;
  /*
   * Firmware Update Meta Data Get + Report
   */
  const uint8_t EXPECTED_FRAME[] = {
                                    COMMAND_CLASS_FIRMWARE_UPDATE_MD_V5,
                                    FIRMWARE_UPDATE_MD_GET_V5,
                                    OTA_CACHE_SIZE / FRAGMENT_SIZE, // Number of reports
                                    0, // Report number MSB
                                    1  // Report number LSB
  };
  zaf_transport_rx_to_tx_options_Ignore();
  zaf_transport_tx_ExpectAndReturn(EXPECTED_FRAME, sizeof(EXPECTED_FRAME), NULL, NULL, true);
  zaf_transport_tx_IgnoreArg_callback();
  zaf_transport_tx_IgnoreArg_zaf_tx_options();

  /*
   * The transmission of Firmware Update Meta Data Get is triggered by the ack callback on
   * Firmware Update Meta Data Request Report.
   */

  transmission_result_t pTxResult = {
    .status = TRANSMIT_COMPLETE_OK
  };
  request_get_cb(&pTxResult);

  command_handler_input_t * pFirmwareUpdateMDReport;

  pFirmwareUpdateMDReport = firmware_update_meta_data_report_frame_create(
      true,
      1,
      FRAGMENT_SIZE);

  zaf_transport_resume_Expect();

  mock_call_expect(TO_STR(TimerStart), &pMock);
  pMock->compare_rule_arg[0] = COMPARE_NOT_NULL;
  pMock->expect_arg[1].v = 100;
  pMock->return_code.v = ESWTIMER_STATUS_SUCCESS;

  mock_call_expect(TO_STR(zpal_bootloader_write_data), &pMock);
  pMock->expect_arg[0].v = 0;
  pMock->expect_arg[1].p = &(pFirmwareUpdateMDReport->frame.as_byte_array[4]);
  pMock->expect_arg[2].v = FRAGMENT_SIZE;
  pMock->return_code.v = ZPAL_STATUS_OK;

  zaf_transport_resume_Expect();
  status = INVOKE_CC_HANDLER(handleCommandClassFWUpdate, pFirmwareUpdateMDReport);

  char str[100];
  sprintf(str, "%s", TO_STR(pFirmwareUpdateMDReport));
  TEST_ASSERT_MESSAGE(status == RECEIVED_FRAME_STATUS_SUCCESS, str);

  void(*pTimerotasuccess_Callback)(SSwTimer* pTimer);
  pTimerotasuccess_Callback = pMock_AppTimerRegister_Timerotasuccess->actual_arg[2].p;
  SSwTimer * pTimer = pMock_AppTimerRegister_Timerotasuccess->actual_arg[0].p;

  mock_call_expect(TO_STR(zpal_bootloader_verify_image), &pMock);
  pMock->return_code.v = ZPAL_STATUS_OK;

  mock_call_expect(TO_STR(TimerIsActive), &pMock);
  pMock->compare_rule_arg[0] = COMPARE_NOT_NULL;
  pMock->return_code.v = true;

  mock_call_expect(TO_STR(TimerStop), &pMock);
  pMock->compare_rule_arg[0] = COMPARE_NOT_NULL;

  SQueueNotifying  m_ZwCommandQueue;

  mock_call_expect(TO_STR(ZAF_getZwCommandQueue), &pMock);
  pMock->return_code.p = &m_ZwCommandQueue;

  mock_call_expect(TO_STR(QueueNotifyingSendToBack), &pMock);
  pMock->expect_arg[ARG0].p = &m_ZwCommandQueue;
  pMock->compare_rule_arg[ARG1] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG2].value = 0;
  pMock->return_code.value = EQUEUENOTIFYING_STATUS_SUCCESS;

  pTimerotasuccess_Callback(pTimer);

  /*
   * Initialization
   */
  mock_call_expect(TO_STR(zpal_bootloader_init), &pMock);
  pMock->return_code.v = ZPAL_STATUS_OK;

  mock_call_expect(TO_STR(zpal_bootloader_get_info), &pMock);
  pMock->compare_rule_arg[0] = COMPARE_NOT_NULL;
  zpal_bootloader_info_t info_2 = {
                                  .type = ZPAL_BOOTLOADER_PRESENT,
                                  .capabilities = ZPAL_BOOTLOADER_CAPABILITY_STORAGE
  };
  pMock->output_arg[0].p = &info_2;

  /*
   * Make sure that ZAF_nvm_get_object_size() returns ZPAL_STATUS_OK indicating that the file is OK.
   */
  mock_call_expect(TO_STR(ZAF_nvm_get_object_size), &pMock);
  pMock->compare_rule_arg[0] = COMPARE_ANY;
  pMock->compare_rule_arg[1] = COMPARE_NOT_NULL;
  size_t fileSize_2 = ZAF_FILE_SIZE_CC_FIRMWARE_UPDATE;
  pMock->output_arg[1].p     = &fileSize_2;
  pMock->return_code.v       = ZPAL_STATUS_OK;

  mock_call_expect(TO_STR(AppTimerRegister), &pMock_AppTimerRegister_FrameGet);
  pMock_AppTimerRegister_FrameGet->compare_rule_arg[0] = COMPARE_NOT_NULL;
  pMock_AppTimerRegister_FrameGet->expect_arg[1].v = false;
  pMock_AppTimerRegister_FrameGet->compare_rule_arg[2] = COMPARE_NOT_NULL;
  pMock_AppTimerRegister_FrameGet->return_code.v = true;

  mock_call_expect(TO_STR(AppTimerRegister), &pMock_AppTimerRegister_Timerotasuccess);
  pMock_AppTimerRegister_Timerotasuccess->compare_rule_arg[0] = COMPARE_NOT_NULL;
  pMock_AppTimerRegister_Timerotasuccess->expect_arg[1].v = false;
  pMock_AppTimerRegister_Timerotasuccess->compare_rule_arg[2] = COMPARE_NOT_NULL;

  //After successful update, Status Report will be sent.
  updated_successfully = true;
  mock_call_expect(TO_STR(zpal_bootloader_is_first_boot), &pMock);
  pMock->compare_rule_arg[0] = COMPARE_NOT_NULL;
  pMock->output_arg[0].p     = &updated_successfully;
  pMock->return_code.v = true;

  const uint8_t EXPECTED_STATUS_REPORT_FRAME[] = {
                                    COMMAND_CLASS_FIRMWARE_UPDATE_MD_V5,
                                    FIRMWARE_UPDATE_MD_STATUS_REPORT,
                                    FIRMWARE_UPDATE_MD_STATUS_REPORT_SUCCESSFULLY,
                                    0, // wait time
                                    0  // wait tim
  };
  zaf_transport_rx_to_tx_options_Ignore();
  zaf_transport_tx_ExpectAndReturn(EXPECTED_STATUS_REPORT_FRAME, sizeof(EXPECTED_STATUS_REPORT_FRAME), NULL, NULL, true);
  zaf_transport_tx_IgnoreArg_callback();
  zaf_transport_tx_IgnoreArg_zaf_tx_options();

  CC_FirmwareUpdate_Init(NULL, NULL, false);

  test_common_command_handler_input_free(pFirmwareUpdateMDReport);
  test_common_command_handler_input_free(pRequestGet);

  mock_calls_verify();
}

/*
 * Perform a firmware update with delayed reboot
 *
 * Prerequisites (part of the test):
 * 1. Application initializes the CC with delayed activation enabled.
 * 2. Initiator sends Reguest Get with activation enabled.
 *
 * The test verifies the following:
 * 1. An attempt to activate with incorrect parameters will fail and transmit a report stating that.
 * 2. An attempt to activate with correct parameters will invoke a reboot and install.
 */
void test_successful_firmware_update_with_activation(void)
{
  mock_t * pMock = NULL;
  mock_t * pMock_AppTimerRegister_Timerotasuccess = NULL;
  mock_t * pMock_AppTimerRegister_FrameGet = NULL;

  const uint16_t MANUFACTURER_ID = APP_MANUFACTURER_ID;
  const uint16_t PRODUCT_TYPE_ID = 0x00CC;
  const uint16_t PRODUCT_ID = 0x00DD;
  const uint16_t FIRMWARE_ID = 0xCCDD; // Firmware ID is a combination of product type ID and product ID.
  const uint8_t FIRMWARE_TARGET = 0;
  const uint16_t CHECKSUM = 0x3378;
  const uint8_t HARDWARE_VERSION = 1;
  const uint8_t FRAGMENT_SIZE = 20;

  mock_call_use_as_stub(TO_STR(ZAF_nvm_write)); // Invoked in Request Get handler
  mock_call_use_as_stub(TO_STR(zpal_bootloader_reset_page_counters));

  /* ************************************************************************************** *
   * Initialization
   * ************************************************************************************** */

  // The following function is verified in test_successful_firmware_update().
  mock_call_use_as_stub(TO_STR(zpal_bootloader_init));

  mock_call_expect(TO_STR(zpal_bootloader_get_info), &pMock);
  pMock->compare_rule_arg[0] = COMPARE_NOT_NULL;
  zpal_bootloader_info_t info = {
                                  .type = ZPAL_BOOTLOADER_PRESENT,
                                  .capabilities = ZPAL_BOOTLOADER_CAPABILITY_STORAGE
  };
  pMock->output_arg[0].p = &info;

  /*
   * Make sure that ZAF_nvm_get_object_size() returns ZPAL_STATUS_OK indicating that the file is OK.
   */
  mock_call_expect(TO_STR(ZAF_nvm_get_object_size), &pMock);
  pMock->compare_rule_arg[0] = COMPARE_ANY;
  pMock->compare_rule_arg[1] = COMPARE_NOT_NULL;
  size_t fileSize = ZAF_FILE_SIZE_CC_FIRMWARE_UPDATE;
  pMock->output_arg[1].p     = &fileSize;
  pMock->return_code.v       = ZPAL_STATUS_OK;

  // The following function is verified in test_successful_firmware_update().
  mock_call_use_as_stub(TO_STR(zpal_bootloader_is_first_boot));

  mock_call_expect(TO_STR(AppTimerRegister), &pMock_AppTimerRegister_FrameGet);
  pMock_AppTimerRegister_FrameGet->compare_rule_arg[0] = COMPARE_NOT_NULL;
  pMock_AppTimerRegister_FrameGet->expect_arg[1].v = false;
  pMock_AppTimerRegister_FrameGet->compare_rule_arg[2] = COMPARE_NOT_NULL;
  pMock_AppTimerRegister_FrameGet->return_code.v = true;

  mock_call_expect(TO_STR(AppTimerRegister), &pMock_AppTimerRegister_Timerotasuccess);
  pMock_AppTimerRegister_Timerotasuccess->compare_rule_arg[0] = COMPARE_NOT_NULL;
  pMock_AppTimerRegister_Timerotasuccess->expect_arg[1].v = false;
  pMock_AppTimerRegister_Timerotasuccess->compare_rule_arg[2] = COMPARE_NOT_NULL;
  pMock_AppTimerRegister_Timerotasuccess->return_code.v = true;

  // Notice the activation support is enabled.
  CC_FirmwareUpdate_Init(NULL, NULL, true);

  /* ************************************************************************************** *
   * Firmware Update Meta Data Request Get + Request Report
   * ************************************************************************************** */

  command_handler_input_t * pRequestGet;

  pRequestGet = firmware_update_md_request_get_v5_frame_create(
      MANUFACTURER_ID,
      FIRMWARE_ID,
      CHECKSUM,
      FIRMWARE_TARGET,
      FRAGMENT_SIZE,
      true,
      HARDWARE_VERSION);

  mock_call_use_as_stub(TO_STR(Check_not_legal_response_job));
  mock_call_use_as_stub(TO_STR(zaf_config_get_hardware_version));
  mock_call_use_as_stub(TO_STR(zaf_config_get_firmware_target_count));

  mock_call_expect(TO_STR(zaf_config_get_manufacturer_id), &pMock);
  pMock->return_code.v = MANUFACTURER_ID;

  mock_call_expect(TO_STR(zaf_config_get_product_type_id), &pMock);
  pMock->return_code.v = PRODUCT_TYPE_ID;

  mock_call_expect(TO_STR(zaf_config_get_product_id), &pMock);
  pMock->return_code.v = PRODUCT_ID;

  mock_call_expect(TO_STR(ZAF_getAppHandle), &pMock);
  SApplicationHandles AppHandles;
  SNetworkInfo NetworkInfo;
  AppHandles.pNetworkInfo = &NetworkInfo;
  NetworkInfo.MaxPayloadSize = 52;  // max payload size for S0
  pMock->return_code.p = &AppHandles;

  // The following functions are verified in test_successful_firmware_update().
  mock_call_use_as_stub(TO_STR(TimerStop));
  mock_call_use_as_stub(TO_STR(TimerIsActive));

  zaf_transport_rx_to_tx_options_Ignore();
  zaf_transport_tx_ExpectAndReturn(NULL, 0, NULL, NULL, true);
  zaf_transport_tx_IgnoreArg_frame();
  zaf_transport_tx_IgnoreArg_frame_length();
  zaf_transport_tx_IgnoreArg_callback();
  zaf_transport_tx_IgnoreArg_zaf_tx_options();

  received_frame_status_t status;
  zaf_transport_resume_Expect();
  status = INVOKE_CC_HANDLER(handleCommandClassFWUpdate, pRequestGet);

  TEST_ASSERT_MESSAGE(status == RECEIVED_FRAME_STATUS_SUCCESS, "Wrong frame status :(");

  /* ************************************************************************************** *
   * Firmware Update Meta Data Get + Report
   * ************************************************************************************** */

  // The following functions are verified in test_successful_firmware_update().
  mock_call_use_as_stub(TO_STR(TimerIsActive));
  mock_call_use_as_stub(TO_STR(AppTimerRegister));


  mock_call_expect(TO_STR(TimerStart), &pMock);
  pMock->compare_rule_arg[0] = COMPARE_NOT_NULL; // Timer handle
  pMock->expect_arg[1].v = TIMER_START_FWUPDATE_FRAME_GET;
  pMock->return_code.v = ESWTIMER_STATUS_SUCCESS; // Timer handle


  const uint8_t EXPECTED_FRAME[] = {
                                    COMMAND_CLASS_FIRMWARE_UPDATE_MD_V5,
                                    FIRMWARE_UPDATE_MD_GET_V5,
                                    OTA_CACHE_SIZE / FRAGMENT_SIZE, // Number of reports
                                    0, // Report number MSB
                                    1  // Report number LSB
  };
  zaf_transport_rx_to_tx_options_Ignore();
  zaf_transport_tx_ExpectAndReturn(EXPECTED_FRAME, sizeof(EXPECTED_FRAME), NULL, NULL, true);
  zaf_transport_tx_IgnoreArg_callback();
  zaf_transport_tx_IgnoreArg_zaf_tx_options();

  /*
   * The transmission of Firmware Update Meta Data Get is triggered by the ack callback on
   * Firmware Update Meta Data Request Report.
   */
  transmission_result_t pTxResult = {
    .status = TRANSMIT_COMPLETE_OK
  };
  request_get_cb(&pTxResult);

  command_handler_input_t * pFirmwareUpdateMDReport;

  pFirmwareUpdateMDReport = firmware_update_meta_data_report_frame_create(
      true,
      1,
      FRAGMENT_SIZE);

  zaf_transport_resume_Expect();

  // The following functions are verified in test_successful_firmware_update().
  mock_call_expect(TO_STR(TimerStart), &pMock);
  pMock->compare_rule_arg[0] = COMPARE_NOT_NULL; // Timer handle
  pMock->expect_arg[1].v = 100;
  pMock->return_code.v = ESWTIMER_STATUS_SUCCESS; // Timer handle

  mock_call_use_as_stub(TO_STR(zpal_bootloader_write_data));

  zaf_transport_resume_Expect();
  status = INVOKE_CC_HANDLER(handleCommandClassFWUpdate, pFirmwareUpdateMDReport);

  char str[100];
  sprintf(str, "%s", TO_STR(pFirmwareUpdateMDReport));
  TEST_ASSERT_MESSAGE(status == RECEIVED_FRAME_STATUS_SUCCESS, str);

  void(*pTimerotasuccess_Callback)(SSwTimer* pTimer);
  pTimerotasuccess_Callback = pMock_AppTimerRegister_Timerotasuccess->actual_arg[2].p;
  SSwTimer * pTimer = pMock_AppTimerRegister_Timerotasuccess->actual_arg[0].p;

  // Verified in test_successful_firmware_update().
  mock_call_use_as_stub(TO_STR(zpal_bootloader_verify_image));

  const uint8_t EXPECTED_STATUS_REPORT[] = {
                                    COMMAND_CLASS_FIRMWARE_UPDATE_MD_V5,
                                    FIRMWARE_UPDATE_MD_STATUS_REPORT_V5,
                                    FIRMWARE_UPDATE_MD_STATUS_REPORT_SUCCESSFULLY_WAITING_FOR_ACTIVATION_V5,
                                    0, // Report number MSB
                                    0  // Report number LSB
  };
  zaf_transport_rx_to_tx_options_Ignore();
  zaf_transport_tx_ExpectAndReturn(EXPECTED_STATUS_REPORT, sizeof(EXPECTED_STATUS_REPORT), NULL, NULL, true);
  zaf_transport_tx_IgnoreArg_callback();
  zaf_transport_tx_IgnoreArg_zaf_tx_options();

  pTimerotasuccess_Callback(pTimer);

  transmission_result_t tx_result;

  SQueueNotifying  m_ZwCommandQueue;

  mock_call_expect(TO_STR(ZAF_getZwCommandQueue), &pMock);
  pMock->return_code.p = &m_ZwCommandQueue;

  mock_call_expect(TO_STR(QueueNotifyingSendToBack), &pMock);
  pMock->expect_arg[ARG0].p = &m_ZwCommandQueue;
  pMock->compare_rule_arg[ARG1] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG2].value = 0;
  pMock->return_code.value = EQUEUENOTIFYING_STATUS_SUCCESS;

  g_request_get_cb(&tx_result);

  /* ************************************************************************************** *
   * Send Activation Set with incorrect parameters and expect a report stating that.
   * ************************************************************************************** */

  // Create non-const variables because the following code needs to change the values.
  uint16_t manufacturer_id  = MANUFACTURER_ID;
  uint16_t firmware_id      = FIRMWARE_ID;
  uint16_t checksum         = CHECKSUM;
  uint8_t  firmware_target  = FIRMWARE_TARGET;
  uint8_t  hardware_version = HARDWARE_VERSION;

  /*
   * Create an array of argument pointers so that we can loop through them and change the values to
   * something incorrect.
   */
  typedef struct t_argument_ {
   uint8_t argSize;
   void * argPtr;
  } t_argument;
  t_argument arguments[] = {{.argSize = sizeof(uint16_t), .argPtr = (void *)&manufacturer_id},
                            {.argSize = sizeof(uint16_t), .argPtr = (void *)&firmware_id},
                            {.argSize = sizeof(uint16_t), .argPtr = (void *)&checksum},
                            {.argSize = sizeof(uint8_t), .argPtr = (void *)&firmware_target},
                            {.argSize = sizeof(uint8_t), .argPtr = (void *)&hardware_version}};

  for (int i = 0; i < sizeof_array(arguments); i++)
  {
    if (sizeof(uint16_t) == arguments[i].argSize) {
      *(uint16_t *)arguments[i].argPtr ^=0xFFFF;
    } else {
      *(uint8_t *)arguments[i].argPtr ^=0xFF;
    }

    if (*(uint8_t *)arguments[3].argPtr == FIRMWARE_TARGET)
    {
      // If the firmware target is correct, expect calls to get product type ID and product ID
      mock_call_expect(TO_STR(zaf_config_get_product_type_id), &pMock);
      pMock->return_code.v = PRODUCT_TYPE_ID;

      mock_call_expect(TO_STR(zaf_config_get_product_id), &pMock);
      pMock->return_code.v = PRODUCT_ID;
    }

    mock_call_expect(TO_STR(zaf_config_get_manufacturer_id), &pMock);
    pMock->return_code.v = MANUFACTURER_ID;

    const uint8_t EXPECTED_REPORT[] = {
        COMMAND_CLASS_FIRMWARE_UPDATE_MD_V5,
        FIRMWARE_UPDATE_ACTIVATION_STATUS_REPORT_V5,
        (uint8_t)(*(uint16_t *)arguments[0].argPtr >> 8), // Manufacturer MSB
        (uint8_t)*(uint16_t *)arguments[0].argPtr,        // Manufacturer LSB
        (uint8_t)(*(uint16_t *)arguments[1].argPtr >> 8), // Firmware ID MSB
        (uint8_t)*(uint16_t *)arguments[1].argPtr,        // Firmware ID LSB
        (uint8_t)(*(uint16_t *)arguments[2].argPtr >> 8), // Checksum MSB
        (uint8_t)*(uint16_t *)arguments[2].argPtr,        // Checksum LSB
        *(uint8_t *)arguments[3].argPtr,                  // Firmware target
        FIRMWARE_UPDATE_ACTIVATION_STATUS_REPORT_INVALID_COMBINATION_V5,
        *(uint8_t *)arguments[4].argPtr                   // Hardware version
    };

    command_handler_input_t * pActivationSet;
    pActivationSet = firmware_update_activation_set_frame_create(*(uint16_t *)arguments[0].argPtr,
                                                                 *(uint16_t *)arguments[1].argPtr,
                                                                 *(uint16_t *)arguments[2].argPtr,
                                                                 *(uint8_t *)arguments[3].argPtr,
                                                                 *(uint8_t *)arguments[4].argPtr);

    ZW_APPLICATION_TX_BUFFER frameOut;
    uint8_t frameOutLength;

    invoke_cc_handler_v2(&pActivationSet->rxOptions,
                        &pActivationSet->frame.as_zw_application_tx_buffer,
                        pActivationSet->frameLength, &frameOut, &frameOutLength);

    TEST_ASSERT_EQUAL_INT8_MESSAGE(frameOutLength,
                                    sizeof(EXPECTED_REPORT),
                                    "Frame size does not match");
    TEST_ASSERT_EQUAL_INT8_ARRAY_MESSAGE(EXPECTED_REPORT, &frameOut,
                                          sizeof(EXPECTED_REPORT),
                                          "Frame does not match");
    if (sizeof(uint16_t) == arguments[i].argSize) {
      *(uint16_t *)arguments[i].argPtr ^=0xFFFF;
    } else {
      *(uint8_t *)arguments[i].argPtr ^=0xFF;
    }

    test_common_command_handler_input_free(pActivationSet);
  }

  /* ************************************************************************************** *
   * Send Activation Set with correct parameters and expect the device to start a timer then to reboot and install.
   * ************************************************************************************** */

  mock_call_expect(TO_STR(zaf_config_get_product_type_id), &pMock);
  pMock->return_code.v = PRODUCT_TYPE_ID;

  mock_call_expect(TO_STR(zaf_config_get_product_id), &pMock);
  pMock->return_code.v = PRODUCT_ID;

  mock_call_expect(TO_STR(zaf_config_get_manufacturer_id), &pMock);
  pMock->return_code.v = MANUFACTURER_ID;

  // Send Activation Set and expect the device to reboot and install previously transfered image.

  mock_call_expect(TO_STR(TimerStart), &pMock);
  pMock->compare_rule_arg[0] = COMPARE_NOT_NULL; // Timer handle
  pMock->expect_arg[1].v = 100;
  pMock->return_code.v = ESWTIMER_STATUS_SUCCESS; // Timer handle
  command_handler_input_t * pActivationSet1 = firmware_update_activation_set_frame_create(MANUFACTURER_ID,
                                                                                          FIRMWARE_ID,
                                                                                          CHECKSUM,
                                                                                          FIRMWARE_TARGET,
                                                                                          HARDWARE_VERSION);

  zaf_transport_resume_Expect();
  INVOKE_CC_HANDLER(handleCommandClassFWUpdate, pActivationSet1);

  // Call the function given to AppTimerRegister (i.e. simulate the timer triggered)
  pTimerotasuccess_Callback = pMock_AppTimerRegister_Timerotasuccess->actual_arg[2].p;
  pTimer = pMock_AppTimerRegister_Timerotasuccess->actual_arg[0].p;

  mock_call_expect(TO_STR(ZAF_getZwCommandQueue), &pMock);
  pMock->return_code.p = &m_ZwCommandQueue;

  mock_call_expect(TO_STR(QueueNotifyingSendToBack), &pMock);
  pMock->expect_arg[ARG0].p = &m_ZwCommandQueue;
  pMock->compare_rule_arg[ARG1] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG2].value = 0;
  pMock->return_code.value = EQUEUENOTIFYING_STATUS_SUCCESS;

  pTimerotasuccess_Callback(pTimer);

/* ************************************************************************************** *
   * Send Activation Set with correct parameters and expect the device NOT to Start a timer then to reboot and install.
   * ************************************************************************************** */

  mock_call_expect(TO_STR(zaf_config_get_product_type_id), &pMock);
  pMock->return_code.v = PRODUCT_TYPE_ID;

  mock_call_expect(TO_STR(zaf_config_get_product_id), &pMock);
  pMock->return_code.v = PRODUCT_ID;

  mock_call_expect(TO_STR(zaf_config_get_manufacturer_id), &pMock);
  pMock->return_code.v = MANUFACTURER_ID;

  // Send Activation Set and expect the device to reboot and install previously transfered image.

  mock_call_expect(TO_STR(TimerStart), &pMock);
  pMock->compare_rule_arg[0] = COMPARE_NOT_NULL; // Timer handle
  pMock->expect_arg[1].v = 100;
  pMock->return_code.v = ESWTIMER_STATUS_FAILED; // Timer handle
  command_handler_input_t * pActivationSet2 = firmware_update_activation_set_frame_create(MANUFACTURER_ID,
                                                                                          FIRMWARE_ID,
                                                                                          CHECKSUM,
                                                                                          FIRMWARE_TARGET,
                                                                                          HARDWARE_VERSION);

  zaf_transport_resume_Expect();
  INVOKE_CC_HANDLER(handleCommandClassFWUpdate, pActivationSet2);

  test_common_command_handler_input_free(pFirmwareUpdateMDReport);
  test_common_command_handler_input_free(pRequestGet);
  test_common_command_handler_input_free(pActivationSet1);
  test_common_command_handler_input_free(pActivationSet2);

  mock_calls_verify();
}

/*
 * Testcast to validate we can handle and recover from receiving one OTA fragment
 * with CRC error
 */
void test_ota_util_CRC_error_in_one_fragment(void)
{
  uint8_t otaFragment[] = {0xde, 0xad, 0xbe, 0xef};

  mock_t * pMock = NULL;
  mock_t * pMock_AppTimerRegister_FrameGet = NULL;

  const uint16_t MANUFACTURER_ID = APP_MANUFACTURER_ID;
  const uint16_t PRODUCT_TYPE_ID = 0x00CC;
  const uint16_t PRODUCT_ID = 0x00DD;
  const uint16_t FIRMWARE_ID = 0xCCDD; // Firmware ID is a combination of product type ID and product ID.

  // We do not care about the following functions in this test.
  mock_call_use_as_stub(TO_STR(zpal_bootloader_init));
  mock_call_use_as_stub(TO_STR(zpal_bootloader_write_data));
  mock_call_use_as_stub(TO_STR(MSC_Init));
  mock_call_use_as_stub(TO_STR(Check_not_legal_response_job));
  mock_call_use_as_stub(TO_STR(zaf_config_get_firmware_target_count));
  mock_call_use_as_stub(TO_STR(zpal_bootloader_is_first_boot));
  mock_call_use_as_stub(TO_STR(ZAF_nvm_write)); // Invoked in Request Get handler
  mock_call_use_as_stub(TO_STR(zpal_bootloader_reset_page_counters));

  /*
   * Step 1. Initialization
   */
  mock_call_expect(TO_STR(zpal_bootloader_get_info), &pMock);
  pMock->compare_rule_arg[0] = COMPARE_NOT_NULL;
  zpal_bootloader_info_t info = {
                                  .type = ZPAL_BOOTLOADER_PRESENT,
                                  .capabilities = ZPAL_BOOTLOADER_CAPABILITY_STORAGE
  };
  pMock->output_arg[0].p = &info;

  /*
   * Make sure that ZAF_nvm_get_object_size() returns ZPAL_STATUS_OK indicating that the file is OK.
   */
  mock_call_expect(TO_STR(ZAF_nvm_get_object_size), &pMock);
  pMock->compare_rule_arg[0] = COMPARE_ANY;
  pMock->compare_rule_arg[1] = COMPARE_NOT_NULL;
  size_t fileSize = ZAF_FILE_SIZE_CC_FIRMWARE_UPDATE;
  pMock->output_arg[1].p     = &fileSize;
  pMock->return_code.v       = ZPAL_STATUS_OK;

  // Mock registration for AppTimerRegister call in CC_FirmwareUpdate_Init
  mock_call_expect(TO_STR(AppTimerRegister), &pMock_AppTimerRegister_FrameGet);
  pMock_AppTimerRegister_FrameGet->compare_rule_arg[0] = COMPARE_NOT_NULL;
  pMock_AppTimerRegister_FrameGet->expect_arg[1].v = false;
  pMock_AppTimerRegister_FrameGet->compare_rule_arg[2] = COMPARE_NOT_NULL;
  pMock_AppTimerRegister_FrameGet->return_code.v = true; // Timer handle

  mock_call_expect(TO_STR(AppTimerRegister), &pMock);
  pMock->compare_rule_arg[0] = COMPARE_NOT_NULL;
  pMock->expect_arg[1].v = false;
  pMock->compare_rule_arg[2] = COMPARE_NOT_NULL;
  pMock->return_code.v = true; // Timer handle

  CC_FirmwareUpdate_Init(NULL, NULL, false);

  /*
   * Step 2. Start the OTA process
   */

  mock_call_expect(TO_STR(zaf_config_get_manufacturer_id), &pMock);
  pMock->return_code.v = MANUFACTURER_ID;

  mock_call_expect(TO_STR(zaf_config_get_product_type_id), &pMock);
  pMock->return_code.v = PRODUCT_TYPE_ID;

  mock_call_expect(TO_STR(zaf_config_get_product_id), &pMock);
  pMock->return_code.v = PRODUCT_ID;

  mock_call_expect(TO_STR(ZAF_getAppHandle), &pMock);
  SApplicationHandles AppHandles;
  SNetworkInfo NetworkInfo;
  AppHandles.pNetworkInfo = &NetworkInfo;
  NetworkInfo.MaxPayloadSize = 10;
  pMock->return_code.p = &AppHandles;

  // Simulate that we received a Firmware Update Md Request Get to initiate the OTA
  command_handler_input_t *pRequestGet;
  pRequestGet = firmware_update_md_request_get_v5_frame_create(MANUFACTURER_ID,
                                                               FIRMWARE_ID,
                                                               0xFFFF,
                                                               0,
                                                               sizeof(otaFragment),
                                                               false,
                                                               0);

  mock_call_expect(TO_STR(zaf_config_get_hardware_version), &pMock);
  pMock->return_code.v = 0;

  const uint8_t EXPECTED_REQUEST_REPORT[] = {
                                             COMMAND_CLASS_FIRMWARE_UPDATE_MD_V5,
                                             FIRMWARE_UPDATE_MD_REQUEST_REPORT_V5,
                                             FIRMWARE_UPDATE_MD_REQUEST_REPORT_VALID_COMBINATION_V5
  };
  zaf_transport_rx_to_tx_options_Ignore();
  zaf_transport_tx_ExpectAndReturn(EXPECTED_REQUEST_REPORT, sizeof(EXPECTED_REQUEST_REPORT), NULL, NULL, true);
  zaf_transport_tx_IgnoreArg_callback();
  zaf_transport_tx_IgnoreArg_zaf_tx_options();

  mock_call_expect(TO_STR(TimerIsActive), &pMock);
  pMock->compare_rule_arg[0] = COMPARE_NOT_NULL;
  pMock->return_code.v = false;

  zaf_transport_resume_Expect();
  INVOKE_CC_HANDLER(handleCommandClassFWUpdate, pRequestGet);

  mock_call_expect(TO_STR(TimerIsActive), &pMock);
  pMock->compare_rule_arg[0] = COMPARE_NOT_NULL;
  pMock->return_code.v = false;

  // Mock registration for TimerStart call in TimerStartFwUpdateFrameGet
  mock_call_expect(TO_STR(TimerStart), &pMock);
  pMock->compare_rule_arg[0] = COMPARE_NOT_NULL; // Timer handle
  pMock->expect_arg[1].v = TIMER_START_FWUPDATE_FRAME_GET;
  pMock->return_code.v = 0; // Timer handle

  // Setup the mock for validating the first Firmware Update Md Get command the node sends for OTA fragment 1
  uint8_t numberOfReports = OTA_CACHE_SIZE / sizeof(otaFragment);
  uint16_t nextReportNumber = 1;
  const uint8_t EXPECTED_FRAME[] = {
                                    COMMAND_CLASS_FIRMWARE_UPDATE_MD_V5,
                                    FIRMWARE_UPDATE_MD_GET_V5,
                                    numberOfReports,
                                    nextReportNumber >> 8,    // Report number MSB
                                    nextReportNumber & 0x00FF // Report number LSB - expect to request fragment number 1.
  };

  zaf_transport_rx_to_tx_options_Ignore();
  zaf_transport_tx_ExpectAndReturn(EXPECTED_FRAME, sizeof(EXPECTED_FRAME), NULL, NULL, true);
  zaf_transport_tx_IgnoreArg_callback();
  zaf_transport_tx_IgnoreArg_zaf_tx_options();

  // Trigger the callback function to complete the Firmware Update Md Request Report tx.
  // This will then trigger the node to send the first Firmware Update Md Get command for OTA fragment 1.
  transmission_result_t pTxResult = {
    .status = TRANSMIT_COMPLETE_OK
  };
  ZCB_CmdClassFwUpdateMdReqReport(&pTxResult);


  /*
   * Step 3. Receive the first fragment with CRC error
   */
  /* Simulate we received the first OTA fragment and it has CRC error */
  command_handler_input_t * pReport;
  pReport = firmware_update_meta_data_report_frame_create(false,
                                                          1,
                                                          sizeof(otaFragment));
  pReport->frame.as_byte_array[pReport->frameLength-1] = 0x01; // Corrupt the last byte (CRC)

  zaf_transport_resume_Expect();
  INVOKE_CC_HANDLER(handleCommandClassFWUpdate, pReport);

  const uint8_t FIRMWARE_UPDATE_REQUEST_TIMEOUTS = 5;

  // Setup the mock for the Firmware Update Md Get command, and call the retry timer callback
  // function enough times to trigger the node asking for OTA fragment 1 once again.
  const uint8_t EXPECTED_FRAME_2[] = {
                                    COMMAND_CLASS_FIRMWARE_UPDATE_MD_V5,
                                    FIRMWARE_UPDATE_MD_GET_V5,
                                    numberOfReports,
                                    nextReportNumber << 8, // Report number MSB
                                    nextReportNumber & 0xFF  // Report number LSB
  };

  for (uint8_t i=0; i <= FIRMWARE_UPDATE_REQUEST_TIMEOUTS; i++)
  {
    zaf_transport_rx_to_tx_options_Ignore();
    zaf_transport_tx_ExpectAndReturn(EXPECTED_FRAME_2, sizeof(EXPECTED_FRAME_2), NULL, NULL, true);
    zaf_transport_tx_IgnoreArg_callback();
    zaf_transport_tx_IgnoreArg_zaf_tx_options();

    zaf_transport_resume_Expect();

    // Call the function given to AppTimerRegister (i.e. simulate the timer triggered)
    void (* AppTimerRegister_callback)(void);
    AppTimerRegister_callback = pMock_AppTimerRegister_FrameGet->actual_arg[2].p;
    AppTimerRegister_callback();
  }

  /*
   * Step 4: Repeat the sequence but this time without injecting CRC error.
   * The expected behavior is then that the fragment exchange will recover and the node will
   * accept OTA fragment no. 1 and then go on and ask for OTA fragment no. 2.
   */

  // Setup the mock for validating the Firmware Update Md Get command the node sends
  // for OTA fragment no. 2
  numberOfReports -= 1;  // one report successfully received, now reducing remaining numberOfReports
  nextReportNumber += 1; // and increasing next expected report
  const uint8_t EXPECTED_FRAME_3[] = {
                                      COMMAND_CLASS_FIRMWARE_UPDATE_MD_V5,
                                      FIRMWARE_UPDATE_MD_GET_V5,
                                      numberOfReports,
                                      nextReportNumber << 8, // Report number MSB
                                      nextReportNumber & 0xFF  // Report number LSB
                                      // Node should now ask for 'numberOfReports' OTA fragments starting with  no. 2
  };

  // Simulate that we again receive OTA fragment no. 1 and this time without CRC error
  // numberOfReports om FW Update GET is greater than 1, so no need to send MD GET again.
  command_handler_input_t * pReport2;
  pReport2 = firmware_update_meta_data_report_frame_create(false,
                                                          1,
                                                          sizeof(otaFragment));

  mock_call_expect(TO_STR(ZAF_transportSendDataAbort), &pMock);

  zaf_transport_resume_Expect();
  INVOKE_CC_HANDLER(handleCommandClassFWUpdate, pReport2);

  /*
   * Done. If the test reaches this point without failing, then we have established that the OTA
   * state machine can handle and recover from receiving an OTA fragment with CRC error
   *
   * Finish up by sending one last fragment with the FIRMWARE_UPDATE_MD_REPORT_PROPERTIES1_LAST_BIT_MASK
   * properties bit set.
   * This is just to leave the state machine in a known finished state. The final CRC checksum of the
   * complete image doesn't match. We don't care. It's out of the scope of this test to validate that.
   */

  // Mock registration for TimerStop call in TimerCancelFwUpdateFrameGet
  mock_call_expect(TO_STR(TimerIsActive), &pMock);
  pMock->expect_arg[0].p = pMock_AppTimerRegister_FrameGet->actual_arg[0].p;
  pMock->return_code.v = true;

  mock_call_expect(TO_STR(TimerStop), &pMock);
  pMock->expect_arg[0].p = pMock_AppTimerRegister_FrameGet->actual_arg[0].p; // Compare with the previously registered timer handle

  // For the last fragment we expect the state machine to abort the update process with
  // status="Unable to receive" because the CRC checksum of the entire image doesn't match

  // Simulate that we receive the last fragment with FIRMWARE_UPDATE_MD_REPORT_PROPERTIES1_LAST_BIT_MASK bit set
  command_handler_input_t * pReport3;
  pReport3 = firmware_update_meta_data_report_frame_create(true,  // Last report number
                                                           3,
                                                           sizeof(otaFragment));

  for (uint8_t i=0; i < FIRMWARE_UPDATE_MAX_RETRY-1 ; i++)
  {
    // Repeat sending MD Get until max retries reached.
    zaf_transport_rx_to_tx_options_Ignore();
    zaf_transport_tx_ExpectAndReturn(EXPECTED_FRAME_3, sizeof(EXPECTED_FRAME_3), NULL, NULL, true);
    zaf_transport_tx_IgnoreArg_callback();
    zaf_transport_tx_IgnoreArg_zaf_tx_options();

    zaf_transport_resume_Expect();

    // Call the function given to AppTimerRegister (i.e. simulate the timer triggered)
    void (* AppTimerRegister_callback)(void);
    AppTimerRegister_callback = pMock_AppTimerRegister_FrameGet->actual_arg[2].p;
    AppTimerRegister_callback();
  }

  const uint8_t EXPECTED_FRAME_4[] = {
                                      COMMAND_CLASS_FIRMWARE_UPDATE_MD_V5,
                                      FIRMWARE_UPDATE_MD_STATUS_REPORT_V5,
                                      0, // status = FIRMWARE_UPDATE_MD_STATUS_REPORT_UNABLE_TO_RECEIVE_WITHOUT_CHECKSUM_ERROR_V5
                                      0, // Wait time MSB
                                      2  // Wait time LSB - waitTime = WAITTIME_FWU_FAIL
  };
  zaf_transport_rx_to_tx_options_Ignore();
  zaf_transport_tx_ExpectAndReturn(EXPECTED_FRAME_4, sizeof(EXPECTED_FRAME_4), NULL, NULL, true);
  zaf_transport_tx_IgnoreArg_callback();
  zaf_transport_tx_IgnoreArg_zaf_tx_options();

  zaf_transport_resume_Expect();

  // Trigger callback one more time. This should simulate Max retry.
  // Instead of sending FW Update MD Get again, it should send Status Report.
  void (* AppTimerRegister_callback)(void);
  AppTimerRegister_callback = pMock_AppTimerRegister_FrameGet->actual_arg[2].p;
  AppTimerRegister_callback();

  zaf_transport_resume_Expect();
  INVOKE_CC_HANDLER(handleCommandClassFWUpdate, pReport3);

  test_common_command_handler_input_free(pRequestGet);
  test_common_command_handler_input_free(pReport);
  test_common_command_handler_input_free(pReport2);
  test_common_command_handler_input_free(pReport3);

  mock_calls_verify();
}


/*
 * @attention Please do not alter this unit-test as it is made to replicate a test-case made by SAQ.
 *
 * The test case is defined in: http://testrail.silabs.com/index.php?/cases/view/67890
 *
 * What is to be tested:
 *   The DUT being a WakeUp device is expected upon receiving a bad fragment with a CRC error to
 *   ask for the same fragment again.
 *
 * @notice This behavior is not defined by the Command Class Protocol specifications for
 *         CC_Firmware_Update and is implementation dependent!
 */
void test_ota_util_CRC_error_in_one_fragment_as_in_C67890(void)
{
  mock_calls_clear();

  uint8_t otaFragment[] = {0xde, 0xad, 0xbe, 0xef};

  mock_t * pMock = NULL;
  mock_t * pMock_AppTimerRegister_FrameGet = NULL;

  const uint16_t MANUFACTURER_ID = APP_MANUFACTURER_ID;
  const uint16_t PRODUCT_TYPE_ID = 0x00CC;
  const uint16_t PRODUCT_ID = 0x00DD;
  const uint16_t FIRMWARE_ID = 0xCCDD; // Firmware ID is a combination of product type ID and product ID.

  // We do not care about the following functions in this test.
  mock_call_use_as_stub(TO_STR(zpal_pm_register));
  mock_call_use_as_stub(TO_STR(ZAF_transportSendDataAbort));
  mock_call_use_as_stub(TO_STR(MSC_Init));
  mock_call_use_as_stub(TO_STR(Check_not_legal_response_job));
  mock_call_use_as_stub(TO_STR(zaf_config_get_firmware_target_count));
  mock_call_use_as_stub(TO_STR(TimerIsActive));
  mock_call_use_as_stub(TO_STR(TimerStop));
  mock_call_use_as_stub(TO_STR(zpal_bootloader_init));
  mock_call_use_as_stub(TO_STR(zpal_bootloader_is_first_boot));
  mock_call_use_as_stub(TO_STR(ZAF_nvm_write));
  mock_call_use_as_stub(TO_STR(zpal_bootloader_reset_page_counters));

  /*
   * Step 1. Initialization
   */
  mock_call_expect(TO_STR(zpal_bootloader_get_info), &pMock);
  pMock->compare_rule_arg[0] = COMPARE_NOT_NULL;
  zpal_bootloader_info_t info = {
                                  .type = ZPAL_BOOTLOADER_PRESENT,
                                  .capabilities = ZPAL_BOOTLOADER_CAPABILITY_STORAGE
  };
  pMock->output_arg[0].p = &info;

  /*
   * Make sure that nvm3_getObjectInfo() returns ECODE_OK indicating that the file is OK.
   */
  mock_call_expect(TO_STR(ZAF_nvm_get_object_size), &pMock);
  pMock->compare_rule_arg[0] = COMPARE_ANY;
  pMock->compare_rule_arg[1] = COMPARE_ANY;
  size_t fileSize = ZAF_FILE_SIZE_CC_FIRMWARE_UPDATE;
  pMock->output_arg[1].p     = &fileSize;
  pMock->return_code.v       = ZPAL_STATUS_OK;

  // Mock registration for AppTimerRegister call in CC_FirmwareUpdate_Init
  mock_call_expect(TO_STR(AppTimerRegister), &pMock_AppTimerRegister_FrameGet);
  pMock_AppTimerRegister_FrameGet->compare_rule_arg[0] = COMPARE_NOT_NULL;
  pMock_AppTimerRegister_FrameGet->expect_arg[1].v = false;
  pMock_AppTimerRegister_FrameGet->compare_rule_arg[2] = COMPARE_NOT_NULL;
  pMock_AppTimerRegister_FrameGet->return_code.v = true; // Timer handle

  mock_call_expect(TO_STR(AppTimerRegister), &pMock);
  pMock->compare_rule_arg[0] = COMPARE_NOT_NULL;
  pMock->expect_arg[1].v = false;
  pMock->compare_rule_arg[2] = COMPARE_NOT_NULL;
  pMock->return_code.v = true; // Timer handle

  CC_FirmwareUpdate_Init(NULL, NULL, false);

  /*
   * Step 2. Start the OTA process
   *
   * Send a FW Update Request Get Command to self to initiate the OTA process.
   * Then send a FIRMWARE_UPDATE_MD_REQUEST_REPORT as a response to the request.
   */

  mock_call_expect(TO_STR(zaf_config_get_manufacturer_id), &pMock);
  pMock->return_code.v = MANUFACTURER_ID;

  mock_call_expect(TO_STR(zaf_config_get_product_type_id), &pMock);
  pMock->return_code.v = PRODUCT_TYPE_ID;

  mock_call_expect(TO_STR(zaf_config_get_product_id), &pMock);
  pMock->return_code.v = PRODUCT_ID;

  mock_call_expect(TO_STR(ZAF_getAppHandle), &pMock);
  SApplicationHandles AppHandles;
  SNetworkInfo NetworkInfo;
  AppHandles.pNetworkInfo = &NetworkInfo;
  NetworkInfo.MaxPayloadSize = 10;
  pMock->return_code.p = &AppHandles;

  /**
   * Simulate that we received a Firmware Update Md Request Get to initiate the OTA.
   */
  command_handler_input_t *pRequestGet;
  pRequestGet = firmware_update_md_request_get_v5_frame_create(MANUFACTURER_ID,
                                                               FIRMWARE_ID,
                                                               0xFFFF,
                                                               0,
                                                               sizeof(otaFragment),
                                                               false,  // We are initiating an OTA and not applying an image that was already transmitted.
                                                               0);

  mock_call_expect(TO_STR(zaf_config_get_hardware_version), &pMock);
  pMock->return_code.v = 0;

  /**
   * Prepare a response for the FIRMWARE_UPDATE_MD_REQUEST_Get that we send to self.
   */

  const uint8_t EXPECTED_REQUEST_REPORT[] = {
                                             COMMAND_CLASS_FIRMWARE_UPDATE_MD_V5,
                                             FIRMWARE_UPDATE_MD_REQUEST_REPORT_V5,
                                             FIRMWARE_UPDATE_MD_REQUEST_REPORT_VALID_COMBINATION_V5
  };
  zaf_transport_rx_to_tx_options_Ignore();
  zaf_transport_tx_ExpectAndReturn(EXPECTED_REQUEST_REPORT, sizeof(EXPECTED_REQUEST_REPORT), NULL, NULL, true);
  zaf_transport_tx_IgnoreArg_callback();
  zaf_transport_tx_IgnoreArg_zaf_tx_options();

  /***********************************************************************
   * This test is for SWPROT-5886 (added as part of SWPROT-5921) (PART 1)
   **********************************************************************/
  mock_call_expect(TO_STR(zpal_pm_stay_awake), &pMock);
  pMock->compare_rule_arg[0] = COMPARE_NOT_NULL;
  pMock->expect_arg[1].v = 2400000;  // OTA_AWAKE_PERIOD_LONG_TERM

  zaf_transport_resume_Expect();
  INVOKE_CC_HANDLER(handleCommandClassFWUpdate, pRequestGet);  // send the FW Update Request Get Command to self.
  //printf("handleCommandClassFWUpdate() - send the FW Update Request Get Command to self\n");
  //printf("\n  zpal_pm_stay_awake() with 2400000ms PASS! (C67890 part 1) \n\n");

  /*
   * The reception of the ACK for the above Request Get command is simulated below with the call to
   * ZCB_CmdClassFwUpdateMdReqReport() which is also sending the first FIRMWARE_UPDATE_MD_GET.
   */

  /**
   * The DUT is now requesting an image fraction with a FIRMWARE_UPDATE_MD_GET with report number 1 (first fragment).
   */

  // Setup the mock for validating the first Firmware Update Md Get command the node sends for OTA fragment 1
  uint8_t numberOfReports = OTA_CACHE_SIZE / sizeof(otaFragment);
  uint16_t nextReportNumber = 1;
  const uint8_t EXPECTED_MD_GET_FRAME[] = {
                                    COMMAND_CLASS_FIRMWARE_UPDATE_MD_V5,
                                    FIRMWARE_UPDATE_MD_GET_V5,
                                    numberOfReports,
                                    nextReportNumber >> 8,    // Report number MSB
                                    nextReportNumber & 0x00FF // Report number LSB - expect to request fragment number 1.
  };

  zaf_transport_rx_to_tx_options_Ignore();
  zaf_transport_tx_ExpectAndReturn(EXPECTED_MD_GET_FRAME, sizeof(EXPECTED_MD_GET_FRAME), NULL, NULL, true);
  zaf_transport_tx_IgnoreArg_callback();
  zaf_transport_tx_IgnoreArg_zaf_tx_options();

  // Trigger the callback function to complete the Firmware Update Md Request Report tx.
  // This will then trigger the node to send the first Firmware Update Md Get command for OTA fragment 1.
  transmission_result_t pTxResult = {
    .status = TRANSMIT_COMPLETE_OK
  };
  ZCB_CmdClassFwUpdateMdReqReport(&pTxResult);  // This starts the OTA binary data flow.

  /*
   * Md Get CMD transmitted!
   */

  printf("The OTA binary data flow has started! (First Md Get CMD transmitted)\n");


  /*****************************************************************************
   * Step 3a. Create and transmit the first Md Report CMD to self.
   * (Receive the first fragment with report number 1 with correct CRC!)
   ****************************************************************************/

  printf(" == TRANSMIT REPORT NUMBER 1 \"with correct CRC\"! ==\n");

  // Create report
  command_handler_input_t * pReport1;
  pReport1 = firmware_update_meta_data_report_frame_create(false,
                                                          1,  // report number
                                                          sizeof(otaFragment));

  // Mock registration for TimerStart call in timerFwUpdateFrameGetStart
  mock_call_expect(TO_STR(TimerStart), &pMock);
  pMock->compare_rule_arg[0] = COMPARE_NOT_NULL; // Timer handle
  pMock->expect_arg[1].v = TIMER_START_FWUPDATE_FRAME_GET + 0;  // + no retry, since it is good the first time we send the report.
  pMock->return_code.v = 0;  // Success

  printf("handleCommandClassFWUpdate() - Receive the report. (Send the report to self)\n");
  zaf_transport_resume_Expect();
  INVOKE_CC_HANDLER(handleCommandClassFWUpdate, pReport1);  // Receive report 1. (Send the report to self)
  // A timer is being started here with a callback. When timed out, then further Md Get CMDs are sent to keep the flow going.
  // This is simulated with us calling its timeout callback below, AppTimerRegister_callback() (ZCB_TimerOutFwUpdateFrameGet()).

  /*
   * Md Get timer timed out! (Send new Md Get CMD)
   */

  /*
   * Expected condition:
   * We know now that the report has been accepted due to correct CRC.
   */

  // Setup the mock for validating the Firmware Update Md Get command the node sends
  // for OTA fragment no. 2
  numberOfReports--;  // one report successfully received, now reducing remaining numberOfReports.
  nextReportNumber++; // and increasing next expected report number.
  printf("    numberOfReports: %d, nextReportNumber: %d\n", numberOfReports, nextReportNumber);

  const uint8_t EXPECTED_MD_GET_FRAME_2[] = {
                                    COMMAND_CLASS_FIRMWARE_UPDATE_MD_V5,
                                    FIRMWARE_UPDATE_MD_GET_V5,
                                    numberOfReports,
                                    nextReportNumber >> 8,     // Report number MSB
                                    nextReportNumber & 0x00FF  // Report number LSB
  };

  // Run once, no retry of sending Md Get CMD needed! (Simulated Md Get CMD timeout and transmit)
  {
    // Make transmission of FIRMWARE_UPDATE_MD_GET

    // Mock registration for TimerStart call in timerFwUpdateFrameGetStart
    mock_call_expect(TO_STR(TimerStart), &pMock);
    pMock->compare_rule_arg[0] = COMPARE_NOT_NULL; // Timer handle
    pMock->expect_arg[1].v = TIMER_START_FWUPDATE_FRAME_GET + 1500;  // + one retry.
    pMock->return_code.v = 0;  // Success

    zaf_transport_rx_to_tx_options_Ignore();
    zaf_transport_tx_ExpectAndReturn(EXPECTED_MD_GET_FRAME_2, sizeof(EXPECTED_MD_GET_FRAME_2), NULL, NULL, true);
    zaf_transport_tx_IgnoreArg_callback();
    zaf_transport_tx_IgnoreArg_zaf_tx_options();

    zaf_transport_resume_Expect();

    // Call the CB function given to AppTimerRegister (i.e. simulate the timer triggered)
    void (* AppTimerRegister_callback)(void);
    AppTimerRegister_callback = pMock_AppTimerRegister_FrameGet->actual_arg[2].p;
    AppTimerRegister_callback();  // Simulate timeout and send the new Md Get CMD.

    zaf_transport_pause_Expect();

    g_request_get_cb(NULL);
  }

  /*
   * Md Get CMD transmitted!
   */

  printf("===================================================================================\n");


  /*****************************************************************************
   * Step 3b. Create and transmit the second Md Report CMD to self.
   * (Receive the second fragment with report number 2 with CRC error!)
   ****************************************************************************/

  printf(" == TRANSMIT REPORT NUMBER 2 \"with CRC error\"! ==\n");

  // Create report
  command_handler_input_t * pReport2;
  pReport2 = firmware_update_meta_data_report_frame_create(false,
                                                          2,  // report number
                                                          sizeof(otaFragment));

  // Corrupt the CRC
  printf("The last byte is the LSB of the CRC: 0x%02x\n", pReport2->frame.as_byte_array[pReport2->frameLength-1]);
  pReport2->frame.as_byte_array[pReport2->frameLength - 1] = 0x01; // Corrupt the last byte (CRC)
  printf("The LSB of the CRC was corrupted to: 0x%02x\n", pReport2->frame.as_byte_array[pReport2->frameLength-1]);

  printf("handleCommandClassFWUpdate() - Receive the report. (Send the report to self)\n");
  zaf_transport_resume_Expect();
  INVOKE_CC_HANDLER(handleCommandClassFWUpdate, pReport2);  // Receive report 2. (Send the report to self)
  // A timer is being started here with a callback. When timed out, then further Md Get CMDs are sent to keep the flow going.
  // This is simulated with us calling its timeout callback below, AppTimerRegister_callback() (ZCB_TimerOutFwUpdateFrameGet()).

  /*
   * Md Get timer timed out! (Send new Md Get CMD)
   */

  /*
   * Expected condition:
   * We know that the Md Report has been dropped due to CRC error.
   * A timer for the next Md Get CMD is ticking at this point.
   * Setup a mock for the Firmware Update Md Get command to be send, and call the retry timer
   * callback function that triggers the transmission enough times to trigger the node asking
   * for OTA fragment 2 once again.
   */

  // Setup the mock for validating the Firmware Update Md Get command the node re-sends
  // for OTA fragment no. 2 again
  //  numberOfReports--;  // Last report was not accepted after reception!
  //  nextReportNumber++;
  printf("    numberOfReports: %d, nextReportNumber: %d\n", numberOfReports, nextReportNumber);

  const uint8_t EXPECTED_MD_GET_FRAME_2a[] = {
                                    COMMAND_CLASS_FIRMWARE_UPDATE_MD_V5,
                                    FIRMWARE_UPDATE_MD_GET_V5,
                                    numberOfReports,
                                    nextReportNumber >> 8,     // Report number MSB
                                    nextReportNumber & 0x00FF  // Report number LSB
  };

  // Run once, no retry of sending Md Get CMD needed! (Simulated Md Get CMD timeout and transmit)
  {
    // Make transmission of FIRMWARE_UPDATE_MD_GET

    // Mock registration for TimerStart call in timerFwUpdateFrameGetStart
    mock_call_expect(TO_STR(TimerStart), &pMock);
    pMock->compare_rule_arg[0] = COMPARE_NOT_NULL; // Timer handle
    pMock->expect_arg[1].v = TIMER_START_FWUPDATE_FRAME_GET + 3000;  // + two retry. // TODO this might be a problem, 1500 is correct!
    pMock->return_code.v = 0;  // Success

    zaf_transport_rx_to_tx_options_Ignore();
    zaf_transport_tx_ExpectAndReturn(EXPECTED_MD_GET_FRAME_2a, sizeof(EXPECTED_MD_GET_FRAME_2a), NULL, NULL, true);
    zaf_transport_tx_IgnoreArg_callback();
    zaf_transport_tx_IgnoreArg_zaf_tx_options();

    zaf_transport_resume_Expect();

    // Call the CB function given to AppTimerRegister (i.e. simulate the timer triggered)
    void (* AppTimerRegister_callback)(void);
    AppTimerRegister_callback = pMock_AppTimerRegister_FrameGet->actual_arg[2].p;
    AppTimerRegister_callback();  // Simulate timeout and send the new Md Get CMD.

    zaf_transport_pause_Expect();

    g_request_get_cb(NULL);
  }

  /*
   * Md Get CMD transmitted!
   */

  printf("===================================================================================\n");



  /********************************************************************************************
   * Step 4: Repeat the sequence but this time without injecting CRC error.
   * The expected behavior is then that the fragment exchange will recover and the node will
   * accept OTA fragment no. 2 and then go on and ask for OTA fragment no. 3 in the next step.
   *******************************************************************************************/

  printf(" == TRANSMIT REPORT NUMBER 2 again \"with correct CRC\"! ==\n");

  // Create report
  command_handler_input_t * pReport2a;
  pReport2a = firmware_update_meta_data_report_frame_create(false,
                                                          2,  // report number
                                                          sizeof(otaFragment));

  // Mock registration for TimerStart call in timerFwUpdateFrameGetStart
  mock_call_expect(TO_STR(TimerStart), &pMock);
  pMock->compare_rule_arg[0] = COMPARE_NOT_NULL; // Timer handle
  pMock->expect_arg[1].v = TIMER_START_FWUPDATE_FRAME_GET + 0;  // + no retry, since it is good the first time we send the report.
  pMock->return_code.v = 0;  // Success

  zaf_transport_resume_Expect();
  INVOKE_CC_HANDLER(handleCommandClassFWUpdate, pReport2a);  // Receive report 2 agin. (Send the report to self)
  // A timer is being started here with a callback. When timed out, then further Md Get CMDs are sent to keep the flow going.
  // This is simulated with us calling its timeout callback below, AppTimerRegister_callback() (ZCB_TimerOutFwUpdateFrameGet()).

  /*
   * Md Get timer timed out! (Send new Md Get CMD)
   */

  /*
   * Expected condition:
   * We know now that the report has been accepted due to correct CRC.
   * If the test reaches this point without failing, then we have established that the OTA
   * state machine can handle and recover from receiving an OTA fragment with CRC error.
   */

  // Setup the mock for validating the Firmware Update Md Get command the node sends
  // for OTA fragment no. 3
  numberOfReports--;  // one report successfully received, now reducing remaining numberOfReports.
  nextReportNumber++; // and increasing next expected report number.
  printf("    numberOfReports: %d, nextReportNumber: %d\n", numberOfReports, nextReportNumber);

  const uint8_t EXPECTED_MD_GET_FRAME_3[] = {
                                      COMMAND_CLASS_FIRMWARE_UPDATE_MD_V5,
                                      FIRMWARE_UPDATE_MD_GET_V5,
                                      numberOfReports,
                                      nextReportNumber << 8,     // Report number MSB
                                      nextReportNumber & 0x00FF  // Report number LSB
                                      // Node should now ask for 'numberOfReports' OTA fragments starting with  no. 2
  };

  // Run once, no retry of sending Md Get CMD needed! (Simulated Md Get CMD timeout and transmit)
  {
    // Make transmission of FIRMWARE_UPDATE_MD_GET

    // Mock registration for TimerStart call in timerFwUpdateFrameGetStart
    mock_call_expect(TO_STR(TimerStart), &pMock);
    pMock->compare_rule_arg[0] = COMPARE_NOT_NULL; // Timer handle
    pMock->expect_arg[1].v = TIMER_START_FWUPDATE_FRAME_GET + 1500;  // + one retry.
    pMock->return_code.v = 0;  // Success

    zaf_transport_rx_to_tx_options_Ignore();
    zaf_transport_tx_ExpectAndReturn(EXPECTED_MD_GET_FRAME_3, sizeof(EXPECTED_MD_GET_FRAME_3), NULL, NULL, true);
    zaf_transport_tx_IgnoreArg_callback();
    zaf_transport_tx_IgnoreArg_zaf_tx_options();

    zaf_transport_resume_Expect();

    // Call the function given to AppTimerRegister (i.e. simulate the timer triggered)
    void (* AppTimerRegister_callback)(void);
    AppTimerRegister_callback = pMock_AppTimerRegister_FrameGet->actual_arg[2].p;
    AppTimerRegister_callback();  // Simulate timeout and send the new Md Get CMD.

    zaf_transport_pause_Expect();

    g_request_get_cb(NULL);
  }

  /*
   * Md Get CMD transmitted for report number 3!
   */

  printf("===================================================================================\n");

  /********************************************************************************************
   * Step 5: Create and transmit the last Md Report CMD to self.
   *******************************************************************************************/

  printf(" == TRANSMIT REPORT NUMBER 3 (last) \"with correct CRC\"! ==\n");

  /*
   * Finish up by sending one last fragment with the FIRMWARE_UPDATE_MD_REPORT_PROPERTIES1_LAST_BIT_MASK
   * properties bit set.
   * This is just to leave the state machine in a known finished state. The final CRC checksum of the
   * complete image doesn't match. We don't care. It's out of the scope of this test to validate that.
   */

  // Simulate that we receive the last fragment with FIRMWARE_UPDATE_MD_REPORT_PROPERTIES1_LAST_BIT_MASK bit set
  command_handler_input_t * pReport3;
  pReport3 = firmware_update_meta_data_report_frame_create(false,  // Last report number
                                                           3,     // report number
                                                           sizeof(otaFragment));

  // Mock registration for TimerStart call in timerFwUpdateFrameGetStart
  mock_call_expect(TO_STR(TimerStart), &pMock);
  pMock->compare_rule_arg[0] = COMPARE_NOT_NULL; // Timer handle
  pMock->expect_arg[1].v = TIMER_START_FWUPDATE_FRAME_GET + 0;  // + no retry, since it is good the first time we send the report.
  pMock->return_code.v = 0;  // Success

  zaf_transport_resume_Expect();
  INVOKE_CC_HANDLER(handleCommandClassFWUpdate, pReport3);  // Receive report 3. (Send the report to self)
  printf("Delivered report number 3 \n");
  // A timer is being started here with a callback. When timed out, then further Md Get CMDs are sent to keep the flow going.
  // This is simulated with us calling its timeout callback below, AppTimerRegister_callback() (ZCB_TimerOutFwUpdateFrameGet()).

  /*******************************************************************************************************
   * Test is complete and we have recovered from a failed report due to CRC error.
   * We are ending this test case by creating an error to terminate the OTA process and finish with
   * sending the status report.
   * We will hit the retry limit for sending the same Md Get CMD and see the ZCB_FinishFwUpdate()
   * being called.
   * Requesting report number 4 will not lead to any response.
   * It is only used to make many subsequent request and hit the retry limit and reset
   * the state machine of the OTA module.
   ******************************************************************************************************/
  /*
   * To set the state machine to IDLE, we need to hit the retry limit here and generate the event:
   * FW_EVENT_MAX_RETRIES_REACHED, so that the FIRMWARE_UPDATE_MD_STATUS_REPORT is being sent.
   */

  /// This GET will never receive its report!
  // Setup the mock for validating the Firmware Update Md Get command the node sends
  // for OTA fragment no. 4
  numberOfReports--;  // one report successfully received, now reducing remaining numberOfReports.
  nextReportNumber++; // and increasing next expected report number.
  printf("    numberOfReports: %d, nextReportNumber: %d\n", numberOfReports, nextReportNumber);

  const uint8_t EXPECTED_MD_GET_FRAME_4[] = {
                                      COMMAND_CLASS_FIRMWARE_UPDATE_MD_V5,
                                      FIRMWARE_UPDATE_MD_GET_V5,
                                      numberOfReports,
                                      nextReportNumber << 8,     // Report number MSB
                                      nextReportNumber & 0x00FF  // Report number LSB
                                      // Node should now ask for 'numberOfReports' OTA fragments starting with  no. 2
  };

  // Hit the retry limit...
  for (uint8_t i = 0; i < FIRMWARE_UPDATE_MAX_RETRY - 1; i++)
  {
    // Mock registration for TimerStart call in timerFwUpdateFrameGetStart
    mock_call_expect(TO_STR(TimerStart), &pMock);
    pMock->compare_rule_arg[0] = COMPARE_NOT_NULL; // Timer handle
    pMock->expect_arg[1].v = TIMER_START_FWUPDATE_FRAME_GET + 1500 * (i + 1);  // + i number of retry.
    pMock->return_code.v = 0;  // Success

    // Make transmission of FIRMWARE_UPDATE_MD_GET
    zaf_transport_rx_to_tx_options_Ignore();
    zaf_transport_tx_ExpectAndReturn(EXPECTED_MD_GET_FRAME_4, sizeof(EXPECTED_MD_GET_FRAME_4), NULL, NULL, true);
    zaf_transport_tx_IgnoreArg_callback();
    zaf_transport_tx_IgnoreArg_zaf_tx_options();

    /**
     * Md Get timer timed out! (Send new Md Get CMD)
     * (retry 10 times until the OTA is aborted)
     */

    zaf_transport_resume_Expect();

    // Call the function given to AppTimerRegister (i.e. simulate the timer triggered)
    void (* AppTimerRegister_callback)(void);
    AppTimerRegister_callback = pMock_AppTimerRegister_FrameGet->actual_arg[2].p;
    AppTimerRegister_callback();  // Simulate timeout and send the new Md Get CMD.

    zaf_transport_pause_Expect();

    g_request_get_cb(NULL);
  }

  /**
   * We have exhausted our tries.
   */

  /********************************************************************
   * This will make a FIRMWARE_UPDATE_MD_STATUS_REPORT transmission.
   *******************************************************************/

  {
    const uint8_t EXPECTED_MD_STATUS_REPORT_FRAME_4[] = {
                                        COMMAND_CLASS_FIRMWARE_UPDATE_MD_V5,
                                        FIRMWARE_UPDATE_MD_STATUS_REPORT_V5,
                                        0, // status = FIRMWARE_UPDATE_MD_STATUS_REPORT_UNABLE_TO_RECEIVE_WITHOUT_CHECKSUM_ERROR_V5
                                        0, // Wait time MSB
                                        2  // Wait time LSB - waitTime = WAITTIME_FWU_FAIL
    };

    zaf_transport_rx_to_tx_options_Ignore();
    zaf_transport_tx_ExpectAndReturn(EXPECTED_MD_STATUS_REPORT_FRAME_4, sizeof(EXPECTED_MD_STATUS_REPORT_FRAME_4), NULL, NULL, true);
    zaf_transport_tx_IgnoreArg_callback();
    zaf_transport_tx_IgnoreArg_zaf_tx_options();

    zaf_transport_resume_Expect();

    // Trigger callback one more time. This should simulate Max retry.
    // Instead of sending FW Update MD Get again, it should send Status Report.
    void (* AppTimerRegister_callback)(void);
    AppTimerRegister_callback = pMock_AppTimerRegister_FrameGet->actual_arg[2].p;
    AppTimerRegister_callback();

    /***********************************************************************
     * This test is for SWPROT-5886 (added as part of SWPROT-5921) (PART 2)
     **********************************************************************/
    mock_call_expect(TO_STR(zpal_pm_stay_awake), &pMock);
    pMock->compare_rule_arg[0] = COMPARE_NOT_NULL;
    pMock->expect_arg[1].v = 100;  // OTA_AWAKE_PERIOD_GRACEFUL_OFF

    zaf_transport_pause_Expect();

    g_request_get_cb(NULL);
    printf("\n  zpal_pm_stay_awake() with 100ms PASS! (C67890 part 2) \n\n");
  }

  test_common_command_handler_input_free(pRequestGet);
  test_common_command_handler_input_free(pReport1);
  test_common_command_handler_input_free(pReport2);
  test_common_command_handler_input_free(pReport2a);
  test_common_command_handler_input_free(pReport3);

  mock_calls_verify();
}



/*
 * Testcase to validate we can handle a situation where the same OTA fragment continues to have
 * CRC error.
 * The expected behavior is that the OTA state machine aborts the update process with status="Unable
 * to receive" after a defined number of retries.
 */
void test_ota_util_CRC_error_in_all_fragments(void)
{
  uint8_t otaFragment[] = {0xde, 0xad, 0xbe, 0xef};

  mock_t * pMock = NULL;
  mock_t * pMockAppTimerRegister_otaSuccess = NULL;
  mock_t * pMock_AppTimerRegister_FrameGet = NULL;

  const uint16_t MANUFACTURER_ID = APP_MANUFACTURER_ID;
  const uint16_t PRODUCT_TYPE_ID = 0x00CC;
  const uint16_t PRODUCT_ID = 0x00DD;
  const uint16_t FIRMWARE_ID = 0xCCDD; // Firmware ID is a combination of product type ID and product ID.

  mock_call_use_as_stub(TO_STR(zpal_bootloader_init));
  mock_call_use_as_stub(TO_STR(zpal_bootloader_write_data));
  mock_call_use_as_stub(TO_STR(MSC_Init));
  mock_call_use_as_stub(TO_STR(Check_not_legal_response_job));
  mock_call_use_as_stub(TO_STR(zaf_config_get_firmware_target_count));
  mock_call_use_as_stub(TO_STR(zpal_bootloader_is_first_boot));
  mock_call_use_as_stub(TO_STR(ZAF_nvm_write)); // Invoked in Request Get handler
  mock_call_use_as_stub(TO_STR(zpal_bootloader_reset_page_counters));

  /*
   * Step 1. Initialization
   */
  mock_call_expect(TO_STR(zpal_bootloader_get_info), &pMock);
  pMock->compare_rule_arg[0] = COMPARE_NOT_NULL;
  zpal_bootloader_info_t info = {
                                  .type = ZPAL_BOOTLOADER_PRESENT,
                                  .capabilities = ZPAL_BOOTLOADER_CAPABILITY_STORAGE
  };
  pMock->output_arg[0].p = &info;

  /*
   * Make sure that ZAF_nvm_get_object_size() returns ZPAL_STATUS_OK indicating that the file is OK.
   */
  mock_call_expect(TO_STR(ZAF_nvm_get_object_size), &pMock);
  pMock->compare_rule_arg[0] = COMPARE_ANY;
  pMock->compare_rule_arg[1] = COMPARE_NOT_NULL;
  size_t fileSize = ZAF_FILE_SIZE_CC_FIRMWARE_UPDATE;
  pMock->output_arg[1].p     = &fileSize;
  pMock->return_code.v       = ZPAL_STATUS_OK;

  // Mock registration for AppTimerRegister of timerFwUpdateFrameGet in CC_FirmwareUpdate_Init
  mock_call_expect(TO_STR(AppTimerRegister), &pMock_AppTimerRegister_FrameGet);
  pMock_AppTimerRegister_FrameGet->compare_rule_arg[0] = COMPARE_NOT_NULL;
  pMock_AppTimerRegister_FrameGet->expect_arg[1].v = false;
  pMock_AppTimerRegister_FrameGet->compare_rule_arg[2] = COMPARE_NOT_NULL;
  pMock_AppTimerRegister_FrameGet->return_code.v = true; // Timer handle

  // Mock registration for AppTimerRegister of timerOtaSuccess call in CC_FirmwareUpdate_Init
  mock_call_expect(TO_STR(AppTimerRegister), &pMockAppTimerRegister_otaSuccess);
  pMockAppTimerRegister_otaSuccess->compare_rule_arg[0] = COMPARE_NOT_NULL;
  pMockAppTimerRegister_otaSuccess->expect_arg[1].v = false;
  pMockAppTimerRegister_otaSuccess->compare_rule_arg[2] = COMPARE_NOT_NULL;
  pMockAppTimerRegister_otaSuccess->return_code.v = true; // Timer handle

  CC_FirmwareUpdate_Init(NULL, NULL, false);

  /*
   * Step 2. Start the OTA process
   */
  mock_call_expect(TO_STR(zaf_config_get_manufacturer_id), &pMock);
  pMock->return_code.v = MANUFACTURER_ID;

  mock_call_expect(TO_STR(zaf_config_get_product_type_id), &pMock);
  pMock->return_code.v = PRODUCT_TYPE_ID;

  mock_call_expect(TO_STR(zaf_config_get_product_id), &pMock);
  pMock->return_code.v = PRODUCT_ID;

  mock_call_use_as_stub(TO_STR(ZAF_getAppHandle));

  // Simulate that we received a Firmware Update Md Request Get to initiate the OTA
  command_handler_input_t *pRequestGet;
  pRequestGet = firmware_update_md_request_get_v5_frame_create(MANUFACTURER_ID,
                                                               FIRMWARE_ID,
                                                               0xFFFF,
                                                               0,
                                                               sizeof(otaFragment),
                                                               false,
                                                               0);

  mock_call_expect(TO_STR(zaf_config_get_hardware_version), &pMock);
  pMock->return_code.v = 0;

  const uint8_t EXPECTED_REQUEST_REPORT[] = {
                                             COMMAND_CLASS_FIRMWARE_UPDATE_MD_V5,
                                             FIRMWARE_UPDATE_MD_REQUEST_REPORT_V5,
                                             FIRMWARE_UPDATE_MD_REQUEST_REPORT_VALID_COMBINATION_V5
  };

  mock_call_expect(TO_STR(TimerIsActive), &pMock);
  pMock->compare_rule_arg[0] = COMPARE_NOT_NULL;
  pMock->return_code.v = false;

  zaf_transport_rx_to_tx_options_Ignore();
  zaf_transport_tx_ExpectAndReturn(EXPECTED_REQUEST_REPORT, sizeof(EXPECTED_REQUEST_REPORT), NULL, NULL, true);
  zaf_transport_tx_IgnoreArg_callback();
  zaf_transport_tx_IgnoreArg_zaf_tx_options();

  zaf_transport_resume_Expect();
  INVOKE_CC_HANDLER(handleCommandClassFWUpdate, pRequestGet);

  // Setup the mock for validating the first Firmware Update Md Get command the node sends for OTA fragment 1
  const uint8_t EXPECTED_FRAME[] = {
                                    COMMAND_CLASS_FIRMWARE_UPDATE_MD_V5,
                                    FIRMWARE_UPDATE_MD_GET_V5,
                                    OTA_CACHE_SIZE / sizeof(otaFragment),
                                    0, // Report number MSB
                                    1  // Report number LSB - expect to request fragment number 1.
  };

  zaf_transport_rx_to_tx_options_Ignore();
  zaf_transport_tx_ExpectAndReturn(EXPECTED_FRAME, sizeof(EXPECTED_FRAME), NULL, NULL, true);
  zaf_transport_tx_IgnoreArg_callback();
  zaf_transport_tx_IgnoreArg_zaf_tx_options();

  // Trigger the callback function to complete the Firmware Update Md Request Report tx.
  // This will then trigger the node to send the first Firmware Update Md Get command for OTA fragment 1.
  transmission_result_t pTxResult = {
    .status = TRANSMIT_COMPLETE_OK
  };
  ZCB_CmdClassFwUpdateMdReqReport(&pTxResult);

  /*
   * Step 3. Receive the same fragment multiple times with continuous CRC error. Expect the OTA state
   * machine to abort the process after a defined number of retries (10).
   */
  uint8_t retransmitRetries;
  for (retransmitRetries=0; retransmitRetries < FIRMWARE_UPDATE_MAX_RETRY; retransmitRetries++)
  {
    if (retransmitRetries < (FIRMWARE_UPDATE_MAX_RETRY - 1))
    {
      // For the first "FIRMWARE_UPDATE_MAX_RETRY - 1" times, we expect the state machine to ask for
      // retransmission of the OTA fragment that continues to have CRC error.
      const uint8_t EXPECTED_FRAME[] = {
                                        COMMAND_CLASS_FIRMWARE_UPDATE_MD_V5,
                                        FIRMWARE_UPDATE_MD_GET_V5,
                                        OTA_CACHE_SIZE / sizeof(otaFragment),
                                        0, // Report number MSB
                                        1  // Report number LSB - expect to request fragment number 1.
      };

      zaf_transport_rx_to_tx_options_Ignore();
      zaf_transport_tx_ExpectAndReturn(EXPECTED_FRAME, sizeof(EXPECTED_FRAME), NULL, NULL, true);
      zaf_transport_tx_IgnoreArg_callback();
      zaf_transport_tx_IgnoreArg_zaf_tx_options();
    }
    else
    {
      // For the last attempt, we expect the state machine to abort the update process with
      // status="Unable to receive"
      const uint8_t EXPECTED_FRAME_4[] = {
                                          COMMAND_CLASS_FIRMWARE_UPDATE_MD_V5,
                                          FIRMWARE_UPDATE_MD_STATUS_REPORT_V5,
                                          0, // status = FIRMWARE_UPDATE_MD_STATUS_REPORT_UNABLE_TO_RECEIVE_WITHOUT_CHECKSUM_ERROR_V5
                                          0, // Wait time MSB
                                          2  // Wait time LSB - waitTime = WAITTIME_FWU_FAIL
      };

      zaf_transport_rx_to_tx_options_Ignore();
      zaf_transport_tx_ExpectAndReturn(EXPECTED_FRAME_4, sizeof(EXPECTED_FRAME_4), NULL, NULL, true);
      zaf_transport_tx_IgnoreArg_callback();
      zaf_transport_tx_IgnoreArg_zaf_tx_options();

      mock_call_expect(TO_STR(TimerIsActive), &pMock);
      pMock->expect_arg[0].p = pMock_AppTimerRegister_FrameGet->actual_arg[0].p;
      pMock->return_code.v = true;

      // Mock registration for TimerStop call in TimerCancelFwUpdateFrameGet
      mock_call_expect(TO_STR(TimerStop), &pMock);
      pMock->expect_arg[0].p = pMock_AppTimerRegister_FrameGet->actual_arg[0].p; // Compare with the previously registered timer handle
    }

    /* Simulate we receive OTA fragment with CRC error */
    command_handler_input_t * pReport;
    pReport = firmware_update_meta_data_report_frame_create(false,
                                                            1,
                                                            sizeof(otaFragment));
    pReport->frame.as_byte_array[pReport->frameLength-1] = 1; // Corrupt the last byte (CRC)

    zaf_transport_resume_Expect();
    INVOKE_CC_HANDLER(handleCommandClassFWUpdate, pReport);

    zaf_transport_resume_Expect();

    // Call the function given to AppTimerRegister (i.e. simulate the timer triggered)
    void (* AppTimerRegister_callback)(void);
    AppTimerRegister_callback = pMock_AppTimerRegister_FrameGet->actual_arg[2].p;
    AppTimerRegister_callback();

    test_common_command_handler_input_free(pReport);
  }

  /*
   * Done. If the test reaches this point without failing, then we have established that the OTA
   * state machine aborts the update process (after defined number of retries) as expected when
   * an OTA fragment continues to have CRC error.
   */
  test_common_command_handler_input_free(pRequestGet);

  mock_calls_verify();
}

/**
 * Verifies that a new file is created with default values if it does not exist already.
 */
void test_creation_of_file(void)
{
  mock_t * pMock;

  mock_call_use_as_stub(TO_STR(zpal_bootloader_init));
  mock_call_use_as_stub(TO_STR(zpal_bootloader_is_first_boot));
  mock_call_use_as_stub(TO_STR(TimerIsActive));
  mock_call_use_as_stub(TO_STR(AppTimerRegister));

  mock_call_expect(TO_STR(zpal_bootloader_get_info), &pMock);
  pMock->compare_rule_arg[0] = COMPARE_NOT_NULL;
  zpal_bootloader_info_t info = {
                                  .type = ZPAL_BOOTLOADER_PRESENT,
                                  .capabilities = ZPAL_BOOTLOADER_CAPABILITY_STORAGE
  };
  pMock->output_arg[0].p = &info;

  /*
   * Make sure that ZAF_nvm_get_object_size() returns something different than ZPAL_STATUS_OK to trigger
   * a write of the default file.
   */
  mock_call_expect(TO_STR(ZAF_nvm_get_object_size), &pMock);
  pMock->expect_arg[0].v = ZAF_FILE_ID_CC_FIRMWARE_UPDATE;
  pMock->compare_rule_arg[1] = COMPARE_NOT_NULL;
  size_t fileSize = ZAF_FILE_SIZE_CC_FIRMWARE_UPDATE;
  pMock->output_arg[1].p     = &fileSize;
  pMock->return_code.v       = ZPAL_STATUS_FAIL;

  // Verify that a default file is written
  mock_call_expect(TO_STR(ZAF_nvm_write), &pMock);
  pMock->expect_arg[0].v     = ZAF_FILE_ID_CC_FIRMWARE_UPDATE;
  pMock->compare_rule_arg[1] = COMPARE_NOT_NULL;
  pMock->expect_arg[2].v     = ZAF_FILE_SIZE_CC_FIRMWARE_UPDATE;
  pMock->return_code.v       = ZPAL_STATUS_OK;

  CC_FirmwareUpdate_Init(NULL, NULL, true);

  mock_calls_verify();
}

void test_first_successful_boot_without_activation(void)
{
  mock_t * pMock;

  mock_call_use_as_stub(TO_STR(zpal_bootloader_init));
  mock_call_use_as_stub(TO_STR(RTimerIsActive));
  mock_call_use_as_stub(TO_STR(AppTimerRegister));

  mock_call_expect(TO_STR(zpal_bootloader_get_info), &pMock);
  pMock->compare_rule_arg[0] = COMPARE_NOT_NULL;
  zpal_bootloader_info_t info = {
                                  .type = ZPAL_BOOTLOADER_PRESENT,
                                  .capabilities = ZPAL_BOOTLOADER_CAPABILITY_STORAGE
  };
  pMock->output_arg[0].p = &info;

  /*
   * Make sure that ZAF_nvm_get_object_size() returns ZPAL_STATUS_OK indicating that the file is OK.
   */
  mock_call_expect(TO_STR(ZAF_nvm_get_object_size), &pMock);
  pMock->compare_rule_arg[0] = COMPARE_ANY;
  pMock->compare_rule_arg[1] = COMPARE_NOT_NULL;
  size_t fileSize = ZAF_FILE_SIZE_CC_FIRMWARE_UPDATE;
  pMock->output_arg[1].p     = &fileSize;
  pMock->return_code.v       = ZPAL_STATUS_OK;

  // Make sure to return true to fake a first boot.
  bool updated_successfully = true;
  mock_call_expect(TO_STR(zpal_bootloader_is_first_boot), &pMock);
  pMock->compare_rule_arg[0] = COMPARE_NOT_NULL;
  pMock->output_arg[0].p     = &updated_successfully;
  pMock->return_code.v = true;

  mock_call_expect(TO_STR(ZAF_nvm_get_object_size), &pMock);
  pMock->compare_rule_arg[0] = COMPARE_ANY;
  pMock->compare_rule_arg[1] = COMPARE_NOT_NULL;
  pMock->output_arg[1].p     = &fileSize;
  pMock->return_code.v       = ZPAL_STATUS_OK;

  mock_call_expect(TO_STR(ZAF_nvm_read), &pMock);
  pMock->expect_arg[0].v = ZAF_FILE_ID_CC_FIRMWARE_UPDATE;
  pMock->compare_rule_arg[1] = COMPARE_NOT_NULL;
  uint8_t file[] = {0x00, 0xAB}; // 0x00 = No activation
  pMock->output_arg[1].p = file;
  pMock->expect_arg[2].v = ZAF_FILE_SIZE_CC_FIRMWARE_UPDATE;

  const uint8_t EXPECTED_STATUS_REPORT[] = {
                                    COMMAND_CLASS_FIRMWARE_UPDATE_MD_V5,
                                    FIRMWARE_UPDATE_MD_STATUS_REPORT_V5,
                                    FIRMWARE_UPDATE_MD_STATUS_REPORT_SUCCESSFULLY_V5,
                                    0, // Wait time MSB
                                    0  // Wait time LSB
  };
  zaf_transport_rx_to_tx_options_Ignore();
  zaf_transport_tx_ExpectAndReturn(EXPECTED_STATUS_REPORT, sizeof(EXPECTED_STATUS_REPORT), NULL, NULL, true);
  zaf_transport_tx_IgnoreArg_callback();
  zaf_transport_tx_IgnoreArg_zaf_tx_options();

  CC_FirmwareUpdate_Init(NULL, NULL, true);

  mock_calls_verify();
}

void test_first_successful_boot_with_activation(void)
{
  mock_t * pMock;

  mock_call_use_as_stub(TO_STR(zpal_bootloader_init));
  mock_call_use_as_stub(TO_STR(RTimerIsActive));
  mock_call_use_as_stub(TO_STR(AppTimerRegister));

  const uint16_t CHECKSUM = 0xAABB;
  const uint16_t PRODUCT_TYPE_ID = 0x00CC;
  const uint16_t PRODUCT_ID = 0x00DD;
  const uint16_t FIRMWARE_ID = 0xCCDD; // Firmware ID is a combination of product type ID and product ID.
  const uint8_t HARDWARE_VERSION = 10;
  const uint8_t FIRMWARE_TARGET = 0;
  const uint8_t FILE_VERSION = 1;

  mock_call_expect(TO_STR(zpal_bootloader_get_info), &pMock);
  pMock->compare_rule_arg[0] = COMPARE_NOT_NULL;
  zpal_bootloader_info_t info = {
                                  .type = ZPAL_BOOTLOADER_PRESENT,
                                  .capabilities = ZPAL_BOOTLOADER_CAPABILITY_STORAGE
  };
  pMock->output_arg[0].p = &info;

  /*
   * Make sure that ZAF_nvm_get_object_size() returns ZPAL_STATUS_OK indicating that the file is OK.
   */
  mock_call_expect(TO_STR(ZAF_nvm_get_object_size), &pMock);
  pMock->compare_rule_arg[0] = COMPARE_ANY;
  pMock->compare_rule_arg[1] = COMPARE_NOT_NULL;
  size_t fileSize = ZAF_FILE_SIZE_CC_FIRMWARE_UPDATE;
  pMock->output_arg[1].p     = &fileSize;
  pMock->return_code.v       = ZPAL_STATUS_OK;

  // Make sure to return true to fake a first boot.
  bool updated_successfully = true;
  mock_call_expect(TO_STR(zpal_bootloader_is_first_boot), &pMock);
  pMock->compare_rule_arg[0] = COMPARE_NOT_NULL;
  pMock->output_arg[0].p     = &updated_successfully;
  pMock->return_code.v = true;

  mock_call_expect(TO_STR(ZAF_nvm_get_object_size), &pMock);
  pMock->compare_rule_arg[0] = COMPARE_ANY;
  pMock->compare_rule_arg[1] = COMPARE_NOT_NULL;
  pMock->output_arg[1].p     = &fileSize;
  pMock->return_code.v       = ZPAL_STATUS_OK;

  mock_call_expect(TO_STR(ZAF_nvm_read), &pMock);
  pMock->expect_arg[0].v = ZAF_FILE_ID_CC_FIRMWARE_UPDATE;
  pMock->compare_rule_arg[1] = COMPARE_NOT_NULL;
  uint8_t file[] = {0x81, FILE_VERSION, (uint8_t)(CHECKSUM), (uint8_t)(CHECKSUM >> 8)}; // 0x00 = No activation
  pMock->output_arg[1].p = file;
  pMock->expect_arg[2].v = ZAF_FILE_SIZE_CC_FIRMWARE_UPDATE;

  mock_call_expect(TO_STR(zaf_config_get_manufacturer_id), &pMock);
  pMock->return_code.v = APP_MANUFACTURER_ID;

  mock_call_expect(TO_STR(zaf_config_get_product_type_id), &pMock);
  pMock->return_code.v = PRODUCT_TYPE_ID;

  mock_call_expect(TO_STR(zaf_config_get_product_id), &pMock);
  pMock->return_code.v = PRODUCT_ID;

  mock_call_expect(TO_STR(zaf_config_get_hardware_version), &pMock);
  pMock->return_code.v = HARDWARE_VERSION;

  const uint8_t EXPECTED_STATUS_REPORT[] = {
      COMMAND_CLASS_FIRMWARE_UPDATE_MD_V5,
      FIRMWARE_UPDATE_ACTIVATION_STATUS_REPORT_V5,
      (uint8_t)(APP_MANUFACTURER_ID >> 8),
      (uint8_t)(APP_MANUFACTURER_ID),
      (uint8_t)(FIRMWARE_ID >> 8),
      (uint8_t)(FIRMWARE_ID),
      (uint8_t)(CHECKSUM >> 8),
      (uint8_t)(CHECKSUM),
      FIRMWARE_TARGET,
      FIRMWARE_UPDATE_ACTIVATION_STATUS_REPORT_FIRMWARE_UPDATE_COMPLETED_SUCCESSFULLY_V5,
      HARDWARE_VERSION
  };
  zaf_transport_rx_to_tx_options_Ignore();
  zaf_transport_tx_ExpectAndReturn(EXPECTED_STATUS_REPORT, sizeof(EXPECTED_STATUS_REPORT), NULL, NULL, true);
  zaf_transport_tx_IgnoreArg_callback();
  zaf_transport_tx_IgnoreArg_zaf_tx_options();

  CC_FirmwareUpdate_Init(NULL, NULL, true);

  mock_calls_verify();
}

void test_file_migration(void)
{
  mock_t * pMock;

  mock_call_use_as_stub(TO_STR(zpal_bootloader_init));
  mock_call_use_as_stub(TO_STR(RTimerIsActive));
  mock_call_use_as_stub(TO_STR(AppTimerRegister));

  const uint8_t  ACTIVATION  = 0x81; //ACTIVATION_SUPPORT_ENABLED_MASK
  const uint16_t CHECKSUM    = 0xAABB;
  const uint8_t  SRCNODEID   = 0x12;
  const uint8_t  SRCENDPOINT = 0x34;
  const uint8_t  RXSTATUS    = 0x56;
  const uint32_t SECURITYKEY = 0x789ABCDE;

  const uint16_t MANUFACTURER_ID = APP_MANUFACTURER_ID;
  const uint16_t PRODUCT_TYPE_ID = 0x00CC;
  const uint16_t PRODUCT_ID = 0x00DD;
  const uint16_t FIRMWARE_ID = 0xCCDD; // Firmware ID is a combination of product type ID and product ID.
  const uint8_t  HARDWARE_VERSION = 10;
  const uint8_t  FIRMWARE_TARGET  = 0;

  mock_call_expect(TO_STR(zpal_bootloader_get_info), &pMock);
  pMock->compare_rule_arg[0] = COMPARE_NOT_NULL;
  zpal_bootloader_info_t info = {
                                  .type = ZPAL_BOOTLOADER_PRESENT,
                                  .capabilities = ZPAL_BOOTLOADER_CAPABILITY_STORAGE
  };
  pMock->output_arg[0].p = &info;

  /*
   * Make sure that ZAF_nvm_get_object_size() returns ZPAL_STATUS_OK indicating that the file is OK.
   */
  mock_call_expect(TO_STR(ZAF_nvm_get_object_size), &pMock);
  pMock->compare_rule_arg[0] = COMPARE_ANY;
  pMock->compare_rule_arg[1] = COMPARE_NOT_NULL;
  size_t oldFileSize = 12;
  pMock->output_arg[1].p     = &oldFileSize;
  pMock->return_code.v       = ZPAL_STATUS_OK;

  // Make sure to return true to fake a first boot.
  bool updated_successfully = true;
  mock_call_expect(TO_STR(zpal_bootloader_is_first_boot), &pMock);
  pMock->compare_rule_arg[0] = COMPARE_NOT_NULL;
  pMock->output_arg[0].p     = &updated_successfully;
  pMock->return_code.v = true;

  mock_call_expect(TO_STR(ZAF_nvm_get_object_size), &pMock);
  pMock->compare_rule_arg[0] = COMPARE_ANY;
  pMock->compare_rule_arg[1] = COMPARE_NOT_NULL;
  pMock->output_arg[1].p     = &oldFileSize;
  pMock->return_code.v       = ZPAL_STATUS_OK;

  uint8_t oldFile[] = {
    ACTIVATION,
    0xFF, // Random Padding
    (uint8_t)(CHECKSUM & 0xFF),
    (uint8_t)(CHECKSUM >> 8),
    SRCNODEID,
    SRCENDPOINT,
    RXSTATUS,
    0xFF, // Random Padding
    (uint8_t)(SECURITYKEY & 0xFF),
    (uint8_t)((SECURITYKEY >> 8) & 0xFF),
    (uint8_t)((SECURITYKEY >> 16) & 0xFF),
    (uint8_t)((SECURITYKEY >> 24) & 0xFF),
  };

  //read old version file
  mock_call_expect(TO_STR(ZAF_nvm_read), &pMock);
  pMock->expect_arg[0].v = ZAF_FILE_ID_CC_FIRMWARE_UPDATE;
  pMock->compare_rule_arg[1] = COMPARE_NOT_NULL;
  pMock->output_arg[1].p = oldFile;
  pMock->expect_arg[2].v = 12;

  uint8_t file[] = {
    ACTIVATION,
    0x01,
    (uint8_t)(CHECKSUM & 0xFF),
    (uint8_t)(CHECKSUM >> 8),
    (uint8_t)(SRCNODEID & 0xFF),
    (uint8_t)0x00,
    SRCENDPOINT,
    RXSTATUS,
    (uint8_t)(SECURITYKEY & 0xFF),
    (uint8_t)((SECURITYKEY >> 8) & 0xFF),
    (uint8_t)((SECURITYKEY >> 16) & 0xFF),
    (uint8_t)((SECURITYKEY >> 24) & 0xFF),
    0x00,
    0x00,
    0x00,
    0x00
  };

  mock_call_expect(TO_STR(ZAF_nvm_write), &pMock);
  pMock->expect_arg[0].v = ZAF_FILE_ID_CC_FIRMWARE_UPDATE;
  pMock->compare_rule_arg[1] = COMPARE_NOT_NULL;
  pMock->output_arg[1].p = file;
  pMock->expect_arg[2].v = sizeof(file);
  pMock->return_code.v = ZPAL_STATUS_OK;

  mock_call_expect(TO_STR(zaf_config_get_manufacturer_id), &pMock);
  pMock->return_code.v = MANUFACTURER_ID;

  mock_call_expect(TO_STR(zaf_config_get_product_type_id), &pMock);
  pMock->return_code.v = PRODUCT_TYPE_ID;

  mock_call_expect(TO_STR(zaf_config_get_product_id), &pMock);
  pMock->return_code.v = PRODUCT_ID;

  mock_call_expect(TO_STR(zaf_config_get_hardware_version), &pMock);
  pMock->return_code.v = HARDWARE_VERSION;

  const uint8_t EXPECTED_STATUS_REPORT[] = {
      COMMAND_CLASS_FIRMWARE_UPDATE_MD_V5,
      FIRMWARE_UPDATE_ACTIVATION_STATUS_REPORT_V5,
      (uint8_t)(APP_MANUFACTURER_ID >> 8),
      (uint8_t)(APP_MANUFACTURER_ID),
      (uint8_t)(FIRMWARE_ID >> 8),
      (uint8_t)(FIRMWARE_ID),
      (uint8_t)(CHECKSUM >> 8),
      (uint8_t)(CHECKSUM),
      FIRMWARE_TARGET,
      FIRMWARE_UPDATE_ACTIVATION_STATUS_REPORT_FIRMWARE_UPDATE_COMPLETED_SUCCESSFULLY_V5,
      HARDWARE_VERSION
  };
  zaf_transport_rx_to_tx_options_Ignore();
  zaf_transport_tx_ExpectAndReturn(EXPECTED_STATUS_REPORT, sizeof(EXPECTED_STATUS_REPORT), NULL, NULL, true);
  zaf_transport_tx_IgnoreArg_callback();
  zaf_transport_tx_IgnoreArg_zaf_tx_options();

  CC_FirmwareUpdate_Init(NULL, NULL, true);

  mock_calls_verify();
}

#if CC_FIRMWARE_UPDATE_CONFIG_OTA_MULTI_FRAME
void test_multidata_frame_storage_NumberOfReports(void)
{
  // Tests that if fragment size changes, Number of reports change as well
  // Needed:
  // 1. FW Update Request Get -> Request Report. (to get fragment size)
  // 2. FW Update MD Get -> (to get Number or reports)

  mock_t * pMock = NULL;

  const uint8_t FRAGMENT_SIZE_1 = 10;
  uint8_t numberOfReports1 = OTA_CACHE_SIZE / FRAGMENT_SIZE_1;
  printf ("numberOfReports = %d = %d/%d\n", numberOfReports1, OTA_CACHE_SIZE, FRAGMENT_SIZE_1);

  mock_call_use_as_stub(TO_STR(Check_not_legal_response_job));
  mock_call_use_as_stub(TO_STR(ZAF_nvm_write));
  mock_call_use_as_stub(TO_STR(zpal_bootloader_reset_page_counters));

  mock_call_use_as_stub(TO_STR(TimerStop));
  mock_call_use_as_stub(TO_STR(TimerStart));
  mock_call_use_as_stub(TO_STR(TimerRestart));
  mock_call_use_as_stub(TO_STR(TimerIsActive));

///////////////////////////////////////////////////////////////////////////////
/// First Request Get

  call_FW_update_Request_Get(pMock, FRAGMENT_SIZE_1, false);

/// First FW Update MD Get
  const uint8_t MD_GET_EXPECTED_FRAME[] = {
                   COMMAND_CLASS_FIRMWARE_UPDATE_MD_V5,
                   FIRMWARE_UPDATE_MD_GET_V5,
                   numberOfReports1,
                   0, // Report number MSB
                   1  // Report number LSB
  };

  zaf_transport_rx_to_tx_options_Ignore();
  zaf_transport_tx_ExpectAndReturn(MD_GET_EXPECTED_FRAME, sizeof(MD_GET_EXPECTED_FRAME), NULL, NULL, true);
  zaf_transport_tx_IgnoreArg_callback();
  zaf_transport_tx_IgnoreArg_zaf_tx_options();

  // callback of Request Report triggers sending of FW Update MD Get.
  transmission_result_t pTxResult = {
    .status = TRANSMIT_COMPLETE_OK
  };
  request_get_cb(&pTxResult);

/////////////////////////////////////////////////////////////////////////////
/// PART 2 -  Use different fragment size and compare number of reports.

  const uint8_t FRAGMENT_SIZE_2 = 50;
  uint8_t numberOfReports2 = OTA_CACHE_SIZE / FRAGMENT_SIZE_2;
  //printf ("numberOfReports = %d = %d/%d\n", numberOfReports2, OTA_CACHE_SIZE, FRAGMENT_SIZE_2);

/// Second Request Get
  call_FW_update_Request_Get(pMock, FRAGMENT_SIZE_2, false);

/// Second FW Update MD Get
  const uint8_t MD_GET_EXPECTED_FRAME_2[] = {
                                           COMMAND_CLASS_FIRMWARE_UPDATE_MD_V5,
                                           FIRMWARE_UPDATE_MD_GET_V5,
                                           numberOfReports2,
                                           0, // Report number MSB
                                           1  // Report number LSB
  };

  zaf_transport_rx_to_tx_options_Ignore();
  zaf_transport_tx_ExpectAndReturn(MD_GET_EXPECTED_FRAME_2, sizeof(MD_GET_EXPECTED_FRAME_2), NULL, NULL, true);
  zaf_transport_tx_IgnoreArg_callback();
  zaf_transport_tx_IgnoreArg_zaf_tx_options();

  // callback of Request Report triggers sending of FW Update MD Get.
  request_get_cb(&pTxResult);

  // Verify that number of Reports decreases, if fragment size increases.
  TEST_ASSERT_MESSAGE(
      (numberOfReports1 > numberOfReports2) && (FRAGMENT_SIZE_1 < FRAGMENT_SIZE_2),
      "Unexpected Number of Reports for given Fragment size :(");

  // TODO: Verify that Number of Reports is 1, even when there is no room for it in internal storage.

  mock_calls_verify();
}

void test_multidata_frame_receiving(void)
{
  mock_t * pMock;

  // Tests that requested number of FW Update MD reports will be successfully received.
  // Test:
  // 1. Make sure that numberOfReports Reports are received successfully after single MD GET
  // 2. MD Reports are coming in order.
  // 3. Next MD Get is sent after last MD Report is received

  const uint8_t FRAGMENT_SIZE = 50;
  uint8_t numberOfReports = OTA_CACHE_SIZE / FRAGMENT_SIZE;
  //printf ("numberOfReports = %d = %d/%d\n", numberOfReports, OTA_CACHE_SIZE, FRAGMENT_SIZE);

  mock_call_use_as_stub(TO_STR(Check_not_legal_response_job));
  mock_call_use_as_stub(TO_STR(ZAF_nvm_write));
  mock_call_use_as_stub(TO_STR(zpal_bootloader_write_data));
  mock_call_use_as_stub(TO_STR(MSC_Init));
  mock_call_use_as_stub(TO_STR(TimerStart));
  mock_call_use_as_stub(TO_STR(TimerRestart));
  mock_call_use_as_stub(TO_STR(TimerStop));
  mock_call_use_as_stub(TO_STR(TimerIsActive));
  mock_call_use_as_stub(TO_STR(zpal_bootloader_reset_page_counters));

///////////////////////////////////////////////////////////////////////////////
/// Request Get
  call_FW_update_Request_Get(pMock, FRAGMENT_SIZE, false);

/// Part 1 - requested MD Reports are received with success after first MD Get
/// FW Update MD Get

  uint16_t md_get_iter = 0; // how many MD Get were sent so far
  uint16_t next_report_no = md_get_iter * numberOfReports + 1; // next expected MD Report
  const uint8_t MD_GET_EXPECTED_FRAME[] = {
                                           COMMAND_CLASS_FIRMWARE_UPDATE_MD_V5,
                                           FIRMWARE_UPDATE_MD_GET_V5,
                                           numberOfReports,
                                           next_report_no << 8, // Report number MSB
                                           next_report_no & 0xFF// Report number LSB
  };

  zaf_transport_rx_to_tx_options_Ignore();
  zaf_transport_tx_ExpectAndReturn(MD_GET_EXPECTED_FRAME, sizeof(MD_GET_EXPECTED_FRAME), NULL, NULL, true);
  zaf_transport_tx_IgnoreArg_callback();
  zaf_transport_tx_IgnoreArg_zaf_tx_options();

  // callback of Request Report triggers sending of FW Update MD Get.
  transmission_result_t pTxResult = {
    .status = TRANSMIT_COMPLETE_OK
  };
  request_get_cb(&pTxResult);
  md_get_iter += 1;

  // FW Update MD Report. Expected numberOfReports reports.
  command_handler_input_t * pFirmwareUpdateMDReport;
  received_frame_status_t status;

  // Expect ZAF_transportSendDataAbort() once
  mock_call_expect(TO_STR(ZAF_transportSendDataAbort), &pMock);

  // Receive numberOfReports - 1. Frames are just written in internal storage.
  for (; next_report_no < numberOfReports * md_get_iter; next_report_no++)
  {
    pFirmwareUpdateMDReport = firmware_update_meta_data_report_frame_create(
        false,
        next_report_no,
        FRAGMENT_SIZE);

    zaf_transport_resume_Expect();
    status = INVOKE_CC_HANDLER(handleCommandClassFWUpdate, pFirmwareUpdateMDReport);
    TEST_ASSERT_MESSAGE(status == RECEIVED_FRAME_STATUS_SUCCESS, "Wrong frame status in part 1 :(");

    test_common_command_handler_input_free(pFirmwareUpdateMDReport);
  }

  // Next report should be the last one requested by MD Get.
  // After that one, new MD Get should be sent, with new report number.
  TEST_ASSERT_MESSAGE(next_report_no == numberOfReports, "Wrong MD Get report number in part 1 :(");

  pFirmwareUpdateMDReport = firmware_update_meta_data_report_frame_create(
      false,
      next_report_no++,
      FRAGMENT_SIZE);

  // Expect next FW Update MD Get here, report Number = numberOfReports + 1
  const uint8_t MD_GET_EXPECTED_FRAME_2[] = {
                                           COMMAND_CLASS_FIRMWARE_UPDATE_MD_V5,
                                           FIRMWARE_UPDATE_MD_GET_V5,
                                           numberOfReports,
                                           (next_report_no << 8) & 0xFF, // Report number MSB
                                           next_report_no & 0xFF// Report number LSB
  };

  zaf_transport_rx_to_tx_options_Ignore();
  zaf_transport_tx_ExpectAndReturn(MD_GET_EXPECTED_FRAME_2, sizeof(MD_GET_EXPECTED_FRAME_2), NULL, NULL, true);
  zaf_transport_tx_IgnoreArg_callback();
  zaf_transport_tx_IgnoreArg_zaf_tx_options();

  zaf_transport_resume_Expect();

  zaf_transport_resume_Expect();
  status = INVOKE_CC_HANDLER(handleCommandClassFWUpdate, pFirmwareUpdateMDReport);
  TEST_ASSERT_MESSAGE(status == RECEIVED_FRAME_STATUS_SUCCESS, "Wrong frame status in part 1 :(");
  md_get_iter += 1;

  test_common_command_handler_input_free(pFirmwareUpdateMDReport);

  // Next, make sure that MD Reports keep coming in order as requested in MD Get.
  for (; next_report_no < numberOfReports * md_get_iter; next_report_no++)
  {
    pFirmwareUpdateMDReport = firmware_update_meta_data_report_frame_create(
        false,
        next_report_no,
        FRAGMENT_SIZE);

    zaf_transport_resume_Expect();
    status = INVOKE_CC_HANDLER(handleCommandClassFWUpdate, pFirmwareUpdateMDReport);
    TEST_ASSERT_MESSAGE(status == RECEIVED_FRAME_STATUS_SUCCESS, "Wrong frame status in part 2 :(");

    test_common_command_handler_input_free(pFirmwareUpdateMDReport);
  }

  // if test reaches this point without failure,
  // it means that next report gets correctly calculated by ota_utils.
  mock_calls_verify();
}

void test_multidata_frame_with_invalid_md_report(void)
{
  mock_t * pMock;
  /*
   * Tests parameters of FW Update MD Get after invalid MD Report
   * Test :
   * If one of received MD Reports is invalid, make sure that
   * 1. MD Get is sent (after timeout expires).
   * 2. Number of Reports is reduced by the number of successfully received
   *    MD Reports after last MD Get.
   */

  /// Part 0 - Setup
  const uint8_t FRAGMENT_SIZE = 30;
  uint8_t numberOfReports = OTA_CACHE_SIZE / FRAGMENT_SIZE;
  //printf ("numberOfReports = %d = %d/%d\n", numberOfReports, OTA_CACHE_SIZE, FRAGMENT_SIZE);

  mock_call_use_as_stub(TO_STR(Check_not_legal_response_job));
  mock_call_use_as_stub(TO_STR(zpal_bootloader_is_first_boot));
  mock_call_use_as_stub(TO_STR(ZAF_nvm_write));
  mock_call_use_as_stub(TO_STR(zpal_bootloader_init));
  mock_call_use_as_stub(TO_STR(zpal_bootloader_write_data));
  mock_call_use_as_stub(TO_STR(MSC_Init));
  mock_call_use_as_stub(TO_STR(zpal_bootloader_reset_page_counters));

  /// Part 0 - Initialize CC Firmware update

  mock_call_expect(TO_STR(zpal_bootloader_get_info), &pMock);
  pMock->compare_rule_arg[0] = COMPARE_NOT_NULL;
  zpal_bootloader_info_t info = {
                                  .type = ZPAL_BOOTLOADER_PRESENT,
                                  .capabilities = ZPAL_BOOTLOADER_CAPABILITY_STORAGE
  };
  pMock->output_arg[0].p = &info;

  /*
   * Make sure that ZAF_nvm_get_object_size() returns ZPAL_STATUS_OK indicating that the file is OK.
   */
  mock_call_expect(TO_STR(ZAF_nvm_get_object_size), &pMock);
  pMock->compare_rule_arg[0] = COMPARE_ANY;
  pMock->compare_rule_arg[1] = COMPARE_NOT_NULL;
  size_t fileSize = ZAF_FILE_SIZE_CC_FIRMWARE_UPDATE;
  pMock->output_arg[1].p     = &fileSize;
  pMock->return_code.v       = ZPAL_STATUS_OK;

  // Mock registration for AppTimerRegister call in CC_FirmwareUpdate_Init
  mock_t * pMock_AppTimerRegister_FrameGet = NULL;
  mock_call_expect(TO_STR(AppTimerRegister), &pMock_AppTimerRegister_FrameGet);
  pMock_AppTimerRegister_FrameGet->compare_rule_arg[0] = COMPARE_NOT_NULL;
  pMock_AppTimerRegister_FrameGet->expect_arg[1].v = false;
  pMock_AppTimerRegister_FrameGet->compare_rule_arg[2] = COMPARE_NOT_NULL;
  pMock_AppTimerRegister_FrameGet->return_code.v = true; // Timer handle

  mock_call_expect(TO_STR(AppTimerRegister), &pMock);
  pMock->compare_rule_arg[0] = COMPARE_NOT_NULL;
  pMock->expect_arg[1].v = false;
  pMock->compare_rule_arg[2] = COMPARE_NOT_NULL;
  pMock->return_code.v = true; // Timer handle

  CC_FirmwareUpdate_Init(NULL, NULL, false);

  /// Part 0 - Request Get
  call_FW_update_Request_Get(pMock, FRAGMENT_SIZE, true);

  /// Part 1 - Send FW Update MD Get

  // next expected MD Report
  uint16_t next_report_no = 1;
  const uint8_t MD_GET_EXPECTED_FRAME[] = {
                                           COMMAND_CLASS_FIRMWARE_UPDATE_MD_V5,
                                           FIRMWARE_UPDATE_MD_GET_V5,
                                           numberOfReports,
                                           next_report_no << 8, // Report number MSB
                                           next_report_no & 0xFF// Report number LSB
  };

  mock_call_expect(TO_STR(TimerIsActive), &pMock);
  pMock->compare_rule_arg[0] = COMPARE_NOT_NULL;
  pMock->return_code.v = false;

  zaf_transport_rx_to_tx_options_Ignore();
  zaf_transport_tx_ExpectAndReturn(MD_GET_EXPECTED_FRAME, sizeof(MD_GET_EXPECTED_FRAME), NULL, NULL, true);
  zaf_transport_tx_IgnoreArg_callback();
  zaf_transport_tx_IgnoreArg_zaf_tx_options();

  // callback of Request Report triggers sending of FW Update MD Get.
  transmission_result_t pTxResult = {
    .status = TRANSMIT_COMPLETE_OK
  };
  request_get_cb(&pTxResult);

  //////////////////////////////////////////////////////////////////////////////
  /// Part 2 - fake receiving invalid tests

  command_handler_input_t * pFirmwareUpdateMDReport;
  received_frame_status_t status;
  uint8_t expected_valid_reports = numberOfReports - numberOfReports/2;

  mock_call_expect(TO_STR(TimerIsActive), &pMock);
  pMock->compare_rule_arg[0] = COMPARE_NOT_NULL;
  pMock->return_code.v = false;

  mock_call_expect(TO_STR(TimerStart), &pMock);
  pMock->compare_rule_arg[0] = COMPARE_NOT_NULL;
  pMock->expect_arg[1].v = TIMER_START_FWUPDATE_FRAME_GET;
  pMock->return_code.v = ESWTIMER_STATUS_SUCCESS;

  // Receive expected_valid_reports frames. Frames are just written in internal storage.
  uint8_t received_reports = 0;
  for (; next_report_no < expected_valid_reports; next_report_no++, received_reports++)
  {
    pFirmwareUpdateMDReport = firmware_update_meta_data_report_frame_create(
        false,
        next_report_no,
        FRAGMENT_SIZE);

    zaf_transport_resume_Expect();
    status = INVOKE_CC_HANDLER(handleCommandClassFWUpdate, pFirmwareUpdateMDReport);
    TEST_ASSERT_MESSAGE(status == RECEIVED_FRAME_STATUS_SUCCESS, "Wrong frame status in part 1 :(");

    test_common_command_handler_input_free(pFirmwareUpdateMDReport);
  }

  // Next, Simulate that next MD Report is not valid.
  pFirmwareUpdateMDReport = firmware_update_meta_data_report_frame_create(
      false,
      next_report_no,
      FRAGMENT_SIZE/2); // invalid fragment size

  // Expect next FW Update MD Get here, report Number = numberOfReports + 1
  const uint8_t MD_GET_EXPECTED_FRAME_2[] = {
                                             COMMAND_CLASS_FIRMWARE_UPDATE_MD_V5,
                                             FIRMWARE_UPDATE_MD_GET_V5,
                                             numberOfReports - received_reports,
                                             (next_report_no << 8) & 0xFF, // Report number MSB
                                             next_report_no & 0xFF// Report number LSB
  };

  // Save cb function from AppTimerRegister
  void (* AppTimerRegister_callback)(void);
  AppTimerRegister_callback = pMock_AppTimerRegister_FrameGet->actual_arg[2].p;

  // OTA should wait for it until timer expires, and then send new MD Get.
  zaf_transport_rx_to_tx_options_Ignore();
  zaf_transport_tx_ExpectAndReturn(MD_GET_EXPECTED_FRAME_2, sizeof(MD_GET_EXPECTED_FRAME_2), NULL, NULL, true);
  zaf_transport_tx_IgnoreArg_callback();
  zaf_transport_tx_IgnoreArg_zaf_tx_options();

  zaf_transport_resume_Expect();

  // Simulate that timer dedicated for waiting MD Report - triggered
  AppTimerRegister_callback();

  zaf_transport_resume_Expect();
  status = INVOKE_CC_HANDLER(handleCommandClassFWUpdate, pFirmwareUpdateMDReport);
  TEST_ASSERT_MESSAGE(status == RECEIVED_FRAME_STATUS_SUCCESS, "Wrong frame status in part 2 :(");

  test_common_command_handler_input_free(pFirmwareUpdateMDReport);

  mock_calls_verify();
}

void test_multidata_frame_storage_success(void)
{
  mock_t * pMock;
  /*
   * Test storage:
   * All received reports from one shot are successfully written to flash.
   */
  /// Part 0 - Setup
  const uint8_t FRAGMENT_SIZE = 30;
  uint8_t numberOfReports = OTA_CACHE_SIZE / FRAGMENT_SIZE;
  //printf ("numberOfReports = %d = %d/%d\n", numberOfReports, OTA_CACHE_SIZE, FRAGMENT_SIZE);

  mock_call_use_as_stub(TO_STR(Check_not_legal_response_job));
  mock_call_use_as_stub(TO_STR(zpal_bootloader_is_first_boot));
  mock_call_use_as_stub(TO_STR(ZAF_nvm_write));
  mock_call_use_as_stub(TO_STR(zpal_bootloader_reset_page_counters));

  mock_call_use_as_stub(TO_STR(AppTimerRegister));
  mock_call_use_as_stub(TO_STR(TimerIsActive));
  mock_call_use_as_stub(TO_STR(TimerStop));
  mock_call_use_as_stub(TO_STR(TimerRestart));

  /// Part 0 - Initialize CC Firmware update

  mock_call_expect(TO_STR(zpal_bootloader_init), &pMock);
  pMock->return_code.v = 0L; //BOOTLOADER_OK;

  mock_call_expect(TO_STR(zpal_bootloader_get_info), &pMock);
  pMock->compare_rule_arg[0] = COMPARE_NOT_NULL;
  zpal_bootloader_info_t info = {
                                   .type = ZPAL_BOOTLOADER_PRESENT,
                                   .capabilities = ZPAL_BOOTLOADER_CAPABILITY_STORAGE
  };
  pMock->output_arg[0].p = &info;

  /*
   * Make sure that ZAF_nvm_get_object_size() returns ZPAL_STATUS_OK indicating that the file is OK.
   */
  mock_call_expect(TO_STR(ZAF_nvm_get_object_size), &pMock);
  pMock->compare_rule_arg[0] = COMPARE_ANY;
  pMock->compare_rule_arg[1] = COMPARE_NOT_NULL;
  size_t fileSize = ZAF_FILE_SIZE_CC_FIRMWARE_UPDATE;
  pMock->output_arg[1].p     = &fileSize;
  pMock->return_code.v       = ZPAL_STATUS_OK;

  CC_FirmwareUpdate_Init(NULL, NULL, false);

  /// Part 0 - Request Get
  call_FW_update_Request_Get(pMock, FRAGMENT_SIZE, false);

  /// Part 1 - Send FW Update MD Get

  // next expected MD Report
  uint16_t next_report_no = 1;
  const uint8_t MD_GET_EXPECTED_FRAME[] = {
                                           COMMAND_CLASS_FIRMWARE_UPDATE_MD_V5,
                                           FIRMWARE_UPDATE_MD_GET_V5,
                                           numberOfReports,
                                           next_report_no << 8, // Report number MSB
                                           next_report_no & 0xFF// Report number LSB
  };

  zaf_transport_rx_to_tx_options_Ignore();
  zaf_transport_tx_ExpectAndReturn(MD_GET_EXPECTED_FRAME, sizeof(MD_GET_EXPECTED_FRAME), NULL, NULL, true);
  zaf_transport_tx_IgnoreArg_callback();
  zaf_transport_tx_IgnoreArg_zaf_tx_options();

  // callback of Request Report triggers sending of FW Update MD Get.
  transmission_result_t pTxResult = {
    .status = TRANSMIT_COMPLETE_OK
  };
  request_get_cb(&pTxResult);


  // FW Update MD Report. Expected numberOfReports reports.
  command_handler_input_t * pFirmwareUpdateMDReport;
  received_frame_status_t status;

  // place to store MD reports before they are written to flash
  uint8_t temp_storage[OTA_CACHE_SIZE];
  uint8_t *p_temp_storage = &temp_storage[0];

  mock_call_expect(TO_STR(ZAF_transportSendDataAbort), &pMock);

  // Receive numberOfReports - 1. Frames are just written in internal storage.
  for (; next_report_no < numberOfReports; next_report_no++)
  {
    pFirmwareUpdateMDReport = firmware_update_meta_data_report_frame_create(
        false,
        next_report_no,
        FRAGMENT_SIZE);

    mock_call_expect(TO_STR(TimerStart), &pMock);
    pMock->compare_rule_arg[0] = COMPARE_NOT_NULL;
    pMock->expect_arg[1].v = TIMER_START_FWUPDATE_FRAME_GET;
    pMock->return_code.v = ESWTIMER_STATUS_SUCCESS;

    memcpy(p_temp_storage,
           &(pFirmwareUpdateMDReport->frame.as_byte_array[4]),
           FRAGMENT_SIZE);
    p_temp_storage += FRAGMENT_SIZE;

    zaf_transport_resume_Expect();
    status = INVOKE_CC_HANDLER(handleCommandClassFWUpdate, pFirmwareUpdateMDReport);
    TEST_ASSERT_MESSAGE(status == RECEIVED_FRAME_STATUS_SUCCESS, "Wrong frame status in part 1 :(");

    test_common_command_handler_input_free(pFirmwareUpdateMDReport);
  }

  /// Part 2 - write to flash

  // Next report should be the last one requested by MD Get.
  // After that one, it should  happen writing flash

  pFirmwareUpdateMDReport = firmware_update_meta_data_report_frame_create(
      false,
      next_report_no++,
      FRAGMENT_SIZE);

  mock_call_expect(TO_STR(TimerStart), &pMock);
  pMock->compare_rule_arg[0] = COMPARE_NOT_NULL;
  pMock->expect_arg[1].v = TIMER_START_FWUPDATE_FRAME_GET;
  pMock->return_code.v = ESWTIMER_STATUS_SUCCESS;

  // also write the last report into temp storage
  memcpy(p_temp_storage,
         &(pFirmwareUpdateMDReport->frame.as_byte_array[4]),
         FRAGMENT_SIZE);

  mock_call_expect(TO_STR(zpal_bootloader_write_data), &pMock);
  pMock->expect_arg[0].v = 0;
  pMock->expect_arg[1].p = temp_storage;
  pMock->expect_arg[2].v = FRAGMENT_SIZE * numberOfReports;
  pMock->return_code.v = ZPAL_STATUS_OK;

  // Next FW Update MD Get. Not relevant for this test.
  zaf_transport_rx_to_tx_options_Ignore();
  zaf_transport_tx_IgnoreAndReturn(true);

  zaf_transport_resume_Expect();

  zaf_transport_resume_Expect();
  status = INVOKE_CC_HANDLER(handleCommandClassFWUpdate, pFirmwareUpdateMDReport);
  TEST_ASSERT_MESSAGE(status == RECEIVED_FRAME_STATUS_SUCCESS, "Wrong frame status in part 2 :(");

  test_common_command_handler_input_free(pFirmwareUpdateMDReport);

  mock_calls_verify();
}
#endif // CC_FIRMWARE_UPDATE_CONFIG_OTA_MULTI_FRAME