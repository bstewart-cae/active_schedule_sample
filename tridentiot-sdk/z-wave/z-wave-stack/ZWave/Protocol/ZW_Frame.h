// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file
 * @copyright 2022 Silicon Laboratories Inc.
 *
 * @brief This header contains type definitions for Z-Wave frames.
 */
#ifndef ZWAVE_PROTOCOL_ZW_FRAME_H_
#define ZWAVE_PROTOCOL_ZW_FRAME_H_

#include <ZW_protocol.h>

#define HOME_ID_LENGTH    4 /* Length of Home ID in bytes. */

#define TRANSMISSION_FRAME_PAYLOAD_SIZE 8

/*
 * @brief Communication profiles to used when transmitting Z-Wave frames.
 * This enum is used as an index for the
 */
typedef enum
{
  PROFILE_UNSUPPORTED = 0x00,               /**< Unsupported profile. */
  /*
   * 2CH transmission functions!
   */
  RF_PROFILE_9_6K,                          /**< Communication profile supporting 9600 baud rate. */
  RF_PROFILE_40K,                           /**< Communication profile supporting 40k baud rate. */
  RF_PROFILE_40K_WAKEUP_250,                /**< Communication profile supporting 40k baud rate with transmission of wakeup beam for 250 ms prior to frame transmission. */
  RF_PROFILE_40K_WAKEUP_1000,               /**< Communication profile supporting 40k baud rate with transmission of wakeup beam for 1000 ms prior to frame transmission. */
  RF_PROFILE_100K,                          /**< Communication profile supporting 100k baud rate. */
  /*
   * LR transmission functions!
   */
  RF_PROFILE_100K_LR_A,                     /**< Communication profile supporting 100k baud rate on long range channel A. */
  RF_PROFILE_100K_LR_B,                     /**< Communication profile supporting 100k baud rate on long range channel B. */
  RF_PROFILE_100K_LR_WAKEUP_FRAGMENTED_A,   /**< Communication profile supporting 100k baud rate on LR. A fragmented wakeup beam is sent prior to frame transmission on LR channel A. */
  RF_PROFILE_100K_LR_WAKEUP_FRAGMENTED_B,   /**< Communication profile supporting 100k baud rate on LR. A fragmented wakeup beam is sent prior to frame transmission on LR channel B. */
  /**
   * 3CH transmission functions!
   *
   * The section below follows a different methodology in order not to create a separate static
   * transmit function for all the different channels, but only use one for RF_PROFILE_3CH_100K and
   * RF_PROFILE_3CH_100K_WAKEUP_FRAGMENTED.
   *
   * Only the relative positions between the RF_PROFILE_3CH_100K_CH_x and RF_PROFILE_3CH_100K_WAKEUP_FRAGMENTED_x
   * are important!
   */
  RF_PROFILE_3CH_100K,                      /**< Communication profile supporting 100k baud rate on 3 channel. Listen before talk will determine if frame can be send. Channel is selected base on last successful tx channel. */
  RF_PROFILE_3CH_100K_WAKEUP_FRAGMENTED,    /**< Communication profile supporting 100k baud rate on 3 channel. A fragmented wakeup beam is sent prior to frame transmission. */
  /*
   * Not to be used directly for frame transmission!!
   * Can only be used to pass back to link layer for transmitting ACK frames only where the communication profile of
   * the received frame is returned. This is do that the ACK is transmitted on the channel that the incoming frame was received on!
   */
  RF_PROFILE_3CH_100K_CH_A,                 /**< Communication profile supporting 100k baud rate on channel A. This communication profile is to be used only when replying to a immediately received frame., e.g. when transmitting an ACK for an incoming frame */
  RF_PROFILE_3CH_100K_CH_B,                 /**< Communication profile supporting 100k baud rate on channel B. This communication profile is to be used only when replying to a immediately received frame., e.g. when transmitting an ACK for an incoming frame */
  RF_PROFILE_3CH_100K_CH_C,                 /**< Communication profile supporting 100k baud rate on channel C. This communication profile is to be used only when replying to a immediately received frame., e.g. when transmitting an ACK for an incoming frame */
  RF_PROFILE_3CH_100K_WAKEUP_FRAGMENTED_A,  /**< Communication profile supporting 100k baud rate on 3 channel. A fragmented wakeup beam is sent prior to frame transmission. */
  RF_PROFILE_3CH_100K_WAKEUP_FRAGMENTED_B,  /**< Communication profile supporting 100k baud rate on 3 channel. A fragmented wakeup beam is sent prior to frame transmission. */
  RF_PROFILE_3CH_100K_WAKEUP_FRAGMENTED_C,  /**< Communication profile supporting 100k baud rate on 3 channel. A fragmented wakeup beam is sent prior to frame transmission. */
} CommunicationProfile_t;


