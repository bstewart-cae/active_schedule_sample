// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file
 * @copyright 2022 Silicon Laboratories Inc.
 */
#ifndef _ZW_DATALINKLAYER_H_
#define _ZW_DATALINKLAYER_H_

#include <stdint.h>
#include <zpal_radio.h>
#include <ZW_Frame.h>

/**
 * @defgroup zwll Z-Wave Data Link Layer
 *
 * Module for low level transmission and filter parsing of received Z-Wave frames.
 *
 * This module handles the basic Z-Wave frame header for 2 and 3 channel Z-Wave radio systems.
 *
 * @section transmission Z-Wave Frame transmission
 *
 * Module dependencies for frame transmission can be seen in the following graph.
 *
 * @startuml
 * object "ZW_Transport.c" as tl
 * object "ZW_Explorer.c" as el
 * object "ZW_Routing.c" as rl
 * object "ZW_tx_queue.c" as queue
 * object "ZW_DataLinkLayer.c" as ll
 * object "ZW_Radio.c" as phy
 *
 * tl --> queue
 * tl --> el
 * tl --> rl
 * el --> queue
 * rl --> queue
 * queue --> ll
 * ll --> phy
 *
 * @enduml
 *
 * Going forward, the TX queue should be cleaner in its responsibilities and thus be pure queue
 * oriented where you can add / pull from the queue.
 *
 * @startuml
 * object "ZW_Transport.c" as tl
 * object "ZW_Explorer.c" as el
 * object "ZW_Routing.c" as rl
 * object "ZW_tx_queue.c" as queue
 * object "ZW_DataLinkLayer.c" as ll
 * object "ZW_Radio.c" as phy
 *
 * tl --> el
 * tl --> rl
 * tl --> ll
 * el --> ll
 * rl --> ll
 * ll -right-> queue
 * queue --> ll
 * ll --> phy
 *
 * @enduml
 *
 *@section reception Z-Wave Frame reception
 *
 * Message Sequence Charts describing how a frame is received and parsed to listening layers.
 *
 * The following MSC illustrates how a filter can be added and how a received single cast frame is
 * processed in this layer and passed to pReceiveFilter->frameHandler upon a match
 *
 * \startuml
 * participant  Radio as radio
 * participant "Data Link Layer" as ll
 * participant "Upper Layer" as ul
 *
 * ul -> ll : llReceiveFilterAdd(pReceiveFilter)
 * activate ll
 *
 * ll -> ll : receiveFilterAdd(...)
 *
 * ll --> ul
 * deactivate ll
 *
 * |||
 * |||
 *
 * radio -> ll : llReceiveHandler(pRxParameters, pFrame)
 * activate ll
 *
 * ll -> ll    : singleCastHandler(pRxParameters, pFrame)
 * activate ll
 *
 * ll -> ul   : filter.frameHandler(pFrame)
 * ul --> ll
 * deactivate ll
 *
 * ll --> radio
 * deactivate ll
 *
 * \enduml
 *
 *
 * The following MSC illustrates how a filter can be added and how a received single cast frame is
 * processed in this layer and discarded when there is no matching filter
 *
 * \startuml
 * participant  Radio as radio
 * participant "Data Link Layer" as ll
 * participant "Upper Layer" as ul
 *
 * ul -> ll : llReceiveFilterAdd(pReceiveFilter)
 * activate ll
 *
 * ll -> ll : receiveFilterAdd(...)
 *
 * ll --> ul
 * deactivate ll
 *
 * |||
 * |||
 *
 * radio -> ll : llReceiveHandler(pRxParameters, pFrame)
 * activate ll
 *
 * ll -> ll    : singleCastHandler(pRxParameters, pFrame)
 * activate ll
 *
 * ll ->x ul   : discard
 * |||
 * deactivate ll
 *
 * ll --> radio
 * deactivate ll
 *
 * \enduml
 *
 *
 *
 * @{
 */

/**
 * @defgroup zwll_filter_flags Z-Wave Data Link Layer - Filter Flags
 * @{
 *
 * This group of macros define flags used by filters during frame reception.
 *
 * When configuring a filter, the filter flag corresponding to frame content that must be matched
 * must be set, e.g.
 * \code filter.flag = HOMEID_FILTER_FLAG | DESTINATION_NODE_ID_FILTER_FLAG\endcode
 * denotes that this filter will only receive the frame if Home Id and Destination Id of the
 * received frame matches the filter.
 *
 *
 * The received frame is compared to the filter flags, so the filter with highest flag value that
 * matches the frame will have its \ref ZW_ReceiveFilter_t.frameHandler executed.
 *
 *
 * As example: A filter with \ref HOMEID_FILTER_FLAG and \ref DESTINATION_NODE_ID_FILTER_FLAG
 * defined will have the value:
 *
 * \code HOMEID_FILTER_FLAG | DESTINATION_NODE_ID_FILTER_FLAG = 0b10000000 | 0b00100000 = 0b10100000 \endcode
 *
 * whereas, the pure Home Id filter \ref HOMEID_FILTER_FLAG has the value
 *
 * \code HOMEID_FILTER_FLAG = 0b10000000 \endcode
 *
 */
