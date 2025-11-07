// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file
 * @copyright 2022 Silicon Laboratories Inc.
 */
#ifndef _ZW_BASIS_H_
#define _ZW_BASIS_H_

#include <ZW_typedefs.h>
#include <ZW_basis_api.h>
#include "ZW_transport_transmit_cb.h"

/*===========================   ZW_SendNodeInformation   ====================*/
/**
* \ingroup BASIS
*
* \macro{ ZW_SEND_NODE_INFO(node,option,func) }
* Create and transmit a "Node Information" frame. The Z Wave transport layer builds a frame,
* request application node information (see ApplicationNodeInformation) and queue the
* "Node Information" frame for transmission. The completed call back function (completedFunc)
* is called when the transmission is complete.
*
* The Node Information Frame is a protocol frame and will therefore not be directly
* available to the application on the receiver. The API call ZW_SetLearnMode() can be used
* to instruct the protocol to pass the Node Information Frame to the application.
*
* When ZW_SendNodeInformation() is used in learn mode for adding or removing the node
* from the network the transmit option TRANSMIT_OPTION_LOW_POWER should NOT be used.
* \note ZW_SendNodeInformation uses the transmit queue in the API, so using other transmit functions before the complete callback has been called by the API is not recommended.
*
* \return true  If frame was put in the transmit queue
* \return false If it was not (callback will not be called)
*
* \param[in] destNode Destination Node ID (NODE_BROADCAST == all nodes)
* \param[in] txOptions  Transmit option flags. (see ZW_SendData)
* \param[in] completedFunc  Transmit completed call back function
*
* Callback function Parameters:
* \param[in] txStatus (see \ref ZW_SendData)
*
* Timeout: 65s
* Exception recovery: Resume normal operation, no recovery needed
* \serialapi{
* HOST->ZW: REQ | 0x12 | destNode | txOptions | funcID
* ZW->HOST: RES | 0x12 | retVal
* ZW->HOST: REQ | 0x12 | funcID | txStatus
* }
*
*/
uint8_t                                  /*RET  false if transmitter queue overflow   */
ZW_SendNodeInformation(
  node_id_t   destNode,                  /*IN  Destination Node ID  */
  TxOptions_t txOptions,                 /*IN  Transmit option flags         */
  const STransmitCallback* pTxCallback); /*IN  Transmit completed call back function  */

/*===========================   ZW_SendExcludeRequestLR   ====================*/
/**
* \ingroup BASIS
*
* Creates and transmits a "Exclude Request Command" frame for requesting exclusion
* from a Z-Wave Long Range network.
* The completed call back function (completedFunc) is called when the transmission
* is complete.
*
* The Exclude Request Command Frame is a protocol frame and will therefore not be
* directly available to the application on the receiver.
*
* \note ZW_SendNodeInformation uses the transmit queue in the API, so using other
* transmit functions before the complete callback has been called by the API is not
* recommended.
*
* \return true  If frame was put in the transmit queue
* \return false If it was not (callback will not be called)
*
* \param[in] completedFunc  Transmit completed call back function
*
* Callback function Parameters:
* \param[in] txStatus (see \ref ZW_SendData)
**
*/
bool                    /*RET  false if transmitter queue overflow   */
ZW_SendExcludeRequestLR(
  const STransmitCallback* pTxCallback); /*IN  Transmit completed call back function  */