/*
 * @Brief Z-Wave Frame header format types
 */
typedef enum
{
  HDRFORMATTYP_2CH,         //< 2 Channel header format
  HDRFORMATTYP_3CH,         //< 3 Channel header format
  HDRFORMATTYP_LR,          //< LR Channel header format
  HDRFORMATTYP_UNDEFINED    //< Undefined Channel header format
} ZW_HeaderFormatType_t;


/*
 * @brief Z-Wave Frame header types
 */
typedef enum
{
  HDRTYP_SINGLECAST     = 1, //< Single cast. Frame should be transmitted as single cast
  HDRTYP_MULTICAST      = 2, //< Multi cast. Frame should be transmitted using multicast
  HDRTYP_TRANSFERACK    = 3, //< Acknowledge frame
  HDRTYP_FLOODED        = 4, //< Flooded
  HDRTYP_EXPLORE        = 5, //< Explorer frame. Transmit this frame using explorer
  HDRTYPE_AVFRAME       = 6, //< Deprecated. Value is reserved. Do Not Use
  HDRTYPE_AVACKNOWLEDGE = 7, //< Deprecated. Value is reserved. Do Not Use
  HDRTYP_ROUTED         = 8, //< Routed frame. Frame should be transmitted with routing. @note When used on a 2 channel system, the header on the air will be of type single cast.
  HDRTYP_RAW            = 9  //< Deprecated. Raw frame. The frame should be transmitted without building a frame header.
} ZW_FrameType_t;

/*
 * @brief Basic Z-Wave options specifying how the corresponding frame was received / transmitted.
 */
typedef struct
{
    uint8_t homeId[HOME_ID_LENGTH]; //< Home ID used in frame. */
    node_id_t sourceNodeId;         //< Source Node Id used in frame. */
    node_id_t destinationNodeId;    //< Destination Node Id used in frame. */
    uint8_t speedModified :1;       //< Frame is/shall be transmitted at lower speed than supported by both source and destination node. */
    uint8_t lowPower:1;             //< Frame is/shall be transmitted at low power. */
    uint8_t multicastfollowup:1;    //< Frame is a multicast followup frame. */
    uint8_t acknowledge :1;         //< An ACK Frame is to be transmitted in response to this frame. */
    uint8_t routed:1;               //< This is a routed frame. Note: Only valid for 2 channel frames. */ // ToDo: Investigate if this field can perish and be automatically determined on 2 channel, using ZW_FrameType_t HDRTYP_ROUTED.
    uint8_t sequenceNumber;         //< Sequence number of frame. Note Only 4 bits are used on 2 channel frames.
    //    uint8_t reserved:1;       // Should be opaque ?
    uint8_t wakeup250ms:1;          //< Source node defined in this frame requires a 250 ms wakeup before receiving a frame. */
    uint8_t wakeup1000ms:1;         //< Source node defined in this frame requires a 1000 ms wakeup before receiving a frame. */
    uint8_t extended:1;             //< When this field is 1 then the header is extended with one byte in 3 channel frame*/
    int8_t noiseFloor;              //< Noise Floor value (LR frames only) */
    int8_t txPower;                 //< Tx Power value (LR frames only) */
    ZW_FrameType_t frameType;       //< Z-Wave Frame type. */
} ZW_BasicFrameOptions_t;


