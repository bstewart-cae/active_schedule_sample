// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file ZW_protocol.h
 * @copyright 2022 Silicon Laboratories Inc.
 *
 * @brief Z-Wave protocol defines.
 */
#ifndef _ZW_PROTOCOL_H_
#define _ZW_PROTOCOL_H_

/****************************************************************************/
/*             Defines used in the protocol specific include files          */
/****************************************************************************/
#include <ZW_protocol_def.h>

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/

#include <ZW_transport_api.h>

/* 2.4GHz protocol */
#include <ZW_protocolGHz.h>
/* Sub GHz protocols */
#include <ZW_protocolSubGHz.h>
/* Z-Wave Long range  protocols */
#include <ZW_protocolLR.h>
/* Z-Wave Frames definntion */
//#include <ZW_Frame.h>

/****************************************************************************/
/*                     EXPORTED TYPES and DEFINITIONS                       */
/****************************************************************************/

/* Common definitions */

/* The number of bytes that will be allocated for a frame */
#define MAX_FRAME_LEN         (RX_MAX-1)
#define MAX_FRAME_LEN_TIMING  (64) /* Temp define used until we have correct 100k timing*/

/* Multicast information fields */
#define MASK_MULTI_BYTES  0x1F
#define MASK_MULTI_OFFSET 0xE0

#define MASK_MULTI_BYTES_LR  0x1F
#define MASK_MULTI_OFFSET_LR 0xE0


/* Routing information fields */
#define MASK_ROUT_DIR     0x01
#define MASK_ROUT_ACK     0x02
#define MASK_ROUT_ERR     0x04
#define MASK_ROUT_EXTEND  0x08


#define MASK_ROUT_HOPS    0x0F
#define MASK_ROUT_REPTS   0xF0
#define MASK_ROUT_ERR_HOP 0xF0

/* Routed frame header masks in routeStatus */
#define MASK_DIRECTION    0x01
#define MASK_ROUTE_ACK    0x02
#define MASK_ROUTE_ERR    0x04

/* Protocol timeout values */
#define TRANSFER_RETRIES      3
#define ROUTING_RETRIES       3

/* Frame transmit time */
/* 4800 bps and max 64 bytes */
/* 1/4800 * 8 * 64 = 0.107 sec = 107 ms */
/* 9600 bps and max 64 bytes */
/* 1/9600 * 8 * 64 = 0.053 sec = 53 ms */
/* 40000 bps and max 64 bytes */
/* 1/40000 * 8 * 64 = 0.0128 sec = 12.8 ms */

#define MIN_FRAME_TIME_MS         (uint8_t)((4+8+10)*8*10/(BITRATE/100)) /* 18.3ms (9600) */
#define MAX_FRAME_TIME_MS         (uint8_t)((4+8+MAX_FRAME_LEN_TIMING)*8*10/(BITRATE/100)) /* 63.3 ms (9600) */
#define REQUEST_FRAME_TIME_MS     (uint8_t)((4+8+10+2)*8*10/(BITRATE/100)) /* 20ms (9600) */
#define ROUTED_ACK_TIME_MS        (uint8_t)((4+8+5+9)*8*10/(BITRATE/100)) /* 21.7ms (9600) */
#define ACKNOWLEDGE_TIME_MS       (uint8_t)((4+8+9)*8*10/(BITRATE/100))  /* eq 17.5ms  */

/*The maximum time we try to send a frame if sending is failing because AIR is busy*/
#define MAX_LBT_WAIT_TIME_MS      1200
#define TRANSFER_LBT_DELAY_MS     (llIsLBTEnabled()?300:0)
#define TRANSFER_9_6K_LBT_DELAY_MS   TRANSFER_LBT_DELAY_MS
#define TRANSFER_100K_LBT_DELAY_MS   TRANSFER_LBT_DELAY_MS
/* RF setup time */
#define ADDITIONAL_WAIT_TIME_MS   15
/* Collision avoidance extra time 0 - 30 msec (average 15 msec) */
#define TRANSFER_RX_WAIT_TIME_MS  (MAX_FRAME_TIME_MS + 15) // 78.3 ms
/* Acknowledge timeout. Calculated from End Of Frame transmission */
#define TRANSFER_ACK_WAIT_TIME_MS  (ACKNOWLEDGE_TIME_MS + ADDITIONAL_WAIT_TIME_MS) // 32.5 ms
/* Routed acknowledge timeout. Calculated from End Of Frame transmission */
#define TRANSFER_ROUTED_ACK_WAIT_TIME_MS  (ROUTED_ACK_TIME_MS + 40 /*ADDITIONAL_ROUTED_WAIT_TIME_MS*/)
/* Min. lenght frame transmission failure timeout (average) */
#define TRANSFER_MIN_FRAME_WAIT_TIME_MS (TRANSFER_RX_WAIT_TIME_MS + TRANSFER_RETRIES * (MIN_FRAME_TIME_MS + 3*TRANSFER_ACK_WAIT_TIME_MS + ADDITIONAL_WAIT_TIME_MS + TRANSFER_LBT_DELAY_MS ))
/* Max. length frame transmission failure time */
#define TRANSFER_MAX_FRAME_WAIT_TIME_MS (TRANSFER_RX_WAIT_TIME_MS + TRANSFER_RETRIES * (MAX_FRAME_TIME_MS + 3*TRANSFER_ACK_WAIT_TIME_MS + ADDITIONAL_WAIT_TIME_MS))
/* Request transmission failure time */
#define TRANSFER_REQUEST_WAIT_TIME_MS (TRANSFER_RX_WAIT_TIME_MS + TRANSFER_RETRIES * (REQUEST_FRAME_TIME_MS + 3*TRANSFER_ACK_WAIT_TIME_MS) + TRANSFER_MAX_FRAME_WAIT_TIME_MS)
/* Max. length frame routed transmission per hop failure time */
#define TRANSFER_ROUTED_FRAME_WAIT_TIME_MS (TRANSFER_RX_WAIT_TIME_MS + ROUTING_RETRIES * (MAX_FRAME_TIME_MS + 3*TRANSFER_ACK_WAIT_TIME_MS + ROUTED_ACK_TIME_MS)) // 625.8 ms


/* Timeout values are based on the 10 msec system timer ticks */
#define MAX_FRAME_TIME          (uint8_t)((MAX_FRAME_TIME_MS / 10) + 1)
#define MIN_FRAME_TIME          (uint8_t)((MIN_FRAME_TIME_MS / 10) + 1)

#define TRANSFER_ACK_WAIT_TIME   (uint8_t)((TRANSFER_ACK_WAIT_TIME_MS / 10) + 1) /* eq 4.25 => 40ms /- 10ms */
#define TRANSFER_ROUTED_ACK_WAIT_TIME   (uint8_t)((TRANSFER_ROUTED_ACK_WAIT_TIME_MS / 10) + 1) /* eq 6.1 => 60ms /- 10ms */

#define TRANSFER_MIN_FRAME_WAIT_TIME (uint8_t)((TRANSFER_MIN_FRAME_WAIT_TIME_MS / 10) + 1)
#define TRANSFER_MAX_FRAME_WAIT_TIME (uint8_t)((TRANSFER_MAX_FRAME_WAIT_TIME_MS / 10) + 1)
#define TRANSFER_REQUEST_WAIT_TIME   (uint8_t)((TRANSFER_REQUEST_WAIT_TIME_MS / 10) + 1)
#define TRANSFER_ROUTED_FRAME_WAIT_TIME (uint8_t)(TRANSFER_ROUTED_FRAME_WAIT_TIME_MS)

