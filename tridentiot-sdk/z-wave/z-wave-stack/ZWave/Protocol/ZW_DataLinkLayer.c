// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/*
 * @file
 * Z-Wave Data Link Layer module
 *
 * @copyright 2020 Silicon Laboratories Inc.
 */

#include <stdint.h>
#include <string.h>
#include <ZW_protocol_def.h>
#include <Assert.h>
#include <SizeOf.h>
#include <ZW_DataLinkLayer_utils.h>
#include "ZW_Frame.h"
#include <ZW_home_id_hash.h>
#include <zpal_radio_utils.h>
#include <zpal_radio.h>

//#define DO_CHECKSUM_CHECK

//#define DEBUGPRINT
#include <DebugPrint.h>

#include "ZW_basis.h"

// Use Retention RAM for storing the TX Power for Slave devices

#define DATA_RATE_1_INDEX 0 /**< Index in active communication profile for data rate 1 */
#define DATA_RATE_2_INDEX 1 /**< Index in active communication profile for data rate 2 */
#define DATA_RATE_3_INDEX 2 /**< Index in active communication profile for data rate 3 */
#define DATA_RATE_4_INDEX 3 /**< Index in active communication profile for data rate 4 */
#define DATA_RATE_5_INDEX 4 /**< Index in active communication profile for data rate 5 */

#define CHANNEL_0_ID    0x00 /**< Channel ID 0 definition used for radio phy configuration */
#define CHANNEL_1_ID    0x01 /**< Channel ID 1 definition used for radio phy configuration */
#define CHANNEL_2_ID    0x02 /**< Channel ID 2 definition used for radio phy configuration */
#define CHANNEL_3_ID    0x03 /**< Channel ID 3 definition used for radio phy configuration */
#define CHANNEL_4_ID    0x04 /**< Channel ID 4 definition used for radio phy configuration */

#define HEADER_CONSTRUCTOR          3     /**< Value used when using communication profile to header construction index. 2 Channel communication profiles has range 0-5. 3 Channel communication profiles has value >=0x08. So bit shift 3 decides if header construction should match 2 or 3 channel in \ref HeaderConstructorFunc. */

/**************************************************
 * FRAME BYTE INDENTIFIERS
 *************************************************/
#define BEAM_FRAME_BEAM_TAG         0x55  /**< This is the BEAM TAG allowing awake nodes to determine that this is a BEAM frame being transmitted. It is the same as the preamble! */

#define START_OF_FRAME_CH2CH3       0xF0  /**< The start of frame byte for 2CH and 3CH */
#define START_OF_FRAME_LR           0x5E  /**< The start of frame byte for Long-Range */

#define PREAMBLE_BYTE_CH2CH3        0x55  /**< The preamble byte used to create the preamble sequence for 2CH and 3CH */
#define PREAMBLE_BYTE_LR            0x00  /**< The preamble byte for LR (Due to spread-spectrum, 0x00 is not what is being transmitted!) */

/**************************************************
 * Continuous-BEAM construction parameters on 2CH!
 *************************************************/
#define TX_BEAM_2CH_INFO_LENGTH     3                                /**< Number of bytes in Wakeup info part, i.e. BEAM_FRAME_BEAM_TAG, Node Id, Home Id Hash */
#define TX_BEAM_2CH_SOF_LENGTH      1                                /**< Number of bytes in Start Of Frame */
#define TX_BEAM_2CH_PREAMBLE_LEN    20                               /**< Number of bytes in a preamble in a wakeup beam @40k baud rate */
#define TX_BEAM_2CH_LENGTH_FRAME    (TX_BEAM_2CH_PREAMBLE_LEN +   \
                                     TX_BEAM_2CH_SOF_LENGTH +     \
                                     TX_BEAM_2CH_INFO_LENGTH)        /**< Total number to transmit before the frame when adding start of frame, beam tag (BEAM_FRAME_BEAM_TAG), NodeID, HomeID hash, reference ITU T-REC-G.9959, page 47 */

#define TX_BEAM_2CH_DURATION_1000   1100                                                 /**< Beam time in ms, plus 10% to ensure a FLiRS registers the beam */
#define TX_BEAM_2CH_LENGTH_1000     ((40000/8)*TX_BEAM_2CH_DURATION_1000)/1000           /**< Number of bytes in a single wakeup beam */
#define TX_BEAM_2CH_REPEAT_1000     1+(TX_BEAM_2CH_LENGTH_1000/TX_BEAM_2CH_LENGTH_FRAME) /**< Number of beam repetitions required for waking up a 1000 ms FLiRS node */

#define TX_BEAM_2CH_TIME_250        275                                                    /**< Beam time in ms, plus 10% to ensure a FLiRS registers the beam */
#define TX_BEAM_2CH_LENGTH_250      (((40000/8)*TX_BEAM_2CH_TIME_250)/1000)                /**< Number of bytes in a single wakeup beam */
#define TX_BEAM_2CH_REPEAT_250      (1+(TX_BEAM_2CH_LENGTH_250/TX_BEAM_2CH_LENGTH_FRAME))  /**< Number of beam repetitions required for waking up a 250 ms FLiRS node */

/**************************************************
 * Continuous-BEAM construction parameters on 3CH!
 *************************************************/
#define TX_BEAM_3CH_INFO_LENGTH     3                                /**< Number of bytes in Wakeup info part, i.e. BEAM_FRAME_BEAM_TAG, Node Id, Home Id Hash */
#define TX_BEAM_3CH_SOF_LENGTH      1                                /**< Number of bytes in Start Of Frame */
#define TX_BEAM_3CH_PREAMBLE_LEN    8                                /**< Number of bytes in a preamble in a wakeup beam @40k baud rate */
#define TX_BEAM_3CH_LENGTH_FRAME    (TX_BEAM_3CH_PREAMBLE_LEN +   \
                                     TX_BEAM_3CH_SOF_LENGTH +     \
                                     TX_BEAM_3CH_INFO_LENGTH)        /**< Total number to transmit before the frame when adding start of frame, beam tag (BEAM_FRAME_BEAM_TAG), NodeID, HomeID hash, reference ITU T-REC-G.9959, page 47 */
// Calculate fragment size
#define TX_BEAM_3CH_FRAG_DURATION_MS    100      /* 100 ms - required by JP regulation */           /**< Beam time in ms, not with additional 10% that might be needed to ensure a FLiRS registers the beam */
#define TX_BEAM_3CH_FRAG_LENGTH_BYTE    (((100000/8)*TX_BEAM_3CH_FRAG_DURATION_MS)/1000)            /**< Number of bytes in a single wakeup beam */
#define TX_BEAM_3CH_FRAG_REPEAT_COUNT   (1+(TX_BEAM_3CH_FRAG_LENGTH_BYTE/TX_BEAM_3CH_LENGTH_FRAME)) /**< Number of beam repetitions required for waking up a 1000 ms FLiRS node */

/**************************************************
 * Continuous-BEAM construction parameters on LR!
 *************************************************/
#define TX_BEAM_LR_INFO_LENGTH      4                                /**< Number of bytes in Wakeup info part, i.e. BEAM_FRAME_BEAM_TAG, Node Id, Home Id Hash */
#define TX_BEAM_LR_SOF_LENGTH       1                                /**< Number of bytes in Start Of Frame */
#define TX_BEAM_LR_PREAMBLE_LEN     8                                /**< Number of bytes in a preamble in a wakeup beam @40k baud rate */
#define TX_BEAM_LR_LENGTH_FRAME     (TX_BEAM_LR_PREAMBLE_LEN +   \
                                     TX_BEAM_LR_SOF_LENGTH +     \
                                     TX_BEAM_LR_INFO_LENGTH)         /**< Total number to transmit before the frame when adding start of frame, beam tag (BEAM_FRAME_BEAM_TAG), NodeID, HomeID hash, reference ITU T-REC-G.9959, page 47 */
// Calculate fragment size
#define TX_BEAM_LR_FRAG_DURATION_MS     114                                                       /**< Beam time in ms, plus 14% to ensure a FLiRS registers the beam */
#define TX_BEAM_LR_FRAG_LENGTH_BYTE     (((100000/8)*TX_BEAM_LR_FRAG_DURATION_MS)/1000)           /**< Number of bytes in a single wakeup beam */
#define TX_BEAM_LR_FRAG_REPEAT_COUNT    (1+(TX_BEAM_LR_FRAG_LENGTH_BYTE/TX_BEAM_LR_LENGTH_FRAME)) /**< Number of beam repetitions required for waking up a 1000 ms FLiRS node */
// TX_BEAM_LR_FRAG_REPEAT_COUNT might need to be experimentally adjusted! (see above)

////////////////////////////////////////////////

/*********************************************************************
 * Field related definitions of the MPDU header format!
 ********************************************************************/

#define FRAME_HOME_ID_INDEX                         0  /**< Index location in frame header array where home id field begins. */
#define FRAME_CONTROL_INDEX                         5  /**< Index location in frame header array where control index field begins. */

#define FRAME_LENGTH_INDEX                          7  /**< Index location in frame array where length field is located. */

#define GENERAL_HEADER_2CH_INDEX_OFFSET            -9  /**< Negative offset to use for 2 channel header construction */
#define MULTICAST_HEADER_2CH_INDEX_OFFSET          -8  /**< Negative offset to use for 2 channel Multicast header construction */

#define GENERAL_HEADER_2CH_LENGTH                   9  /**< Length of basic 2 channel header */
#define MULTICAST_HEADER_2CH_LENGTH                 8  /**< Length of Multicast 2 channel header */
#define GENERAL_HEADER_2CH_HOME_ID_INDEX            0  /**< Index location in 2 channel frame header array where home id field begins */
#define GENERAL_HEADER_2CH_SOURCE_INDEX             4  /**< Index location in 2 channel frame header array where source node id field is located */
#define GENERAL_HEADER_2CH_FRAME_CONTROL_INDEX      5  /**< Index location in 2 channel frame header array where control field begins */
#define GENERAL_HEADER_2CH_LENGTH_INDEX             7  /**< Index location in 2 channel frame header array where length field begins */
#define GENERAL_HEADER_2CH_DESTINATION_INDEX        8  /**< Index location in 2 channel frame header array where destination node id field is located */

#define GENERAL_ROUTED_HEADER_2CH_ROUTESTATUS_INDEX 9  /**< Index location in 2 channel frame header array where routed header routeStatus field begins */
#define GENERAL_ROUTED_HEADER_2CH_CONTROL_INDEX     10 /**< Index location in 2 channel frame header array where routed header numRepsNumHops field begins */
#define GENERAL_ROUTED_HEADER_2CH_REPEATERLIST_INDEX 11 /**< Index location in 2 channel frame header array where routed header repeaterList fields begins */

#define SPEED_MODIFIED_2CH_FRAME_CONTROL_POS        4  /**< The position of the speed modified bit in the 5th header byte of the 2 channel frame header array*/
#define LOW_POWER_2CH_FRAME_CONTROL_POS             5  /**< The position of the low power bit in the 5th header byte of the 2 channel frame header array*/
#define REQ_ACK_2CH_FRAME_CONTROL_POS               6  /**< The position of the ack Bits in the 5th header byte of the 2 channel frame header array*/
#define ROUTED_2CH_FRAME_CONTROL_POS                7  /**< The position of the routed frame bit in the 5th header byte of the 2 channel frame header array*/
#define SEQ_NO_2CH_FRAME_CONTROL_POS                0  /**< The position of the sequence number Bits in the 6th header byte of the 2 channel frame header array*/
#define SOURCE_WAKEUP_250MS_2CH_POS                 5  /**< The position of the 250 source wakeup bit in the 6th header byte of the 2 channel frame header array*/
#define SOURCE_WAKEUP_1000MS_2CH_POS                6  /**< The position of the 1000 source wakeup bit in the 6th header byte of the 2 channel frame header array*/
#define MULTICAST_FOLLOWUP_2CH_FRAME_CONTROL_POS    7  /**< The position of the multicast followup bit in the 6th header byte of the 2 channel frame header array*/
#define GENERAL_HEADER_3CH_INDEX_OFFSET           -10 /**< Negative offset to use for 3 channel header construction */
#define MULTICAST_HEADER_3CH_INDEX_OFFSET          -9  /**< Negative offset to use for 3 channel Multicast header construction */

#define GENERAL_HEADER_3CH_LENGTH                  10 /**< Length of basic 3 channel header */
#define MULTICAST_HEADER_3CH_LENGTH                 9  /**< Length of Multicast 3 channel header */
#define GENERAL_HEADER_3CH_HOME_ID_INDEX            0  /**< Index location in 3 channel frame header array where home id field begins */
#define GENERAL_HEADER_3CH_SOURCE_INDEX             4  /**< Index location in 3 channel frame header array where source node id field is located */
#define GENERAL_HEADER_3CH_FRAME_CONTROL_INDEX      5  /**< Index location in 3 channel frame header array where control field begins */
#define GENERAL_HEADER_3CH_EXTENDED_INDEX           6  /**< Index location in 3 channel frame header array where extended field is located */
#define GENERAL_HEADER_3CH_LENGTH_INDEX             7  /**< Index location in 3 channel frame header array where length field begins */
#define GENERAL_HEADER_3CH_SEQUENCE_NO_INDEX        8  /**< Index location in 3 channel frame header array where sequence number field is located */
#define GENERAL_HEADER_3CH_DESTINATION_INDEX        9  /**< Index location in 3 channel frame header array where destination node id field is located */

#define GENERAL_ROUTED_HEADER_3CH_ROUTESTATUS_INDEX 10  /**< Index location in 3 channel frame header array where routed header routeStatus field begins */
#define GENERAL_ROUTED_HEADER_3CH_CONTROL_INDEX     11 /**< Index location in 3 channel frame header array where routed header numRepsNumHops field begins */
#define GENERAL_ROUTED_HEADER_3CH_REPEATERLIST_INDEX 12 /**< Index location in 3 channel frame header array where routed header repeaterList fields begins */

#define MULTICAST_FOLLOWUP_3CH_FRAME_CONTROL_POS    5    /**< The position of the multicast followup bit in the 5th byte off the 3 channel frame header array*/
#define LOW_POWER_3CH_FRAME_CONTROL_POS             6    /**< The position of the low power bit in the 5th byte off the 3 channel frame header array*/
#define REQ_ACK_3CH_FRAME_CONTROL_POS               7    /**< The position of the ack bit in the 5th byte off the 3 channel frame header array*/
#define SOURCE_WAKEUP_250MS_3CH_POS                 4    /**< The position of the 250ms source wakeup beam bit in the 6th byte off the 3 channel frame header array*/
#define SOURCE_WAKEUP_1000MS_3CH_POS                5    /**< The position of the 1000ms source wakeup beam bit in the 6th byte off the 3 channel frame header array*/
#define EXTENDED_3CH_FRAME_EXTENDEDHEADER_POS       7    /**< The position of the extended bit in the 6th byte off the 3 channel frame header array*/

// 2CH & 3CH BEAM FRAME indexes
#define WAKEUP_BEAM_2CH3CH_NODE_ID_INDEX            1  /**< Index location in wakeup beam frame array where node id field of node to be awaken is located */
#define WAKEUP_BEAM_2CH3CH_HOME_ID_HASH_INDEX       2  /**< Index location in wakeup beam frame array where home id hash field is located */

// LR BEAM FRAME indexes
#define WAKEUP_BEAM_LR_TX_PWR_MSN_NODEID_IDX        1     /**< Index location in the wakeup beam frame array for the TX-Power field (nibble) and the MSN of the 12bit Node-ID. */
#define WAKEUP_BEAM_LR_NODE_ID_IDX                  2     /**< Index location in the wakeup beam frame array for the MSB of the 12 Node-ID of the node to be awaken is located. Also @see WAKEUP_BEAM_LR_TX_PWR_MSN_NODEID_IDX. */
#define WAKEUP_BEAM_LR_HOME_ID_HASH_IDX             3     /**< Index location in wakeup beam frame array where home id hash field is located */
#define WAKEUP_BEAM_LR_NODE_ID_MSN_mask             (0x0F00)  /**< A mask to strip the LSB and MSN (Most Significant Nibble) of the 12bit NodeID! */
#define BEAM_FRAME_TX_PWR_POS                       4     /**< The TX Power occupies the Most Significant Nibble of the second byte in the MPDU Header! See "Z-Wave Long Range PHY/MAC layer specification" */


#define REQ_ACK_LR_FRAME_POS                        7    /**< The position of the ack bit in the 9th byte off the LR frame header array*/
#define EXTENDED_LR_FRAME_POS                       6    /**< The position of the extended bit in the 9th byte off the LR frame header array*/
#define HEADERTYPE_LR_FRAME_POS_OFFSET              0    /**< The offset of the header type field in the 9th byte off the LR frame header array*/

/* Frame header mask bits for the 5' byte in the header */
#define MASK_HDRTYP               0x0f /**< Mask for extracting Z-Wave header type data */

/* Frame header mask bits for the 6' byte in the 2 channel header */
#define MASK_SEQNO_2CH            0x0f /**< Mask for extracting 4 bit sequence number, \note 2 channel only */

#define PACKET_FILTER_SIZE        5    /**< Number of filters supported by each packet type, SingleCast, Explorer, Routed. */

/**
 * These values are used by the convertTXPowerToIndex() function!
 * These values define the boundary of values that can be converted.
 * If these values are exceeded below or above MIN and MAX, the value
 * to be converted will be nearest valid value before conversion.
 */
#define TX_PWR_TO_INDEX_CONVERSION_PWR_MIN        -6  // The minimum TX power in dBm that can be converted.
#define TX_PWR_TO_INDEX_CONVERSION_PWR_MAX        30  // The maximum TX power in dBm that can be converted.
#define TX_PWR_TO_INDEX_CONVERSION_INDEX_MAX      15  // The max index value that may be used!

/////////////////////////////////////////////////////////////////////////////////////////////////

/**@brief Function pointer declaration for transmitting a Z-Wave frame using the provided
 *        communication profile.
 */
typedef ZW_ReturnCode_t (*CommunicationProfileFunc)(CommunicationProfile_t   communicationProfile,
                                                    ZW_TransmissionFrame_t * pFrame);

/**@brief Function pointer declaration for construction of a Z-Wave header using the provided
 *        communication profile.
 */
typedef void            (*HeaderConstructorFunc)(CommunicationProfile_t   communicationProfile,
                                                 ZW_TransmissionFrame_t * pFrame);

/**@brief Function pointer declaration for handling of a received Z-Wave frame using the provided
 *        communication profile.
 */
typedef void (*FrameHandlerFunc)(CommunicationProfile_t   communicationProfile,
                                 zpal_radio_rx_parameters_t       * pRxParameters,
                                 ZW_ReceiveFrame_t      * pFrame);

void radioFrameReceiveHandler(zpal_radio_rx_parameters_t * pRxParameters, zpal_radio_receive_frame_t * pFrame);