/*===========================   ZW_SendTestFrame   ==========================*/
/*
**       normalPower : Max power possible
**       minus2dBm    - normalPower - 2dBm
**       minus4dBm    - normalPower - 4dBm
**       minus6dBm    - normalPower - 6dBm
**       minus8dBm    - normalPower - 8dBm
**       minus10dBm   - normalPower - 10dBm
**       minus12dBm   - normalPower - 12dBm
**       minus14dBm   - normalPower - 14dBm
**       minus16dBm   - normalPower - 16dBm
**       minus18dBm   - normalPower - 18dBm
*/
/**
* \ingroup BASIS
*
* \macro {ZW_SEND_TEST_FRAME(nodeID, power, func)}
*
* Send a test frame directly to nodeID without any routing, RF transmission
* power is previously set to powerlevel by calling ZW_RF_POWERLEVEL_SET. The
* test frame is acknowledged at the RF transmission powerlevel indicated by the
* parameter powerlevel by nodeID (if the test frame got through).  This test will
* be done using 9600 kbit/s transmission rate.
*
* \note This function should only be used in an install/test link situation.
*
* \param[IN] nodeID Node ID on the node ID (1..232) the test frame should be transmitted to.
* \param[IN] powerLevel Powerlevel to use in RF transmission, valid values:
*  - normalPower Max power possible
*  -  minus1dB  Normal power - 1dB (mapped to minus2dB )
*  -  minus2dB  Normal power - 2dB
*  -  minus3dB  Normal power - 3dB (mapped to minus4dB)
*  -  minus4dB  Normal power - 4dB
*  -  minus5dB  Normal power - 5dB (mapped to minus6dB)
*  -  minus6dB  Normal power - 6dB
*  -  minus7dB  Normal power - 7dB (mapped to minus8dB)
*  -  minus8dB  Normal power - 8dB
*  -  minus9dB  Normal power - 9dB (mapped to minus10dB)
* \param[in] func Callback function called when done.
* Callback function Parameters:
* \param[in] txStatus (see \ref ZW_SendData)
*
* \return false If transmit queue overflow.
*
* Timeout: 200ms
* Exception recovery: Resume normal operation, no recovery needed
*
*/
uint8_t               /*RET false if transmitter busy else true */
ZW_SendTestFrame(
  uint8_t nodeID,     /* IN nodeID to transmit to */
  uint8_t powerLevel, /* IN powerlevel index */
  const STransmitCallback* pTxCallback); /* Call back function called when done */



/*===========================   ZW_ExploreRequestInclusion   =================*/
/* TODO - Unify ZW_ExploreRequestInclusion and setlearnMode - Let protocol handle */
/* the inclusion - ZW_ExploreRequestInclusion should internally call SetLearnMode not let App do it */
/**
* \ingroup BASIS
*
* This function sends out an explorer frame requesting inclusion into a network.
* If the inclusion request is accepted by a controller in network wide inclusion
* mode then the application on this node will get notified through the callback
* from the ZW_SetLearnMode() function. Once a callback is received from ZW_SetLearnMode()
* saying that the inclusion process has started the application should not make further
* calls to this function.
*
* \note Recommend not to call this function more than once every 4 seconds.
*
* \return true  Inclusion request queued for transmission
* \return false Node is not in learn mode
* \serialapi{
* HOST->ZW: REQ | 0x5E
* ZW->HOST: RES | 0x5E | retVal
* }
*
*/
bool
ZW_ExploreRequestInclusion(STransmitCallback* pTxCallback);


/*===========================   ZW_ExploreRequestExclusion   =================*/
/* TODO - Unify ZW_ExploreRequestExclusion and setlearnMode - Let protocol handle */
/* the exclusion - ZW_ExploreRequestExclusion should internally call SetLearnMode not let App do it */
/**
* \ingroup BASIS
*
* This function sends out an explorer frame requesting exclusion from the network.
* If the exclusion request is accepted by a controller in network wide exclusion
* mode then the application on this node will get notified through the callback
* from the ZW_SetLearnMode() function. Once a callback is received from ZW_SetLearnMode()
* saying that the exclusion process has started the application should not make further
* calls to this function.
*
* \note Recommend not to call this function more than once every 4 seconds.
*
* \return true  Exclusion request queued for transmission
* \return false Node is not in learn mode
* \serialapi{
* HOST->ZW: REQ | 0x5F
* ZW->HOST: RES | 0x5F | retVal
* }
*
*/
uint8_t
ZW_ExploreRequestExclusion(STransmitCallback* pTxCallback);


/*========================= ZW_NetworkLearnModeStart =========================
 *
 * \ingroup BASIS
 * \ref ZW_NetworkLearnModeStart is used to enable/disable the Z-Wave Network
 * Node inclusion/exclusion functionality.
 *
 * Declared in: ZW_basis_api.h
 *
 * \return TRUE  Requested Network Learn process initiated
 * \return FALSE
 * \param[in] bMode
      - \ref E_NETWORK_LEARN_MODE_DISABLE       Disable learn process
      - \ref E_NETWORK_LEARN_MODE_INCLUSION     Enable the learn process to do an inclusion
      - \ref E_NETWORK_LEARN_MODE_EXCLUSION     Enable the learn process to do an exclusion
      - \ref E_NETWORK_LEARN_MODE_EXCLUSION_NWE Enable the learn process to do an network wide exclusion
      - \ref E_NETWORK_LEARN_MODE_INCLUSION_SMARTSTART Enable the learn process to do an SMARTSTART inclusion based on local DSK
 * \b Serial API:
 *
 * Uses ZW_SetLearnMode functionality.
 *
 */