/* frameHeaderExplore.ver_cmd          */
#define EXPLORE_CMD_MASK                      0x1F
/* Normal explore frame */
#define EXPLORE_CMD_NORMAL                    0x00
/* Special inclusion request explore frame */
#define EXPLORE_CMD_AUTOINCLUSION             0x01
/* All nodes which receives a frame with this command should */
/* purge all explore frame currently in the explore queue if the EXPLORE_OPTION_STOP bit is set and */
/* note the nodeID-sequence number in the ignoreQueue. The nodes */
/* which are present in the frames source routed repeaterlist should only */
/* note the nodeID-sequence number in the ignoreQueue when the node */
/* have repeated the frame. */
#define EXPLORE_CMD_SEARCH_RESULT             0x02

#define EXPLORE_VER_MASK                      0xE0
#define EXPLORE_VER_NOEXPLOREHEADER           0x00
#define EXPLORE_VER                           0x20

#define EXPLORE_VER_DEFAULT                   EXPLORE_VER
/* Random interval tick is 2ms */
/* 500ms default randomization interval - 250 = 500ms */
#define EXPLORE_RANDOM_INTERVAL_DEFAULT       250
/* 200ms hopfactor which kicks in at repeater 2 - 200ms */
#define EXPLORE_HOPFACTOR_DEFAULT             200
/* 50ms delay on all repeated explore frames - 50 = 50ms */
#define EXPLORE_REPEATER_DELAY_OFFSET         50
/* 200ms timeout on repeating explore frames */
#define EXPLORE_REPEAT_FRAME_TIMEOUT          200
/* 4000ms timeout on Explore frame result */
#define EXPLORE_FRAME_TIMEOUT                 4000
/* 2000ms timeout on Explore frame result */
#define EXPLORE_FRAME_SMART_START_TIMEOUT     2000

/* frameHeaderExplore.option bit flag */
/* Bit flag telling if received explore frame is sourcerouted, which */
/* means that the route is embedded into the explore frame by the source */
/* and a repeater, which finds itself in the repeaterlist should check */
/* with the TTL if it should repeat the frame. */
#define EXPLORE_OPTION_SOURCEROUTED        0x01
/* Bit flag telling which direction the repeater in the repeater list */
/* should be accessed - if ZERO the first repeater is the first repeater */
/* in the list. If ONE the first repeater is the last repeater in the list. */
#define EXPLORE_OPTION_DIRECTION           0x02
/* Bit flag telling if the exploreQueue in the receiver should be purged */
#define EXPLORE_OPTION_STOP                0x04


/* frameHeaderExplore.sessionTxRandomInterval */
/* Byte telling in which range the Tx delay for a repeated/flooded explore */
/* frame has been randomized over */

/* frameHeaderExplore.repeaterCountSessionTTL */
#define EXPLORE_REPEATERCOUNTSESSIONTTL_COUNT_MASK   0x0F
#define EXPLORE_REPEATERCOUNTSESSIONTTL_TTL_MASK     0xF0

/* What to increment/decrement the repeaterCountTTL variable with, */
/* to increment/decrement TTL */
#define EXPLORE_REPEATERCOUNTSESSIONTTL_TTL_UNIT     0x10


/* Persistent System state variable NVM_SYSTEM_STATE definitions */
/* System Idle */
#define  NVM_SYSTEM_STATE_IDLE         0x00
/* System in Smart Start mode */
#define  NVM_SYSTEM_STATE_SMART_START  0x01



/* Common types */
/* Because of the use of CHECKSUM_LENGTH in the code is is set to 1 here to
   avoid to many changes in the code between 2 and 3 channel code. But in
   some cases (MAX_PAYLOAD defines) we need the actual CRC16 length to
   make correct calculations */
#define CHECKSUM_LENGTH   1
#define CRC16_LENGTH      2

/**********************************************/
/* Generic frame for use in TxQueue           */
/**********************************************/

/* This frameTx type contains the full headers
 * for repeaterlists and other variable-length
 * fields. In contrast, the "frame" type below
 * only defines the first element of those
 * fields.
 */

typedef union _frameTx_
{
  frameHeader header;
  frameHeader3ch header3ch;
  frameHeaderLR headerLR;
  frameHeaderSinglecast singlecast;
  frameHeaderSinglecast3ch singlecast3ch;
  frameHeaderSinglecastLR singlecastLR;
  frameHeaderMulticast multicast;
  frameHeaderMulticast3ch multicast3ch;
  frameHeaderSinglecastRouted singlecastRouted;
  frameHeaderSinglecastRouted3ch singlecastRouted3ch;
  frameTransferACK transferACK;
  frameHeaderTransferACK3ch transferACK3ch;
  frameTransferACKLR transferACKLR;
  frameHeaderExplore explore;
  frameHeaderExplore3ch explore3ch;
  frameHeaderRoutedACK routedACK;
  frameHeaderRoutedACK3ch routedACK3ch;
  frameHeaderRoutedERR routedERR;
  frameHeaderRoutedERR3ch routedERR3ch;
} frameTx;


/**********************************************/
/* Generic frame                              */
/**********************************************/
typedef union _frame_
{
  frameHeader header;
  frameHeader3ch header3ch;
  frameHeaderLR headerLR;
  frameSinglecast singlecast;
  frameHeaderSinglecast3ch singlecast3ch;
  frameHeaderSinglecastLR singlecastLR;
  frameMulticast multicast;
  frameHeaderMulticast3ch multicast3ch;
  frameSinglecastRouted singlecastRouted;
  frameHeaderSinglecastRouted3ch singlecastRouted3ch;
  frameTransferACK transferACK;
  frameHeaderTransferACK3ch transferACK3ch;
  frameTransferACKLR transferACKLR;
  frameHeaderExplore explore;
  frameHeaderExplore3ch explore3ch;
  frameHeaderRoutedACK3ch routedACK3ch;
  frameHeaderRoutedERR3ch routedERR3ch;
} frame;


/**
 *  Persistent System state variable NVM_SYSTEM_STATE definitions
 */
typedef enum _E_NVM_SYSTE_STATE_
{
  /**
   *  System Idle
   */
  E_NVM_SYSTEM_STATE_IDLE = 0,
  /**
   *  System in Smart Start mode
   */
  E_NVM_SYSTEM_STATE_SMART_START
} E_NVM_SYSTE_STATE;

/********************************************/
/********************************************/
/* 2 Channel types and macro definitions    */
/********************************************/
/********************************************/

#define MAX_SINGLECAST_PAYLOAD        (RX_MAX - sizeof(frameHeaderSinglecast) - CRC16_LENGTH)
#define MAX_ROUTED_PAYLOAD            (RX_MAX - (sizeof(frameHeaderSinglecastRouted) - MAX_EXTEND_BODY) - CRC16_LENGTH)
#define MAX_ROUTED_PAYLOAD_FLIRS      (RX_MAX - sizeof(frameHeaderSinglecastRouted) - CRC16_LENGTH)
#define MAX_MULTICAST_PAYLOAD         (RX_MAX - sizeof(frameHeaderMulticast) - CRC16_LENGTH)

#define MAX_EXPLORE_PAYLOAD_LEGACY            (RX_MAX_LEGACY - sizeof(frameHeaderExplore) - sizeof(frameHeader) - sizeof(uint8_t) - CHECKSUM_LENGTH)