/** Forward declarations of transmission functions */
static ZW_ReturnCode_t unsupported_transmit    (CommunicationProfile_t communicationProfile, ZW_TransmissionFrame_t *pFrame);
static ZW_ReturnCode_t transmit9_6k            (CommunicationProfile_t communicationProfile, ZW_TransmissionFrame_t *pFrame);
static ZW_ReturnCode_t transmit40k             (CommunicationProfile_t communicationProfile, ZW_TransmissionFrame_t *pFrame);
static ZW_ReturnCode_t transmit100k            (CommunicationProfile_t communicationProfile, ZW_TransmissionFrame_t *pFrame);
static ZW_ReturnCode_t transmit100kLR          (CommunicationProfile_t communicationProfile, ZW_TransmissionFrame_t *pFrame);
static ZW_ReturnCode_t transmit100kLR2         (CommunicationProfile_t communicationProfile, ZW_TransmissionFrame_t *pFrame);
static ZW_ReturnCode_t transmit_wakeup_250ms   (CommunicationProfile_t communicationProfile, ZW_TransmissionFrame_t *pFrame);
static ZW_ReturnCode_t transmit_wakeup_1000ms  (CommunicationProfile_t communicationProfile, ZW_TransmissionFrame_t *pFrame);
static ZW_ReturnCode_t transmit_wakeup_fragmented_LR   (CommunicationProfile_t communicationProfile, ZW_TransmissionFrame_t *pFrame);
static ZW_ReturnCode_t transmit_wakeup_fragmented_LR2  (CommunicationProfile_t communicationProfile, ZW_TransmissionFrame_t *pFrame);

/** Transmit function for Korea and Japan */
static ZW_ReturnCode_t transmit_3ch_100k       (CommunicationProfile_t communicationProfile, ZW_TransmissionFrame_t *pFrame);
static ZW_ReturnCode_t transmit_3ch_100k_wakeup(CommunicationProfile_t communicationProfile, ZW_TransmissionFrame_t *pFrame);

/** Forward declarations of header construction functions */
static void construct2chSinglecastHeader    (CommunicationProfile_t communicationProfile, ZW_TransmissionFrame_t *pFrame);
static void construct3chSinglecastHeader    (CommunicationProfile_t communicationProfile, ZW_TransmissionFrame_t *pFrame);
static void constructLRSinglecastHeader     (CommunicationProfile_t communicationProfile, ZW_TransmissionFrame_t *pFrame);
static void construct2chExplorerFrameHeader (CommunicationProfile_t communicationProfile, ZW_TransmissionFrame_t *pFrame);
static void construct3chExplorerFrameHeader (CommunicationProfile_t communicationProfile, ZW_TransmissionFrame_t *pFrame);
static void construct2chAckFrameHeader      (CommunicationProfile_t communicationProfile, ZW_TransmissionFrame_t *pFrame);
static void construct3chAckFrameHeader      (CommunicationProfile_t communicationProfile, ZW_TransmissionFrame_t *pFrame);
static void constructLRAckFrameHeader       (CommunicationProfile_t communicationProfile, ZW_TransmissionFrame_t *pFrame);
static void construct2chMulticastFrameHeader(CommunicationProfile_t communicationProfile, ZW_TransmissionFrame_t *pFrame);
static void construct3chMulticastFrameHeader(CommunicationProfile_t communicationProfile, ZW_TransmissionFrame_t *pFrame);
static void constructNotApplicable          (CommunicationProfile_t communicationProfile, ZW_TransmissionFrame_t *pFrame);

/** Forward declarations of handler function for received frames */
static void singlecast2chHandler      (CommunicationProfile_t communicationProfile, zpal_radio_rx_parameters_t * pRxParameters, ZW_ReceiveFrame_t * pFrame);
static void singlecast3chHandler      (CommunicationProfile_t communicationProfile, zpal_radio_rx_parameters_t * pRxParameters, ZW_ReceiveFrame_t * pFrame);
static void singlecastLRHandler       (CommunicationProfile_t communicationProfile, zpal_radio_rx_parameters_t * pRxParameters, ZW_ReceiveFrame_t * pFrame);
static void singlecastRouted2chHandler(CommunicationProfile_t communicationProfile, zpal_radio_rx_parameters_t * pRxParameters, ZW_ReceiveFrame_t * pFrame);
static void singlecastRouted3chHandler(CommunicationProfile_t communicationProfile, zpal_radio_rx_parameters_t * pRxParameters, ZW_ReceiveFrame_t * pFrame);
static void explorerFrame2chHandler   (CommunicationProfile_t communicationProfile, zpal_radio_rx_parameters_t * pRxParameters, ZW_ReceiveFrame_t * pFrame);
static void explorerFrame3chHandler   (CommunicationProfile_t communicationProfile, zpal_radio_rx_parameters_t * pRxParameters, ZW_ReceiveFrame_t * pFrame);
static void ackFrame2chHandler        (CommunicationProfile_t communicationProfile, zpal_radio_rx_parameters_t * pRxParameters, ZW_ReceiveFrame_t * pFrame);
static void ackFrame3chHandler        (CommunicationProfile_t communicationProfile, zpal_radio_rx_parameters_t * pRxParameters, ZW_ReceiveFrame_t * pFrame);
static void ackFrameLRHandler         (CommunicationProfile_t communicationProfile, zpal_radio_rx_parameters_t * pRxParameters, ZW_ReceiveFrame_t * pFrame);
static void multicastFrame2chHandler  (CommunicationProfile_t communicationProfile, zpal_radio_rx_parameters_t * pRxParameters, ZW_ReceiveFrame_t * pFrame);
static void multicastFrame3chHandler  (CommunicationProfile_t communicationProfile, zpal_radio_rx_parameters_t * pRxParameters, ZW_ReceiveFrame_t * pFrame);
static void notApplicableFrameHandler (CommunicationProfile_t communicationProfile, zpal_radio_rx_parameters_t * pRxParameters, ZW_ReceiveFrame_t * pFrame);

/** Array containing function pointers for transmission functions.
 *  The communication profile is used to index the corresponding function pointer in the array to
 *  be used for the transmission.
 */

static const CommunicationProfileFunc m_cCommunicationProfileFunctions[] =
{
  unsupported_transmit,
  // 2CH transmit functions
  transmit9_6k,
  transmit40k,
  transmit_wakeup_250ms, transmit_wakeup_1000ms,                  // 40kbps Classic Beam transmission
  transmit100k,                                                   // 100kbps Classic transmission
  // Long-Range transmit functions
  transmit100kLR, transmit100kLR2,                                // Long-Range transmission
  transmit_wakeup_fragmented_LR, transmit_wakeup_fragmented_LR2,  // Long-Range Beam transmission
  // 3CH transmit functions
  transmit_3ch_100k, transmit_3ch_100k_wakeup,
};

/** Array containing function pointers for functions used for frame header construction when
 *  transmitting single cast frames.
 */
static const HeaderConstructorFunc m_cSingleCastConstructorFunc[] =
{
  construct2chSinglecastHeader,
  construct3chSinglecastHeader,
  constructLRSinglecastHeader,
  constructNotApplicable
};

/** Array containing function pointers for functions used for frame header construction when
 *  transmitting explorer frames.
 */
static const HeaderConstructorFunc m_cExplorerFrameConstructorFunc[] =
{
  construct2chExplorerFrameHeader,
  construct3chExplorerFrameHeader,
  constructNotApplicable,     // LR doesn't support explore
  constructNotApplicable
};

/** Array containing function pointers for functions used for frame header construction when
 *  transmitting acknowledge frames.
 */
static const HeaderConstructorFunc m_cAckFrameConstructorFunc[] =
{
  construct2chAckFrameHeader,
  construct3chAckFrameHeader,
  constructLRAckFrameHeader,
  constructNotApplicable
};

/** Array containing function pointers for functions used for frame header construction when
 *  transmitting multicast frames.
 */
static const HeaderConstructorFunc m_cMulticastFrameConstructorFunc[] =
{
  construct2chMulticastFrameHeader,
  construct3chMulticastFrameHeader,
  constructNotApplicable,      // LR doesn't support multicast
  constructNotApplicable
};


static const FrameHandlerFunc m_cSingleCastHandlerFunc[] =
{
  singlecast2chHandler,
  singlecast3chHandler,
  singlecastLRHandler,
  notApplicableFrameHandler
};

static const FrameHandlerFunc m_cSingleCastRoutedHandlerFunc[] =
{
  singlecastRouted2chHandler,
  singlecastRouted3chHandler,
  notApplicableFrameHandler,   // LR doesn't support routing
  notApplicableFrameHandler
};

static const FrameHandlerFunc m_cExplorerHandlerFunc[] =
{
  explorerFrame2chHandler,
  explorerFrame3chHandler,
  notApplicableFrameHandler,  // LR doesn't support explore
  notApplicableFrameHandler
};

static const FrameHandlerFunc m_cMultiCastHandlerFunc[] =
{
  multicastFrame2chHandler,
  multicastFrame3chHandler,
  notApplicableFrameHandler,  // LR doesn't support multicast
  notApplicableFrameHandler
};

static const FrameHandlerFunc m_cTransferAckHandlerFunc[] =
{
  ackFrame2chHandler,
  ackFrame3chHandler,
  ackFrameLRHandler,
  notApplicableFrameHandler,
};


/** Member array containing active communication profiles in the system */
static CommunicationProfile_t     mCommunicationProfileActive[5] = {PROFILE_UNSUPPORTED, };

static const ZW_ReceiveFilter_t * mSingleCastFilter[PACKET_FILTER_SIZE];       /**< Array containing pointers to single cast receive filters */
static uint8_t                    mSingleCastFilterPaused[PACKET_FILTER_SIZE]; /**< Array for pausing single cast filters */
static uint32_t                   mSingleCastFilterIndex;                      /**< Current free index single cast receive filters array */

static const ZW_ReceiveFilter_t * mSingleCastRoutedFilter[PACKET_FILTER_SIZE];      /**< Array containing pointers to single cast routed receive filters */
static uint8_t                    mSingleCastRoutedFilterPaused[PACKET_FILTER_SIZE];/**< Array for pausing single cast routed filters */
static uint32_t                   mSingleCastRoutedFilterIndex;                     /**< Current free index single cast routed receive filters array */

static const ZW_ReceiveFilter_t * mExplorerFrameFilter[PACKET_FILTER_SIZE];       /**< Array containing pointers to explorer frame receive filters */
static uint8_t                    mExplorerFrameFilterPaused[PACKET_FILTER_SIZE]; /**< Array for pausing explorer frame receive filters */
static uint32_t                   mExplorerFrameFilterIndex;                      /**< Current free index explorer frame receive filters array */

static const ZW_ReceiveFilter_t * mMulticastFrameFilter[PACKET_FILTER_SIZE];       /**< Array containing pointers to multicast frame receive filters */
static uint8_t                    mMulticastFrameFilterPaused[PACKET_FILTER_SIZE]; /**< Array for pausing multicast frame receive filters */
static uint32_t                   mMulticastFrameFilterIndex;                      /**< Current free index multicast frame receive filters array */

static const ZW_ReceiveFilter_t * mTransferAckFilter[PACKET_FILTER_SIZE];       /**< Array containing pointers to acknowledge frame receive filters */
static uint8_t                    mTransferAckFilterPaused[PACKET_FILTER_SIZE]; /**< Array for pausing acknowledge frame receive filters */
static uint32_t                   mTransferAckFilterIndex;                      /**< Current free index acknowledge frame receive filters array */


static uint16_t  mWakeupBeamTimeLength = 0;  //Time in ms of last continous wake up beam fragment

// DataLinkLayer statistics
#ifdef DO_CHECKSUM_CHECK
static uint32_t dataLinkLayerStatisticRxFailedLRC = 0;
static uint32_t dataLinkLayerStatisticRxFailedCRC = 0;
#endif

///////////////////////////////////////////////////////////////////////////////////////
// NOTE: Preamble length is 40 bytes for 2ch 100k
//       Preamble length is 24 bytes for 3ch 100k
//       Preamble length is  8 bytes for 3ch 100k wakeup beam
// NOTE: Minimum preamble length for single-/broadcast is 10 bytes according ITUT-REC-G.9959,
//       however older nodes used 20 bytes, so to ensure backward compatibility, the preamble is set
//       to 20 bytes.

/*******************************
 * Static const values used when transmitting frame on 9.6k, 40, 100k, and wakeup beams.
 * 2CH transmission parameters!
 ******************************/
static const zpal_radio_transmit_parameter_t m_cTxParameter9_6kCh0       = {.speed = ZPAL_RADIO_SPEED_9600, .channel_id = CHANNEL_2_ID, .crc = ZPAL_RADIO_CRC_8_BIT_XOR,   .preamble = PREAMBLE_BYTE_CH2CH3, .preamble_length = 10, .start_of_frame = START_OF_FRAME_CH2CH3, .repeats = 0};
static const zpal_radio_transmit_parameter_t m_cTxParameter40kCh0        = {.speed = ZPAL_RADIO_SPEED_40K,  .channel_id = CHANNEL_1_ID, .crc = ZPAL_RADIO_CRC_8_BIT_XOR,   .preamble = PREAMBLE_BYTE_CH2CH3, .preamble_length = 20, .start_of_frame = START_OF_FRAME_CH2CH3, .repeats = 0};
static const zpal_radio_transmit_parameter_t m_cTxParameter100kCh1       = {.speed = ZPAL_RADIO_SPEED_100K, .channel_id = CHANNEL_0_ID, .crc = ZPAL_RADIO_CRC_16_BIT_CCITT,.preamble = PREAMBLE_BYTE_CH2CH3, .preamble_length = 40, .start_of_frame = START_OF_FRAME_CH2CH3, .repeats = 0};
static const zpal_radio_transmit_parameter_t m_cTxParameterWakeup250ms   = {.speed = ZPAL_RADIO_SPEED_40K,  .channel_id = CHANNEL_1_ID, .crc = ZPAL_RADIO_CRC_NONE,        .preamble = PREAMBLE_BYTE_CH2CH3, .preamble_length = 20, .start_of_frame = START_OF_FRAME_CH2CH3, .repeats = TX_BEAM_2CH_REPEAT_250};
static const zpal_radio_transmit_parameter_t m_cTxParameterWakeup1000ms  = {.speed = ZPAL_RADIO_SPEED_40K,  .channel_id = CHANNEL_1_ID, .crc = ZPAL_RADIO_CRC_NONE,        .preamble = PREAMBLE_BYTE_CH2CH3, .preamble_length = 20, .start_of_frame = START_OF_FRAME_CH2CH3, .repeats = TX_BEAM_2CH_REPEAT_1000};

/*******************************
 * 3CH transmission parameters!
 ******************************/
static zpal_radio_transmit_parameter_t m_c3chTxParameter100k             = {.speed = ZPAL_RADIO_SPEED_100K, .channel_id = CHANNEL_0_ID, .crc = ZPAL_RADIO_CRC_16_BIT_CCITT, .preamble = PREAMBLE_BYTE_CH2CH3, .preamble_length = 24, .start_of_frame = START_OF_FRAME_CH2CH3, .repeats = 0};
static zpal_radio_transmit_parameter_t m_cTxParameterWakeupFragmented    = {.speed = ZPAL_RADIO_SPEED_100K, .channel_id = CHANNEL_0_ID, .crc = ZPAL_RADIO_CRC_NONE,         .preamble = PREAMBLE_BYTE_CH2CH3, .preamble_length = 8,  .start_of_frame = START_OF_FRAME_CH2CH3, .repeats = TX_BEAM_3CH_FRAG_REPEAT_COUNT};

/*******************************
 * LR transmission parameters!
 ******************************/
static const zpal_radio_transmit_parameter_t m_cTxParameter100kLR        = {.speed = ZPAL_RADIO_SPEED_100K, .channel_id = CHANNEL_3_ID, .crc = ZPAL_RADIO_CRC_16_BIT_CCITT, .preamble = PREAMBLE_BYTE_LR, .preamble_length = 40,                      .start_of_frame = START_OF_FRAME_LR, .repeats = 0};
static const zpal_radio_transmit_parameter_t m_cTxParameter100kLR2       = {.speed = ZPAL_RADIO_SPEED_100K, .channel_id = CHANNEL_4_ID, .crc = ZPAL_RADIO_CRC_16_BIT_CCITT, .preamble = PREAMBLE_BYTE_LR, .preamble_length = 40,                      .start_of_frame = START_OF_FRAME_LR, .repeats = 0};
static const zpal_radio_transmit_parameter_t m_cTxParameter100kWakeupLR  = {.speed = ZPAL_RADIO_SPEED_100K, .channel_id = CHANNEL_3_ID, .crc = ZPAL_RADIO_CRC_NONE,         .preamble = PREAMBLE_BYTE_LR, .preamble_length = TX_BEAM_LR_PREAMBLE_LEN, .start_of_frame = START_OF_FRAME_LR, .repeats = TX_BEAM_LR_FRAG_REPEAT_COUNT};
static const zpal_radio_transmit_parameter_t m_cTxParameter100kWakeupLR2 = {.speed = ZPAL_RADIO_SPEED_100K, .channel_id = CHANNEL_4_ID, .crc = ZPAL_RADIO_CRC_NONE,         .preamble = PREAMBLE_BYTE_LR, .preamble_length = TX_BEAM_LR_PREAMBLE_LEN, .start_of_frame = START_OF_FRAME_LR, .repeats = TX_BEAM_LR_FRAG_REPEAT_COUNT};

///////////////////////////////////////////////////////////////////////////////////////



static ZW_ReturnCode_t convertZpalStatusToReturnCode(zpal_status_t status)
{
  ZW_ReturnCode_t returnCode = UNKNOWN_ERROR;
  switch(status)
  {
    case ZPAL_STATUS_OK:
      returnCode = SUCCESS;
      break;
    case ZPAL_STATUS_INVALID_ARGUMENT:
      returnCode = INVALID_PARAMETERS;
      break;
    case ZPAL_STATUS_BUFFER_FULL:
      returnCode = NO_MEMORY;
      break;
    case ZPAL_STATUS_BUSY:
      returnCode = BUSY;
      break;
    default:
      returnCode = UNKNOWN_ERROR;
      break;
  }
  return returnCode;
}


/**@brief Dummy function to catch all occurrences of frames being transmitted using an invalid
 *  communication profile.
 *
 *  @param[in] communicationProfile Profile to be used for transmission
 *  @param[in] pFrame               Pointer to frame to transmit
 *
 *  @retval UNSUPPORTED
 */
static ZW_ReturnCode_t unsupported_transmit(__attribute__((unused)) CommunicationProfile_t   communicationProfile,
                                            __attribute__((unused)) ZW_TransmissionFrame_t * pFrame)
{
  DPRINTF("\r\nUnsupported transmit, communicationProfile=0x%x", communicationProfile);
  ASSERT(false);
  return UNSUPPORTED;
}

/**@brief Internal function for converting TX Communication Profile to Header Type
 *
 *  @param[in] communicationProfile Profile to be used for transmission
 *
 *  @retval Header Type
 */
ZW_HeaderFormatType_t convertTxCommunicationProfileToHeaderType(CommunicationProfile_t communicationProfile)
{
  if(communicationProfile == PROFILE_UNSUPPORTED)
  {
    return HDRFORMATTYP_UNDEFINED;
  }

  if ((communicationProfile == RF_PROFILE_100K_LR_A) || (communicationProfile == RF_PROFILE_100K_LR_B))
  {
    return HDRFORMATTYP_LR;
  }
  else if (communicationProfile < RF_PROFILE_3CH_100K)
  {
    return HDRFORMATTYP_2CH;
  }
  else
  {
    return HDRFORMATTYP_3CH;
  }
}


/**
 * @brief This function converts the 8bit TX power to a 4bit TX power index.
 * The indexes are that of a given table specific for LR beam TX!
 *
 * @attention This function will return the index that corresponds to an equal power value or one that is one step higher!
 *
 * The table described in section 6.3.6.2 on page 57 of "Z-Wave Long Range PHY/MAC layer specification" is implemented here!
 * @param txPower The signed TX Power in dBm at which the LinkLayer transmits the frame.
 * @return The index corresponding to the dBm power value as in the table specified above. Max value is 15!
 */