/*
 * @brief Z-Wave transmission frame.
 */
typedef struct
{
    CommunicationProfile_t profile;       /**< Communication profile indicating the speed on which this frame should be transmitted. */
    ZW_BasicFrameOptions_t frameOptions;  /**< Basic frame options when transmitting the frame. The options will be part of the frame header on the air. */
    uint8_t   status;                     /**< LEGACY: Status field kept for backward compatibility. Used by transport.c and others */
    uint8_t   rssi;                       /**< LEGACY: Rssi sample. Used by transport.c and others */
    uint8_t   useLBT;                     /**< Used to select transmission with or without LBT */
    int8_t    txPower;                    /**< Used to specify the wanted Tx power */
    uint8_t   headerLength;               /**< Frame header length */
    frameTx   header;                     /**< Frame header contents */
    uint8_t   payloadLength;              /**< Length of payload following this frame. */
    uint8_t   payload[TRANSMISSION_FRAME_PAYLOAD_SIZE];  /**< Payload content. Payload is set to 8 bytes to avoid warnings. Ensure to initialize the frame object with more space for the payload. */
} ZW_TransmissionFrame_t;


/*
 * @brief Z-Wave receive frame.
 */
typedef struct
{
    CommunicationProfile_t profile;            /**< Communication profile indicating the speed/channel on which this frame was received. */
    ZW_BasicFrameOptions_t frameOptions;       /**< Basic frame options on the received frame. */
    uint8_t   channelId;                       /**< Channel Id indicating on which channel frame was received on */
    ZW_HeaderFormatType_t channelHeaderFormat; /**< Z-Wave Header format used in channel frame was received on */
    uint8_t   status;                          /**< LEGACY: Status field kept for backward compatibility. Used by transport.c and others */
    int8_t    rssi;                            /**< Rssi with which frame has been received with */
    uint8_t * pPayloadStart;                   /**< Pointer to where payload starts seen by current layer. Each protocol layer must update this pointer so higher layer can parse destined content correctly. */
    uint8_t   frameContentLength;              /**< Length of payload following this frame. */
    uint8_t*  frameContent;                    /**< Pointer to payload buffer with complete frame data received. NOTE: Use \ref pPayloadStart to correctly index location of header and payload content for data handling. */
} ZW_ReceiveFrame_t;