#define HOMEID_FILTER_FLAG              0x80 ///< Home ID filtering is enabled
#define SOURCE_NODE_ID_FILTER_FLAG      0x40 ///< Source Node ID filtering is enabled
#define DESTINATION_NODE_ID_FILTER_FLAG 0x20 ///< Destination Node ID filtering is enabled
#define PAYLOAD_INDEX_1_FILTER_FLAG     0x10 ///< Payload Index 1 filtering is enabled
#define PAYLOAD_INDEX_2_FILTER_FLAG     0x08 ///< Payload Index 2 filtering is enabled
/**
 * @}
 */

#define RETRANSMIT_FRAME  0x01

typedef void (*ZW_DataLinkLayerFrameHandler)(ZW_ReceiveFrame_t * pFrame);

/**@brief union for homeid filter definition
 *
 */
typedef union _U_HOMEID_T_
{
    uint8_t   array[HOME_ID_LENGTH];
    uint32_t  word;
} u_homeid_t;

/**@brief Structure for receive filter settings
 *
 * @details Adding a filter to frame reception handling is done by specifying the filter values to
 *          compare in a received frame.
 *          Thereafter update the flag field with the information of which parts of the filter is
 *          active.
 *          To activate a filter, invoke \ref llReceiveFilterAdd.
 *          A filter can be removed by invoking \ref llReceiveFilterRemove.
 *
 */
typedef struct ZW_ReceiveFilter
{
    u_homeid_t homeId;                         ///< Home ID filter for frames that should be accepted. Use HomeID FFFFFFFF for accept all home id and only use payload filter.
    node_id_t sourceNodeId;                    ///< Source Node Id matching value for the filter.
    node_id_t destinationNodeId;               ///< Destination Node Id matching value for the filter.
    uint8_t payloadIndex1;                     ///< Index in payload where payload filter should be compared.
    uint8_t payloadFilterValue1;               ///< Filter value for frames that should be accepted.
    uint8_t payloadIndex2;                     ///< Index in payload where payload filter should be compared.
    uint8_t payloadFilterValue2;               ///< Filter value for frames that should be accepted.
    uint8_t flag;                              ///< Flag specifying the active filter, \ref HOMEID_FILTER_FLAG, \ref SOURCE_NODE_ID_FILTER_FLAG, \ref DESTINATION_NODE_ID_FILTER_FLAG, \ref PAYLOAD_INDEX_1_FILTER_FLAG, \ref PAYLOAD_INDEX_2_FILTER_FLAG
    ZW_FrameType_t               headerType;   ///< Frame type to which this filter applies.
    ZW_DataLinkLayerFrameHandler frameHandler; ///< Frame handler to be called when a frame matching this filter is received.
} ZW_ReceiveFilter_t;


/**
 * @brief Return codes definitions.
 */
typedef enum
{
  SUCCESS = 0,              /**< Value indicating function SUCCESS. **/
  INVALID_PARAMETERS = 1,   /**< Value indicating function parameter/s not valid. **/
  UNSUPPORTED = 2,          /**< Value indicating functionality requested not supported. **/
  NO_MEMORY = 3,            /**< Value indicating function fail due to low memory. **/
  BUSY = 4,                 /**< Value indicating function fail due to an undefined error. **/
  UNKNOWN_ERROR,
} ZW_ReturnCode_t;


// FIXME we should probably remove baud rate struct
/**@brief Baud rates used for configuring the radio during initialization.
*/
typedef enum
{
  BAUD_RATE_9_6k = 0x01,     /**< Value indicating 9.6 kbit/s data rate. **/
  BAUD_RATE_9_6k_40k = 0x02, /**< Value indicating 9.6 and 40 kbit/s data rate. **/
  BAUD_RATE_100k = 0x04,     /**< Value indicating 100 kbit/s data rate. **/
  BAUD_RATE_ALL = 0x06       /**< Value indicating all data rate, i.e. 9.6/40/100 kbit/s data rate. **/
} ZW_BaudRate_t;


/**@brief Function for initializing the Link Layer.
 *
 * @details This function will also ensure proper setup of the RF Phy layer
 *          with the provided Radio Profile.
 *
 * @param[in] pRfProfile Pointer to the RF Profile to use for Radio Configuration
 *
 * @retval SUCCESS            When the layer and radio is successfully initialized
 * @retval INVALID_PARAMETERS When the provided parameters in the zpal_radio_profile_t contains
 *                            incompatible values, eg. Invalid region selected.
 */
ZW_ReturnCode_t llInit(zpal_radio_profile_t* const pRfProfile);


bool llChangeLrChannelConfig(zpal_radio_lr_channel_config_t eLrChCfg);


/**@brief Function for transmitting a Z-Wave frame.
 *
 * @details When this function return, the frame has been copied to the radio for transmission and
 *          the buffer can be released.
 *
 * @param[in] communicationProfile Communication profile to use when transmitting this frame
 * @param[in] pFrame               Pointer to the frame to be transmitted
 *
 * @retval SUCCESS When a frame was successfully delivered to the radio subsystem
 */