#define MAX_SINGLECAST_PAYLOAD_LEGACY         (RX_MAX_LEGACY - sizeof(frameHeaderSinglecast) - CHECKSUM_LENGTH)
#define MAX_ROUTED_PAYLOAD_LEGACY             (RX_MAX_LEGACY - (sizeof(frameHeaderSinglecastRouted) - MAX_EXTEND_BODY) - CHECKSUM_LENGTH)
#define MAX_ROUTED_PAYLOAD_FLIRS_LEGACY       (RX_MAX_LEGACY - sizeof(frameHeaderSinglecastRouted) - CHECKSUM_LENGTH)
#define MAX_MULTICAST_PAYLOAD_LEGACY          (RX_MAX_LEGACY - sizeof(frameHeaderMulticast) - CHECKSUM_LENGTH)

/***************************************/
/* Macros for reading the frameheaders */
/***************************************/
#define IS_SINGLECAST(fr)   (((fr).header.headerInfo & MASK_HDRTYP)==HDRTYP_SINGLECAST)
#define IS_MULTICAST(fr)    (((fr).header.headerInfo & MASK_HDRTYP)==HDRTYP_MULTICAST)
//#define IS_EXPLORE(fr)      (((fr).header.headerInfo & MASK_HDRTYP)==HDRTYP_EXPLORE)
#define IS_EXPLORE(fr)      ((fr).frameOptions.frameType==HDRTYP_EXPLORE)
#define IS_TRANSFERACK(fr)  (((fr).header.headerInfo & MASK_HDRTYP)==HDRTYP_TRANSFERACK)
#define DO_ACK(fr)          ((fr).header.headerInfo & MASK_ACKNOWLEDGE)
#define IS_ACK(fr)          ((fr).header.headerInfo & MASK_ACKNOWLEDGE)
#define IS_LOWPOWER(fr)     ((fr).header.headerInfo & MASK_LOWPOWER)
#define IS_ROUTEACK(fr)     ((fr).singlecastRouted.routeStatus & MASK_ROUT_ACK)
#define IS_ROUTEERR(fr)     ((fr).singlecastRouted.routeStatus & MASK_ROUT_ERR)
#define IS_ROUTEACKERR(fr)  ((fr).singlecastRouted.routeStatus & (MASK_ROUT_ERR | MASK_ROUT_ACK))


/*
  Note that SET_HEADERTYPE *MUST* be used *BEFORE* SET_OUTGOING/INCOMING and
   SET_ROUTED/NON_ROUTED!
*/
#define GET_HEADERTYPE(fr)      ((fr).header.headerInfo & MASK_HDRTYP)
#define SET_HEADERTYPE(fr, type) ((fr).header.headerInfo = (((fr).header.headerInfo & ~MASK_HDRTYP) | type))

#define SET_ACK(fr)         ((fr).header.headerInfo |= MASK_ACKNOWLEDGE)
#define SET_LOWPOWER(fr)    ((fr).header.headerInfo |= MASK_LOWPOWER)

#define SET_INCOMING(fr)    ((fr).singlecastRouted.routeStatus |= MASK_ROUT_DIR)
#define SET_OUTGOING(fr)    ((fr).singlecastRouted.routeStatus &= ~MASK_ROUT_DIR)

#define SET_ROUTE_ACK(fr)   ((fr).singlecastRouted.routeStatus |= MASK_ROUT_ACK)
#define SET_ROUTE_ERR(fr)   ((fr).singlecastRouted.routeStatus |= MASK_ROUT_ERR)

/************************************************/
/* Macros for use with routed singlecast frames */
/************************************************/

/* Get the number of repeaters in the route */
#define GET_ROUTE_LEN(fr)              (((fr).singlecastRouted.numRepsNumHops) >> 4)
#define SET_ROUTE_LEN(fr,len)          ((fr).singlecastRouted.numRepsNumHops = (len<<4))
/* Get the number of hops the frame has been through */
#define GET_HOPS(fr)                   ((fr).singlecastRouted.numRepsNumHops & 0x0f)
#define SET_HOPS(fr,hops)              ((fr).singlecastRouted.numRepsNumHops = ((fr).singlecastRouted.numRepsNumHops&0xf0)|(hops&0x0f))
#define NOT_OUTGOING(fr)    ((fr).singlecastRouted.routeStatus & MASK_DIRECTION)
#define IS_OUTGOING(fr)    (!NOT_OUTGOING(fr))
#define IS_OUTGOING_3CH(fr)    (!NOT_OUTGOING_3CH(fr))

// this macros should only be used when a frame is received
#define GET_FRAME_LEN(fr)     ((fr).payloadLength + (fr).headerLength)

/***************************************/
/* Macros for reading the frameheaders */
/***************************************/
#define IS_ROUTED(fr)       ((fr).header.headerInfo & MASK_ROUTED)

/* These macros replace the deprecated state variables FRAME_x, x=SINGLE,ROUTED,MULTI,ACK,FLOODED */
#define IS_FRAME_SINGLECAST(fr) (!IS_ROUTED((fr)) && IS_SINGLECAST((fr)))
/* TO-DO: IS_ROUTED => IS_SINGLECAST, so second factor is redundant */
#define IS_FRAME_ROUTED(fr) (IS_ROUTED((fr)) && IS_SINGLECAST((fr)))
#define IS_FRAME_ACK(fr) IS_ACK(fr)
/* TO-DO: Implement IS_FRAME_FLOODED and _MULTI */
#define IS_FRAME_MULTI(fr)
#define IS_FRAME_FLOODED(fr)

/* _EX macro can be used for both direct and routed frames */
#define IS_SPEED_MODIFIED(elm) (elm->frame.frameOptions.routed ? IS_ROUTED_SPEED_MODIFIED(elm->frame.header) : elm->frame.frameOptions.speedModified)

#define GET_SEQNUMBER(fr)   ((fr).header.reserved & MASK_SEQNO)
//#define SET_SEQNUMBER(fr, seqno)  ((fr).header.reserved = (((fr).header.reserved & ~MASK_SEQNO) | seqno))

#define SET_ROUTED(fr)      ((fr).header.headerInfo |= MASK_ROUTED)
#define SET_NOT_ROUTED(fr)  ((fr).header.headerInfo &= ~MASK_ROUTED)

/* Get the nodeID of a repeater in the route */
#define READ_REPEATER(fr,number)       (*((&(fr).singlecastRouted.startOfRepeaterList)+number))
/* Set the nodeID of a repeater in the route */
#define SET_REPEATER(fr,number,id)     (*((&(fr).singlecastRouted.startOfRepeaterList)+number) = id)

/* Get the Payload as an array */
#define SINGLECAST_PAYLOAD(fr)         (&(fr).singlecast.startOfPayload)

/* Get the number of bytes in the Payload */
#define SINGLECAST_PAYLOAD_LEN(fr) ((fr).header.length-sizeof(frameHeader) - (GET_EXTEND_PRESENT(fr) ? (1 + GET_EXTEND_LENGTH(fr)) : 0))

/****************************************/
/* Macros for use with multicast frames */
/****************************************/

/* Get the offset of a multicast frame */
#define MULTICAST_OFFSET(fr)  ((fr).multicast.addrOffsetNumMaskBytes & MASK_MULTI_OFFSET)
/* Number of addres bytes in destination mask */
#define MULTICAST_ADDRESS_LEN(fr)  ((fr).multicast.addrOffsetNumMaskBytes & MASK_MULTI_BYTES)
/* Get the ReceiverMask as an array */
#define RECEIVER_MASK(fr)         (&((fr).multicast.startOfDestinationMask))

