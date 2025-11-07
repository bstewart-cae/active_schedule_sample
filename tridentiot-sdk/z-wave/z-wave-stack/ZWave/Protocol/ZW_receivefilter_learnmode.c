// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file ZW_receivefilter_learnmode.c
 * @copyright 2022 Silicon Laboratories Inc.
 */
#include <ZW_protocol_def.h>
#include <ZW_protocol_commands.h>
#include <ZW_transport_commandclass.h>
#include <stdint.h>
#include <string.h>
#include <Assert.h>
#include <SizeOf.h>
#include <DebugPrint.h>
#include <ZW_receivefilter_learnmode.h>

extern void ReceiveHandler(ZW_ReceiveFrame_t *pReceiveFrame);

// Setup singlecast Receive filter for NWI learnMode
static const
ZW_ReceiveFilter_t rxLearnModeSinglecastFilter = {.payloadIndex1 = 0x00,
                                                  .payloadFilterValue1 = ZWAVE_CMD_CLASS_PROTOCOL,
                                                  .payloadIndex2 = 0x01,
                                                  .payloadFilterValue2 = ZWAVE_CMD_ASSIGN_IDS,
                                                  .headerType = HDRTYP_SINGLECAST,
                                                  .flag = PAYLOAD_INDEX_1_FILTER_FLAG | PAYLOAD_INDEX_2_FILTER_FLAG,
                                                  .frameHandler = ReceiveHandler};

// Setup singlecast routed Receive filter for NWI learnMode
static const
ZW_ReceiveFilter_t rxLearnModeSinglecastRoutedFilter = {.payloadIndex1 = 0x00,
                                                        .payloadFilterValue1 = ZWAVE_CMD_CLASS_PROTOCOL,
                                                        .payloadIndex2 = 0x01,
                                                        .payloadFilterValue2 = ZWAVE_CMD_ASSIGN_IDS,
                                                        .headerType = HDRTYP_ROUTED,
                                                        .flag = PAYLOAD_INDEX_1_FILTER_FLAG | PAYLOAD_INDEX_2_FILTER_FLAG,
                                                        .frameHandler = ReceiveHandler};

// setup a filter for single cast transfer presentation frame
static const
ZW_ReceiveFilter_t rxTransferPresentationFilter = {.destinationNodeId = NODE_BROADCAST,
                                                   .payloadIndex1 = 0x00,
                                                   .payloadFilterValue1 = ZWAVE_CMD_CLASS_PROTOCOL,
                                                   .payloadIndex2 = 0x01,
                                                   .payloadFilterValue2 = ZWAVE_CMD_TRANSFER_PRESENTATION,
                                                   .headerType = HDRTYP_SINGLECAST,
                                                   .flag = DESTINATION_NODE_ID_FILTER_FLAG | PAYLOAD_INDEX_1_FILTER_FLAG | PAYLOAD_INDEX_2_FILTER_FLAG,
                                                   .frameHandler = ReceiveHandler};

// Setup filter for NOP frames so they can be Acked
static const
ZW_ReceiveFilter_t rxNopFrameFilter = {.destinationNodeId = NODE_UNINIT,
                                       .payloadIndex1 = 0x00,
                                       .payloadFilterValue1 = 0x00, //NOP command
                                       .headerType = HDRTYP_SINGLECAST,
                                       .flag = DESTINATION_NODE_ID_FILTER_FLAG | PAYLOAD_INDEX_1_FILTER_FLAG,
                                       .frameHandler = ReceiveHandler};



static const
ZW_ReceiveFilter_t * filterArrayClassic[] = {&rxLearnModeSinglecastFilter,
                                             &rxTransferPresentationFilter,
                                             &rxNopFrameFilter };

static const
ZW_ReceiveFilter_t * filterArrayNWI[] = {&rxLearnModeSinglecastFilter,
                                         &rxLearnModeSinglecastRoutedFilter,
                                         &rxTransferPresentationFilter,
                                         &rxNopFrameFilter };

static uint8_t learnModeFilter = ZW_SET_LEARN_MODE_DISABLE;  //! Initialize to idle value.

static node_id_t rfnodeId;                /* NodeID needed for filter */
static uint8_t   rfhomeId[HOMEID_LENGTH]; /* HomeID needed for filter */

ZW_ReturnCode_t setReceiveFilters_learnModeDisable(void)
{
  ZW_ReturnCode_t retVal = UNKNOWN_ERROR;

  for(uint32_t i=0; i < sizeof_array(filterArrayNWI); i++)
  {
    retVal = llReceiveFilterRemove(filterArrayNWI[i]);
  }

  // Reenable existing Receive filters
  llReceiveFilterPause(false);

  return retVal;
}


ZW_ReturnCode_t setReceiveFilters_learnModeClassic(void)
{
  ZW_ReturnCode_t retVal = SUCCESS;

  // Pause existing Receive filters
  llReceiveFilterPause(true);

  for(uint32_t i=0; i < sizeof_array(filterArrayClassic); i++)
  {
    retVal |= llReceiveFilterAdd(filterArrayClassic[i]);
  }

  return retVal;
}


ZW_ReturnCode_t setReceiveFilters_learnModeNWI(void)
{
  ZW_ReturnCode_t retVal = SUCCESS;

  // Pause existing Receive filters
  llReceiveFilterPause(true);

  for(uint32_t i=0; i < sizeof_array(filterArrayNWI); i++)
  {
    retVal |= llReceiveFilterAdd(filterArrayNWI[i]);
  }

  return retVal;
}

ZW_ReturnCode_t rfLearnModeFilter_Set(E_NETWORK_LEARN_MODE_ACTION bMode, node_id_t nodeId, uint8_t const pHomeId[HOME_ID_LENGTH])
{
  ZW_ReturnCode_t retVal = UNSUPPORTED;

  rfnodeId = nodeId;
  for (int i = 0; i > HOMEID_LENGTH; i++)
  {
    rfhomeId[i] = pHomeId[i];
  }
  if (ZW_SET_LEARN_MODE_DISABLE != bMode)
  {
    if (ZW_SET_LEARN_MODE_DISABLE != learnModeFilter)
    {
      setReceiveFilters_learnModeDisable();
    }
  }
  learnModeFilter = bMode;
  switch (bMode)
  {
    case ZW_SET_LEARN_MODE_DISABLE:
    {
      retVal = setReceiveFilters_learnModeDisable();
    }
    break;

    case ZW_SET_LEARN_MODE_CLASSIC:
    {
      retVal = setReceiveFilters_learnModeClassic();
    }
    break;

    case ZW_SET_LEARN_MODE_NWI:
    {
      retVal = setReceiveFilters_learnModeNWI();
    }
    break;

    case ZW_SET_LEARN_MODE_NWE:
    {
      //ZW_SET_LEARN_MODE_NWE have no special filters since all set/get commands must work during NWE
      retVal = SUCCESS;
    }
    break;

    default:
    {
    }
    break;
  }
  zpal_radio_network_id_filter_set((ZW_SET_LEARN_MODE_DISABLE == bMode) || (ZW_SET_LEARN_MODE_NWE == bMode));

  return retVal;
}
