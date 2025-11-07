// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file ZW_protocolGHz.h
 * @copyright 2022 Silicon Laboratories Inc.
 *
 * @brief Z-Wave protocol defines.
 */
#ifndef _ZW_PROTOCOLGHZ_H_
#define _ZW_PROTOCOLGHZ_H_

#define RX_MAX_3CH          170         /* Max frame that can be received */
#define RX_MAX_LEGACY_3CH   RX_MAX_3CH  /* Only used for coding reasons */

/* Frame header mask bits for the 6' byte in the header */
//#define MASK_HDRTYP                     0x0f
#define MASK_ACKNOWLEDGE_3CH              0x80
#define MASK_LOWPOWER_3CH                 0x40
#define MASK_MULTICAST_FOLLOWUP_3CH       0x20

/* Frame header mask bits for the 7' byte in the header */
#define MASK_HEADER_EXTENDED_3CH          0x80
#define MASK_HEADER_SOURCE_WAKEUP_3CH     0x70
#define HEADER_SOURCE_WAKEUP_250MS_3CH    0x10
#define HEADER_SOURCE_WAKEUP_1000MS_3CH   0x20

/* Max size of the extended header */
#define MAX_EXTEND_BODY_3CH               7

/* Mask for the extension length */
#define MASK_EXTENSION_LENGTH_3CH         0x07

/* Mask for destination wakeup in routed frames */
#define MASK_ROUTED_DEST_WAKEUP_3CH       0x07

/************************************************/
/* Definition of the common frame header format */
/************************************************/
typedef struct _frameHeader3ch_
{
  uint8_t	homeID[HOMEID_LENGTH];
  uint8_t	sourceID;
  uint8_t  headerInfo;
  uint8_t  headerInfo2;
  uint8_t  length;
  uint8_t  sequenceNumber;
} frameHeader3ch;

/**********************************************************/
/* Definition of the common frame header extension format */
/**********************************************************/
typedef struct _frameHeaderExtension3ch_
{
  uint8_t extensionLength;
  uint8_t extensionBody[MAX_EXTEND_BODY_3CH];
} frameHeaderExtension3ch;

/************************************************/
/* Definition of the singlecast frame header format */
/************************************************/
typedef struct _frameHeaderSinglecast3ch_
{
  frameHeader3ch            header;
  uint8_t                   destinationID;
  frameHeaderExtension3ch   extension;
} frameHeaderSinglecast3ch;

/************************************************/
/* Definition of the multicast frame header format */
/************************************************/
typedef struct _frameHeaderMulticast3ch_
{
  frameHeader3ch            header;
  uint8_t                   addrOffsetNumMaskBytes;
  uint8_t                   destinationMask[ZW_MAX_NODES/8];
  frameHeaderExtension3ch   extension;
} frameHeaderMulticast3ch;

/************************************************/
/* Definition of the routed singlecast frame header format */
/************************************************/
typedef struct _frameHeaderSinglecastRouted3ch_
{
  frameHeader3ch  header;
  uint8_t                   destinationID;
  uint8_t                   routeStatus;    /* RoutingERR, RoutingACK, RoutingDIR */
  uint8_t                   numRepsNumHops; /* NumRepeaters, NumHops */
  uint8_t                   repeaterList[MAX_REPEATERS];
  uint8_t                   destWakeup;
  frameHeaderExtension3ch   extension;
} frameHeaderSinglecastRouted3ch;

/************************************************/
/* Definition of the routed ACK frame header format */
/************************************************/
typedef struct _frameHeaderRoutedACK3ch_
{
  frameHeader3ch  header;
  uint8_t                  destinationID;
  uint8_t                  routeStatus;    /* RoutingERR, RoutingACK, RoutingDIR */
  uint8_t                  numRepsNumHops; /* NumRepeaters, NumHops */
  uint8_t                  repeaterList[MAX_REPEATERS];
  frameHeaderExtension3ch  extension;
} frameHeaderRoutedACK3ch;

/************************************************/
/* Definition of the routed error frame header format */
/************************************************/
typedef frameHeaderRoutedACK3ch frameHeaderRoutedERR3ch;

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
typedef struct _frameHeaderExplore3ch_
{
//  frameHeader3ch            header;
//  uint8_t                   destinationID;
  uint8_t                   ver_Cmd;
  uint8_t                   option;
  uint8_t                   sessionTxRandomInterval;
  uint8_t                   repeaterCountSessionTTL;
  uint8_t                   repeaterList[MAX_REPEATERS];
} frameHeaderExplore3ch;

/**********************************************/
/* Definition of the transferACK frame format */
/**********************************************/
typedef struct _frameTransferACK3ch_
{
  frameHeader3ch            header;
  uint8_t                   destinationID;
  frameHeaderExtension3ch   extension;
} frameHeaderTransferACK3ch;

#define frameTransferACK3ch frameHeaderTransferACK3ch

#endif /* _ZW_PROTOCOLGHZ_H_ */