uint8_t
ZW_NetworkLearnModeStart(E_NETWORK_LEARN_MODE_ACTION bMode);

/**
 * @brief This Function is used to get the LR channel configuration needed for the given Rf profile
 * used to select the Phy configuration
 * @param[in] pRfProfile Pointer on the radio profile to use for PHY configuration.
 * @return               Returns the long range channel configuration to apply
 */
zpal_radio_lr_channel_config_t ZW_LrChannelConfigToUse(const zpal_radio_profile_t * pRfProfile);


/*========================= ZW_Power_Management_Init =========================
 *
 * \ingroup BASIS
 * \ref ZW_Power_Management_Init is used to setup number of seconds a
 * non listening node sleeps inbetween wakeups (use WUTtimer)
 *
 * Declared in: ZW_basis_api.h
 *
 * \return Nothing
 *
 * \b Serial API:
 *
 * Not implemented.
 */
void
ZW_Power_Management_Init(
  uint32_t sleep,        /* IN number of seconds to sleep until next wakeup */
  uint8_t IntEnable);    /* IN interrupts Enable bitmask, If a bit is 1, */


/*===============================   ZW_SetRFReceiveMode   ===================
**    Initialize the Z-Wave RF chip.
**    Mode on:  Set the RF chip in receive mode and starts the data sampling.
**    Mode off: Set the RF chip in power down mode.
**
**--------------------------------------------------------------------------*/
extern uint8_t         /*RET TRUE if operation was executed successfully, FALSE if not */
ZW_SetRFReceiveMode(
  uint8_t mode);       /* IN TRUE: On; FALSE: Off mode */


#ifdef ZW_PROMISCUOUS_MODE
/*============================   ZW_SetPromiscuousMode   ======================
**
**  Function description.
**   Enable / disable the installer library promiscuous mode.
**   When promiscuous mode is enabled, all application layer frames will be passed
**   to the application layer regardless if the frames are addressed to another node.
**   When promiscuous mode is disabled, only application frames addressed to the node will be passed
**   to the application layer.
**
**  Side effects:
**
**--------------------------------------------------------------------------*/
void                /*Nothing */
ZW_SetPromiscuousMode(
  bool state);     /* IN TRUE to enable the promiscuous mode, FALSE to disable it.*/
#endif /* ZW_PROMISCUOUS_MODE */


/*===================   ZW_GetBackgroundRSSI   ===========================
**
**  Returns an estimate of the current background RSSI level.
**
**  A separate measurement is returned for each channel.
**
**-------------------------------------------------------------------------*/
void
ZW_GetBackgroundRSSI(
    RSSI_LEVELS *noise_levels);    /*OUT Structure to be filled with noise levels */


/*==================   ZW_RegisterBackgroundRSSICallback   =====================
**
**  Register a callback function to be called whenever new background
**  RSSI samples are available.
**
**  Side effects: None
**
**-------------------------------------------------------------------------*/
void ZW_RegisterBackgroundRSSICallback(
    VOID_CALLBACKFUNC(cbFun)(RSSI_LEVELS *)); /* IN Callback function to register*/

/*===========================   ZW_GetDefaultPowerLevel   ==================*/
/**
 * \ingroup BASIS
 *
 * Get the default power levels used for transmitting Z-Wave frames
 *
 * \serialapi{
 *    Not supported (See serialAPI documentation of FUNC_ID_SERIAL_API_SETUP)
 * }
 *
**/
void ZW_GetDefaultPowerLevels(uint8_t *pPowerlevels);

/**
 * Calculates and returns the maximum size of the data payload, with or without security.
 * The calculation includes all headers, except for the CC header. (6 bytes)
 *
 * Takes security encapsulation into consideration.
 * @param keys Granted security keys.
 * @return Max payload size.
 */
uint16_t ZW_GetMaxPayloadSize(uint8_t keys);

/**
 * Returns the max payload size of Long Range frames.
 *
 * @return Max payload size.
 */
#ifdef ZW_CONTROLLER
uint16_t ZW_GetMaxLRPayloadSize(void);
#endif

#endif /* _ZW_BASIS_API_H_ */
