// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file ZW_DataLinkLayer_utils.c
 * @copyright 2022 Silicon Laboratories Inc.
 *
 * @brief This module provide functions to needed by the datalink layer.
 */
#include  <ZW_lib_defines.h>
#include "ZW_DataLinkLayer.h"
#include <zpal_radio.h>
#include <zpal_radio_utils.h>
#include <stdint.h>
#include <string.h>
#include <ZW_protocol_def.h>
#include <ZW_Channels.h>

#ifdef ZW_CONTROLLER
#include "ZW_node.h"
#include "ZW_controller_network_info_storage.h"
#endif
#ifdef ZW_SLAVE
#include "ZW_slave_network_info_storage.h"
#endif
#include "ZW_Frame.h"

//#define DEBUGPRINT
#include <DebugPrint.h>

// Includes needed for Dynamic Transmission Power Algorithm
#include "ZW_dynamic_tx_power.h"
#include "ZW_dynamic_tx_power_algorithm.h"
#include <ZW_noise_detect.h>
#include "ZW_basis.h"
#include "ZW_protocol.h"

static uint8_t mTxRetryCount;

/**@brief Function for updating the txPower field for ACK and singlecast LR frames
 *
 * @param[in,out]    pFrame Pointer to the frame. The txPower field is updated
 *                   based on the dynamic Tx Power algorithm (ZW_DynamicTxPowerAlgorithm)
 *                   which calculates a new txPower based on the current noiseFloor, and
 *                   the RSSI and txPower of the last frame received.
 */
void updateLRFrameTXPower(ZW_TransmissionFrame_t * pFrame)
{
  if (pFrame->status & RETRANSMIT_FRAME)
  {
    // Read last TX Power from the RentionRegister if a Slave device
#ifdef ZW_SLAVE
    int8_t tmpRssi; // Must save RSSI so it can be written back below
    ReadTxPowerAndRSSI(&(pFrame->frameOptions.txPower), &tmpRssi);
    if ( mTxRetryCount++ < (uint8_t)(TRANSFER_RETRIES-1))
    {
#endif
      // Read last TX Power from Nodelist keeping track of the TX Power for all nodes if a controller.
#ifdef ZW_CONTROLLER
      pFrame->frameOptions.txPower = GetTXPowerforLRNode(pFrame->frameOptions.destinationNodeId);
#endif

      /* Call the Dynamic Transmission Power Algorithm, which will Increase TX Power with 3 dBm (Retransmission)
        *  All the other parameters apart from TxPower and RETRANSMIT_FRAME can be neglected
        */
      pFrame->frameOptions.txPower = ZW_DynamicTxPowerAlgorithm(pFrame->frameOptions.txPower, 0, 0, RETRANSMISSION);
#ifdef ZW_SLAVE
      tmpRssi = ZPAL_RADIO_RSSI_NOT_AVAILABLE;
#endif
      // Save Updated TX Power to Retention RAM for Slave devices
#ifdef ZW_SLAVE
      SaveTxPowerAndRSSI(pFrame->frameOptions.txPower, tmpRssi);
    }
#endif

    // Save Tx Power to nodelist for a controller
#ifdef ZW_CONTROLLER
    SetTXPowerforLRNode(pFrame->frameOptions.destinationNodeId, (int8_t)(pFrame->frameOptions.txPower));
#endif
  }
  else
  {
    mTxRetryCount = 0;
    // Get The last noisefloor measurement
    SampleNoiseLevel();
    RSSI_LEVELS noisefloorMeasured = { 0 };
    ZW_GetBackgroundRSSI( &noisefloorMeasured);

    // Load the noisefloor into the header and the frameoptions
    uint8_t zwaveChannel = ZW_GetChannelIndexLR(pFrame->profile, zpal_radio_get_lr_channel_config());

    // Put the measured Noisefloor into the frame
    pFrame->header.singlecastLR.header.noiseFloor = noisefloorMeasured.rssi_dBm[zwaveChannel];
    pFrame->frameOptions.noiseFloor = pFrame->header.singlecastLR.header.noiseFloor;
    zwaveChannel += (zpal_radio_get_lr_channel_config() == ZPAL_RADIO_LR_CH_CFG2) ? 1 : 0;
    zpal_radio_rf_channel_statistic_background_rssi_average_update(zwaveChannel, pFrame->frameOptions.noiseFloor);

    //ACK frames must be returned with the same tx power as the frame they ack.
    if (HDRTYP_TRANSFERACK == pFrame->frameOptions.frameType)
    {
      //The tx power for ACKs is stored to pFrame->txPower in ZW_transport.c ReceiveHandler()
      pFrame->header.singlecastLR.header.txPower = pFrame->txPower;
      pFrame->frameOptions.txPower = pFrame->txPower;
      return;
    }

    if ((NODE_BROADCAST_LR == pFrame->frameOptions.destinationNodeId) && (HDRTYP_TRANSFERACK != pFrame->frameOptions.frameType)) {
      pFrame->frameOptions.txPower = zpal_radio_get_maximum_lr_tx_power();
    } else {
      // Load the TX Power from retention RAM if the device is a slave
#ifdef ZW_SLAVE
      int8_t txPower;
      int8_t rssi;
      ReadTxPowerAndRSSI(&txPower, &rssi);

      pFrame->frameOptions.txPower = ZW_DynamicTxPowerAlgorithm(txPower, rssi, pFrame->frameOptions.noiseFloor, NOT_RETRANSMISSION);
      SaveTxPowerAndRSSI(pFrame->frameOptions.txPower, rssi);
#endif
      // Load the TX Power from the nodelist if a controller
#ifdef ZW_CONTROLLER
      pFrame->frameOptions.txPower = GetTXPowerforLRNode( pFrame->frameOptions.destinationNodeId);
#endif
    }
  }
  // Insert the updated TX Power in the LR Frame
  pFrame->header.singlecastLR.header.txPower = pFrame->frameOptions.txPower;
  pFrame->txPower = pFrame->frameOptions.txPower;
}