/* Defines for Wakeup Beam settings for the source, set in the reserved byte in frame */
#define SET_WAKEUP_BEAM_SPEED_SRC(fr, wakeupspeed) ((fr).header.reserved = (((fr).header.reserved & ~MASK_HEADER_WAKEUP_BEAM_SPEED_SRC) | (wakeupspeed & MASK_HEADER_WAKEUP_BEAM_SPEED_SRC)))
#define CLR_WAKEUP_BEAM_SPEED_SRC(fr) ((fr).header.reserved &= ~MASK_HEADER_WAKEUP_BEAM_SPEED_SRC)
#define GET_WAKEUP_BEAM_SPEED_SRC(fr) ((fr).header.reserved & MASK_HEADER_WAKEUP_BEAM_SPEED_SRC)


/* Defines for setting/getting and clearing the Extended route header body present bit */
/* This bit, if true, indicates if an extended route header body is present and placed */
/* inbetween the route header and the payload in a routed frame*/
#define SET_EXTEND_PRESENT(fr)  ((fr).singlecastRouted.routeStatus |= MASK_ROUT_EXTEND)
#define CLR_EXTEND_PRESENT(fr)  ((fr).singlecastRouted.routeStatus &= ~MASK_ROUT_EXTEND)
#define GET_EXTEND_PRESENT(fr)  ((fr).singlecastRouted.routeStatus & MASK_ROUT_EXTEND)

/* Extended route header body definitions */
/* Extended route header minimum size */
#define EXTEND_HEADER_MIN_SIZE                  1

/**************************************************/
/* Definition of the Extended route header format */
/**************************************************/
/* typedef struct _extendedRouteHeader_           */
/* {                                              */
/*   uint8_t extendedRouteHeaderLengthType;          */
/*   uint8_t extendedHeaderBody[];                   */
/* } extendedRouteHeader;                         */
/*                                                */
/* - extendedRouteHeaderLengthType contains in the upper nibble (bit4-bit7) */
/* the length of the extendedRouteHeaderBody   */
/* - extendedRouteHeaderLengthType contains in the lower nibble (bit0-bit3) */
/* the extendedRouteHeaderType. */
/* Current defined extendedRouteHeaderType : */
/*  EXTEND_TYPE_WAKEUP_TYPE (0) indicates that the extended routed header body in */
/* the first byte contains beam wakeup definitions for the source and destination */
/* nodes and the following bytes in the body indicates the beam wakeup definitions */
/* for the repeaters used */

/* Definitions for type of Extend body  */
#define EXTEND_TYPE_WAKEUP_TYPE                 0
#define EXTEND_TYPE_RSSI_INCOMING               1

/* Defintions for offsets in the ExtendedBody for ExtendedRouteHeaderType */
/* EXTEND_TYPE_WAKEUP_TYPE */
#define EXTEND_TYPE_WAKEUP_TYPE_SRC_DEST_OFFSET 0
/* Wakeup type bits for destination - if none zero the destination node is a node which needs a beam */
#define EXTEND_TYPE_WAKEUP_TYPE_DEST              0x60
#define EXTEND_TYPE_WAKEUP_TYPE_DEST_WAKEUP_1000  0x40
#define EXTEND_TYPE_WAKEUP_TYPE_DEST_WAKEUP_250   0x20
#define EXTEND_TYPE_WAKEUP_TYPE_DEST_WAKEUP_RES   0x60
/* Wakeup type bits for source - if none zero the source node is a node which needs a beam */
#define EXTEND_TYPE_WAKEUP_TYPE_SRC               0x18
#define EXTEND_TYPE_WAKEUP_TYPE_SRC_WAKEUP_1000   0x10
#define EXTEND_TYPE_WAKEUP_TYPE_SRC_WAKEUP_250    0x08
#define EXTEND_TYPE_WAKEUP_TYPE_SRC_WAKEUP_RES    0x18

/* The length of an EXTEND_TYPE_RSSI_INCOMING header */
#define EXTEND_TYPE_RSSI_INCOMING_LEN 4

/* Defines defining offsets for accessing routed header in repeatBuffer */
/* when repeating and transmitting routed ACK/ERR frames */
#define EXTEND_BODY_REPEATBUFFER_DESTID_OFFSET              0
#define EXTEND_BODY_REPEATBUFFER_ROUTESTATUS_OFFSET         (EXTEND_BODY_REPEATBUFFER_DESTID_OFFSET + 1)
#define EXTEND_BODY_REPEATBUFFER_NUMREPSNUMHOPS_OFFSET      (EXTEND_BODY_REPEATBUFFER_ROUTESTATUS_OFFSET + 1)
#define EXTEND_BODY_REPEATBUFFER_STARTOFREPEATERLIST_OFFSET (EXTEND_BODY_REPEATBUFFER_NUMREPSNUMHOPS_OFFSET + 1)

#define CLR_EXTEND_PRESENT_REPEAT() (repeatBuffer[EXTEND_BODY_REPEATBUFFER_ROUTESTATUS_OFFSET] &= ~MASK_ROUT_EXTEND)

/* Macros for Extend body handling to be used when accessing via repeatBuffer */
/* First set Extend Type then length */
#define SET_EXTEND_TYPE_REPEAT(extendType) (*(repeatBuffer[EXTEND_BODY_REPEATBUFFER_STARTOFREPEATERLIST_OFFSET + ((repeatBuffer[EXTEND_BODY_REPEATBUFFER_NUMREPSNUMHOPS_OFFSET]) >> 4)]) = extendType)
#define GET_EXTEND_TYPE_REPEAT() (*(repeatBuffer[EXTEND_BODY_REPEATBUFFER_STARTOFREPEATERLIST_OFFSET + ((repeatBuffer[EXTEND_BODY_REPEATBUFFER_NUMREPSNUMHOPS_OFFSET]) >> 4)]))
#define SET_EXTEND_LENGTH_REPEAT(length) (*(repeatBuffer[EXTEND_BODY_REPEATBUFFER_STARTOFREPEATERLIST_OFFSET + ((repeatBuffer[EXTEND_BODY_REPEATBUFFER_NUMREPSNUMHOPS_OFFSET]) >> 4)]) |= (length << 4))
#define GET_EXTEND_LENGTH_REPEAT() (*(repeatBuffer[EXTEND_BODY_REPEATBUFFER_STARTOFREPEATERLIST_OFFSET + ((repeatBuffer[EXTEND_BODY_REPEATBUFFER_NUMREPSNUMHOPS_OFFSET]) >> 4)]))
#define SET_EXTEND_BODY_REPEAT(offset, val) (*(repeatBuffer[EXTEND_BODY_REPEATBUFFER_STARTOFREPEATERLIST_OFFSET + ((repeatBuffer[EXTEND_BODY_REPEATBUFFER_NUMREPSNUMHOPS_OFFSET]) >> 4) + 1 + offset]) = val)
#define GET_EXTEND_BODY_REPEAT(offset) (*(repeatBuffer[EXTEND_BODY_REPEATBUFFER_STARTOFREPEATERLIST_OFFSET + ((repeatBuffer[EXTEND_BODY_REPEATBUFFER_NUMREPSNUMHOPS_OFFSET]) >> 4) + 1 + offset]))