ZW_ReturnCode_t llTransmitFrame(CommunicationProfile_t  communicationProfile,
                                ZW_TransmissionFrame_t *pFrame);


/**@brief Function for Re-transmitting a Z-Wave frame.
 *
 * @details When this function return, the frame is ready to be retransmitted
 *
 * @note This function must only be used when a frame already has been transmitted using @ref llTransmitFrame
 *
 * @param[in] pFrame               Pointer to the frame to be transmitted
 */
void llReTransmitStart(ZW_TransmissionFrame_t *pFrame);

/**@brief Function for stopping Re-transmitting a Z-Wave frame.
 *
 * @details When this function return, the frame will not be retransmitted
 *
 * @param[in] pFrame               Pointer to the frame to be transmitted
 */
void llReTransmitStop(ZW_TransmissionFrame_t *pFrame);

/**@brief Function for Testing if Re-transmitting a Z-Wave frame is enabled.
 *
 * @param[in] pFrame               Pointer to the frame to be transmitted
 *
 * @retval true if re transmit is enabled else false
 */
uint8_t llIsReTransmitEnabled(ZW_TransmissionFrame_t *pFrame);


/**@brief Function for adding a receive filter.
 *
 * @details When the function returns the filter has been added to list of active filters and the memory
 * pointed to by pReceiveFilter can be released.
 * \note Each frame type, \ref ZW_FrameType_t, has its own list of active filters
 *
 *
 * @param[in] pReceiveFilter Pointer  to the filter to be active
 *
 * @retval SUCCESS             The filter was successfully add to the list of active filters
 * @retval NO_MEMORY           No more room left in list for the specified frame type
 *                             \ref ZW_FrameType_t. See \ref PACKET_FILTER_SIZE for number of
 *                             active filters in each frame type
 * @retval INVALID_PARAMETERS The filter settings and frame type contains invalid combinations,
 *                            e.g. payload filtering and an ACK frame.
 */
ZW_ReturnCode_t llReceiveFilterAdd(const ZW_ReceiveFilter_t * pReceiveFilter);

/**@brief Function for removing a receive filter.
 *
 * @details When the function returns the matching filter has been removed from list of active
 *         filters.
 *
 * @param[in] pReceiveFilter Pointer  to the filter to be removed
 *
 * @retval SUCCESS            The filter was successfully add to the list of active filters
 * @retval INVALID_PARAMETERS No filter matching pReceiveFilter could be found in the list.
 */
ZW_ReturnCode_t llReceiveFilterRemove(const ZW_ReceiveFilter_t * pReceiveFilter);

/**@brief Function for pausing/enabling all exiting filters.
 *
 * @details When the function returns all exiting filters are paused/enabled
 *
 * @param[in] pause uint8_t instructing if existing filters is to be paused (0) or enabled (1).
 *
 * @retval SUCCESS            The existing filter successfully paused/enabled according to parameter
 * @retval UNSUPPORTED        No filters exists to pause/enable
 */
ZW_ReturnCode_t llReceiveFilterPause(uint8_t pause);

/**@brief Function for converting from transmit profile to channel number
 *
 * @param profile CommunicationProfile_t for the profile to convert to channel
 * @return The PHY channel nbr/ID to use for the specified communication profile.
 */
uint8_t
llConvertTransmitProfileToPHYChannel(CommunicationProfile_t profile);

/**@brief Function for setting NetworkId in lower levels
 *
 * @param[in] homeId Network HomeId to set
 * @param[in] nodeId Network NodeId to set
 * @param[in] nodeMode Node run mode - listening, none-listening or FLiRS node
 */
void llSetNetworkId(uint32_t homeId, node_id_t nodeId, zpal_radio_mode_t nodeMode);

/**@brief Function for determining if current Frame Header Format is 3 channel
 *
 * @retval 1      The current Frame Header Format is 3 channel
 * @retval 0      The current Frame Header Format is NOT 3 channel - eg. it must be 2 channel
 */
uint32_t llIsHeaderFormat3ch(void);

/**@brief Function To read the current RF configuration LBT status
 *
 * @retval  true if LBT is enabled else false
 */
uint32_t llIsLBTEnabled(void);

/**@brief Function to read the time length of current RF configuration Wake up beam fragments. It returns
 *  the time spent beaming plus the time spent in listen before talk.
 *
 * @retval  time in ms
 */
uint16_t llGetWakeUpBeamFragmentTime(void);

/**@brief Function for changing the Long Range channel configuration used by the node
 *
 * @param[in] eLrChCfg Target long range channel configuration we want to setup
 *
 * @retval true       if long range is setup successfully or if requested channel configuration is already set
 * @retval false      if llChangeRfPHYToLrChannelConfig fails
 */
bool llChangeRfPHYToLrChannelConfig(zpal_radio_lr_channel_config_t eLrChCfg);

/**
 * @}
 */

#endif // _ZW_DATALINKLAYER_H_