/**@brief Function for determining current Frame Header Format
 *
 * @param[in] wNodeID            The node ID for the node we want to get the used frame format.
 * @param[in] forceLR            Force the frame format to Long range
 *
 * @retval HDRFORMATTYP_3CH      The current Frame Header Format is 3 channel
 * @retval HDRFORMATTYP_2CH      The current Frame Header Format is 2 channel
 * @retval HDRFORMATTYP_LR       The current Frame Header Format is Long range
 */
ZW_HeaderFormatType_t llGetCurrentHeaderFormat(__attribute__((unused)) node_id_t wNodeID , uint8_t forceLR)
{
  zpal_radio_protocol_mode_t rfMode = zpal_radio_get_protocol_mode();
  if (ZPAL_RADIO_PROTOCOL_MODE_1 == rfMode)
  {
     return HDRFORMATTYP_2CH;
  }
  if (ZPAL_RADIO_PROTOCOL_MODE_2 == rfMode)
  {
     return HDRFORMATTYP_3CH;
  }
  if (ZPAL_RADIO_PROTOCOL_MODE_3 == rfMode)
  {
    if (forceLR)
    {
      return HDRFORMATTYP_LR;
    }
    else
    {
#ifdef ZW_CONTROLLER
#ifdef ZW_CONTROLLER_TEST_LIB
      if((NODE_BROADCAST_LR == wNodeID) || (ZW_nodeIsLRNodeID(wNodeID)) || CtrlStorageLongRangeGet(wNodeID))
#else
      if((NODE_BROADCAST_LR == wNodeID) || CtrlStorageLongRangeGet(wNodeID))
#endif
      {
        return HDRFORMATTYP_LR;
      }
      else
      {
        return HDRFORMATTYP_2CH;
      }

#endif
#ifdef ZW_SLAVE
       if (zpal_radio_is_long_range_locked())
       {
          return HDRFORMATTYP_LR;
       }
       else
       {
          return HDRFORMATTYP_2CH;
       }
#endif
    }
  }
  if (ZPAL_RADIO_PROTOCOL_MODE_4 == rfMode)
  {
    return HDRFORMATTYP_LR;
  }
  return HDRFORMATTYP_UNDEFINED;
}


bool
IsReceivedFrameMatchNodeConfig (__attribute__((unused)) ZW_HeaderFormatType_t headerType)
{
#ifdef ZW_SLAVE
   /*If a slave node is already included as a LR then we drop frames received on classic channels*/
  if ((true == zpal_radio_is_long_range_locked()) && (HDRFORMATTYP_LR != headerType))
  {
     return false;
  }
  /*If a slave node is already included as ZWave node then we drop frames received on ZWave LR channel*/
  if ((0 != g_nodeID) && (false == zpal_radio_is_long_range_locked()) && (HDRFORMATTYP_LR == headerType))
  {
     return false;
  }
#endif
  return true;

}