/* Macros for Extend body handling to be used when accessing via activeTransmit header */
#define SET_EXTEND_TYPE_HEADER(fr, extendType) (*(((fr).singlecastRouted.repeaterList) + (((fr).singlecastRouted.numRepsNumHops) >> 4)) = extendType)
#define GET_EXTEND_TYPE_HEADER(fr) ((*(((fr).singlecastRouted.repeaterList) + (((fr).singlecastRouted.numRepsNumHops) >> 4))) & 0x0f)
#define SET_EXTEND_LENGTH_HEADER(fr, length) (*(((fr).singlecastRouted.repeaterList) + (((fr).singlecastRouted.numRepsNumHops) >> 4)) |= (length << 4))
#define GET_EXTEND_LENGTH_HEADER(fr) ((*(((fr).singlecastRouted.repeaterList) + (((fr).singlecastRouted.numRepsNumHops) >> 4))) >> 4)
/* TO#01793 - fix Routing in 1000/250ms populated FLiRS networks can result in 1000ms wakeup beams to 250ms FLiRS */
#define SET_EXTEND_BODY_HEADER(fr, offset, val) (*(((fr).singlecastRouted.repeaterList) + (((fr).singlecastRouted.numRepsNumHops) >> 4) + 1 + offset) = (uint8_t)val)
#define GET_EXTEND_BODY_HEADER(fr, offset) (*(((fr).singlecastRouted.repeaterList) + (((fr).singlecastRouted.numRepsNumHops) >> 4) + 1 + offset))

/* Macros for Extend body handling to be used when accessing via received frame */
#define SET_EXTEND_TYPE(fr, extendType) (*(((fr).singlecastRouted.repeaterList) + (((fr).singlecastRouted.numRepsNumHops) >> 4)) = extendType)
#define GET_EXTEND_TYPE(fr) ((*((&(fr).singlecastRouted.startOfRepeaterList) + (((fr).singlecastRouted.numRepsNumHops) >> 4))) & 0x0f)
/* GET_EXTEND_TYPE2 is for TxQueueElement->sFrameheader, GET_EXTEND_TYPE is for RX_FRAME.*/
#define GET_EXTEND_TYPE2(fr) ((*(((fr).singlecastRouted.repeaterList) + (((fr).singlecastRouted.numRepsNumHops) >> 4))) & 0x0f)
#define SET_EXTEND_LENGTH(fr, length) (*(((fr).singlecastRouted.repeaterList) + (((fr).singlecastRouted.numRepsNumHops) >> 4)) |= (length << 4))
#define GET_EXTEND_LENGTH(fr) ((*((&(fr).singlecastRouted.startOfRepeaterList) + (((fr).singlecastRouted.numRepsNumHops) >> 4))) >> 4)

/* TO#01793 - fix Routing in 1000/250ms populated FLiRS networks can result in 1000ms wakeup beams to 250ms FLiRS */
#define SET_EXTEND_BODY(fr, offset, val) (*(((fr).singlecastRouted.repeaterList) + (((fr).singlecastRouted.numRepsNumHops) >> 4) + 1 + offset) = val)
#define GET_EXTEND_BODY(fr, offset) (*((&(fr).singlecastRouted.startOfRepeaterList) + (((fr).singlecastRouted.numRepsNumHops) >> 4) + 1 + offset))
//#define GET_ROUTED_SINGLECAST_HEADER_EXTENSION_LENGTH(fr) (((fr).header.headerInfo2 & MASK_HEADER_EXTENDED) ? *(&((fr).singlecastRouted.extension.extensionLength) - (MAX_REPEATERS - GET_ROUTE_LEN(fr))) & MASK_EXTENSION_LENGTH : 0)
#define GET_ROUTED_SINGLECAST_HEADER_EXTENSION_LENGTH(fr) GET_EXTEND_LENGTH(fr)
/* Get pointer to extend body */
#define GET_EXTEND_BODY_ADDR(fr, offset) (&((fr).singlecastRouted.startOfRepeaterList) + (((fr).singlecastRouted.numRepsNumHops) >> 4) + 1 + offset)

/* Get the Payload as an array */
#define SINGLECAST_ROUTED_PAYLOAD(fr)     (&((fr).singlecastRouted.startOfRepeaterList)+(((fr).singlecastRouted.numRepsNumHops)>>4) \
                                             + (GET_EXTEND_PRESENT(fr) ? (1 + GET_EXTEND_LENGTH(fr)) : 0))
/* Get the number of bytes in the Payload */
#define SINGLECAST_ROUTED_PAYLOAD_LEN(fr) ((fr).header.length-sizeof(frameHeader)-4*sizeof(uint8_t)-(((fr).singlecastRouted.numRepsNumHops)>>4) \
                                            - (GET_EXTEND_PRESENT(fr) ? (1 + GET_EXTEND_LENGTH(fr)) : 0))

/* Get the Payload as an array */
#define EXPLORE_PAYLOAD(fr)       (&(fr).explore.repeaterList[MAX_REPEATERS])
/* Get the number of bytes in the payload*/
#define EXPLORE_PAYLOAD_LEN(fr)   ((fr).header.length - sizeof(frameHeaderExplore) - 1*sizeof(uint8_t))

/* Get the Payload as an array */
#define MULTICAST_PAYLOAD(fr)     (&((fr).multicast.startOfDestinationMask)+((fr).multicast.addrOffsetNumMaskBytes & MASK_MULTI_BYTES))
/* Get the number of bytes in the payload*/
#define MULTICAST_PAYLOAD_LEN(fr) ((fr).header.length - sizeof(frameHeader) - 2*sizeof(uint8_t) - ((fr).multicast.addrOffsetNumMaskBytes  & MASK_MULTI_BYTES))

#define IS_ROUTED_SPEED_MODIFIED(fr) (IS_ROUTED(fr) && (fr).singlecastRouted.routeStatus & MASK_ROUT_SPEED_MODIFIED)
/********************************************/
/********************************************/
/* 3 channel types and macro definitions    */
/********************************************/
/********************************************/

/***************************************/
/* Macros for reading the frameheaders */
/***************************************/
#define MAX_SINGLECAST_PAYLOAD_3CH      (RX_MAX - sizeof(frameHeaderSinglecast3ch) - CRC16_LENGTH + sizeof(frameHeaderExtension3ch))
#define MAX_ROUTED_PAYLOAD_3CH          (RX_MAX - sizeof(frameHeaderSinglecastRouted3ch) - CRC16_LENGTH + sizeof(frameHeaderExtension3ch))
#define MAX_ROUTED_PAYLOAD_LEGACY_3CH   MAX_ROUTED_PAYLOAD_3CH
#define MAX_MULTICAST_PAYLOAD_3CH       (RX_MAX - sizeof(frameHeaderMulticast3ch) - CRC16_LENGTH + sizeof(frameHeaderExtension3ch))
#define MAX_EXPLORE_PAYLOAD_3CH         (RX_MAX - sizeof(frameHeaderExplore3ch) - sizeof(frameHeader3ch) - sizeof(uint8_t) - CRC16_LENGTH)


#define SET_ACK_3CH(fr)         ((fr).header3ch.headerInfo |= MASK_ACKNOWLEDGE_3CH)
#define SET_LOWPOWER_3CH(fr)    ((fr).header3ch.headerInfo |= MASK_LOWPOWER_3CH)

#define SET_INCOMING_3CH(fr)    ((fr).singlecastRouted3ch.routeStatus |= MASK_ROUT_DIR)
#define SET_OUTGOING_3CH(fr)    ((fr).singlecastRouted3ch.routeStatus &= ~MASK_ROUT_DIR)