static uint8_t convertTXPowerToIndex(int8_t txPower)
{
  uint8_t i;
  // The conversion table as in the referenced documentation above! (scope limited to this function)
  static int8_t dBmConversionTable[] = {-6, -2, +2, +6, +10, +13, +16, +19, +21, +23, +25, +26, +27, +28, +29, +30};

  /* The conversion can only handle values within a specified boundary,
   * and must therefore move the out of bounds value of txPowre into the defined area before conversion! */
  if (txPower < TX_PWR_TO_INDEX_CONVERSION_PWR_MIN)
  {
    txPower = TX_PWR_TO_INDEX_CONVERSION_PWR_MIN;
  }
  else if (txPower > TX_PWR_TO_INDEX_CONVERSION_PWR_MAX)
  {
    txPower = TX_PWR_TO_INDEX_CONVERSION_PWR_MAX;
  }

  // Search for an element in the table that is equal or bigger than txPower, and return its index!
  for (i = 0; i < sizeof_array(dBmConversionTable); i++)
  {
    if (txPower <= dBmConversionTable[i])
    {
      break;  // Found the index. (This index could be the next higher index on the table!! This is safer.)
    }
  }

  // The maximum TX Power index value to guard for. The TX Power frame field is of 4 bits.
  if(i > TX_PWR_TO_INDEX_CONVERSION_INDEX_MAX)
  {
    i = TX_PWR_TO_INDEX_CONVERSION_INDEX_MAX;  // This needs the integrity check above
  }
  return i;
}


#ifdef DO_CHECKSUM_CHECK
static uint8_t doLRCCheck(uint8_t length, uint8_t *pData)
{
  uint8_t checksumLRC = 0xFF;
  uint32_t i = length;

  for (; i > 0;)
  {
    checksumLRC ^= pData[--i];
  }
  return checksumLRC;
}


/**@brief Function for XOR CRC calculation on a Z-Wave frame
 *
 * @param[in]     length Length of frame data
 * @param[in,out] pData  Pointer to data array containing the Z-Wave frame. @note the CRC will be
 *                       appended at the end of the data array at index \ref length - 1
 */
static void xorCalculator(uint8_t length, uint8_t *pData)
{
  pData[length - 1] = doLRCCheck(length - 1, pData);
}


static uint16_t crc16CcittCalc(uint8_t length, uint8_t *pData)
{
  uint8_t bitMask;
  uint8_t newBit;
  uint16_t crc16 = CRC_INITAL_VALUE;
  uint32_t i;

  for (i = 0; i < length; i++)
  {
    for (bitMask = 0x80; bitMask != 0; bitMask >>= 1)
    {
      /* Align test bit with next bit of the message byte, starting with msb. */
      newBit = ((pData[i] & bitMask) != 0) ^ ((crc16 & 0x8000) != 0);
      crc16 <<= 1;
      if (newBit)
      {
        crc16 ^= 0x1021;
      }
    } /* for (bitMask = 0x80; bitMask != 0; bitMask >>= 1) */
  }
  return crc16;
}

static bool doCrc16Check(uint16_t crc16ToTest, uint8_t dataLength, uint8_t *pData)
{
  uint16_t crc16 = crc16CcittCalc(dataLength, pData);

  return (crc16 == crc16ToTest);
}


/**@brief Function for CRC CCITT calculation on a Z-Wave frame
 *
 * @param[in]     length Length of frame data
 * @param[in,out] pData  Pointer to data array containing the Z-Wave frame.
 *                       @note the CRC will be appended at the end of the data array at indices
 *                       @ref length - 1 and @ref length - 2
 */
static void crcCcittCalculator(uint8_t length, uint8_t *pData)
{
  uint16_t crc = crc16CcittCalc(length -2, pData);
  pData[length-2] = crc >> 8;
  pData[length-1] = crc & 0xFF;
}
#endif


/**@brief Function for constructing a 2 channel single cast header
 *
 * @param[in]     communicationProfile (void) Not used for header construction but specified to
 *                                     allow compiler to better utilize R0
 * @param[in,out] pFrame Pointer to the frame. The header will be constructed based on settings in
 *                       pFrame->frameOptions. This function will update the head pointed to by
 *                       pFrame->pFrameHeaderStart minus the header length.
 *                       For 2 channel header, the header length is @ref GENERAL_HEADER_2CH_LENGTH.
 */
static void construct2chSinglecastHeader(__attribute__((unused)) CommunicationProfile_t   communicationProfile,
                                         ZW_TransmissionFrame_t * pFrame)
{

  DPRINTF("single constructor comProfile %d\n", communicationProfile);
  memcpy(&pFrame->header.singlecast.header.homeID,
         pFrame->frameOptions.homeId,
         sizeof(pFrame->frameOptions.homeId));

  pFrame->header.singlecast.header.sourceID = pFrame->frameOptions.sourceNodeId;

  pFrame->header.singlecast.header.headerInfo =
      (pFrame->frameOptions.acknowledge << REQ_ACK_2CH_FRAME_CONTROL_POS) |
      (pFrame->frameOptions.routed << ROUTED_2CH_FRAME_CONTROL_POS) |
      (pFrame->frameOptions.speedModified << SPEED_MODIFIED_2CH_FRAME_CONTROL_POS) |
      HDRTYP_SINGLECAST;

  pFrame->header.singlecast.header.reserved =
      (pFrame->frameOptions.sequenceNumber << SEQ_NO_2CH_FRAME_CONTROL_POS) |
      (pFrame->frameOptions.multicastfollowup << MULTICAST_FOLLOWUP_2CH_FRAME_CONTROL_POS);

  pFrame->header.singlecast.destinationID = pFrame->frameOptions.destinationNodeId;
}


/** Function for constructing a 3 channel single cast header
 *
 * @param[in]     communicationProfile (void) Not used for header construction but specified to
 *                                     allow compiler to better utilize R0
 * @param[in,out] pFrame Pointer to the frame. The header will be constructed based on settings in
 *                       pFrame->frameOptions. This function will update the head pointed to by
 *                       pFrame->pFrameHeaderStart minus the header length.
 *                       For 3 channel header, the header length is @ref GENERAL_HEADER_3CH_LENGTH.
 */
static void construct3chSinglecastHeader(__attribute__((unused)) CommunicationProfile_t   communicationProfile,
                                         ZW_TransmissionFrame_t * pFrame)
{
  DPRINTF("single constructor comProfile %d\n", communicationProfile);
  memcpy(&pFrame->header.singlecast3ch.header.homeID,
         pFrame->frameOptions.homeId,
         sizeof(pFrame->frameOptions.homeId));

  pFrame->header.singlecast3ch.header.sourceID = pFrame->frameOptions.sourceNodeId;

  pFrame->header.singlecast3ch.header.headerInfo =
      ((pFrame->frameOptions.routed) ? HDRTYP_ROUTED : HDRTYP_SINGLECAST) |
      (pFrame->frameOptions.acknowledge << REQ_ACK_3CH_FRAME_CONTROL_POS) |
      (pFrame->frameOptions.multicastfollowup << MULTICAST_FOLLOWUP_3CH_FRAME_CONTROL_POS);

  pFrame->header.singlecast3ch.header.sequenceNumber = pFrame->frameOptions.sequenceNumber;

  pFrame->header.singlecast3ch.destinationID = pFrame->frameOptions.destinationNodeId;

  pFrame->header.singlecast3ch.header.headerInfo2 =
      (pFrame->frameOptions.extended << EXTENDED_3CH_FRAME_EXTENDEDHEADER_POS)|
      (pFrame->frameOptions.wakeup250ms << SOURCE_WAKEUP_250MS_3CH_POS)|
      (pFrame->frameOptions.wakeup1000ms << SOURCE_WAKEUP_1000MS_3CH_POS);
}

/** Function for constructing a Long Range single cast header
 *
 * @param[in]     communicationProfile (void) Not used for header construction but specified to
 *                                     allow compiler to better utilize R0
 * @param[in,out] pFrame Pointer to the frame. The header will be constructed based on settings in
 *                       pFrame->frameOptions. This function will update the head pointed to by
 *                       pFrame->pFrameHeaderStart minus the header length.
 *                       For 3 channel header, the header length is @ref GENERAL_HEADER_3CH_LENGTH.
 */
static void constructLRSinglecastHeader(__attribute__((unused)) CommunicationProfile_t    communicationProfile,
                                         ZW_TransmissionFrame_t * pFrame)
{
  frame* fr = (frame *)&pFrame->header;

  DPRINTF("single LR constructor comProfile %d\n", communicationProfile);

  memcpy(&pFrame->header.singlecastLR.header.homeID,
         pFrame->frameOptions.homeId,
         sizeof(pFrame->frameOptions.homeId));

  SET_SINGLECAST_SOURCE_NODEID_LR(*fr, pFrame->frameOptions.sourceNodeId);

  SET_SINGLECAST_DESTINATION_NODEID_LR(*fr, pFrame->frameOptions.destinationNodeId);

  pFrame->header.singlecastLR.header.headerInfo =
      (pFrame->frameOptions.acknowledge << REQ_ACK_LR_FRAME_POS) |
      (pFrame->frameOptions.extended << EXTENDED_LR_FRAME_POS) |
      (HDRTYP_SIMGLECAST_FRAME_LR << HEADERTYPE_LR_FRAME_POS_OFFSET);

  pFrame->header.singlecastLR.header.sequenceNumber = pFrame->frameOptions.sequenceNumber;

  updateLRFrameTXPower(pFrame);
}


/** Function for constructing a 2 channel explorer frame header
 *
 * @param[in]     communicationProfile (void) Not used for header construction but specified to
 *                                     allow compiler to better utilize R0
 * @param[in,out] pFrame Pointer to the frame. The header will be constructed based on settings in
 *                       pFrame->frameOptions. This function will update the head pointed to by
 *                       pFrame->pFrameHeaderStart minus the header length.
 *                       For 2 channel header, the header length is @ref GENERAL_HEADER_2CH_LENGTH.
 */
static void construct2chExplorerFrameHeader(__attribute__((unused)) CommunicationProfile_t   communicationProfile,
                                            ZW_TransmissionFrame_t * pFrame)
{
  pFrame->headerLength = sizeof(pFrame->header.singlecast);
  DPRINTF("construct2chExplorerFrameHeader: pFrame->headerLength = %02X\n", pFrame->headerLength);
  memcpy(&pFrame->header.singlecast.header.homeID,
         pFrame->frameOptions.homeId,
         sizeof(pFrame->frameOptions.homeId));

  pFrame->header.singlecast.header.sourceID = pFrame->frameOptions.sourceNodeId;

  pFrame->header.singlecast.header.headerInfo =
      (pFrame->frameOptions.acknowledge << REQ_ACK_2CH_FRAME_CONTROL_POS)|
      (pFrame->frameOptions.speedModified << SPEED_MODIFIED_2CH_FRAME_CONTROL_POS)|
      HDRTYP_EXPLORE;

  pFrame->header.singlecast.header.reserved =
      (pFrame->frameOptions.sequenceNumber << SEQ_NO_2CH_FRAME_CONTROL_POS)|
      (pFrame->frameOptions.wakeup250ms << SOURCE_WAKEUP_250MS_2CH_POS)|
      (pFrame->frameOptions.wakeup1000ms << SOURCE_WAKEUP_1000MS_2CH_POS);

  pFrame->header.singlecast.destinationID = pFrame->frameOptions.destinationNodeId;
}


static void construct3chExplorerFrameHeader(__attribute__((unused)) CommunicationProfile_t   communicationProfile,
                                            ZW_TransmissionFrame_t * pFrame)
{
  pFrame->headerLength = (sizeof(pFrame->header.singlecast3ch) - sizeof(pFrame->header.singlecast3ch.extension));
  memcpy(&pFrame->header.singlecast3ch.header.homeID,
         pFrame->frameOptions.homeId,
         sizeof(pFrame->frameOptions.homeId));

  pFrame->header.singlecast3ch.header.sourceID = pFrame->frameOptions.sourceNodeId;

  pFrame->header.singlecast3ch.header.headerInfo =
      (pFrame->frameOptions.acknowledge << REQ_ACK_3CH_FRAME_CONTROL_POS)|
      HDRTYP_EXPLORE;

  pFrame->header.singlecast3ch.header.sequenceNumber = pFrame->frameOptions.sequenceNumber;

  pFrame->header.singlecast3ch.destinationID = pFrame->frameOptions.destinationNodeId;

  pFrame->header.singlecast3ch.header.headerInfo2 =
      (pFrame->frameOptions.wakeup250ms << SOURCE_WAKEUP_250MS_3CH_POS)|
      (pFrame->frameOptions.wakeup1000ms << SOURCE_WAKEUP_1000MS_3CH_POS);
}


/** Function for constructing a 2 channel acknowledge frame header
 *
 * @param[in]     communicationProfile (void) Not used for header construction but specified to
 *                                     allow compiler to better utilize R0
 * @param[in,out] pFrame Pointer to the frame. The header will be constructed based on settings in
 *                       pFrame->frameOptions. This function will update the head pointed to by
 *                       pFrame->pFrameHeaderStart minus the header length.
 *                       For 2 channel header, the header length is @ref GENERAL_HEADER_2CH_LENGTH.
 */
static void construct2chAckFrameHeader(__attribute__((unused)) CommunicationProfile_t   communicationProfile,
                                       ZW_TransmissionFrame_t * pFrame)
{
  pFrame->headerLength = sizeof(pFrame->header.singlecast);
  memcpy(&pFrame->header.singlecast.header.homeID,
         pFrame->frameOptions.homeId,
         sizeof(pFrame->frameOptions.homeId));

  pFrame->header.singlecast.header.sourceID = pFrame->frameOptions.sourceNodeId;

  pFrame->header.singlecast.header.headerInfo =
      (pFrame->frameOptions.acknowledge << REQ_ACK_2CH_FRAME_CONTROL_POS) |
      (pFrame->frameOptions.speedModified << SPEED_MODIFIED_2CH_FRAME_CONTROL_POS) |
      (pFrame->frameOptions.lowPower << LOW_POWER_2CH_FRAME_CONTROL_POS) |
      HDRTYP_TRANSFERACK;

  pFrame->header.singlecast.header.reserved =
      pFrame->frameOptions.sequenceNumber << SEQ_NO_2CH_FRAME_CONTROL_POS;

  pFrame->header.singlecast.destinationID = pFrame->frameOptions.destinationNodeId;
}


static void construct3chAckFrameHeader(__attribute__((unused)) CommunicationProfile_t   communicationProfile,
                                       ZW_TransmissionFrame_t * pFrame)
{
  memcpy(&pFrame->header.singlecast3ch.header.homeID,
         pFrame->frameOptions.homeId,
         sizeof(pFrame->frameOptions.homeId));

  pFrame->header.singlecast3ch.header.sourceID = pFrame->frameOptions.sourceNodeId;

  pFrame->header.singlecast3ch.header.headerInfo =
      (pFrame->frameOptions.acknowledge << REQ_ACK_3CH_FRAME_CONTROL_POS)|
      (pFrame->frameOptions.lowPower << LOW_POWER_3CH_FRAME_CONTROL_POS) |
      HDRTYP_TRANSFERACK;

  pFrame->header.singlecast3ch.header.sequenceNumber = pFrame->frameOptions.sequenceNumber;

  pFrame->header.singlecast3ch.destinationID = pFrame->frameOptions.destinationNodeId;

  pFrame->header.singlecast3ch.header.headerInfo2 = (pFrame->frameOptions.extended << EXTENDED_3CH_FRAME_EXTENDEDHEADER_POS);
}

static void constructLRAckFrameHeader(__attribute__((unused)) CommunicationProfile_t    communicationProfile,
                                       ZW_TransmissionFrame_t * pFrame)
{
  frame* fr = (frame *)&pFrame->header;

  DPRINTF("ack LR constructor comProfile %d\n", communicationProfile);

  memcpy(&pFrame->header.transferACKLR.header.homeID,
         pFrame->frameOptions.homeId,
         sizeof(pFrame->frameOptions.homeId));

  SET_TRANSFERACK_SOURCE_NODEID_LR(*fr, pFrame->frameOptions.sourceNodeId);

  SET_TRANSFERACK_DESTINATION_NODEID_LR(*fr, pFrame->frameOptions.destinationNodeId);

  pFrame->header.transferACKLR.header.length = pFrame->headerLength + pFrame->payloadLength;

  pFrame->header.transferACKLR.header.headerInfo =
      (pFrame->frameOptions.extended << EXTENDED_LR_FRAME_POS) |
      (HDRTYP_ACK_FRAME_LR << HEADERTYPE_LR_FRAME_POS_OFFSET);

  pFrame->header.transferACKLR.header.sequenceNumber = pFrame->frameOptions.sequenceNumber;

  pFrame->header.transferACKLR.receiveRSSI = pFrame->rssi;

  updateLRFrameTXPower(pFrame);
}


/** Function for constructing a 2 channel Multicast frame header
 *
 * @param[in]     communicationProfile (void) Not used for header construction but specified to
 *                                     allow compiler to better utilize R0
 * @param[in,out] pFrame Pointer to the frame. The header will be constructed based on settings in
 *                       pFrame->frameOptions. This function will update the head pointed to by
 *                       pFrame->pFrameHeaderStart minus the header length.
 *                       For 2 channel header, the header length is @ref GENERAL_HEADER_2CH_LENGTH.
 */
static void construct2chMulticastFrameHeader(__attribute__((unused)) CommunicationProfile_t   communicationProfile,
                                             ZW_TransmissionFrame_t * pFrame)
{
  DPRINTF("multi constructor comProfile %d\n", communicationProfile);
  memcpy(&pFrame->header.multicast.header.homeID,
         pFrame->frameOptions.homeId,
         sizeof(pFrame->frameOptions.homeId));

  pFrame->header.multicast.header.sourceID = pFrame->frameOptions.sourceNodeId;

  pFrame->header.multicast.header.headerInfo =
      (pFrame->frameOptions.acknowledge << REQ_ACK_2CH_FRAME_CONTROL_POS) |
      (pFrame->frameOptions.speedModified << SPEED_MODIFIED_2CH_FRAME_CONTROL_POS) |
      HDRTYP_MULTICAST;

  pFrame->header.multicast.header.reserved =
      pFrame->frameOptions.sequenceNumber << SEQ_NO_2CH_FRAME_CONTROL_POS;

  // pFrame->header.multicast.header.length = pFrame->headerLength + pFrame->payloadLength;
}


static void construct3chMulticastFrameHeader(__attribute__((unused)) CommunicationProfile_t   communicationProfile,
                                             ZW_TransmissionFrame_t * pFrame)
{
  DPRINTF("multi constructor comProfile %d\n", communicationProfile);
  memcpy(&pFrame->header.multicast3ch.header.homeID,
         pFrame->frameOptions.homeId,
         sizeof(pFrame->frameOptions.homeId));

  pFrame->header.multicast3ch.header.sourceID = pFrame->frameOptions.sourceNodeId;

  pFrame->header.multicast3ch.header.headerInfo = HDRTYP_MULTICAST;
  pFrame->header.multicast3ch.header.sequenceNumber = pFrame->frameOptions.sequenceNumber;

  pFrame->header.multicast3ch.header.headerInfo2 =
      (pFrame->frameOptions.extended << EXTENDED_3CH_FRAME_EXTENDEDHEADER_POS)|
      (pFrame->frameOptions.wakeup250ms << SOURCE_WAKEUP_250MS_3CH_POS)|
      (pFrame->frameOptions.wakeup1000ms << SOURCE_WAKEUP_1000MS_3CH_POS);

}

