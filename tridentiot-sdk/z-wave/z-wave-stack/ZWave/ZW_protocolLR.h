// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file
 * Z-Wave protocol defines for the long range formate.
 * @copyright 2019 Silicon Laboratories Inc.
 */

#ifndef _ZW_PROTOCOLLR_H_
#define _ZW_PROTOCOLLR_H_

#define RX_MAX_LR            170       /* Max frame that can be received */

/* Frame header mask bits for the 9' byte in the header */
#define MASK_HDRTYP_LR                 0x07
#define MASK_HEADER_EXTENDED_LR        0x40
#define MASK_ACKNOWLEDGE_LR            0x80

/* Max size of the extended header */
#define MAX_EXTEND_BODY_LR                7

/* Mask for the extension length */
#define MASK_EXTENSION_LENGTH_LR       0x07

/* Frame header type definitions */
#define HDRTYP_SIMGLECAST_FRAME_LR     0x01
#define HDRTYP_ACK_FRAME_LR            0x03

/************************************************/
/* Definition of the common frame header format */
/************************************************/
typedef struct
{
  uint8_t   homeID[HOMEID_LENGTH];
  uint8_t   nodeIDs[3]; /* Source Node ID (12bit) and Destination Node ID (12bit) */
  uint8_t   length;
  uint8_t   headerInfo; /* Ack (1), Extended (1), Reserved (3) and HeaderType (3) */
  uint8_t   sequenceNumber;
  int8_t    noiseFloor;
  int8_t    txPower;
} frameHeaderLR;

/**********************************************************/
/* Definition of the common frame header extension format */
/**********************************************************/
typedef struct
{
  uint8_t   extensionInfo; /* Reserved (4), Discard unknown (1) and Extension length (3) */
  uint8_t   extensionBody[MAX_EXTEND_BODY_LR];
} frameHeaderExtensionLR;

/************************************************/
/* Definition of the singlecast frame header format */
/************************************************/
typedef struct
{
  frameHeaderLR           header;
  frameHeaderExtensionLR  extension;
} frameHeaderSinglecastLR;


/**********************************************/
/* Definition of the transferACK frame format */
/**********************************************/
typedef struct
{
  frameHeaderLR header;
  int8_t        receiveRSSI;
} frameTransferACKLR;



#endif /* _ZW_PROTOCOLLR_H_ */