#define SET_ROUTE_ACK_3CH(fr)   ((fr).singlecastRouted3ch.routeStatus |= MASK_ROUT_ACK)
#define SET_ROUTE_ERR_3CH(fr)   ((fr).singlecastRouted3ch.routeStatus |= MASK_ROUT_ERR)

#define GET_SINGLECAST_SOURCE_NODEID_2CH(fr)       ((fr).singlecast.header.sourceID)
#define GET_SINGLECAST_SOURCE_NODEID_3CH(fr)       ((fr).singlecast3ch.header.sourceID)
#define GET_SINGLECAST_SOURCE_NODEID_LR(fr)        (((fr).singlecastLR.header.nodeIDs[0] << 4) | ((((fr).singlecastLR.header.nodeIDs[1]) & 0xF0) >> 4))

#define SET_SINGLECAST_SOURCE_NODEID_2CH(fr, id)   ((fr).singlecast.header.sourceID = id)
#define SET_SINGLECAST_SOURCE_NODEID_3CH(fr, id)   ((fr).singlecast3ch.header.sourceID = id)
#define SET_SINGLECAST_SOURCE_NODEID_LR(fr, id)    ((fr).singlecastLR.header.nodeIDs[0] = (id >> 4));\
                                                   ((fr).singlecastLR.header.nodeIDs[1] = (((fr).singlecastLR.header.nodeIDs[1] & 0x0F) | ((id & 0x0F) << 4)))


#define GET_SINGLECAST_DESTINATION_NODEID_2CH(fr)       ((fr).singlecast.destinationID)
#define GET_SINGLECAST_DESTINATION_NODEID_3CH(fr)       ((fr).singlecast3ch.destinationID)
#define GET_SINGLECAST_DESTINATION_NODEID_LR(fr)        ((((fr).singlecastLR.header.nodeIDs[1] & 0x0F) << 8) | (fr).singlecastLR.header.nodeIDs[2])

#define SET_SINGLECAST_DESTINATION_NODEID_2CH(fr, id)   ((fr).singlecast.destinationID = id)
#define SET_SINGLECAST_DESTINATION_NODEID_3CH(fr, id)   ((fr).singlecast3ch.destinationID = id)
#define SET_SINGLECAST_DESTINATION_NODEID_LR(fr, id)    ((fr).singlecastLR.header.nodeIDs[1] = (((fr).singlecastLR.header.nodeIDs[1] & 0xF0) | ((id >> 8) & 0x0F)));\
                                                        ((fr).singlecastLR.header.nodeIDs[2] = (id & 0xFF))

#define GET_TRANSFERACK_SOURCE_NODEID_LR(fr)           GET_SINGLECAST_SOURCE_NODEID_LR(fr)
#define SET_TRANSFERACK_SOURCE_NODEID_LR(fr, id)       SET_SINGLECAST_SOURCE_NODEID_LR(fr, id)
#define GET_TRANSFERACK_DESTINATION_NODEID_LR(fr)      GET_SINGLECAST_DESTINATION_NODEID_LR(fr)
#define SET_TRANSFERACK_DESTINATION_NODEID_LR(fr, id)  SET_SINGLECAST_DESTINATION_NODEID_LR(fr, id)

/************************************************/
/* Macros for use with routed singlecast frames */
/************************************************/

/* Get the number of repeaters in the route */
#define GET_ROUTE_LEN_3CH(fr)              (((fr).singlecastRouted3ch.numRepsNumHops) >> 4)
#define SET_ROUTE_LEN_3CH(fr,len)          ((fr).singlecastRouted3ch.numRepsNumHops = (len<<4))
/* Get the number of hops the frame has been through */
#define GET_HOPS_3CH(fr)                   ((fr).singlecastRouted3ch.numRepsNumHops & 0x0f)
#define SET_HOPS_3CH(fr,hops)              ((fr).singlecastRouted3ch.numRepsNumHops = ((fr).singlecastRouted3ch.numRepsNumHops&0xf0)|(hops&0x0f))
#define NOT_OUTGOING_3CH(fr)               ((fr).singlecastRouted3ch.routeStatus & MASK_DIRECTION)

#define IS_SINGLECAST_3CH(fr)   (((fr).header.headerInfo & MASK_HDRTYP)==HDRTYP_SINGLECAST)
#define DO_ACK_3CH(fr)          ((fr).header.headerInfo & MASK_ACKNOWLEDGE_3CH)
#define IS_ACK_3CH(fr)          ((fr).header.headerInfo & MASK_ACKNOWLEDGE_3CH)
#define IS_LOWPOWER_3CH(fr)     ((fr).header.headerInfo & MASK_LOWPOWER_3CH)
#define IS_ROUTEACK_3CH(fr)     ((fr).singlecastRouted3ch.routeStatus & MASK_ROUT_ACK)
#define IS_ROUTEERR_3CH(fr)     ((fr).singlecastRouted3ch.routeStatus & MASK_ROUT_ERR)
#define IS_ROUTEACKERR_3CH(fr)  ((fr).singlecastRouted3ch.routeStatus & (MASK_ROUT_ERR | MASK_ROUT_ACK))

#define IS_ROUTED_3CH(fr)       (((fr).header.headerInfo & MASK_HDRTYP)==HDRTYP_ROUTED)

/* These macros replace the deprecated state variables FRAME_x, x=SINGLE,ROUTED,MULTI,ACK,FLOODED */
#define IS_FRAME_SINGLECAST_3CH(fr) IS_SINGLECAST((fr))
/* TO-DO: IS_ROUTED => IS_SINGLECAST, so second factor is redundant */
#define IS_FRAME_ROUTED_3CH(fr) IS_ROUTED_3CH((fr))
#define IS_FRAME_ACK_3CH(fr) IS_ACK_3CH(fr)
/* TO-DO: Implement IS_FRAME_FLOODED and _MULTI */
#define IS_FRAME_MULTI_3CH(fr)

/* LR does not do routing (yet) */
#define IS_ROUTED_LR(fr)       (false)
#define IS_FRAME_ROUTED_LR(fr) IS_ROUTED_LR(fr)
/* LR don't have a low power option (yet) */
#define IS_LOWPOWER_LR(fr)     (false)
/*
  Note that SET_HEADERTYPE *MUST* be used *BEFORE* SET_OUTGOING/INCOMING and
   SET_ROUTED/NON_ROUTED!
*/
#define SET_INCOMING_3CH(fr)    ((fr).singlecastRouted3ch.routeStatus |= MASK_ROUT_DIR)
#define SET_OUTGOING_3CH(fr)    ((fr).singlecastRouted3ch.routeStatus &= ~MASK_ROUT_DIR)

#define SET_ROUTE_ACK_3CH(fr)   ((fr).singlecastRouted3ch.routeStatus |= MASK_ROUT_ACK)
#define SET_ROUTE_ERR_3CH(fr)   ((fr).singlecastRouted3ch.routeStatus |= MASK_ROUT_ERR)

#define GET_SEQNUMBER_3CH(fr)   (fr).singlecast3ch.header.sequenceNumber
#define SET_SEQNUMBER_3CH(fr, seqno)  ((fr).singlecast3ch.header.sequenceNumber = seqno)

#define SET_ROUTED_3CH(fr)      SET_HEADERTYPE(fr, HDRTYP_ROUTED)