// Dummy function for catching non-applicable header type constructions
static void constructNotApplicable(__attribute__((unused)) CommunicationProfile_t   communicationProfile,
                                   __attribute__((unused)) ZW_TransmissionFrame_t * pFrame)
{
  DPRINTF("\r\nNon-applicable header type construction, communicationProfile=0x%x", communicationProfile);
  ASSERT(false);
}


/** Function for transmitting a Z-Wave using at 9600 baud on 2 channel
 *
 * @param[in] communicationProfile Communication profile to use for transmission
 * @param[in] pFrame               Pointer to the frame to be transmitted.
 */
static ZW_ReturnCode_t transmit9_6k(__attribute__((unused)) CommunicationProfile_t   communicationProfile,
                                    ZW_TransmissionFrame_t * pFrame)
{
  pFrame->header.header.length = pFrame->headerLength + pFrame->payloadLength + 1;


  //This profile field it being used in the transport layer to decide the received frame speed.
  pFrame->profile = RF_PROFILE_9_6K;

  zpal_status_t return_status = zpal_radio_transmit(&m_cTxParameter9_6kCh0,
                                                    pFrame->headerLength,
                                                    (uint8_t *)&pFrame->header,
                                                    pFrame->payloadLength,
                                                    (uint8_t *)&pFrame->payload,
                                                    pFrame->useLBT,
                                                    pFrame->txPower);
  return convertZpalStatusToReturnCode(return_status);
}


/** Function for transmitting a Z-Wave using at 40k baud on 2 channel
 *
 * @param[in] communicationProfile Communication profile to use for transmission
 * @param[in] pFrame               Pointer to the frame to be transmitted.
 */
static ZW_ReturnCode_t transmit40k(__attribute__((unused)) CommunicationProfile_t   communicationProfile,
                                   ZW_TransmissionFrame_t * pFrame)
{
  pFrame->header.header.length = pFrame->headerLength + pFrame->payloadLength + 1;

  //This profile field it being used in the transport layer to decide the received frame speed.
  pFrame->profile = RF_PROFILE_40K;

  zpal_status_t return_status = zpal_radio_transmit(&m_cTxParameter40kCh0,
                                                    pFrame->headerLength,
                                                    (uint8_t *)&pFrame->header,
                                                    pFrame->payloadLength,
                                                    (uint8_t *)&pFrame->payload,
                                                    pFrame->useLBT,
                                                    pFrame->txPower);
  return convertZpalStatusToReturnCode(return_status);
}


/** Function for transmitting a Z-Wave frame at 100k baud LR1
 *
 * @param[in] communicationProfile Communication profile to use for transmission
 * @param[in] pFrame               Pointer to the frame to be transmitted.
 */
static ZW_ReturnCode_t transmit100k(__attribute__((unused)) CommunicationProfile_t   communicationProfile,
                                    ZW_TransmissionFrame_t * pFrame)
{
  pFrame->header.header.length = pFrame->headerLength + pFrame->payloadLength + 2;

  //This profile field it being used in the transport layer to decide the received frame speed.
  pFrame->profile = RF_PROFILE_100K;

  zpal_status_t return_status = zpal_radio_transmit(&m_cTxParameter100kCh1,
                                                    pFrame->headerLength,
                                                    (uint8_t *)&pFrame->header,
                                                    pFrame->payloadLength,
                                                    (uint8_t *)&pFrame->payload,
                                                    pFrame->useLBT,
                                                    pFrame->txPower);
  return convertZpalStatusToReturnCode(return_status);
}


/** Function for transmitting a Z-Wave frame using at 100k baud on 2 channel
 *
 * @param[in] communicationProfile Communication profile to use for transmission
 * @param[in] pFrame               Pointer to the frame to be transmitted.
 */
static ZW_ReturnCode_t transmit100kLR(__attribute__((unused)) CommunicationProfile_t   communicationProfile,
                                      ZW_TransmissionFrame_t * pFrame)
{
  pFrame->header.header.length = pFrame->headerLength + pFrame->payloadLength + 2;
  //This profile field it being used in the transport layer to decide the received frame speed.
  pFrame->profile = RF_PROFILE_100K_LR_A;

  zpal_status_t return_status = zpal_radio_transmit(&m_cTxParameter100kLR,
                                                    pFrame->headerLength,
                                                    (uint8_t *)&pFrame->header,
                                                    pFrame->payloadLength,
                                                    (uint8_t *)&pFrame->payload,
                                                    pFrame->useLBT,
                                                    pFrame->txPower);
  return convertZpalStatusToReturnCode(return_status);
}


/** Function for transmitting a Z-Wave frame at 100k baud LR2
 *
 * @param[in] communicationProfile Communication profile to use for transmission
 * @param[in] pFrame               Pointer to the frame to be transmitted.
 */
static ZW_ReturnCode_t transmit100kLR2(__attribute__((unused)) CommunicationProfile_t   communicationProfile,
                                       ZW_TransmissionFrame_t * pFrame)
{
  pFrame->header.header.length = pFrame->headerLength + pFrame->payloadLength + 2;

  //This profile field it being used in the transport layer to decide the received frame speed.
  pFrame->profile = RF_PROFILE_100K_LR_B;

  zpal_status_t return_status = zpal_radio_transmit(&m_cTxParameter100kLR2,
                                                    pFrame->headerLength,
                                                    (uint8_t *)&pFrame->header,
                                                    pFrame->payloadLength,
                                                    (uint8_t *)&pFrame->payload,
                                                    pFrame->useLBT,
                                                    pFrame->txPower);
  return convertZpalStatusToReturnCode(return_status);
}


/** Function for transmitting a Z-Wave wakeup beam to a 250 ms FLiRS node.
 *  After wakeup beam has been transmitted the Z-Wave frame will be transmitted using the
 *  corresponding transmit function as specified by \ref communicationProfile
 *
 * @param[in] communicationProfile Communication profile to use for transmission
 * @param[in] pFrame               Pointer to the frame to be transmitted.
 */
static ZW_ReturnCode_t transmit_wakeup_250ms(__attribute__((unused)) CommunicationProfile_t   communicationProfile,
                                             ZW_TransmissionFrame_t * pFrame)
{
  uint32_t homeId = (pFrame->frameOptions.homeId[3] << 24) |
                    (pFrame->frameOptions.homeId[2] << 16) |
                    (pFrame->frameOptions.homeId[1] << 8)  |
                    (pFrame->frameOptions.homeId[0]);

  // Wakeup on 2 channel is on 40k channel
  // Wakeup on 3ch, which is using fragmented beam
  // See ITU T-REC-G.9959.
  uint8_t mWakeupBeam[TX_BEAM_2CH_INFO_LENGTH]       = {BEAM_FRAME_BEAM_TAG,};  // Beam TAG to notify already awake nodes that this is a BEAM!
  mWakeupBeam[WAKEUP_BEAM_2CH3CH_NODE_ID_INDEX]      = pFrame->frameOptions.destinationNodeId;
  mWakeupBeam[WAKEUP_BEAM_2CH3CH_HOME_ID_HASH_INDEX] = HomeIdHashCalculate(homeId,
                                                                           pFrame->frameOptions.destinationNodeId,
                                                                           zpal_radio_get_protocol_mode());
  mWakeupBeamTimeLength = TX_BEAM_2CH_TIME_250;

  zpal_status_t status = zpal_radio_transmit_beam(&m_cTxParameterWakeup250ms, TX_BEAM_2CH_INFO_LENGTH, mWakeupBeam, pFrame->txPower);
  return convertZpalStatusToReturnCode(status);
}

/** Function for transmitting a Z-Wave wakeup beam to a 1000 ms FLiRS node.
 *  After wakeup beam has been transmitted the Z-Wave frame will be transmitted using the
 *  corresponding transmit function as specified by \ref communicationProfile
 *
 * @param[in] communicationProfile Communication profile to use for transmission
 * @param[in] pFrame               Pointer to the frame to be transmitted.
 */
static ZW_ReturnCode_t transmit_wakeup_1000ms(__attribute__((unused)) CommunicationProfile_t   communicationProfile,
                                              ZW_TransmissionFrame_t * pFrame)
{
  uint32_t homeId = (pFrame->frameOptions.homeId[3] << 24) |
                    (pFrame->frameOptions.homeId[2] << 16) |
                    (pFrame->frameOptions.homeId[1] << 8)  |
                    (pFrame->frameOptions.homeId[0]);

  // Wakeup is always on 40k channel, unless 3ch, which is using fragmented beams,
  // Wakeup on 3ch, which is using fragmented beam
  // See ITU T-REC-G.9959.
  uint8_t mWakeupBeam[TX_BEAM_2CH_INFO_LENGTH]       = {BEAM_FRAME_BEAM_TAG,};  // Beam TAG to notify already awake nodes that this is a BEAM!
  mWakeupBeam[WAKEUP_BEAM_2CH3CH_NODE_ID_INDEX]      = pFrame->frameOptions.destinationNodeId;
  mWakeupBeam[WAKEUP_BEAM_2CH3CH_HOME_ID_HASH_INDEX] = HomeIdHashCalculate(homeId,
                                                                           pFrame->frameOptions.destinationNodeId,
                                                                           zpal_radio_get_protocol_mode());
  mWakeupBeamTimeLength = TX_BEAM_2CH_DURATION_1000;

  zpal_status_t status = zpal_radio_transmit_beam(&m_cTxParameterWakeup1000ms, TX_BEAM_2CH_INFO_LENGTH, mWakeupBeam, pFrame->txPower);
  return convertZpalStatusToReturnCode(status);
}


static ZW_ReturnCode_t transmit_3ch_100k(__attribute__((unused)) CommunicationProfile_t   communicationProfile,
                                         ZW_TransmissionFrame_t * pFrame)
{
  pFrame->header.header.length = pFrame->headerLength + pFrame->payloadLength + 2;

  /**
   * With the next statement, we are calculating the desired PHY channelID!
   * In the transmit API function, we changed the RF_PROFILE_3CH_100K_CH_x
   * to RF_PROFILE_3CH_100K and saved the RF_PROFILE_3CH_100K_CH_x in pFrame->profile.
   *
   * Hence, here we are not using communicationProfile, but use pFrame->profile instead!
   */
  m_c3chTxParameter100k.channel_id = llConvertTransmitProfileToPHYChannel(pFrame->profile);

  zpal_status_t return_status = zpal_radio_transmit(&m_c3chTxParameter100k,
                                                    pFrame->headerLength,
                                                    (uint8_t *)&pFrame->header,
                                                    pFrame->payloadLength,
                                                    (uint8_t *)&pFrame->payload,
                                                    pFrame->useLBT,
                                                    pFrame->txPower);
  return convertZpalStatusToReturnCode(return_status);
}

static ZW_ReturnCode_t transmit_3ch_100k_wakeup(__attribute__((unused)) CommunicationProfile_t   communicationProfile,
                                                ZW_TransmissionFrame_t * pFrame)
{
  uint8_t mWakeupBeam[TX_BEAM_3CH_INFO_LENGTH] = {BEAM_FRAME_BEAM_TAG,};  // Beam TAG to notify already awake nodes that this is a BEAM!
  mWakeupBeam[WAKEUP_BEAM_2CH3CH_NODE_ID_INDEX]  = pFrame->frameOptions.destinationNodeId;
  /*
   * We don't use HomeID hash on 3ch z-wave classic, although we should have!
   * In order to be backward compatible with 1st generation chips that only read the NodeID,
   * we must revert to using the PREAMBLE byte, BEAM_FRAME_BEAM_TAG, which is also the default HomeID Hash.
   * This issue also exists in 2CH, but there, we have made a workaround!
   */
  mWakeupBeam[WAKEUP_BEAM_2CH3CH_HOME_ID_HASH_INDEX] = BEAM_FRAME_BEAM_TAG;

  /**
   * With the next statement, we are calculating the desired PHY channelID!
   * In the transmit API function, we changed the RF_PROFILE_3CH_100K_CH_x
   * to RF_PROFILE_3CH_100K and saved the RF_PROFILE_3CH_100K_CH_x in pFrame->profile.
   *
   * Hence, here we are not using communicationProfile, but use pFrame->profile instead!
   */
  m_cTxParameterWakeupFragmented.channel_id = llConvertTransmitProfileToPHYChannel(pFrame->profile);

  mWakeupBeamTimeLength = TX_BEAM_3CH_FRAG_DURATION_MS;

  zpal_status_t status = zpal_radio_transmit_beam(&m_cTxParameterWakeupFragmented, TX_BEAM_3CH_INFO_LENGTH, mWakeupBeam, pFrame->txPower);
  return convertZpalStatusToReturnCode(status);
}

static ZW_ReturnCode_t transmit_wakeup_fragmented_LR(__attribute__((unused)) CommunicationProfile_t   communicationProfile,
                                                     ZW_TransmissionFrame_t * pFrame)
{
  uint32_t homeId = (pFrame->frameOptions.homeId[3] << 24) |
                    (pFrame->frameOptions.homeId[2] << 16) |
                    (pFrame->frameOptions.homeId[1] << 8)  |
                    (pFrame->frameOptions.homeId[0]);

  uint8_t mWakeupBeam[TX_BEAM_LR_INFO_LENGTH] = {BEAM_FRAME_BEAM_TAG,};  // Beam TAG to notify already awake nodes that this is a BEAM!

  // Convert TX Power to a corresponding index as in the table of the MAC specification.
  uint8_t txPwrIdx = convertTXPowerToIndex(pFrame->txPower);

  // Insert the TX-Power and the MSN of the 12bit Node-ID!
  mWakeupBeam[WAKEUP_BEAM_LR_TX_PWR_MSN_NODEID_IDX] = (txPwrIdx << BEAM_FRAME_TX_PWR_POS)
      | ((pFrame->frameOptions.destinationNodeId & WAKEUP_BEAM_LR_NODE_ID_MSN_mask) >> 8);  // Strip the LSB and MSN!
  // Insert the MSB of the 12bit Node-ID, and the Home-ID hash!
  mWakeupBeam[WAKEUP_BEAM_LR_NODE_ID_IDX]      = (uint8_t)pFrame->frameOptions.destinationNodeId;  // Strip the MSB!
  mWakeupBeam[WAKEUP_BEAM_LR_HOME_ID_HASH_IDX] = HomeIdHashCalculate(homeId,
                                                                     pFrame->frameOptions.destinationNodeId,
                                                                     zpal_radio_get_protocol_mode());
  mWakeupBeamTimeLength = TX_BEAM_LR_FRAG_DURATION_MS;

  zpal_status_t status = zpal_radio_transmit_beam(&m_cTxParameter100kWakeupLR, TX_BEAM_LR_INFO_LENGTH, mWakeupBeam, pFrame->txPower);
  return convertZpalStatusToReturnCode(status);
}


static ZW_ReturnCode_t transmit_wakeup_fragmented_LR2(__attribute__((unused)) CommunicationProfile_t   communicationProfile,
                                                      ZW_TransmissionFrame_t * pFrame)
{
  uint32_t homeId = (pFrame->frameOptions.homeId[3] << 24) |
                    (pFrame->frameOptions.homeId[2] << 16) |
                    (pFrame->frameOptions.homeId[1] << 8)  |
                    (pFrame->frameOptions.homeId[0]);

  uint8_t mWakeupBeam[TX_BEAM_LR_INFO_LENGTH] = {BEAM_FRAME_BEAM_TAG,};  // Beam TAG to notify already awake nodes that this is a BEAM!

  // Convert TX Power to a corresponding index as in the table of the MAC specification.
  uint8_t txPwrIdx = convertTXPowerToIndex(pFrame->txPower);

  // Insert the TX-Power and the MSN of the 12bit Node-ID!
  mWakeupBeam[WAKEUP_BEAM_LR_TX_PWR_MSN_NODEID_IDX] = (txPwrIdx << BEAM_FRAME_TX_PWR_POS)
      | ((pFrame->frameOptions.destinationNodeId & WAKEUP_BEAM_LR_NODE_ID_MSN_mask) >> 8);  // Strip the LSB and MSN!
  // Insert the MSB of the 12bit Node-ID, and the Home-ID hash!
  mWakeupBeam[WAKEUP_BEAM_LR_NODE_ID_IDX]      = (uint8_t)pFrame->frameOptions.destinationNodeId;  // Strip the MSB!
  mWakeupBeam[WAKEUP_BEAM_LR_HOME_ID_HASH_IDX] = HomeIdHashCalculate(homeId,
                                                                     pFrame->frameOptions.destinationNodeId,
                                                                     zpal_radio_get_protocol_mode());
  mWakeupBeamTimeLength = TX_BEAM_LR_FRAG_DURATION_MS;

  zpal_status_t status = zpal_radio_transmit_beam(&m_cTxParameter100kWakeupLR2, TX_BEAM_LR_INFO_LENGTH, mWakeupBeam, pFrame->txPower);
  return convertZpalStatusToReturnCode(status);
}


static ZW_ReturnCode_t llSetupRegion(zpal_radio_region_t region, zpal_radio_lr_channel_config_t eLrChCfg)
{
  zpal_radio_protocol_mode_t protocolMode = zpal_radio_region_get_protocol_mode(region, eLrChCfg);
  if (ZPAL_RADIO_PROTOCOL_MODE_UNDEFINED != protocolMode)
  {
    if (ZPAL_RADIO_PROTOCOL_MODE_1 == protocolMode)
    {
      mCommunicationProfileActive[DATA_RATE_1_INDEX] = RF_PROFILE_9_6K;
      mCommunicationProfileActive[DATA_RATE_2_INDEX] = RF_PROFILE_40K;
      mCommunicationProfileActive[DATA_RATE_3_INDEX] = RF_PROFILE_100K;
    }
    if (ZPAL_RADIO_PROTOCOL_MODE_2 == protocolMode)
    {
      mCommunicationProfileActive[DATA_RATE_1_INDEX] = RF_PROFILE_3CH_100K_CH_A;
      mCommunicationProfileActive[DATA_RATE_2_INDEX] = RF_PROFILE_3CH_100K_CH_B;
      mCommunicationProfileActive[DATA_RATE_3_INDEX] = RF_PROFILE_3CH_100K_CH_C;
    }
    if (ZPAL_RADIO_PROTOCOL_MODE_3 == protocolMode)
    {
      mCommunicationProfileActive[DATA_RATE_1_INDEX] = RF_PROFILE_9_6K;
      mCommunicationProfileActive[DATA_RATE_2_INDEX] = RF_PROFILE_40K;
      mCommunicationProfileActive[DATA_RATE_3_INDEX] = RF_PROFILE_100K;
      if (ZPAL_RADIO_LR_CH_CFG1 == eLrChCfg)
      {
        mCommunicationProfileActive[DATA_RATE_4_INDEX] = RF_PROFILE_100K_LR_A;
      }
      else
      {
        mCommunicationProfileActive[DATA_RATE_4_INDEX] = RF_PROFILE_100K_LR_B;
      }
    }
    if (ZPAL_RADIO_PROTOCOL_MODE_4 == protocolMode)
    {
      mCommunicationProfileActive[DATA_RATE_4_INDEX] = RF_PROFILE_100K_LR_A;
      mCommunicationProfileActive[DATA_RATE_5_INDEX] = RF_PROFILE_100K_LR_B;
    }
    return SUCCESS;
  }
  return INVALID_PARAMETERS;
}


