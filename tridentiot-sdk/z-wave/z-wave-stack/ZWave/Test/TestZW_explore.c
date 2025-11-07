/**
 * @file
 * @copyright 2023 Silicon Laboratories Inc.
 */

#include "ZW_transport.h"
#include "mock_control.h"
#include "ZW_protocol.h"
#include "ZW_explore.h"
#include <string.h>

void setUpSuite(void) {

}

void tearDownSuite(void) {

}

void setUp(void) {

}

void tearDown(void) {

}

bool currentSeqNoUseTX = false;

extern bool IsExploreRouteRepeaterValid(const uint8_t *route, uint8_t index);
extern bool IsExploreRouteValid(const frameHeaderExplore *pExplorerHeader, uint8_t bExploreFrameLength);
extern frameExploreStruct* ExploreSearchResultGenerateRepeatFrame(ZW_ReceiveFrame_t *pRxFrame, frameHeaderExplore *pExplorerHeader, uint8_t singleCastLen, uint8_t RepeaterCountSessionTTL);

void test_IsExploreRouteValid(void)
{
  frameHeaderExplore aExploreFrame[9] = {{.repeaterCountSessionTTL = 0x42, .repeaterList = {1, 2, 3, 4}},
                                         {.repeaterCountSessionTTL = 0x22, .repeaterList = {1, 2, 1, 0}},
                                         {.repeaterCountSessionTTL = 0x22, .repeaterList = {1, 2, 0, 4}},
                                         {.repeaterCountSessionTTL = 0x23, .repeaterList = {1, 2, 3, 0}},
                                         {.repeaterCountSessionTTL = 0x32, .repeaterList = {1, 2, 3, 0}},
                                         {.repeaterCountSessionTTL = 0x31, .repeaterList = {1, 0, 0, 0}},
                                         {.repeaterCountSessionTTL = 0x40, .repeaterList = {0, 0, 0, 0}},
                                         {.repeaterCountSessionTTL = 0x04, .repeaterList = {1, 2, 3, 4}},
                                         {.repeaterCountSessionTTL = 0x13, .repeaterList = {1, 2, 3, 4}}
                                        };
  uint16_t valid = IsExploreRouteValid(&aExploreFrame[0], 16);
  TEST_ASSERT_EQUAL_UINT(0, valid);

  valid = IsExploreRouteValid(&aExploreFrame[1], 16);
  TEST_ASSERT_EQUAL_UINT(0, valid);

  valid = IsExploreRouteValid(&aExploreFrame[2], 16);
  TEST_ASSERT_EQUAL_UINT(0, valid);

  valid = IsExploreRouteValid(&aExploreFrame[3], 16);
  TEST_ASSERT_EQUAL_UINT(0, valid);

  valid = IsExploreRouteValid(&aExploreFrame[4], 16);
  TEST_ASSERT_EQUAL_UINT(0, valid);

  valid = IsExploreRouteValid(&aExploreFrame[5], 16);
  TEST_ASSERT_EQUAL_UINT(1, valid);

  valid = IsExploreRouteValid(&aExploreFrame[6], 16);
  TEST_ASSERT_EQUAL_UINT(1, valid);

  valid = IsExploreRouteValid(&aExploreFrame[7], 16);
  TEST_ASSERT_EQUAL_UINT(1, valid);

  valid = IsExploreRouteValid(&aExploreFrame[8], 16);
  TEST_ASSERT_EQUAL_UINT(1, valid);

  valid = IsExploreRouteRepeaterValid(aExploreFrame[6].repeaterList, 0);
  TEST_ASSERT_EQUAL_UINT(1, valid);

  valid = IsExploreRouteRepeaterValid(aExploreFrame[6].repeaterList, 5);
  TEST_ASSERT_EQUAL_UINT(0, valid);
}

