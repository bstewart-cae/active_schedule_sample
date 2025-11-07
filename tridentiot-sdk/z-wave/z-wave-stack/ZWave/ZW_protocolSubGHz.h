// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file ZW_protocolSubGHz.h
 * @copyright 2022 Silicon Laboratories Inc.
 *
 * @brief Z-Wave protocol defines.
 */
#ifndef _ZW_PROTOCOLSUBGHZ_H_
#define _ZW_PROTOCOLSUBGHZ_H_

#define RX_MAX            170       /* Max frame that can be received */
#define RX_MAX_LEGACY     64

//#define CHECKSUM_LENGTH   1
//#define CRC16_LENGTH      2
//
/* Frame header mask bits for the 6' byte in the header */
#define MASK_HDRTYP                 0x0f
#define MASK_SEQNO                  0x0f
#define MASK_SPEED_MODIFIED         0x10
#define MASK_LOWPOWER               0x20
#define MASK_ACKNOWLEDGE            0x40
#define MASK_ROUTED                 0x80

/* Routing information fields */
#define MASK_ROUT_SPEED_MODIFIED    0x10

// Frame header bits for the 7' byte (reserved) in the header
#define MASK_MULTICAST_FOLLOWUP     0x80

/* Mask for the frame source beam wakeup speed bits in the frame header reserved byte */
#define MASK_HEADER_WAKEUP_BEAM_SPEED_SRC           0x60
#define MASK_HEADER_WAKEUP_BEAM_SPEED_SRC_1000      0x40
#define MASK_HEADER_WAKEUP_BEAM_SPEED_SRC_250       0x20
#define MASK_HEADER_WAKEUP_BEAM_SPEED_SRC_RESERVED  0x60
#define MASK_HEADER_WAKEUP_BEAM_SPEED_SRC_NONE      0x00

/* Max optional extend body size in routing header */
#define MAX_EXTEND_BODY 2  /*Size is selected out from code review of lower layer: PSH & JSI*/

/************************************************/
/* Definition of the common frame header format */
/************************************************/
typedef struct _frameHeader_
{
  uint8_t	homeID[HOMEID_LENGTH];
  uint8_t	sourceID;
  uint8_t  headerInfo; /* Route (1), Direction (1) and HeaderType (4) */
  uint8_t  reserved;
  uint8_t  length;
} frameHeader;
typedef frameHeader * tpFrameHeader;

/************************************************/
/* Definition of the singlecast frame header format */
/************************************************/
typedef struct _frameHeaderSinglecast_
{
  frameHeader header;
  uint8_t     destinationID;
} frameHeaderSinglecast;

/************************************************/
/* Definition of the multicast frame header format */
/************************************************/
typedef struct _frameHeaderMulticast_
{
  frameHeader header;
  uint8_t     addrOffsetNumMaskBytes;
  uint8_t     destinationMask[ZW_MAX_NODES/8];
} frameHeaderMulticast;

/************************************************/
/* Definition of the routed singlecast frame header format */
/************************************************/
typedef struct _frameHeaderSinglecastRouted_
{
  frameHeader header;
  uint8_t        destinationID;
  uint8_t        routeStatus;    /* RoutingERR, RoutingACK, RoutingDIR */
  uint8_t        numRepsNumHops; /* NumRepeaters, NumHops */
  uint8_t        repeaterList[MAX_REPEATERS];
  uint8_t        extendBody[MAX_EXTEND_BODY];
} frameHeaderSinglecastRouted;

/************************************************/
/* Definition of the routed ack frame header format */
/************************************************/
/* This is defined for consistency with GHz protocol */
/* In GHz systems, these two are different. */
typedef frameHeaderSinglecastRouted frameHeaderRoutedACK;

/************************************************/
/* Definition of the routed error frame header format */
/************************************************/
typedef frameHeaderRoutedACK frameHeaderRoutedERR;

/*************************************************/
/* Definition of the explore frame header format */
/*                                               */
/* header:                                       */
/*    normal Z-Wave header                       */
/*                                               */
/* destinationID:                                */
/*    Node to receive the frame                  */
/*                                               */
/* ver_cmd:                                      */
/*    cmd [b0-b4], ver [b5-b7]                   */
/* ver is the version on the configuration       */
/* parameter set in the frame header             */
/*                                               */
/* option:                                       */
/*    frame option byte                          */
/*      sourceRouted [b0]                        */
/*      direction [b1]                           */
/*                                               */
/* repeaterCountTTL:                             */
/*    repeaterCount [b0-b3],                     */
/*    TTL [b4-b7]                                */
/*                                               */
/* repeaterList:                                 */
/*    the list of repeaters which the explore    */
/*    frame has traversed. Array size has a      */
/*    fixed indicated by the repeaterCount       */
/*    thereby making room for the wanted max     */
/*                                               */
/*************************************************/
typedef struct _frameHeaderExplore_
{
//  frameHeader               header;
//  uint8_t                      destinationID;
  uint8_t                      ver_Cmd;
  uint8_t                      option;
  uint8_t                      sessionTxRandomInterval;
  uint8_t                      repeaterCountSessionTTL;
  uint8_t                      repeaterList[MAX_REPEATERS];
} frameHeaderExplore;

/*********************************************/
/* Definition of the singlecast frame format */
/*********************************************/
typedef struct _frameSinglecast_
{
  frameHeader header;
  uint8_t        destinationID;
  uint8_t        startOfPayload;
} frameSinglecast;


/********************************************/
/* Definition of the multicast frame format */
/********************************************/
typedef struct _frameMulticast_
{
  frameHeader header;
  uint8_t        addrOffsetNumMaskBytes;
  uint8_t        startOfDestinationMask;
} frameMulticast;


/****************************************************/
/* Definition of the routed singlecast frame format */
/****************************************************/
typedef struct _frameSinglecastRouted_
{
  frameHeader header;
  uint8_t        destinationID;
  uint8_t        routeStatus;    /* RoutingERR, RoutingACK, RoutingDIR */
  uint8_t        numRepsNumHops; /* NumRepeaters, NumHops */
  uint8_t        startOfRepeaterList;
} frameSinglecastRouted;

/**********************************************/
/* Definition of the transferACK frame format */
/**********************************************/
typedef struct _frameTransferACK_
{
  frameHeader header;
  uint8_t     destinationID;
} frameTransferACK;


/***********************************************************/
/* Macros for accessing the checksum of frames of any type */
/***********************************************************/
#define READ_CHECKSUM(fr)    ((fr).buffer[(fr).header.Length-1])

#endif /* _ZW_PROTOCOLSUBGHZ_H_ */