bool llChangeLrChannelConfig(zpal_radio_lr_channel_config_t eLrChCfg)
{
  if (zpal_radio_change_region(zpal_radio_get_region(), eLrChCfg) == ZPAL_STATUS_OK)
  {
    return true;
  }
  return false;
}


ZW_ReturnCode_t llInit(zpal_radio_profile_t* const pRfProfile)
{
  memset(mCommunicationProfileActive,
         PROFILE_UNSUPPORTED,
         sizeof(mCommunicationProfileActive));

  mSingleCastFilterIndex        = 0;
  mSingleCastRoutedFilterIndex  = 0;
  mExplorerFrameFilterIndex     = 0;
  mMulticastFrameFilterIndex    = 0;
  mTransferAckFilterIndex       = 0;

  memset(mSingleCastFilterPaused,       0, sizeof(mSingleCastFilterPaused));
  memset(mSingleCastRoutedFilterPaused, 0, sizeof(mSingleCastRoutedFilterPaused));
  memset(mExplorerFrameFilterPaused,    0, sizeof(mExplorerFrameFilterPaused));
  memset(mMulticastFrameFilterPaused,   0, sizeof(mMulticastFrameFilterPaused));
  memset(mTransferAckFilterPaused,      0, sizeof(mTransferAckFilterPaused));

  pRfProfile->receive_handler_cb = radioFrameReceiveHandler;

  if (SUCCESS == llSetupRegion(pRfProfile->region, pRfProfile->active_lr_channel_config))
  {
    zpal_radio_init(pRfProfile);
    return SUCCESS;
  }

  return INVALID_PARAMETERS;
}

/**@brief Module scope function for transmitting a Z-Wave Single Cast frame.
 *
 * @details When this function rerfRxCurrentChturn, the frame has been copied to the radio for transmission and
 *          the buffer can be released.
 *
 * @param[in] communicationProfile Communication profile to use when transmitting this frame
 * @param[in] pFrame               Pointer to the frame to be transmitted
 */
static ZW_ReturnCode_t llTransmitSingleCastFrame(CommunicationProfile_t   communicationProfile,
                                                 ZW_TransmissionFrame_t * pFrame)
{
  ZW_HeaderFormatType_t headerType = convertTxCommunicationProfileToHeaderType(communicationProfile);

  // Construct the singlecast header and pass on the frame.
  m_cSingleCastConstructorFunc[headerType](communicationProfile, pFrame);

  return m_cCommunicationProfileFunctions[communicationProfile](communicationProfile, pFrame);
}

/**@brief Module scope function for transmitting a Z-Wave Acknowledge frame.
 *
 * @details When this function return, the frame has been copied to the radio for transmission and
 *          the buffer can be released.
 *
 * @param[in] communicationProfile Communication profile to use when transmitting this frame
 * @param[in] pFrame               Pointer to the frame to be transmitted
 */
static ZW_ReturnCode_t llTransmitAckFrame(CommunicationProfile_t   communicationProfile,
                                          ZW_TransmissionFrame_t * pFrame)
{
#ifdef DEBUGPRINT
  if (RF_PROFILE_9_6K == communicationProfile)
  {
	  DPRINTF("Speed 9600 %d\n", communicationProfile);
  }
#endif

  ZW_HeaderFormatType_t headerType = convertTxCommunicationProfileToHeaderType(communicationProfile);

  // Construct the ack header and pass on the frame.
  m_cAckFrameConstructorFunc[headerType](communicationProfile, pFrame);

  return m_cCommunicationProfileFunctions[communicationProfile](communicationProfile, pFrame);
}

/**@brief Module scope function for transmitting a Z-Wave Explorer frame.
 *
 * @details When this function return, the frame has been copied to the radio for transmission and
 *          the buffer can be released.
 *
 * @param[in] communicationProfile Communication profile to use when transmitting this frame
 * @param[in] pFrame               Pointer to the frame to be transmitted
 */
static ZW_ReturnCode_t llTransmitExplorerFrame(CommunicationProfile_t   communicationProfile,
                                               ZW_TransmissionFrame_t * pFrame)
{
  ZW_HeaderFormatType_t headerType = convertTxCommunicationProfileToHeaderType(communicationProfile);

  // Construct the explorer header and pass on the frame.
  m_cExplorerFrameConstructorFunc[headerType](communicationProfile, pFrame);
  return m_cCommunicationProfileFunctions[communicationProfile](communicationProfile, pFrame);
}

/**@brief Module scope function for transmitting a Z-Wave Multicast frame.
 *
 * @details When this function return, the frame has been copied to the radio for transmission and
 *          the buffer can be released.
 *
 * @param[in] communicationProfile Communication profile to use when transmitting this frame
 * @param[in] pFrame               Pointer to the frame to be transmitted
 */
static ZW_ReturnCode_t llTransmitMulticastFrame(CommunicationProfile_t   communicationProfile,
                                                ZW_TransmissionFrame_t * pFrame)
{
  ZW_HeaderFormatType_t headerType = convertTxCommunicationProfileToHeaderType(communicationProfile);

  // Construct the explorer header and pass on the frame.
  m_cMulticastFrameConstructorFunc[headerType](communicationProfile, pFrame);

  return m_cCommunicationProfileFunctions[communicationProfile](communicationProfile, pFrame);
}

ZW_ReturnCode_t llTransmitFrame(CommunicationProfile_t   communicationProfile,
                                ZW_TransmissionFrame_t * pFrame)
{
  static uint8_t lastFrameStatus = 0;

//  DPRINT("mit\n");
  if (PROFILE_UNSUPPORTED == communicationProfile)
  {
    DPRINT("communicationProfile PROFILE_UNDEFINED\n");
    return UNSUPPORTED;
  }

  if (NULL == pFrame)
  {
    DPRINT("pFrame NULL\n");
    return INVALID_PARAMETERS;
  }

  pFrame->profile = PROFILE_UNSUPPORTED;  // pFrame->profile is not always used. Initialize to invalid parameter to detect error later!

  /*
   * Start of re-use workaround //////////////////////////////////////////////////////////
   *
   * The RF_PROFILE_3CH_100K_CH_A, RF_PROFILE_3CH_100K_CH_B and RF_PROFILE_3CH_100K_CH_C are for 3CH transmission,
   * but should not be used then-less it is to respond with an ACK to a reception.
   * These channel-specifying parameters do not have a dedicated transmit function. To reuse the existing transmit
   * functions, we will save the channel-specifying communicationProfile in the pFrame, and proceed with using
   * RF_PROFILE_3CH_100K to use the existing transmit function. In the transmit function we will then calculate
   * the PHY channel to use based on the saved communicationProfile in pFrame.
   */
  if ((communicationProfile == RF_PROFILE_3CH_100K_CH_A) ||
      (communicationProfile == RF_PROFILE_3CH_100K_CH_B) ||
      (communicationProfile == RF_PROFILE_3CH_100K_CH_C))
  {
    pFrame->profile = communicationProfile;
    communicationProfile = RF_PROFILE_3CH_100K;
    // The transmission is done based on frame-type below!
  }

  // 3CH BEAM
  if ((communicationProfile == RF_PROFILE_3CH_100K_WAKEUP_FRAGMENTED_A) ||
      (communicationProfile == RF_PROFILE_3CH_100K_WAKEUP_FRAGMENTED_B) ||
      (communicationProfile == RF_PROFILE_3CH_100K_WAKEUP_FRAGMENTED_C))
  {
    pFrame->profile = communicationProfile;
    communicationProfile = RF_PROFILE_3CH_100K_WAKEUP_FRAGMENTED;

    return m_cCommunicationProfileFunctions[communicationProfile](communicationProfile, pFrame);
  }
  // End of re-use workaround /////////////////////////////////////////////////////////////

  if ((communicationProfile == RF_PROFILE_100K_LR_A) ||
      (communicationProfile == RF_PROFILE_100K_LR_B))
  {
    pFrame->profile = communicationProfile;
  }

  // 2CH BEAM
  if ((communicationProfile == RF_PROFILE_40K_WAKEUP_1000) ||
      (communicationProfile == RF_PROFILE_40K_WAKEUP_250))
  {
    pFrame->profile = communicationProfile;
    return m_cCommunicationProfileFunctions[communicationProfile](communicationProfile, pFrame);
  }

  // LR BEAM
  if ((communicationProfile == RF_PROFILE_100K_LR_WAKEUP_FRAGMENTED_A) ||
      (communicationProfile == RF_PROFILE_100K_LR_WAKEUP_FRAGMENTED_B))
  {
    if ( (RETRANSMIT_FRAME & pFrame->status) && !(RETRANSMIT_FRAME & lastFrameStatus) )
    {
   	  updateLRFrameTXPower(pFrame);
    }
    lastFrameStatus = pFrame->status;

    pFrame->profile = communicationProfile;
    return m_cCommunicationProfileFunctions[communicationProfile](communicationProfile, pFrame);
  }

  // Do 2CH, 3CH and LR retransmission here
  if (pFrame->status & RETRANSMIT_FRAME)
  {
    zpal_radio_rf_channel_statistic_tx_retries();

    // Only 2CH re-transmissions here!
    if (!llIsHeaderFormat3ch() && (communicationProfile != RF_PROFILE_100K_LR_A) && (communicationProfile != RF_PROFILE_100K_LR_B))
    {
      pFrame->header.header.headerInfo &= ~(1 << SPEED_MODIFIED_2CH_FRAME_CONTROL_POS);
      pFrame->header.header.headerInfo |=
        (pFrame->frameOptions.speedModified << SPEED_MODIFIED_2CH_FRAME_CONTROL_POS);
      pFrame->header.header.reserved =
          (pFrame->frameOptions.sequenceNumber << SEQ_NO_2CH_FRAME_CONTROL_POS) |
          (pFrame->frameOptions.multicastfollowup << MULTICAST_FOLLOWUP_2CH_FRAME_CONTROL_POS);
    }
    else
    {
      // Only 3CH re-transmissions here!
      if (llIsHeaderFormat3ch())
      {
        pFrame->header.header3ch.sequenceNumber = pFrame->frameOptions.sequenceNumber;
      }
      else
      {
        // Only Long Range re-transmission here!
        pFrame->header.headerLR.sequenceNumber = pFrame->frameOptions.sequenceNumber;
        updateLRFrameTXPower(pFrame);
      }
    }
    return m_cCommunicationProfileFunctions[communicationProfile](communicationProfile, pFrame);
  }
  else
  {
    // Only new transmissions here! (no re-transmission)

    switch (pFrame->frameOptions.frameType)
    {
      case HDRTYP_SINGLECAST:
        return llTransmitSingleCastFrame(communicationProfile, pFrame);

      case HDRTYP_EXPLORE:
        return llTransmitExplorerFrame(communicationProfile, pFrame);

      case HDRTYP_TRANSFERACK:
        return llTransmitAckFrame(communicationProfile, pFrame);

      case HDRTYP_MULTICAST:
        return llTransmitMulticastFrame(communicationProfile, pFrame);

      case HDRTYP_ROUTED:
        return llTransmitSingleCastFrame(communicationProfile, pFrame);

      default:
        DPRINTF("frameType %02X\n", pFrame->frameOptions.frameType);
        return UNSUPPORTED;
    }
  }
}

/**Function for extracting the basic 2 channel frame header as part of frame processing
 *
 * @param[in,out] pReceiveFrame Pointer to the structure containing the raw frame data in
 *                              pReceiveFrame->frameOptions
 *                              The processed header content will be placed in the structure pointed
 *                              to by pReceiveFrame->frameContent
 */
static void extract2chHeader(ZW_ReceiveFrame_t * pReceiveFrame)
{
  memcpy(pReceiveFrame->frameOptions.homeId,
         &pReceiveFrame->frameContent[GENERAL_HEADER_2CH_HOME_ID_INDEX],
         sizeof(pReceiveFrame->frameOptions.homeId));

  pReceiveFrame->frameOptions.sourceNodeId =
      pReceiveFrame->frameContent[GENERAL_HEADER_2CH_SOURCE_INDEX];

  pReceiveFrame->frameOptions.routed =
      ((pReceiveFrame->frameContent[GENERAL_HEADER_2CH_FRAME_CONTROL_INDEX] & (1 << ROUTED_2CH_FRAME_CONTROL_POS)) >>
          ROUTED_2CH_FRAME_CONTROL_POS);

  pReceiveFrame->frameOptions.acknowledge =
      ((pReceiveFrame->frameContent[GENERAL_HEADER_2CH_FRAME_CONTROL_INDEX] & (1 << REQ_ACK_2CH_FRAME_CONTROL_POS)) >>
          REQ_ACK_2CH_FRAME_CONTROL_POS);

  pReceiveFrame->frameOptions.speedModified =
      ((pReceiveFrame->frameContent[GENERAL_HEADER_2CH_FRAME_CONTROL_INDEX] & (1 << SPEED_MODIFIED_2CH_FRAME_CONTROL_POS)) >>
          SPEED_MODIFIED_2CH_FRAME_CONTROL_POS);

  pReceiveFrame->frameOptions.multicastfollowup =
      ((pReceiveFrame->frameContent[GENERAL_HEADER_2CH_FRAME_CONTROL_INDEX+1] & (1 << MULTICAST_FOLLOWUP_2CH_FRAME_CONTROL_POS)) >>
          MULTICAST_FOLLOWUP_2CH_FRAME_CONTROL_POS);

  pReceiveFrame->frameOptions.wakeup1000ms =
      ((pReceiveFrame->frameContent[GENERAL_HEADER_2CH_FRAME_CONTROL_INDEX+1] & (1 << SOURCE_WAKEUP_1000MS_2CH_POS)) >>
          SOURCE_WAKEUP_1000MS_2CH_POS);

  pReceiveFrame->frameOptions.wakeup250ms =
      ((pReceiveFrame->frameContent[GENERAL_HEADER_2CH_FRAME_CONTROL_INDEX+1] & (1 << SOURCE_WAKEUP_250MS_2CH_POS)) >>
          SOURCE_WAKEUP_250MS_2CH_POS);

  pReceiveFrame->frameOptions.sequenceNumber =
      ((pReceiveFrame->frameContent[GENERAL_HEADER_2CH_FRAME_CONTROL_INDEX+1] & MASK_SEQNO_2CH) >>
          SEQ_NO_2CH_FRAME_CONTROL_POS);

  pReceiveFrame->frameOptions.destinationNodeId =
      pReceiveFrame->frameContent[GENERAL_HEADER_2CH_DESTINATION_INDEX];
}

/**Function for extracting the basic 3 channel frame header as part of frame processing
 *
 * @param[in,out] pReceiveFrame Pointer to the structure containing the raw frame data in
 *                              pReceiveFrame->frameOptions
 *                              The processed header content will be placed in the structure pointed
 *                              to by pReceiveFrame->frameContent
 */
static void extract3chHeader(ZW_ReceiveFrame_t * pReceiveFrame)
{
  memcpy(pReceiveFrame->frameOptions.homeId,
         &pReceiveFrame->frameContent[GENERAL_HEADER_3CH_HOME_ID_INDEX],
         sizeof(pReceiveFrame->frameOptions.homeId));

  pReceiveFrame->frameOptions.sourceNodeId =
      pReceiveFrame->frameContent[GENERAL_HEADER_3CH_SOURCE_INDEX];

  pReceiveFrame->frameOptions.acknowledge =
      ((pReceiveFrame->frameContent[GENERAL_HEADER_3CH_FRAME_CONTROL_INDEX] & (1 << REQ_ACK_3CH_FRAME_CONTROL_POS)) >>
          REQ_ACK_3CH_FRAME_CONTROL_POS);

  pReceiveFrame->frameOptions.multicastfollowup =
      ((pReceiveFrame->frameContent[GENERAL_HEADER_3CH_FRAME_CONTROL_INDEX] & (1 << MULTICAST_FOLLOWUP_3CH_FRAME_CONTROL_POS)) >>
          MULTICAST_FOLLOWUP_3CH_FRAME_CONTROL_POS);

  pReceiveFrame->frameOptions.wakeup1000ms =
      ((pReceiveFrame->frameContent[GENERAL_HEADER_3CH_EXTENDED_INDEX] & (1 << SOURCE_WAKEUP_1000MS_3CH_POS)) >>
          SOURCE_WAKEUP_1000MS_3CH_POS);

  pReceiveFrame->frameOptions.wakeup250ms =
      ((pReceiveFrame->frameContent[GENERAL_HEADER_3CH_EXTENDED_INDEX] & (1 << SOURCE_WAKEUP_250MS_3CH_POS)) >>
          SOURCE_WAKEUP_250MS_3CH_POS);

  pReceiveFrame->frameOptions.extended =
      ((pReceiveFrame->frameContent[GENERAL_HEADER_3CH_EXTENDED_INDEX] & (1 << EXTENDED_3CH_FRAME_EXTENDEDHEADER_POS)) >> EXTENDED_3CH_FRAME_EXTENDEDHEADER_POS);

  pReceiveFrame->frameOptions.sequenceNumber =
      pReceiveFrame->frameContent[GENERAL_HEADER_3CH_SEQUENCE_NO_INDEX];

  pReceiveFrame->frameOptions.destinationNodeId =
      pReceiveFrame->frameContent[GENERAL_HEADER_3CH_DESTINATION_INDEX];
}

/**Function for extracting the basic Long Range frame header as part of frame processing
 *
 * @param[in,out] pReceiveFrame Pointer to the structure containing the raw frame data in
 *                              pReceiveFrame->frameOptions
 *                              The processed header content will be placed in the structure pointed
 *                              to by pReceiveFrame->frameContent
 */
static void extractLRHeader(ZW_ReceiveFrame_t * pReceiveFrame)
{
  frame* fr = (frame *)pReceiveFrame->frameContent;

  memcpy(pReceiveFrame->frameOptions.homeId,
         &pReceiveFrame->frameContent[FRAME_HOME_ID_INDEX],
         sizeof(pReceiveFrame->frameOptions.homeId));

  pReceiveFrame->frameOptions.sourceNodeId = GET_SINGLECAST_SOURCE_NODEID_LR(*fr);

  pReceiveFrame->frameOptions.destinationNodeId = GET_SINGLECAST_DESTINATION_NODEID_LR(*fr);

  pReceiveFrame->frameOptions.acknowledge = DO_ACK_LR(*fr) ? 1 : 0;

  pReceiveFrame->frameOptions.extended = GET_EXTEND_PRESENT_LR(*fr);

  pReceiveFrame->frameOptions.sequenceNumber = GET_SEQNUMBER_LR(*fr);

  pReceiveFrame->frameOptions.noiseFloor = GET_NOISEFLOOR_LR(*fr);

  pReceiveFrame->frameOptions.txPower = GET_TXPOWER_LR(*fr);

}

/**@brief The is the actual function for handling of single cast frames when \ref llReceiveHandler
 *        has determined the frame type to be a Long Range single cast frame.
 *
 * @param[in] communicationProfile Communication profile used when receiving this frame
 * @param[in] pRxParameters Pointer to the structure with channel and rssi values
 * @param[in] pFrame        Pointer to the received frame. The frame is expected to be located in
 *                          Z-Wave stack reserved memory and allocated throughout lifetime of stack
 *                          processing. Application should copy payload data if required for
 *                          unsynchronized data processing.
 */