__attribute__((used)) bool IsSingleCast(ZW_HeaderFormatType_t headerType, const frame *pFrame);
__attribute__((used)) bool IsMultiCast(ZW_HeaderFormatType_t headerType, const frame *pFrame);
__attribute__((used)) bool IsExplore(ZW_HeaderFormatType_t headerType, const frame *pFrame);
__attribute__((used)) bool IsTransferAck(ZW_HeaderFormatType_t headerType, const frame *pFrame);
__attribute__((used)) bool DoAck(ZW_HeaderFormatType_t headerType, const frame *pFrame);
__attribute__((used)) bool IsAck(ZW_HeaderFormatType_t headerType, const frame *pFrame);
__attribute__((used)) bool IsLowPower(ZW_HeaderFormatType_t headerType, const frame *pFrame);
__attribute__((used)) bool IsRouted(ZW_HeaderFormatType_t headerType, const frame *pFrame);
__attribute__((used)) bool IsRouteErr(ZW_HeaderFormatType_t headerType, const frame *pFrame);
__attribute__((used)) bool IsRouteAckErr(ZW_HeaderFormatType_t headerType, const frame *pFrame);
__attribute__((used)) uint8_t GetHeaderType(ZW_HeaderFormatType_t headerType, const frame *pFrame);
__attribute__((used)) void SetHeaderType(ZW_HeaderFormatType_t headerType, frameTx *pFrame, uint8_t type);
__attribute__((used)) void SetAck(ZW_HeaderFormatType_t headerType, frameTx *pFrame);
__attribute__((used)) void SetInComming(ZW_HeaderFormatType_t headerType, frameTx *pFrame);
__attribute__((used)) void SetOutGoing(ZW_HeaderFormatType_t headerType, frameTx *pFrame);
__attribute__((used)) void SetRouteAck(ZW_HeaderFormatType_t headerType, frameTx *pFrame);
__attribute__((used)) void SetRouteErr(ZW_HeaderFormatType_t headerType, frameTx *pFrame);
__attribute__((used)) uint8_t GetRouteLen(ZW_HeaderFormatType_t headerFormatType, const frame *pFrame);
__attribute__((used)) uint8_t GetHops(ZW_HeaderFormatType_t headerType, const frame *pFrame);
__attribute__((used)) bool IsOutgoing(ZW_HeaderFormatType_t headerType, const frame *pFrame);
__attribute__((used)) bool NotOutgoing(ZW_HeaderFormatType_t headerType, const frame *pFrame);
__attribute__((used)) void SetRouteLen(ZW_HeaderFormatType_t headerType, frameTx *pFrame, uint8_t len);
__attribute__((used)) void SetHops(ZW_HeaderFormatType_t headerType, frameTx *pFrame, uint8_t hops);
__attribute__((used)) bool IsRoutedAck(ZW_HeaderFormatType_t headerType, const frame *pFrame);
__attribute__((used)) bool IsFrameSingleCast(ZW_HeaderFormatType_t headerType, const frame *pFrame);
__attribute__((used)) bool IsFrameRouted(ZW_HeaderFormatType_t headerType, const frame *pFrame);
__attribute__((used)) bool IsFrameAck(ZW_HeaderFormatType_t headerType, const frame *pFrame);
__attribute__((used)) uint32_t GetSeqNumber(ZW_HeaderFormatType_t headerType, const frame *pFrame);
__attribute__((used)) void SetRouted(ZW_HeaderFormatType_t headerType, frameTx *pFrame);
__attribute__((used)) void ClrRouted(ZW_HeaderFormatType_t headerType, frameTx *pFrame);
__attribute__((used)) uint8_t ReadRepeater(ZW_HeaderFormatType_t headerType, const frame *pFrame, uint8_t number);
__attribute__((used)) void SetRepeater(ZW_HeaderFormatType_t headerType, frameTx *pFrame, uint8_t number, uint8_t id);
__attribute__((used)) uint8_t * GetSinglecastPayload(ZW_HeaderFormatType_t headerType, frame *pFrame);
__attribute__((used)) uint8_t GetMulticastOffset(ZW_HeaderFormatType_t headerType, const frame *pFrame);
__attribute__((used)) uint8_t GetMulticastAddrLen(ZW_HeaderFormatType_t headerType, const frame *pFrame);
__attribute__((used)) uint8_t * GetReceiveMask(ZW_HeaderFormatType_t headerType, frame *pFrame);
__attribute__((used)) void SetWakeupBeamSpeedSrc(ZW_HeaderFormatType_t headerType, frameTx *pFrame, uint8_t wakeupspeed);
__attribute__((used)) void ClrWakeupBeamSpeedSrc(ZW_HeaderFormatType_t headerType, frameTx *pFrame);
__attribute__((used)) uint8_t GetWakeupBeamSpeedSrc(ZW_HeaderFormatType_t headerType, const frame *pFrame);
__attribute__((used)) void SetExtendPresent(ZW_HeaderFormatType_t headerType, frameTx *pFrame);
__attribute__((used)) void ClrExtendPresent(ZW_HeaderFormatType_t headerType, frame *pFrame);
__attribute__((used)) uint8_t GetExtendPresent(ZW_HeaderFormatType_t headerType, const frame *pFrame);
__attribute__((used)) void SetExtendTypeHeader(ZW_HeaderFormatType_t headerType, frameTx *pFrame, uint8_t extendType);
__attribute__((used)) uint8_t GetExtendTypeHeader(ZW_HeaderFormatType_t headerType, const frameTx *pFrame);
__attribute__((used)) void SetExtendLenHeader(ZW_HeaderFormatType_t headerType, frameTx *pFrame, uint8_t len);
__attribute__((used)) uint8_t GetExtendLenHeader(ZW_HeaderFormatType_t headerType, const frameTx *pFrame);
__attribute__((used)) void SetExtendBodyHeader(ZW_HeaderFormatType_t headerType, frameTx *pFrame,uint8_t offset,  uint8_t val);
__attribute__((used)) uint8_t GetExtendBodyHeader(ZW_HeaderFormatType_t headerType, const frameTx *pFrame, uint8_t offset);
__attribute__((used)) void SetExtendType(ZW_HeaderFormatType_t headerType, frameTx *pFrame, uint8_t extendType);
__attribute__((used)) uint8_t GetExtendType(ZW_HeaderFormatType_t headerType, frame *pFrame);
__attribute__((used)) uint8_t GetExtendType2(ZW_HeaderFormatType_t headerType, const frameTx *pFrame);
__attribute__((used)) void SetExtendLength(ZW_HeaderFormatType_t headerType, frameTx *pFrame, uint8_t extendLength);
__attribute__((used)) void SetExtendBody(ZW_HeaderFormatType_t headerType, frameTx *pFrame, uint8_t offset, uint8_t value);
__attribute__((used)) uint8_t GetExtendBody(ZW_HeaderFormatType_t headerType, const frame *pFrame, uint8_t offset);
__attribute__((used)) uint8_t GetRoutedHeaderExtensionLen(ZW_HeaderFormatType_t headerType,  const frame *pFrame);
__attribute__((used)) uint8_t GetRoutedAckHeaderExtensionLen(ZW_HeaderFormatType_t headerType,  const frame *pFrame);
__attribute__((used)) uint8_t * GetExtendBodyAddr(ZW_HeaderFormatType_t headerType, frame *pFrame, uint8_t offset);
__attribute__((used)) uint8_t * GetRoutedPayload(ZW_HeaderFormatType_t headerType, frame *pFrame);
__attribute__((used)) uint8_t GetRoutedPayloadLen(ZW_HeaderFormatType_t headerType, const frame *pFrame);
__attribute__((used)) uint8_t * GetExplorePayload(ZW_HeaderFormatType_t headerType, frame *pFrame);
__attribute__((used)) uint8_t GetExplorePayloadLen(ZW_HeaderFormatType_t headerType, const frame *pFrame);
__attribute__((used)) uint8_t * GetMulticastPayload(ZW_HeaderFormatType_t headerType, frame *pFrame);
__attribute__((used)) uint8_t GetMulticastPayloadLen(ZW_HeaderFormatType_t headerType, const frame *pFrame);
__attribute__((used)) uint8_t * GetRoutedAckPayload(ZW_HeaderFormatType_t headerType, frame *pFrame);
__attribute__((used)) uint8_t GetRoutedAckPayloadLen(ZW_HeaderFormatType_t headerType, const frame *pFrame);
__attribute__((used)) node_id_t GetSourceNodeID(ZW_HeaderFormatType_t headerType, const frame *pFrame);
__attribute__((used)) void SetSourceNodeIDSinglecast(ZW_HeaderFormatType_t headerType, frame *pFrame, node_id_t nodeID);
__attribute__((used)) node_id_t GetDestinationNodeIDSinglecast(ZW_HeaderFormatType_t headerType, const frame *pFrame);
__attribute__((used)) void SetDestinationNodeIDSinglecast(ZW_HeaderFormatType_t headerType, frame *pFrame, node_id_t nodeID);

#endif /* ZWAVE_PROTOCOL_ZW_FRAME_H_ */