#define SET_EXTEND_PRESENT_3CH(fr)    {(fr).header3ch.headerInfo2 |= MASK_HEADER_EXTENDED_3CH;(fr).singlecastRouted3ch.routeStatus |= MASK_ROUT_EXTEND;}
#define CLR_EXTEND_PRESENT_3CH(fr)    {(fr).header3ch.headerInfo2 &= ~MASK_HEADER_EXTENDED_3CH;(fr).singlecastRouted3ch.routeStatus &= ~MASK_ROUT_EXTEND;}
#define GET_EXTEND_PRESENT_3CH(fr)     (IS_ROUTED_3CH(fr)? ((fr).singlecastRouted3ch.routeStatus & MASK_ROUT_EXTEND): ((fr).header3ch.headerInfo2 & MASK_HEADER_EXTENDED_3CH))

#define SET_EXTEND_PRESENT_LR(fr)    ((fr).headerLR.headerInfo |= MASK_HEADER_EXTENDED_LR)
#define CLR_EXTEND_PRESENT_LR(fr)    ((fr).headerLR.headerInfo &= ~MASK_HEADER_EXTENDED_LR)
#define GET_EXTEND_PRESENT_LR(fr)     ((fr).headerLR.headerInfo & MASK_HEADER_EXTENDED_LR)

/* Get the nodeID of a repeater in the route */
#define READ_REPEATER_3CH(fr,number)       ((fr).singlecastRouted3ch.repeaterList[number])
/* Set the nodeID of a repeater in the route */
#define SET_REPEATER_3CH(fr,number,id)     ((fr).singlecastRouted3ch.repeaterList[number] = id)

#define GET_SINGLECAST_HEADER_EXTENSION_LENGTH_3CH(fr) (GET_EXTEND_PRESENT_3CH(fr) ? (fr).singlecast3ch.extension.extensionLength & MASK_EXTENSION_LENGTH_3CH : 0)
#define GET_MULTICAST_HEADER_EXTENSION_LENGTH_3CH(fr) (GET_EXTEND_PRESENT_3CH(fr) ? *(&((fr).multicast3ch.extension.extensionLength) - ((ZW_MAX_NODES/8) + ((fr).multicast3ch.addrOffsetNumMaskBytes & MASK_MULTI_BYTES))) & MASK_EXTENSION_LENGTH_3CH : 0)
#define GET_ROUTED_SINGLECAST_HEADER_EXTENSION_LENGTH_3CH(fr) (GET_EXTEND_PRESENT_3CH(fr) ? (fr).singlecastRouted3ch.repeaterList[GET_ROUTE_LEN_3CH(fr) + 1] & MASK_EXTENSION_LENGTH_3CH : 0)
#define GET_ROUTED_ACK_HEADER_EXTENSION_LENGTH_3CH(fr) (GET_EXTEND_PRESENT_3CH(fr) ? (fr).routedACK3ch.repeaterList[GET_ROUTE_LEN_3CH(fr)] & MASK_EXTENSION_LENGTH_3CH : 0)

#define GET_SINGLECAST_HEADER_EXTENSION_LENGTH_LR(fr) (((fr).headerLR.headerInfo & MASK_HEADER_EXTENDED_LR) ? (fr).singlecastLR.extension.extensionInfo & MASK_EXTENSION_LENGTH_LR : 0)

/* Defines for Wakeup Beam settings for the source, set in the reserved byte in frame */
#define SET_WAKEUP_BEAM_SPEED_SRC_3CH(fr, wakeupspeed) ((fr).header.headerInfo2 = (((fr).header.headerInfo2 & ~MASK_HEADER_SOURCE_WAKEUP_3CH) | (wakeupspeed & MASK_HEADER_SOURCE_WAKEUP_3CH)))
#define CLR_WAKEUP_BEAM_SPEED_SRC_3CH(fr) ((fr).header.headerInfo2 &= ~MASK_HEADER_SOURCE_WAKEUP_3CH)
#define GET_WAKEUP_BEAM_SPEED_SRC_3CH(fr) ((fr).header.headerInfo2 & MASK_HEADER_SOURCE_WAKEUP_3CH)

#define SET_ROUTING_DEST_WAKEUP_3CH(fr, wakeupspeed) (*(&(fr).singlecastRouted3ch.destWakeup - (MAX_REPEATERS - GET_ROUTE_LEN_3CH(fr))) |= (uint8_t)wakeupspeed)
#define CLR_ROUTING_DEST_WAKEUP_3CH(fr)   (*(&(fr).singlecastRouted3ch.destWakeup - (MAX_REPEATERS - GET_ROUTE_LEN_3CH(fr))) = 0)
#define GET_ROUTING_DEST_WAKEUP_3CH(fr)  (*(&(fr).singlecastRouted3ch.destWakeup \
           - (MAX_REPEATERS - GET_ROUTE_LEN_3CH(fr))) & MASK_ROUTED_DEST_WAKEUP_3CH)

/* Get pointer to payload (payload starts _after_ extension header) */
#define SINGLECAST_ROUTED_PAYLOAD_3CH(fr)     (&((fr).singlecastRouted3ch.destWakeup)-(MAX_REPEATERS-GET_ROUTE_LEN_3CH(fr)) + 1 \
                                           + (GET_EXTEND_PRESENT_3CH(fr) ? 1 + GET_SINGLECAST_HEADER_EXTENSION_LENGTH_3CH(fr) : 0))

#define ROUTED_ACK_PAYLOAD_3CH(fr)            ( (fr).routedACK3ch.repeaterList + GET_ROUTE_LEN_3CH(fr) \
                                           + (GET_EXTEND_PRESENT_3CH(fr) ? 1 + GET_ROUTED_ACK_HEADER_EXTENSION_LENGTH_3CH(fr): 0))

/* Get the number of bytes in the Payload */
/* 3 is routeStatus, numRepsNumHops and destWakeup bytes */
#define SINGLECAST_ROUTED_PAYLOAD_LEN_3CH(fr)  ((fr).header3ch.length-(sizeof(frameHeaderSinglecast3ch)-sizeof(frameHeaderExtension3ch) \
                                                + GET_ROUTE_LEN_3CH(fr)+3) \
                                                - (GET_EXTEND_PRESENT_3CH(fr) ? 1 + GET_ROUTED_SINGLECAST_HEADER_EXTENSION_LENGTH_3CH(fr) : 0) \
                                                - CHECKSUM_LENGTH)

/* 2 is routeStatus byte and numRepsNumHops byte */
#define ROUTED_ACK_PAYLOAD_LEN_3CH(fr)         ((fr).header3ch.length -(sizeof(frameHeaderSinglecast3ch)-sizeof(frameHeaderExtension3ch) \
                                                + (GET_ROUTE_LEN_3CH(fr) + 2) \
                                                + (GET_EXTEND_PRESENT_3CH(fr) ? 1 + GET_ROUTED_ACK_HEADER_EXTENSION_LENGTH_3CH(fr) : 0) \
                                                + CHECKSUM_LENGTH))

/* Get routing header as array */
#define SINGLECAST_ROUTING_HEADER_3CH(fr)     (&((fr).singlecastRouted3ch.routeStatus))

/* Direct range singlecast frame */
#define SINGLECAST_PAYLOAD_3CH(fr)           (&((fr)->singlecast3ch.destinationID) \
                                              + (GET_EXTEND_PRESENT_3CH(*fr) ? 2 + GET_SINGLECAST_HEADER_EXTENSION_LENGTH_3CH(*fr) : 1))