static void singlecastLRHandler(__attribute__((unused)) CommunicationProfile_t   communicationProfile,
                                __attribute__((unused)) zpal_radio_rx_parameters_t      * pRxParameters,
                                ZW_ReceiveFrame_t     * pFrame)
{
  uint32_t i;
  frame*   fr = (frame *)pFrame->frameContent;

  uint8_t headerLen = sizeof(frameHeaderSinglecastLR) - sizeof(frameHeaderExtensionLR);
  if (GET_EXTEND_PRESENT_LR(*fr))
  {
    headerLen +=  1 + (fr->singlecastLR.extension.extensionInfo & MASK_EXTENSION_LENGTH_LR);
  }


  for (i = 0; i < mSingleCastFilterIndex; i++ )
  {
    const ZW_ReceiveFilter_t * currentFilter = mSingleCastFilter[i];

    if (mSingleCastFilterPaused[i])
    {
      continue;
    }

    uint32_t* pHomeId = (uint32_t*)(&pFrame->frameContent[FRAME_HOME_ID_INDEX]);                  // Use pHomeId to avoid GCC type punned pointer error

    if ((currentFilter->flag & HOMEID_FILTER_FLAG) &&                                              // Check the filter is configured
        (currentFilter->homeId.word != *pHomeId))                                                  // If the home id does not match the filter, then continue looping with next filter
    {
      continue;
    }

    if ((currentFilter->flag & DESTINATION_NODE_ID_FILTER_FLAG) &&                                        // Check the filter is configured
        (currentFilter->destinationNodeId != GET_SINGLECAST_DESTINATION_NODEID_LR(*fr)))                  // If the destination node does not match the filter, then continue looping with next filter
    {
      continue;
    }

    if ((currentFilter->flag & SOURCE_NODE_ID_FILTER_FLAG) &&                                             // Check the filter is configured
        (currentFilter->sourceNodeId != GET_SINGLECAST_SOURCE_NODEID_LR(*fr)))                            // If the source node does not match the filter, then continue looping with next filter
    {
      continue;
    }

    if ((currentFilter->flag & PAYLOAD_INDEX_1_FILTER_FLAG) &&                                            // Filter is configured && (FilterIndex != OutOfBounds || FilterValue != MatchFrameContent)
          ((pFrame->frameContentLength <= (headerLen + currentFilter->payloadIndex1)) ||  // then discard frame for this filter
           (currentFilter->payloadFilterValue1 != pFrame->frameContent[headerLen + currentFilter->payloadIndex1])))
    {
      continue;
    }

    if ((currentFilter->flag & PAYLOAD_INDEX_2_FILTER_FLAG) &&                                            // Filter is configured && (FilterIndex != OutOfBounds || FilterValue != MatchFrameContent)
          ((pFrame->frameContentLength <= (headerLen + currentFilter->payloadIndex2)) ||  // then discard frame for this filter
           (currentFilter->payloadFilterValue2 != pFrame->frameContent[headerLen + currentFilter->payloadIndex2])))
    {
      continue;
    }

    pFrame->pPayloadStart = SINGLECAST_PAYLOAD_LR(fr);

    extractLRHeader(pFrame);

    pFrame->frameOptions.frameType = HDRTYP_SINGLECAST;

    // Invoke receive filter callback.
    currentFilter->frameHandler(pFrame);
    break;
  }
}

/**@brief The is the actual function for handling of single cast frames when \ref llReceiveHandler
 *        has determined the frame type to be a single cast frame.
 *
 * @param[in] communicationProfile Communication profile used when receiving this frame
 * @param[in] pRxParameters Pointer to the structure with channel and rssi values
 * @param[in] pFrame        Pointer to the received frame. The frame is expected to be located in
 *                          Z-Wave stack reserved memory and allocated throughout lifetime of stack
 *                          processing. Application should copy payload data if required for
 *                          unsynchronized data processing.
 */
static void singlecast3chHandler(__attribute__((unused)) CommunicationProfile_t   communicationProfile,
                                 __attribute__((unused)) zpal_radio_rx_parameters_t       * pRxParameters,
                                 ZW_ReceiveFrame_t      * pFrame)
{
  uint32_t i;
  uint8_t headerLen = GENERAL_HEADER_3CH_LENGTH;
  if ((pFrame->frameContent[GENERAL_HEADER_3CH_EXTENDED_INDEX] &
      (1<<EXTENDED_3CH_FRAME_EXTENDEDHEADER_POS)) >> EXTENDED_3CH_FRAME_EXTENDEDHEADER_POS)
  {
    headerLen += pFrame->frameContent[GENERAL_HEADER_3CH_LENGTH] & 0x07;
  }


  for (i = 0; i < mSingleCastFilterIndex; i++ )
  {
    const ZW_ReceiveFilter_t * currentFilter = mSingleCastFilter[i];

    if (mSingleCastFilterPaused[i])
    {
      continue;
    }

    uint32_t* pHomeId = (uint32_t*)(&pFrame->frameContent[FRAME_HOME_ID_INDEX]);                  // Use pHomeId to avoid GCC type punned pointer error

    if ((currentFilter->flag & HOMEID_FILTER_FLAG) &&                                              // Check the filter is configured
        (currentFilter->homeId.word != *pHomeId))                                                  // If the home id does not match the filter, then continue looping with next filter
    {
      continue;
    }

    if ((currentFilter->flag & DESTINATION_NODE_ID_FILTER_FLAG) &&                                        // Check the filter is configured
        (currentFilter->destinationNodeId != pFrame->frameContent[GENERAL_HEADER_3CH_DESTINATION_INDEX])) // If the destination node does not match the filter, then continue looping with next filter
    {
      continue;
    }

    if ((currentFilter->flag & SOURCE_NODE_ID_FILTER_FLAG) &&                                             // Check the filter is configured
        (currentFilter->sourceNodeId != pFrame->frameContent[GENERAL_HEADER_3CH_SOURCE_INDEX]))           // If the source node does not match the filter, then continue looping with next filter
    {
      continue;
    }

    if ((currentFilter->flag & PAYLOAD_INDEX_1_FILTER_FLAG) &&                                            // Filter is configured && (FilterIndex != OutOfBounds || FilterValue != MatchFrameContent)
          ((pFrame->frameContentLength <= (headerLen + currentFilter->payloadIndex1)) ||  // then discard frame for this filter
           (currentFilter->payloadFilterValue1 != pFrame->frameContent[headerLen + currentFilter->payloadIndex1])))
    {
      continue;
    }

    if ((currentFilter->flag & PAYLOAD_INDEX_2_FILTER_FLAG) &&                                            // Filter is configured && (FilterIndex != OutOfBounds || FilterValue != MatchFrameContent)
          ((pFrame->frameContentLength <= (headerLen + currentFilter->payloadIndex2)) ||  // then discard frame for this filter
           (currentFilter->payloadFilterValue2 != pFrame->frameContent[headerLen + currentFilter->payloadIndex2])))
    {
      continue;
    }

    pFrame->pPayloadStart = &pFrame->frameContent[GENERAL_HEADER_3CH_LENGTH];

    extract3chHeader(pFrame);
    pFrame->frameOptions.frameType = HDRTYP_SINGLECAST;

    // Invoke receive filter callback.
    currentFilter->frameHandler(pFrame);
    break;
  }
}

/**@brief The is the actual function for handling of single cast frames when \ref llReceiveHandler
 *        has determined the frame type to be a single cast frame.
 *
 * @param[in] communicationProfile Communication profile used when receiving this frame
 * @param[in] pRxParameters Pointer to the structure with channel and rssi values
 * @param[in] pFrame        Pointer to the received frame. The frame is expected to be located in
 *                          Z-Wave stack reserved memory and allocated throughout lifetime of stack
 *                          processing. Application should copy payload data if required for
 *                          unsynchronized data processing.
 */
static void singlecast2chHandler(__attribute__((unused)) CommunicationProfile_t   communicationProfile,
                                 __attribute__((unused)) zpal_radio_rx_parameters_t       * pRxParameters,
                                 ZW_ReceiveFrame_t      * pFrame)
{
  uint32_t i;

  for (i = 0; i < mSingleCastFilterIndex; i++ )
  {
    const ZW_ReceiveFilter_t * currentFilter = mSingleCastFilter[i];

    if (mSingleCastFilterPaused[i])
    {
      continue;
    }

    uint32_t* pHomeId = (uint32_t*)(&pFrame->frameContent[FRAME_HOME_ID_INDEX]);                  // Use pHomeId to avoid GCC type punned pointer error

    if ((currentFilter->flag & HOMEID_FILTER_FLAG) &&                                              // Check the filter is configured
        (currentFilter->homeId.word != *pHomeId))                                                  // If the home id does not match the filter, then continue looping with next filter
    {
      continue;
    }

    if ((currentFilter->flag & DESTINATION_NODE_ID_FILTER_FLAG) &&                                        // Check the filter is configured
        (currentFilter->destinationNodeId != pFrame->frameContent[GENERAL_HEADER_2CH_DESTINATION_INDEX])) // If the destination node does not match the filter, then continue looping with next filter
    {
      continue;
    }

    if ((currentFilter->flag & SOURCE_NODE_ID_FILTER_FLAG) &&                                             // Check the filter is configured
        (currentFilter->sourceNodeId != pFrame->frameContent[GENERAL_HEADER_2CH_SOURCE_INDEX]))           // If the source node does not match the filter, then continue looping with next filter
    {
      continue;
    }

    if ((currentFilter->flag & PAYLOAD_INDEX_1_FILTER_FLAG) &&                                            // Filter is configured && (FilterIndex != OutOfBounds || FilterValue != MatchFrameContent)
          ((pFrame->frameContentLength <= (GENERAL_HEADER_2CH_LENGTH + currentFilter->payloadIndex1)) ||  // then discard frame for this filter
           (currentFilter->payloadFilterValue1 != pFrame->frameContent[GENERAL_HEADER_2CH_LENGTH + currentFilter->payloadIndex1])))
    {
      continue;
    }

    if ((currentFilter->flag & PAYLOAD_INDEX_2_FILTER_FLAG) &&                                            // Filter is configured && (FilterIndex != OutOfBounds || FilterValue != MatchFrameContent)
          ((pFrame->frameContentLength <= (GENERAL_HEADER_2CH_LENGTH + currentFilter->payloadIndex2)) ||  // then discard frame for this filter
           (currentFilter->payloadFilterValue2 != pFrame->frameContent[GENERAL_HEADER_2CH_LENGTH + currentFilter->payloadIndex2])))
    {
      continue;
    }

    pFrame->pPayloadStart = &pFrame->frameContent[GENERAL_HEADER_2CH_LENGTH];

    extract2chHeader(pFrame);
    pFrame->frameOptions.frameType = HDRTYP_SINGLECAST;

    // Invoke receive filter callback.
    currentFilter->frameHandler(pFrame);
    break;
  }
}


/**@brief The is the actual function for handling of single cast routed frames when \ref llReceiveHandler
 *        has determined the frame type to be a single cast routed frame.
 *
 * @param[in] communicationProfile Communication profile used when receiving this frame
 * @param[in] pRxParameters Pointer to the structure with channel and rssi values
 * @param[in] pFrame        Pointer to the received frame. The frame is expected to be located in
 *                          Z-Wave stack reserved memory and allocated throughout lifetime of stack
 *                          processing. Application should copy payload data if required for
 *                          unsynchronized data processing.
 */
static void singlecastRouted2chHandler(__attribute__((unused)) CommunicationProfile_t communicationProfile,
                                       __attribute__((unused)) zpal_radio_rx_parameters_t * pRxParameters,
                                       ZW_ReceiveFrame_t * pFrame)
{
  uint32_t i;
  uint32_t routeHeaderLength = 1 + 1 + ((pFrame->frameContent[GENERAL_ROUTED_HEADER_2CH_CONTROL_INDEX] & 0xF0) >> 4);
  // TODO - routeHeaderLength also needs to be corrected for Extended route header if present (extend bit in routeStatus)

  for (i = 0; i < mSingleCastRoutedFilterIndex; i++ )
  {
    const ZW_ReceiveFilter_t * currentFilter = mSingleCastRoutedFilter[i];

    if (mSingleCastRoutedFilterPaused[i])
    {
      continue;
    }

    uint32_t* pHomeId = (uint32_t*)(&pFrame->frameContent[FRAME_HOME_ID_INDEX]);                  // Use pHomeId to avoid GCC type punned pointer error

    if ((currentFilter->flag & HOMEID_FILTER_FLAG) &&                                              // Check the filter is configured
        (currentFilter->homeId.word != *pHomeId))                                                  // If the home id does not match the filter, then continue looping with next filter
    {
      continue;
    }

    if ((currentFilter->flag & DESTINATION_NODE_ID_FILTER_FLAG) &&                                        // Check the filter is configured
        (currentFilter->destinationNodeId != pFrame->frameContent[GENERAL_HEADER_2CH_DESTINATION_INDEX])) // If the destination node does not match the filter, then continue looping with next filter
    {
      continue;
    }

    if ((currentFilter->flag & SOURCE_NODE_ID_FILTER_FLAG) &&                                             // Check the filter is configured
        (currentFilter->sourceNodeId != pFrame->frameContent[GENERAL_HEADER_2CH_SOURCE_INDEX]))           // If the source node does not match the filter, then continue looping with next filter
    {
      continue;
    }

    if ((currentFilter->flag & PAYLOAD_INDEX_1_FILTER_FLAG) &&                                            // Filter is configured && (FilterIndex != OutOfBounds || FilterValue != MatchFrameContent)
          ((pFrame->frameContentLength <= (GENERAL_HEADER_2CH_LENGTH + routeHeaderLength + currentFilter->payloadIndex1)) ||  // then discard frame for this filter
           (currentFilter->payloadFilterValue1 != pFrame->frameContent[GENERAL_HEADER_2CH_LENGTH + routeHeaderLength + currentFilter->payloadIndex1])))
    {
      continue;
    }

    if ((currentFilter->flag & PAYLOAD_INDEX_2_FILTER_FLAG) &&                                            // Filter is configured && (FilterIndex != OutOfBounds || FilterValue != MatchFrameContent)
          ((pFrame->frameContentLength <= (GENERAL_HEADER_2CH_LENGTH + routeHeaderLength + currentFilter->payloadIndex2)) ||  // then discard frame for this filter
           (currentFilter->payloadFilterValue2 != pFrame->frameContent[GENERAL_HEADER_2CH_LENGTH + routeHeaderLength + currentFilter->payloadIndex2])))
    {
      continue;
    }

    // TODO - currently we use ReceiveHandler for handling all frames except Explore frames
    pFrame->pPayloadStart = &pFrame->frameContent[GENERAL_HEADER_2CH_LENGTH];

    extract2chHeader(pFrame);
    // TODO - 2 channel - we should internally keep HDRTYP_ROUTED so we can handle routed frames separately in 2 channel also
    pFrame->frameOptions.frameType = HDRTYP_SINGLECAST;

    // Invoke receive filter callback.
    currentFilter->frameHandler(pFrame);
    break;
  }
}

/**@brief The is the actual function for handling of single cast routed frames when \ref llReceiveHandler
 *        has determined the frame type to be a single cast routed frame.
 *
 * @param[in] communicationProfile Communication profile used when receiving this frame
 * @param[in] pRxParameters Pointer to the structure with channel and rssi values
 * @param[in] pFrame        Pointer to the received frame. The frame is expected to be located in
 *                          Z-Wave stack reserved memory and allocated throughout lifetime of stack
 *                          processing. Application should copy payload data if required for
 *                          unsynchronized data processing.
 */
static void singlecastRouted3chHandler(__attribute__((unused)) CommunicationProfile_t communicationProfile,
                                       __attribute__((unused)) zpal_radio_rx_parameters_t * pRxParameters,
                                       ZW_ReceiveFrame_t * pFrame)
{
  uint32_t i;
  uint32_t routeHeaderLength = GENERAL_HEADER_3CH_LENGTH + 3 + ((pFrame->frameContent[GENERAL_ROUTED_HEADER_3CH_CONTROL_INDEX] & 0xF0) >> 4);
  // TODO - routeHeaderLength also needs to be corrected for Extended route header if present (extend bit in routeStatus)
  if ((pFrame->frameContent[GENERAL_HEADER_3CH_EXTENDED_INDEX] &
      (1<<EXTENDED_3CH_FRAME_EXTENDEDHEADER_POS)) >> EXTENDED_3CH_FRAME_EXTENDEDHEADER_POS)
  {
    routeHeaderLength += pFrame->frameContent[routeHeaderLength] & 0x07;
  }

  for (i = 0; i < mSingleCastRoutedFilterIndex; i++ )
  {
    const ZW_ReceiveFilter_t * currentFilter = mSingleCastRoutedFilter[i];

    if (mSingleCastRoutedFilterPaused[i])
    {
      continue;
    }

    uint32_t* pHomeId = (uint32_t*)(&pFrame->frameContent[FRAME_HOME_ID_INDEX]);                  // Use pHomeId to avoid GCC type punned pointer error

    if ((currentFilter->flag & HOMEID_FILTER_FLAG) &&                                              // Check the filter is configured
        (currentFilter->homeId.word != *pHomeId))                                                  // If the home id does not match the filter, then continue looping with next filter
    {
      continue;
    }

    if ((currentFilter->flag & DESTINATION_NODE_ID_FILTER_FLAG) &&                                        // Check the filter is configured
        (currentFilter->destinationNodeId != pFrame->frameContent[GENERAL_HEADER_3CH_DESTINATION_INDEX])) // If the destination node does not match the filter, then continue looping with next filter
    {
      continue;
    }

    if ((currentFilter->flag & SOURCE_NODE_ID_FILTER_FLAG) &&                                             // Check the filter is configured
        (currentFilter->sourceNodeId != pFrame->frameContent[GENERAL_HEADER_3CH_SOURCE_INDEX]))           // If the source node does not match the filter, then continue looping with next filter
    {
      continue;
    }

    if ((currentFilter->flag & PAYLOAD_INDEX_1_FILTER_FLAG) &&                                            // Filter is configured && (FilterIndex != OutOfBounds || FilterValue != MatchFrameContent)
          ((pFrame->frameContentLength <= (routeHeaderLength + currentFilter->payloadIndex1)) ||  // then discard frame for this filter
           (currentFilter->payloadFilterValue1 != pFrame->frameContent[routeHeaderLength + currentFilter->payloadIndex1])))
    {
      continue;
    }

    if ((currentFilter->flag & PAYLOAD_INDEX_2_FILTER_FLAG) &&                                            // Filter is configured && (FilterIndex != OutOfBounds || FilterValue != MatchFrameContent)
          ((pFrame->frameContentLength <= (routeHeaderLength + currentFilter->payloadIndex2)) ||  // then discard frame for this filter
           (currentFilter->payloadFilterValue2 != pFrame->frameContent[routeHeaderLength + currentFilter->payloadIndex2])))
    {
      continue;
    }

    // TODO - currently we use ReceiveHandler for handling all frames except Explore frames
    pFrame->pPayloadStart = &pFrame->frameContent[GENERAL_HEADER_3CH_LENGTH];

    extract3chHeader(pFrame);
    pFrame->frameOptions.frameType = HDRTYP_ROUTED;
    pFrame->frameOptions.routed = 1;

    // Invoke receive filter callback.
    currentFilter->frameHandler(pFrame);
    break;
  }
}

/**@brief The is the actual function for handling of explorer frames when \ref llReceiveHandler
 *        has determined the frame type to be an explorer frame.
 *
 * @param[in] communicationProfile Communication profile used when receiving this frame
 * @param[in] pRxParameters Pointer to the structure with channel and rssi values
 * @param[in] pFrame        Pointer to the received frame. The frame is expected to be located in
 *                          Z-Wave stack reserved memory and allocated throughout lifetime of stack
 *                          processing. Application should copy payload data if required for
 *                          unsynchronized data processing.
 */