void test_ExploreSearchResultGenerateRepeatFrame(void)
{
  ZW_ReceiveFrame_t RxFrame;
  uint8_t aFrame2ch[] =    { 0xFB, 0x22, 0xCD, 0x91, 0x03, 0x05, 0x03, 0x19, 0x01, 0x22, 0x07, 0xFA, 0x31, 0x02, 0x00, 0x00, 0x00, 0x01, 0x03, 0x31, 0x02, 0x00, 0x00, 0x00, 0xBA};
  uint8_t aFrame2chRep[] = { 0xFB, 0x22, 0xCD, 0x91, 0x03, 0x05, 0x03, 0x19, 0x01, 0x22, 0x07, 0x00, 0x40, 0x02, 0x00, 0x00, 0x00, 0x01, 0x03, 0x31, 0x02, 0x00, 0x00, 0x00, 0xBA};
  uint8_t aFrame3ch[] =    { 0xFB, 0x22, 0xCD, 0x91, 0x03, 0x00, 0x05, 0x03, 0x19, 0x01, 0x22, 0x07, 0xFA, 0x31, 0x02, 0x00, 0x00, 0x00, 0x01, 0x03, 0x31, 0x02, 0x00, 0x00, 0x00, 0xBA, 0xBB};
  uint8_t aFrame3chRep[] = { 0xFB, 0x22, 0xCD, 0x91, 0x03, 0x00, 0x05, 0x03, 0x19, 0x01, 0x22, 0x07, 0x00, 0x40, 0x02, 0x00, 0x00, 0x00, 0x01, 0x03, 0x31, 0x02, 0x00, 0x00, 0x00, 0xBA, 0xBB};

  RxFrame.channelId = 1;
  RxFrame.status = 0;
  RxFrame.rssi = 0;
  RxFrame.pPayloadStart = 0;
  RxFrame.frameContentLength = 0;
  RxFrame.frameContent = 0;
  
  // 2 Channel
  RxFrame.profile = RF_PROFILE_40K;
  RxFrame.channelHeaderFormat = HDRFORMATTYP_2CH;
  RxFrame.frameContent = aFrame2ch;
  RxFrame.pPayloadStart = &RxFrame.frameContent[sizeof(frameHeader) + 1];

  frameExploreStruct *pExploreStruct2ch = ExploreSearchResultGenerateRepeatFrame(&RxFrame, (frameHeaderExplore *)RxFrame.pPayloadStart, sizeof(frameHeader) + 1, ((frameHeaderExplore *)RxFrame.pPayloadStart)->repeaterCountSessionTTL & EXPLORE_REPEATERCOUNTSESSIONTTL_COUNT_MASK);
  bool status = memcmp(pExploreStruct2ch->baseFrame.payload, &aFrame2chRep[sizeof(frameHeader) + 1], pExploreStruct2ch->baseFrame.payloadLength);
  TEST_ASSERT_EQUAL_UINT(0, status);

  // 3 Channel
  RxFrame.profile = RF_PROFILE_3CH_100K;
  RxFrame.channelHeaderFormat = HDRFORMATTYP_3CH;
  RxFrame.frameContent = aFrame3ch;
  RxFrame.pPayloadStart = &RxFrame.frameContent[sizeof(frameHeader3ch) + 1];

  frameExploreStruct *pExploreStruct3ch = ExploreSearchResultGenerateRepeatFrame(&RxFrame, (frameHeaderExplore *)RxFrame.pPayloadStart, sizeof(frameHeader3ch) + 1, ((frameHeaderExplore *)RxFrame.pPayloadStart)->repeaterCountSessionTTL & EXPLORE_REPEATERCOUNTSESSIONTTL_COUNT_MASK);
  status = memcmp(pExploreStruct3ch->baseFrame.payload, &aFrame3chRep[sizeof(frameHeader3ch) + 1], pExploreStruct3ch->baseFrame.payloadLength);
  TEST_ASSERT_EQUAL_UINT(0, status);
}