/****************************************/
/* Macros for use with multicast frames */
/****************************************/
#define MULTICAST_PAYLOAD_3CH(fr) ((fr)->multicast3ch.destinationMask + ((fr)->multicast3ch.addrOffsetNumMaskBytes & MASK_MULTI_BYTES) + GET_MULTICAST_HEADER_EXTENSION_LENGTH_3CH(*fr))
#define MULTICAST_PAYLOAD_LENGTH_3CH(fr) ((fr)->header.length - ((sizeof(frameHeaderMulticast3ch) - sizeof(frameHeaderExtension3ch) + GET_MULTICAST_HEADER_EXTENSION_LENGTH_3CH(*fr) - ZW_MAX_NODES/8 + ((fr)->multicast3ch.addrOffsetNumMaskBytes & MASK_MULTI_BYTES) + CHECKSUM_LENGTH)))

/* Get the offset of a multicast frame */
#define MULTICAST_OFFSET_3CH(fr)  ((fr).multicast3ch.addrOffsetNumMaskBytes & MASK_MULTI_OFFSET)
/* Number of addres bytes in destination mask */
#define MULTICAST_ADDRESS_LEN_3CH(fr)  ((fr).multicast3ch.addrOffsetNumMaskBytes & MASK_MULTI_BYTES)
/* Get the ReceiverMask as an array */
#define RECEIVER_MASK_3CH(fr)         (&((fr).multicast3ch.destinationMask[0]))


/****************************************/
/* Macros for use with explore frames */
/****************************************/
/* Get the Payload as an array */
#define EXPLORE_PAYLOAD_3CH(fr)       (&(fr).explore3ch.repeaterList[MAX_REPEATERS])
/* Get the number of bytes in the payload*/
#define EXPLORE_PAYLOAD_LEN_3CH(fr)   ((fr).header.length - sizeof(frameHeaderExplore3ch) - 2*sizeof(uint8_t))


/**************************************************/
/* Definition of the Extended route header format */
/**************************************************/
/* Definitions for type of Extend body  */
/* 0 left unused so 1 has same meaning as subGhz protocol */
#define EXTEND_TYPE_RESERVED_3CH                    0
#define EXTEND_TYPE_RSSI_INCOMING_3CH               1
/* The length of an EXTEND_TYPE_RSSI_INCOMING header */
#define EXTEND_TYPE_RSSI_INCOMING_LEN_3CH 4

/* Macros for Extend body handling to be used when accessing via received frame */
/* This is a pure macro version of the previous one */
#define GET_EXTEND_TYPE2_3CH(fr) (((fr).singlecastRouted3ch.repeaterList[GET_ROUTE_LEN_3CH(fr)]) >> 4)

#define ROUTING_HEADER_CONSTANT_LENGTH_3CH  3
/********************************************/
/********************************************/
/* Long range types and macro definitions    */
/********************************************/
/********************************************/

#define MAX_SINGLECAST_PAYLOAD_LR        (RX_MAX_LR - sizeof(frameHeaderSinglecastLR) - CRC16_LENGTH)
#define MAX_MULTICAST_PAYLOAD_LR         (RX_MAX_LR - sizeof(frameHeaderMulticastLR) - CRC16_LENGTH)

/***************************************/
/* Macros for reading the frameheaders */
/***************************************/
#define IS_SINGLECAST_LR(fr)   (((fr).headerLR.headerInfo & MASK_HDRTYP_LR)==HDRTYP_SINGLECAST)
#define IS_MULTICAST_LR(fr)    (((fr).headerLR.headerInfo & MASK_HDRTYP_LR)==HDRTYP_MULTICAST)
#define IS_TRANSFERACK_LR(fr)  (((fr).headerLR.headerInfo & MASK_HDRTYP_LR)==HDRTYP_TRANSFERACK)
#define DO_ACK_LR(fr)          ((fr).headerLR.headerInfo & MASK_ACKNOWLEDGE_LR)
#define IS_ACK_LR(fr)          ((fr).headerLR.headerInfo & MASK_ACKNOWLEDGE_LR)

/*
  Note that SET_HEADERTYPE *MUST* be used *BEFORE* SET_OUTGOING/INCOMING and
   SET_ROUTED/NON_ROUTED!
*/
#define GET_HEADERTYPE_LR(fr)      ((fr).headerLR.headerInfo & MASK_HDRTYP_LR)
#define SET_HEADERTYPE_LR(fr, type) ((fr).headerLR.headerInfo = (((fr).headerLR.headerInfo & ~MASK_HDRTYP_LR) | type))
#define SET_ACK_LR(fr)         ((fr).headerLR.headerInfo |= MASK_ACKNOWLEDGE_LR)

/* These macros replace the deprecated state variables FRAME_x, x=SINGLE,ROUTED,MULTI,ACK,FLOODED */
#define IS_FRAME_SINGLECAST_LR(fr) IS_SINGLECAST_LR(fr)
#define IS_FRAME_ACK_LR(fr) IS_ACK_LR(fr)
/* TO-DO: Implement IS_FRAME_FLOODED and _MULTI */
#define IS_FRAME_MULTI_LR(fr) IS_MULTICAST_LR(fr)

#define GET_SEQNUMBER_LR(fr)         ((fr).headerLR.sequenceNumber)
#define SET_SEQNUMBER_LR(fr, seqno)  ((fr).headerLR.sequenceNumber = seqno)
#define GET_NOISEFLOOR_LR(fr)        ((fr).headerLR.noiseFloor)
#define GET_TXPOWER_LR(fr)           ((fr).headerLR.txPower)
/* Get the Payload as an array */
#define SINGLECAST_PAYLOAD_LR(fr)           (((uint8_t*)&(fr)->singlecastLR.extension) + ( ((fr)->headerLR.headerInfo & MASK_HEADER_EXTENDED_LR) ? (1 + ((fr)->singlecastLR.extension.extensionInfo & MASK_EXTENSION_LENGTH_LR)) : 0))

/* Get the number of bytes in the Payload */
#define SINGLECAST_PAYLOAD_LEN_LR(fr) ((fr).headerLR.length-sizeof(frameHeaderLR))

/****************************************/
/* Macros for use with multicast frames */
/****************************************/

/* Get the offset of a multicast frame */
#define MULTICAST_OFFSET_LR(fr)  ((fr).multicastLR.addrOffsetNumMaskBytes & MASK_MULTI_OFFSET_LR)
/* Number of addres bytes in destination mask */
#define MULTICAST_ADDRESS_LEN_LR(fr)  ((fr).multicastLR.addrOffsetNumMaskBytes & MASK_MULTI_BYTES_LR)
/* Get the ReceiverMask as an array */
#define RECEIVER_MASK_LR(fr)         (&((fr).multicast3ch.destinationMask[0]))

/* Get the Payload as an array */
#define MULTICAST_PAYLOAD_LR(fr) ((fr)->multicastLR.destinationMask + ((fr)->multicastLR.addrOffsetNumMaskBytes & MASK_MULTI_BYTES))
/* Get the number of bytes in the payload*/
#define MULTICAST_PAYLOAD_LEN_LR(fr) ((fr).headerLR.length - sizeof(frameHeaderLR) - 2*sizeof(uint8_t) - ((fr).multicastLR.addrOffsetNumMaskBytes  & MASK_MULTI_BYTES_LR))

/****************************************************************************/
/*                              EXPORTED DATA                               */
/****************************************************************************/

extern node_id_t g_nodeID;                  /* This nodes Node-ID */


#endif /* _ZW_PROTOCOL_H_ */