static void explorerFrame2chHandler(__attribute__((unused)) CommunicationProfile_t communicationProfile,
                                    __attribute__((unused)) zpal_radio_rx_parameters_t * pRxParameters,
                                    ZW_ReceiveFrame_t * pFrame)
{
  uint32_t i;

  for (i = 0; i < mExplorerFrameFilterIndex; i++ )
  {
    const ZW_ReceiveFilter_t * currentFilter = mExplorerFrameFilter[i];

    if (mExplorerFrameFilterPaused[i])
    {
      continue;
    }

    uint32_t* pHomeId = (uint32_t*)(&pFrame->frameContent[FRAME_HOME_ID_INDEX]);                         // Use pHomeId to avoid GCC type punned pointer error

    if ((currentFilter->flag & HOMEID_FILTER_FLAG) &&                                                     // Check the filter is configured
        (currentFilter->homeId.word != *pHomeId))                                                         // If the home id does not match the filter, then continue looping with next filter
    {
      continue;
    }

    if ((currentFilter->flag & DESTINATION_NODE_ID_FILTER_FLAG) &&                                        // Check the filter is configured
        (currentFilter->destinationNodeId != pFrame->frameContent[GENERAL_HEADER_2CH_DESTINATION_INDEX])) // If the destination node does not match the filter, then continue looping with next filter
    {
      continue;
    }

    if ((currentFilter->flag & SOURCE_NODE_ID_FILTER_FLAG) &&                                             // Check the filter is configured
        (currentFilter->sourceNodeId != pFrame->frameContent[GENERAL_HEADER_2CH_SOURCE_INDEX]))           // If the source node does not match the filter, then continue looping with next filter
    {
      continue;
    }

    pFrame->pPayloadStart = &pFrame->frameContent[GENERAL_HEADER_2CH_LENGTH];

    extract2chHeader(pFrame);
    pFrame->frameOptions.frameType = HDRTYP_EXPLORE;

    // Invoke receive filter callback.
    currentFilter->frameHandler(pFrame);
    break;
  }
}

/**@brief The is the actual function for handling of explorer frames when \ref llReceiveHandler
 *        has determined the frame type to be an explorer frame.
 *
 * @param[in] communicationProfile Communication profile used when receiving this frame
 * @param[in] pRxParameters Pointer to the structure with channel and rssi values
 * @param[in] pFrame        Pointer to the received frame. The frame is expected to be located in
 *                          Z-Wave stack reserved memory and allocated throughout lifetime of stack
 *                          processing. Application should copy payload data if required for
 *                          unsynchronized data processing.
 */
static void explorerFrame3chHandler(__attribute__((unused)) CommunicationProfile_t communicationProfile,
                                    __attribute__((unused)) zpal_radio_rx_parameters_t * pRxParameters,
                                    ZW_ReceiveFrame_t * pFrame)
{
  uint32_t i;

  for (i = 0; i < mExplorerFrameFilterIndex; i++ )
  {
    const ZW_ReceiveFilter_t * currentFilter = mExplorerFrameFilter[i];

    if (mExplorerFrameFilterPaused[i])
    {
      continue;
    }

    uint32_t* pHomeId = (uint32_t*)(&pFrame->frameContent[FRAME_HOME_ID_INDEX]);                         // Use pHomeId to avoid GCC type punned pointer error

    if ((currentFilter->flag & HOMEID_FILTER_FLAG) &&                                                     // Check the filter is configured
        (currentFilter->homeId.word != *pHomeId))                                                         // If the home id does not match the filter, then continue looping with next filter
    {
      continue;
    }

    if ((currentFilter->flag & DESTINATION_NODE_ID_FILTER_FLAG) &&                                        // Check the filter is configured
        (currentFilter->destinationNodeId != pFrame->frameContent[GENERAL_HEADER_3CH_DESTINATION_INDEX])) // If the destination node does not match the filter, then continue looping with next filter
    {
      continue;
    }

    if ((currentFilter->flag & SOURCE_NODE_ID_FILTER_FLAG) &&                                             // Check the filter is configured
        (currentFilter->sourceNodeId != pFrame->frameContent[GENERAL_HEADER_3CH_SOURCE_INDEX]))           // If the source node does not match the filter, then continue looping with next filter
    {
      continue;
    }

    pFrame->pPayloadStart = &pFrame->frameContent[GENERAL_HEADER_3CH_LENGTH];

    extract3chHeader(pFrame);
    pFrame->frameOptions.frameType = HDRTYP_EXPLORE;

    // Invoke receive filter callback.
    currentFilter->frameHandler(pFrame);
    break;
  }
}

/**@brief The is the actual function for handling of multicast frames when \ref llReceiveHandler
 *        has determined the frame type to be a multicast frame.
 *
 * @param[in] communicationProfile Communication profile used when receiving this frame
 * @param[in] pRxParameters Pointer to the structure with channel and rssi values
 * @param[in] pFrame        Pointer to the received frame. The frame is expected to be located in
 *                          Z-Wave stack reserved memory and allocated throughout lifetime of stack
 *                          processing. Application should copy payload data if required for
 *                          unsynchronized data processing.
 */
static void multicastFrame2chHandler(__attribute__((unused)) CommunicationProfile_t communicationProfile,
                                     __attribute__((unused)) zpal_radio_rx_parameters_t * pRxParameters,
                                     ZW_ReceiveFrame_t * pFrame)
{
  uint32_t i;

  for (i = 0; i < mMulticastFrameFilterIndex; i++ )
  {
    const ZW_ReceiveFilter_t * currentFilter = mMulticastFrameFilter[i];

    if (mMulticastFrameFilterPaused[i])
    {
      continue;
    }

    uint32_t* pHomeId = (uint32_t*)(&pFrame->frameContent[FRAME_HOME_ID_INDEX]);                         // Use pHomeId to avoid GCC type punned pointer error

    if ((currentFilter->flag & HOMEID_FILTER_FLAG) &&                                                     // Check the filter is configured
        (currentFilter->homeId.word != *pHomeId))                                                         // If the home id does not match the filter, then continue looping with next filter
    {
      continue;
    }

    if ((currentFilter->flag & SOURCE_NODE_ID_FILTER_FLAG) &&                                             // Check the filter is configured
        (currentFilter->sourceNodeId != pFrame->frameContent[GENERAL_HEADER_2CH_SOURCE_INDEX]))           // If the source node does not match the filter, then continue looping with next filter
    {
      continue;
    }

    if ((currentFilter->flag & PAYLOAD_INDEX_1_FILTER_FLAG) &&                                            // Filter is configured && (FilterIndex != OutOfBounds || FilterValue != MatchFrameContent)
          ((pFrame->frameContentLength <= (MULTICAST_HEADER_2CH_LENGTH + currentFilter->payloadIndex1)) ||// then discard frame for this filter
           (currentFilter->payloadFilterValue1 != pFrame->frameContent[GENERAL_HEADER_2CH_LENGTH + currentFilter->payloadIndex1])))
    {
      continue;
    }

    if ((currentFilter->flag & PAYLOAD_INDEX_2_FILTER_FLAG) &&                                            // Filter is configured && (FilterIndex != OutOfBounds || FilterValue != MatchFrameContent)
          ((pFrame->frameContentLength <= (MULTICAST_HEADER_2CH_LENGTH + currentFilter->payloadIndex2)) ||// then discard frame for this filter
           (currentFilter->payloadFilterValue2 != pFrame->frameContent[GENERAL_HEADER_2CH_LENGTH + currentFilter->payloadIndex2])))
    {
      continue;
    }

    pFrame->pPayloadStart = &pFrame->frameContent[GENERAL_HEADER_2CH_LENGTH];

    extract2chHeader(pFrame);
    pFrame->frameOptions.frameType = HDRTYP_MULTICAST;

    // Invoke receive filter callback.
    currentFilter->frameHandler(pFrame);
    break;
  }
}

/**@brief The is the actual function for handling of multicast frames when \ref llReceiveHandler
 *        has determined the frame type to be a multicast frame.
 *
 * @param[in] communicationProfile Communication profile used when receiving this frame
 * @param[in] pRxParameters Pointer to the structure with channel and rssi values
 * @param[in] pFrame        Pointer to the received frame. The frame is expected to be located in
 *                          Z-Wave stack reserved memory and allocated throughout lifetime of stack
 *                          processing. Application should copy payload data if required for
 *                          unsynchronized data processing.
 */
static void multicastFrame3chHandler(__attribute__((unused)) CommunicationProfile_t communicationProfile,
                                     __attribute__((unused)) zpal_radio_rx_parameters_t * pRxParameters,
                                     ZW_ReceiveFrame_t * pFrame)
{
  uint32_t i;

  for (i = 0; i < mMulticastFrameFilterIndex; i++ )
  {
    const ZW_ReceiveFilter_t * currentFilter = mMulticastFrameFilter[i];

    if (mMulticastFrameFilterPaused[i])
    {
      continue;
    }

    uint32_t* pHomeId = (uint32_t*)(&pFrame->frameContent[FRAME_HOME_ID_INDEX]);                         // Use pHomeId to avoid GCC type punned pointer error

    if ((currentFilter->flag & HOMEID_FILTER_FLAG) &&                                                     // Check the filter is configured
        (currentFilter->homeId.word != *pHomeId))                                                         // If the home id does not match the filter, then continue looping with next filter
    {
      continue;
    }

    if ((currentFilter->flag & SOURCE_NODE_ID_FILTER_FLAG) &&                                             // Check the filter is configured
        (currentFilter->sourceNodeId != pFrame->frameContent[GENERAL_HEADER_3CH_SOURCE_INDEX]))           // If the source node does not match the filter, then continue looping with next filter
    {
      continue;
    }

    if ((currentFilter->flag & PAYLOAD_INDEX_1_FILTER_FLAG) &&                                            // Filter is configured && (FilterIndex != OutOfBounds || FilterValue != MatchFrameContent)
          ((pFrame->frameContentLength <= (MULTICAST_HEADER_3CH_LENGTH + currentFilter->payloadIndex1)) ||// then discard frame for this filter
           (currentFilter->payloadFilterValue1 != pFrame->frameContent[MULTICAST_HEADER_3CH_LENGTH + currentFilter->payloadIndex1])))
    {
      continue;
    }

    if ((currentFilter->flag & PAYLOAD_INDEX_2_FILTER_FLAG) &&                                            // Filter is configured && (FilterIndex != OutOfBounds || FilterValue != MatchFrameContent)
          ((pFrame->frameContentLength <= (MULTICAST_HEADER_3CH_LENGTH + currentFilter->payloadIndex2)) ||// then discard frame for this filter
           (currentFilter->payloadFilterValue2 != pFrame->frameContent[MULTICAST_HEADER_3CH_LENGTH + currentFilter->payloadIndex2])))
    {
      continue;
    }

    pFrame->pPayloadStart = &pFrame->frameContent[MULTICAST_HEADER_3CH_LENGTH];

    extract3chHeader(pFrame);
    pFrame->frameOptions.frameType = HDRTYP_MULTICAST;

    // Invoke receive filter callback.
    currentFilter->frameHandler(pFrame);
    break;
  }
}

// Dummy function for catching non-applicable receive frame types
static void notApplicableFrameHandler(__attribute__((unused)) CommunicationProfile_t communicationProfile,
                                      __attribute__((unused)) zpal_radio_rx_parameters_t * pRxParameters,
                                      __attribute__((unused)) ZW_ReceiveFrame_t * pFrame)
{
  DPRINTF("\r\nNon-applicable receive frame type, communicationProfile=0x%x", communicationProfile);
  ASSERT(false);
}


/**@brief The is the actual function for handling of acknowledge frame when \ref llReceiveHandler
 *        has determined the frame type to be an acknowledge frame.
 *
 * @param[in] communicationProfile Communication profile used when receiving this frame
 * @param[in] pRxParameters Pointer to the structure with channel and rssi values
 * @param[in] pFrame        Pointer to the received frame. The frame is expected to be located in
 *                          Z-Wave stack reserved memory and allocated throughout lifetime of stack
 *                          processing. Application should copy payload data if required for
 *                          unsynchronized data processing.
 */
static void ackFrame2chHandler( __attribute__((unused)) CommunicationProfile_t communicationProfile,
                                __attribute__((unused)) zpal_radio_rx_parameters_t * pRxParameters,
                                ZW_ReceiveFrame_t * pFrame)
{
  uint32_t i;

  for (i = 0; i < mTransferAckFilterIndex; i++ )
  {
    const ZW_ReceiveFilter_t * currentFilter = mTransferAckFilter[i];

    if (mTransferAckFilterPaused[i])
    {
      continue;
    }

    uint32_t* pHomeId = (uint32_t*)(&pFrame->frameContent[FRAME_HOME_ID_INDEX]);    // Use pHomeId to avoid GCC type punned pointer error

    if ((currentFilter->flag & HOMEID_FILTER_FLAG) &&
        (currentFilter->homeId.word != *pHomeId))
    {
      continue;
    }

    if ((currentFilter->flag & DESTINATION_NODE_ID_FILTER_FLAG) &&
        (currentFilter->destinationNodeId != pFrame->frameContent[GENERAL_HEADER_2CH_DESTINATION_INDEX]))
    {
      continue;
    }

    if ((currentFilter->flag & SOURCE_NODE_ID_FILTER_FLAG) &&
        (currentFilter->sourceNodeId != pFrame->frameContent[GENERAL_HEADER_2CH_SOURCE_INDEX]))
    {
      continue;
    }

    pFrame->pPayloadStart = &pFrame->frameContent[GENERAL_HEADER_2CH_LENGTH];

    extract2chHeader(pFrame);
    pFrame->frameOptions.frameType = HDRTYP_TRANSFERACK;

    // Invoke receive filter callback.
    currentFilter->frameHandler(pFrame);
    break;
  }
}


/**@brief The is the actual function for handling of acknowledge frame when \ref llReceiveHandler
 *        has determined the frame type to be an acknowledge frame.
 *
 * @param[in] communicationProfile Communication profile used when receiving this frame
 * @param[in] pRxParameters Pointer to the structure with channel and rssi values
 * @param[in] pFrame        Pointer to the received frame. The frame is expected to be located in
 *                          Z-Wave stack reserved memory and allocated throughout lifetime of stack
 *                          processing. Application should copy payload data if required for
 *                          unsynchronized data processing.
 */
static void ackFrame3chHandler( __attribute__((unused)) CommunicationProfile_t communicationProfile,
                                __attribute__((unused)) zpal_radio_rx_parameters_t * pRxParameters,
                                ZW_ReceiveFrame_t * pFrame)
{
  uint32_t i;

  for (i = 0; i < mTransferAckFilterIndex; i++ )
  {
    const ZW_ReceiveFilter_t * currentFilter = mTransferAckFilter[i];

    if (mTransferAckFilterPaused[i])
    {
      continue;
    }

    uint32_t* pHomeId = (uint32_t*)(&pFrame->frameContent[FRAME_HOME_ID_INDEX]);      // Use pHomeId to avoid GCC type punned pointer error

    if ((currentFilter->flag & HOMEID_FILTER_FLAG) &&
        (currentFilter->homeId.word != *pHomeId))
    {
      continue;
    }

    if ((currentFilter->flag & DESTINATION_NODE_ID_FILTER_FLAG) &&
        (currentFilter->destinationNodeId != pFrame->frameContent[GENERAL_HEADER_3CH_DESTINATION_INDEX]))
    {
      continue;
    }

    if ((currentFilter->flag & SOURCE_NODE_ID_FILTER_FLAG) &&
        (currentFilter->sourceNodeId != pFrame->frameContent[GENERAL_HEADER_3CH_SOURCE_INDEX]))
    {
      continue;
    }

    pFrame->pPayloadStart = &pFrame->frameContent[GENERAL_HEADER_3CH_LENGTH];

    extract3chHeader(pFrame);
    pFrame->frameOptions.frameType = HDRTYP_TRANSFERACK;

    // Invoke receive filter callback.
    currentFilter->frameHandler(pFrame);
    break;
  }
}

/**@brief The is the actual function for handling of Long Range acknowledge frame when \ref llReceiveHandler
 *        has determined the frame type to be an acknowledge frame.
 *
 * @param[in] communicationProfile Communication profile used when receiving this frame
 * @param[in] pRxParameters Pointer to the structure with channel and rssi values
 * @param[in] pFrame        Pointer to the received frame. The frame is expected to be located in
 *                          Z-Wave stack reserved memory and allocated throughout lifetime of stack
 *                          processing. Application should copy payload data if required for
 *                          unsynchronized data processing.
 */
static void ackFrameLRHandler(__attribute__((unused)) CommunicationProfile_t communicationProfile,
                              __attribute__((unused)) zpal_radio_rx_parameters_t * pRxParameters,
                              ZW_ReceiveFrame_t * pFrame)
{
  uint32_t i;
  frame*   fr = (frame *)pFrame->frameContent;

  for (i = 0; i < mTransferAckFilterIndex; i++ )
  {
    const ZW_ReceiveFilter_t * currentFilter = mTransferAckFilter[i];

    if (mTransferAckFilterPaused[i])
    {
      continue;
    }

    uint32_t* pHomeId = (uint32_t*)(&pFrame->frameContent[FRAME_HOME_ID_INDEX]);      // Use pHomeId to avoid GCC type punned pointer error

    if ((currentFilter->flag & HOMEID_FILTER_FLAG) &&
        (currentFilter->homeId.word != *pHomeId))
    {
      continue;
    }

    if ((currentFilter->flag & DESTINATION_NODE_ID_FILTER_FLAG) &&
        (currentFilter->destinationNodeId != pFrame->frameContent[GENERAL_HEADER_3CH_DESTINATION_INDEX]))
    {
      continue;
    }

    if ((currentFilter->flag & SOURCE_NODE_ID_FILTER_FLAG) &&
        (currentFilter->sourceNodeId != pFrame->frameContent[GENERAL_HEADER_3CH_SOURCE_INDEX]))
    {
      continue;
    }

    pFrame->pPayloadStart = SINGLECAST_PAYLOAD_LR(fr);

    extractLRHeader(pFrame);

    pFrame->frameOptions.frameType = HDRTYP_TRANSFERACK;

    // Invoke receive filter callback.
    currentFilter->frameHandler(pFrame);
    break;
  }
}


static CommunicationProfile_t convertCommunicationProfile(zpal_radio_rx_parameters_t * pRxParameters)
{
  if (pRxParameters->channel_id >= (sizeof(mCommunicationProfileActive)/sizeof(CommunicationProfile_t)))
    return PROFILE_UNSUPPORTED;

  return mCommunicationProfileActive[pRxParameters->channel_id];
}

static ZW_HeaderFormatType_t ConvertHeaderFormat(zpal_radio_header_type_t headerFormat)
{
  switch (headerFormat)
  {
    case ZPAL_RADIO_HEADER_TYPE_2CH:
      return HDRFORMATTYP_2CH;
    case ZPAL_RADIO_HEADER_TYPE_3CH:
      return HDRFORMATTYP_3CH;
    case ZPAL_RADIO_HEADER_TYPE_LR:
      return HDRFORMATTYP_LR;
    case ZPAL_RADIO_HEADER_TYPE_UNDEFINED:
    default:
      return HDRFORMATTYP_UNDEFINED;
  }
}

