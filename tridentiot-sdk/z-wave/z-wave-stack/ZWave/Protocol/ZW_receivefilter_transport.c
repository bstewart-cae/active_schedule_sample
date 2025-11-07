// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file ZW_receivefilter_transport.c
 * @copyright 2022 Silicon Laboratories Inc.
 */
#include <ZW_DataLinkLayer.h>
#include <stdint.h>
#include <string.h>
#include <Assert.h>
#include <SizeOf.h>
#include <DebugPrint.h>

extern void ReceiveHandler(ZW_ReceiveFrame_t *pReceiveFrame);

static const ZW_ReceiveFilter_t receiveFilterSinglecast = {.headerType = HDRTYP_SINGLECAST, .flag = 0, .frameHandler = ReceiveHandler};
static const ZW_ReceiveFilter_t receiveFilterSinglecastRouted = {.headerType = HDRTYP_ROUTED, .flag = 0, .frameHandler = ReceiveHandler};
static const ZW_ReceiveFilter_t receiveFilterTransferAck = {.headerType = HDRTYP_TRANSFERACK, .flag = 0, .frameHandler = ReceiveHandler};
static const ZW_ReceiveFilter_t receiveFilterMulticast = {.headerType = HDRTYP_MULTICAST, .flag = 0, .frameHandler = ReceiveHandler};


static const
ZW_ReceiveFilter_t * filterPtrArray[] = {&receiveFilterSinglecast,
                                         &receiveFilterSinglecastRouted,
                                         &receiveFilterTransferAck,
                                         &receiveFilterMulticast };

/**Function for setting default transport receive filters.
 *
 * When the function returns, any potential active receive filters have been removed
 * and default transport receive filters have been added and activated.
 *
 */
ZW_ReturnCode_t rfTransportFilter_Set(void)
{
  ZW_ReturnCode_t eLinkLayerStatus = SUCCESS;

  for(uint32_t i=0; i < sizeof_array(filterPtrArray); i++)
  {
    eLinkLayerStatus |= llReceiveFilterAdd(filterPtrArray[i]);
  }

  return eLinkLayerStatus;
}


/**Function for setting default transport receive filters.
 *
 * When the function returns, any active default transport receive filters have been removed
 *
 */
ZW_ReturnCode_t rfTransportFilter_Remove(void)
{
  ZW_ReturnCode_t eLinkLayerStatus = UNKNOWN_ERROR;

  for(uint32_t i=0; i < sizeof_array(filterPtrArray); i++)
  {
    eLinkLayerStatus = llReceiveFilterRemove(filterPtrArray[i]);
  }

  return eLinkLayerStatus;
}