static CommunicationProfile_t ConvertRadioSpeedToCommunicationProfile(zpal_radio_speed_t speed)
{
  switch(speed)
  {
    case ZPAL_RADIO_SPEED_9600:
      return RF_PROFILE_9_6K;
    case ZPAL_RADIO_SPEED_40K:
      return RF_PROFILE_40K;
    case ZPAL_RADIO_SPEED_100K:
      return RF_PROFILE_100K;
    case ZPAL_RADIO_SPEED_100KLR:
      return RF_PROFILE_100K_LR_A;
    default:
      return PROFILE_UNSUPPORTED;
  }
}

ZW_WEAK
void radioFrameReceiveHandler(zpal_radio_rx_parameters_t * pRxParameters, zpal_radio_receive_frame_t * pZpalFrame)
{
  CommunicationProfile_t communicationProfile = convertCommunicationProfile(pRxParameters);
  ZW_ReceiveFrame_t receiveFrame = {
    .profile = ConvertRadioSpeedToCommunicationProfile(pRxParameters->speed),
    .channelId = pRxParameters->channel_id,
    .channelHeaderFormat = ConvertHeaderFormat(pRxParameters->channel_header_format),
    .rssi = pRxParameters->rssi,
    .frameContentLength = pZpalFrame->frame_content_length,
    .frameContent = pZpalFrame->frame_content
  };
  uint8_t frameType = GetHeaderType(receiveFrame.channelHeaderFormat, (frame *)&receiveFrame.frameContent[0]);

#ifdef DO_CHECKSUM_CHECK
  if (!llIsHeaderFormat3ch())
  {
    if  (pRxParameters->channelId > 0)
    {
      if (doLRCCheck(receiveFrame.frameContentLength - 1, receiveFrame.frameContent) != receiveFrame.frameContent[receiveFrame.frameContentLength - 1])
      {
        dataLinkLayerStatisticRxFailedLRC++;
        DPRINTF("Rx(%d) 9.6kb/40kb wrong LRC %d\n", pRxParameters->channelId, dataLinkLayerStatisticRxFailedLRC);
        return;
      }
    }
    else
    {
      if (!doCrc16Check(((receiveFrame.frameContent[receiveFrame.frameContentLength - 2] << 8) + receiveFrame.frameContent[receiveFrame.frameContentLength - 1]), receiveFrame.frameContentLength - 2, receiveFrame.frameContent))
      {
        dataLinkLayerStatisticRxFailedCRC++;
        DPRINTF("Rx(0) 100kb wrong CRC16 %d\n", dataLinkLayerStatisticRxFailedCRC);
        return;
      }
    }
  }
  else
  {
    if (!doCrc16Check(((receiveFrame.frameContent[receiveFrame.frameContentLength - 2] << 8) + receiveFrame.frameContent[receiveFrame.frameContentLength - 1]), receiveFrame.frameContentLength - 2, receiveFrame.frameContent))
    {
      dataLinkLayerStatisticRxFailedCRC++;
      DPRINTF("Rx(%d) 100kb wrong CRC16 %d\n", pRxParameters->channelId, dataLinkLayerStatisticRxFailedCRC);
      return;
    }
  }
#endif

  // Determine if 2 Channel singlecast frame is routed
  if ((HDRTYP_SINGLECAST == frameType) && (receiveFrame.channelHeaderFormat == HDRFORMATTYP_2CH))
  {
    // 2 channel AND HDRTYP_SINGLECAST - check if routed bit set
    if (0 != (receiveFrame.frameContent[GENERAL_HEADER_2CH_FRAME_CONTROL_INDEX] & (1 << ROUTED_2CH_FRAME_CONTROL_POS)))
    {
      // 2 channel singleCast Routed detected
      frameType = HDRTYP_ROUTED;
    }

  }
  DPRINTF("\r\nradioFrameReceiveHandler: frameType=0x%x, communicationProfile=0x%x, channelHeaderFormat=0x%x\r\n", frameType, communicationProfile, receiveFrame.channelHeaderFormat);
  ASSERT(receiveFrame.channelHeaderFormat != HDRFORMATTYP_UNDEFINED);

  /*
   * TODO remove: When a slave node is included with US_LR, it will change to either ZPAL_RADIO_LR_CH_CFG_NO_LR or
   * ZPAL_RADIO_LR_CH_CFG3 so it should not be possible to receive on a wrong channel anymore. This check might in
   * future work need to be removed.
   */
  /**
   * Will check that the packet is of the same CH-configuration as this slave node is included with.
   * (e.g. if the node is included as classic Z-Wave, then LR frames are dropped here.)
   */
  if (!IsReceivedFrameMatchNodeConfig(receiveFrame.channelHeaderFormat))
  {
    return;
  }
  switch (frameType)
  {
    case HDRTYP_SINGLECAST:
      m_cSingleCastHandlerFunc[receiveFrame.channelHeaderFormat](communicationProfile, pRxParameters, &receiveFrame);
      break;

    case HDRTYP_ROUTED:
      m_cSingleCastRoutedHandlerFunc[receiveFrame.channelHeaderFormat](communicationProfile, pRxParameters, &receiveFrame);
      break;

    case HDRTYP_EXPLORE:
      m_cExplorerHandlerFunc[receiveFrame.channelHeaderFormat](communicationProfile, pRxParameters, &receiveFrame);
      break;

    case HDRTYP_TRANSFERACK:
      m_cTransferAckHandlerFunc[receiveFrame.channelHeaderFormat](communicationProfile, pRxParameters, &receiveFrame);
      break;

    case HDRTYP_MULTICAST:
      m_cMultiCastHandlerFunc[receiveFrame.channelHeaderFormat](communicationProfile, pRxParameters, &receiveFrame);
      break;

    default:
      break;
  }
}


/**Function for comparing two ZW_ReceiveFilter_t
 *
 * @param[in] pFilter0 Pointer to filter 0 used in comparison
 * @param[in] pFilter1 Pointer to filter 1 used in comparison
 *
 * @retval 0 when the content of the to filters are identical
 * @retval 1 when the content of the to filters differs
 */
static uint32_t filterCompare(const ZW_ReceiveFilter_t * pFilter0, const ZW_ReceiveFilter_t * pFilter1)
{
  if (pFilter0->flag != pFilter1->flag)
  {
    return 1;
  }

  if ((pFilter0->flag & HOMEID_FILTER_FLAG) &&
      (pFilter0->homeId.word != pFilter1->homeId.word))
  {
    return 1;
  }

  if ((pFilter0->flag & DESTINATION_NODE_ID_FILTER_FLAG) &&
      (pFilter0->destinationNodeId != pFilter1->destinationNodeId))
  {
    return 1;
  }

  if ((pFilter0->flag & SOURCE_NODE_ID_FILTER_FLAG) &&
      (pFilter0->sourceNodeId != pFilter1->sourceNodeId))
  {
    return 1;
  }

  if ((pFilter0->flag & PAYLOAD_INDEX_1_FILTER_FLAG) &&
      (pFilter0->payloadIndex1 != pFilter1->payloadIndex1))
  {
    return 1;
  }

  if ((pFilter0->flag & PAYLOAD_INDEX_1_FILTER_FLAG) &&
      (pFilter0->payloadFilterValue1 != pFilter1->payloadFilterValue1))
  {
    return 1;
  }

  if ((pFilter0->flag & PAYLOAD_INDEX_2_FILTER_FLAG) &&
      (pFilter0->payloadIndex2 != pFilter1->payloadIndex2))
  {
    return 1;
  }

  if ((pFilter0->flag & PAYLOAD_INDEX_2_FILTER_FLAG) &&
      (pFilter0->payloadFilterValue2 != pFilter1->payloadFilterValue2))
  {
    return 1;
  }

  if (pFilter0->frameHandler != pFilter1->frameHandler)
  {
    return 1;
  }

  return 0;
}


/**@brief Function for adding a receive filter to a specific filter list.
 *
 * @param[in]     pReceiveFilter Pointer  to the filter to be active
 * @param[in,out] pFilterList    Pointer to the filter list where the new filter should be added.
 *                               When the function returns the filter list contains the new filter
 * @param[in,out] pFilterIndex   Current last index in the filter list wit free slot for a filter.
 *                               When the function returns the filter index has been incremented by
 *                               one
 */
static void receiveFilterAdd(const ZW_ReceiveFilter_t * pReceiveFilter,
                             const ZW_ReceiveFilter_t ** pFilterList,
                             uint32_t           * pFilterListIndex)
{
  uint32_t compareIndex = *pFilterListIndex;
  for (; compareIndex > 0; --compareIndex)
  {
    if (pFilterList[compareIndex - 1]->flag < pReceiveFilter->flag)
    {
      pFilterList[compareIndex] = pFilterList[compareIndex - 1];
    }
    else
    {
      pFilterList[compareIndex] = pReceiveFilter;
      break;
    }
  }

  if (0 == compareIndex)
  {
    pFilterList[compareIndex] = pReceiveFilter;
  }

  (*pFilterListIndex)++;
}


ZW_ReturnCode_t llReceiveFilterAdd(const ZW_ReceiveFilter_t * pReceiveFilter)
{
  ZW_FrameType_t frameType = pReceiveFilter->headerType;
  switch (frameType)
  {
    case HDRTYP_SINGLECAST:
      // ToDo: Should we use the function pointer and lookup table for 2ch/3ch handling ?
      if (mSingleCastFilterIndex >= PACKET_FILTER_SIZE)
        return NO_MEMORY;

      receiveFilterAdd(pReceiveFilter, mSingleCastFilter, &mSingleCastFilterIndex);
      return SUCCESS;

    case HDRTYP_ROUTED:
      if (mSingleCastRoutedFilterIndex >= PACKET_FILTER_SIZE)
        return NO_MEMORY;

      receiveFilterAdd(pReceiveFilter, mSingleCastRoutedFilter, &mSingleCastRoutedFilterIndex);
      return SUCCESS;

    case HDRTYP_EXPLORE:
      if (mExplorerFrameFilterIndex >= PACKET_FILTER_SIZE)
        return NO_MEMORY;

      receiveFilterAdd(pReceiveFilter, mExplorerFrameFilter, &mExplorerFrameFilterIndex);
      return SUCCESS;

    case HDRTYP_MULTICAST:
      if (mMulticastFrameFilterIndex >= PACKET_FILTER_SIZE)
        return NO_MEMORY;

      // Multicast frames does not have destination field.
      if (0 != (pReceiveFilter->flag & DESTINATION_NODE_ID_FILTER_FLAG))
      {
        return INVALID_PARAMETERS;
      }

      receiveFilterAdd(pReceiveFilter, mMulticastFrameFilter, &mMulticastFrameFilterIndex);
      return SUCCESS;

    case HDRTYP_TRANSFERACK:
      // ToDo: Should we use the function pointer and lookup table for 2ch/3ch handling ?
      if (mTransferAckFilterIndex>= PACKET_FILTER_SIZE)
        return NO_MEMORY;

      // Ack frames does not have payload, thus payload filtering is not possible.
      if (0 != (pReceiveFilter->flag & (PAYLOAD_INDEX_1_FILTER_FLAG | PAYLOAD_INDEX_2_FILTER_FLAG)))
      {
        return INVALID_PARAMETERS;
      }

      receiveFilterAdd(pReceiveFilter, mTransferAckFilter, &mTransferAckFilterIndex);
      return SUCCESS;

    default:
      return INVALID_PARAMETERS;
  }
}


/**@brief Function for removing a receive filter in a specific filter list.
 *
 * @param[in]     pReceiveFilter Pointer to the filter to be removed
 * @param[in,out] pFilterList    Pointer to the filter list where the new filter should be removed.
 *                               When the function returns the filter list contains an updated list
 *                               where \ref pReceiveFilter has been removed
 * @param[in,out] pFilterIndex   Current last index in the filter list wit free slot for a filter.
 *                               When the function returns the filter index has been decremented by
 *                               one
 */
static ZW_ReturnCode_t receiveFilterRemove(const ZW_ReceiveFilter_t * pReceiveFilter,
                                           const ZW_ReceiveFilter_t ** pFilterList,
                                           uint32_t * pFilterListLength)
{
  uint32_t i;
  for (i = 0; i < *pFilterListLength; i++ )
  {
    if (filterCompare(pFilterList[i], pReceiveFilter) != 0)
      continue;
    break;
  }
  if (*pFilterListLength == i)
  {
    return INVALID_PARAMETERS;
  }

  for (; i < (*pFilterListLength-1);i++)
  {
    pFilterList[i] = pFilterList[i+1];
  }
  (*pFilterListLength)--;
  return SUCCESS;
}

ZW_ReturnCode_t llReceiveFilterRemove(const ZW_ReceiveFilter_t * pReceiveFilter)
{
  ZW_FrameType_t frameType = pReceiveFilter->headerType;
  switch (frameType)
  {
    case HDRTYP_SINGLECAST:
      return receiveFilterRemove(pReceiveFilter, mSingleCastFilter, &mSingleCastFilterIndex);

    case HDRTYP_ROUTED:
      return receiveFilterRemove(pReceiveFilter, mSingleCastRoutedFilter, &mSingleCastRoutedFilterIndex);

    case HDRTYP_EXPLORE:
      return receiveFilterRemove(pReceiveFilter, mExplorerFrameFilter, &mExplorerFrameFilterIndex);

    case HDRTYP_TRANSFERACK:
      return receiveFilterRemove(pReceiveFilter, mTransferAckFilter, &mTransferAckFilterIndex);

    case HDRTYP_MULTICAST:
      return receiveFilterRemove(pReceiveFilter, mMulticastFrameFilter, &mMulticastFrameFilterIndex);

    default:
      return INVALID_PARAMETERS;
  }
}


static bool llIsTransmitBeam(CommunicationProfile_t communicationProfile)
{
  // 3CH BEAM
  if ((communicationProfile == RF_PROFILE_3CH_100K_WAKEUP_FRAGMENTED_A) ||
      (communicationProfile == RF_PROFILE_3CH_100K_WAKEUP_FRAGMENTED_B) ||
      (communicationProfile == RF_PROFILE_3CH_100K_WAKEUP_FRAGMENTED_C))
  {
    return true;
  }

  // 2CH BEAM
  if ((communicationProfile == RF_PROFILE_40K_WAKEUP_1000) ||
      (communicationProfile == RF_PROFILE_40K_WAKEUP_250))
  {
    return true;
  }

  // LR BEAM
  if ((communicationProfile == RF_PROFILE_100K_LR_WAKEUP_FRAGMENTED_A) ||
      (communicationProfile == RF_PROFILE_100K_LR_WAKEUP_FRAGMENTED_B))
  {
    return true;
  }
  return false;
}


void llReTransmitStart(ZW_TransmissionFrame_t *pFrame)
{
  if (!llIsTransmitBeam(pFrame->profile))
  {
    pFrame->status |= RETRANSMIT_FRAME;
  }
}


void llReTransmitStop(ZW_TransmissionFrame_t *pFrame)
{
  pFrame->status &= ~RETRANSMIT_FRAME;
}


uint8_t llIsReTransmitEnabled(ZW_TransmissionFrame_t *pFrame)
{
  return (pFrame->status & RETRANSMIT_FRAME);
}


ZW_ReturnCode_t llReceiveFilterPause(uint8_t pause)
{
  ZW_ReturnCode_t retVal = UNSUPPORTED;
  uint32_t compareIndex = mExplorerFrameFilterIndex;

  for (; compareIndex > 0; --compareIndex)
  {
    mExplorerFrameFilterPaused[compareIndex - 1] = pause;
    retVal = SUCCESS;
  }

  compareIndex = mMulticastFrameFilterIndex;
  for (; compareIndex > 0; --compareIndex)
  {
    mMulticastFrameFilterPaused[compareIndex - 1] = pause;
    retVal = SUCCESS;
  }

  compareIndex = mSingleCastFilterIndex;
  for (; compareIndex > 0; --compareIndex)
  {
    mSingleCastFilterPaused[compareIndex - 1] = pause;
    retVal = SUCCESS;
  }

  compareIndex = mSingleCastRoutedFilterIndex;
  for (; compareIndex > 0; --compareIndex)
  {
    mSingleCastRoutedFilterPaused[compareIndex - 1] = pause;
    retVal = SUCCESS;
  }

  compareIndex = mTransferAckFilterIndex;
  for (; compareIndex > 0; --compareIndex)
  {
    mTransferAckFilterPaused[compareIndex - 1] = pause;
    retVal = SUCCESS;
  }
  return retVal;
}


/**@brief Function for setting NetworkId in lower levels
 *
 * @param[in] homeId Network HomeId to set
 * @param[in] nodeId Network NodeId to set
 * @param[in] nodeMode Node run mode - listening, none-listening or FLiRS node
 */
void llSetNetworkId(uint32_t homeId, node_id_t nodeId, zpal_radio_mode_t nodeMode)
{
  const uint8_t homeIdHash = HomeIdHashCalculate(homeId, nodeId, zpal_radio_get_protocol_mode());
  zpal_radio_set_network_ids(homeId, nodeId, nodeMode, homeIdHash);
}

uint8_t
llConvertTransmitProfileToPHYChannel(CommunicationProfile_t profile)
{
  // 2CH transmission & BEAM
  if ((profile >= RF_PROFILE_9_6K) &&
      (profile <= RF_PROFILE_40K_WAKEUP_1000))
  {
    return CHANNEL_1_ID;
  }

  // 3CH transmission
  if ((profile >= RF_PROFILE_3CH_100K_CH_A) &&
      (profile <= RF_PROFILE_3CH_100K_CH_C))
  {
    return (profile - RF_PROFILE_3CH_100K_CH_A);                  // Channels 0 to 2 are picked here!
  }

  // 3CH BEAM
  if ((profile >= RF_PROFILE_3CH_100K_WAKEUP_FRAGMENTED_A) &&
      (profile <= RF_PROFILE_3CH_100K_WAKEUP_FRAGMENTED_C))
  {
    return (profile - RF_PROFILE_3CH_100K_WAKEUP_FRAGMENTED_A);   // Channels 0 to 2 are picked here!
  }

  // LR transmission
  if (profile == RF_PROFILE_100K_LR_A ||
      profile == RF_PROFILE_100K_LR_B)
  {
    return CHANNEL_3_ID;
  }

  // LR BEAM
  if (profile == RF_PROFILE_100K_LR_WAKEUP_FRAGMENTED_A)
  {
    return CHANNEL_3_ID;
  }
  else if(profile == RF_PROFILE_100K_LR_WAKEUP_FRAGMENTED_B)
  {
    return CHANNEL_4_ID;
  }

  // Anything else must be channel 0
  return CHANNEL_0_ID;
}

uint32_t llIsHeaderFormat3ch(void)
{
  return (ZPAL_RADIO_PROTOCOL_MODE_2 == zpal_radio_get_protocol_mode()) ? 1 : 0;
}


uint32_t llIsLBTEnabled(void)
{
  return zpal_radio_is_lbt_enabled();
}


uint16_t llGetWakeUpBeamFragmentTime(void)
{
  //Returns the time spent beaming plus the time spent on starting up beams plus the
  //time spent on listen before talk. In ms.
  return mWakeupBeamTimeLength + zpal_radio_get_beam_startup_time();
}

/**@brief Function for changing the Long Range channel configuration used by the node
 *
 * @param[in] eLrChCfg Target long range channel configuration we want to setup
 *
 * @retval true       if long range is setup successfully or if requested channel configuration is already set
 * @retval false      if llChangeRfPHYToLrChannelConfig fails
 */
bool llChangeRfPHYToLrChannelConfig(zpal_radio_lr_channel_config_t eLrChCfg)
{
    return llChangeLrChannelConfig(eLrChCfg);
}
