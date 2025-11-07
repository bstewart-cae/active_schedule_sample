// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file ZW_MAC.h
 * @copyright 2022 Silicon Laboratories Inc.
 *
 * @brief Z-Wave Medium Access Control (MAC) functions.
 */
#ifndef _ZW_MAC_H_
#define _ZW_MAC_H_

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/
#include <ZW_typedefs.h>

/****************************************************************************/
/*                     EXPORTED TYPES and DEFINITIONS                       */
/****************************************************************************/
#define BITRATE  9600

typedef enum
{
  RADIO_STATUS_TX_COMPLETE,
  RADIO_STATUS_TX_FAIL_LBT,
  RADIO_STATUS_TX_FAIL,
} ZW_Radio_Status_t;

#define RF_TxComplete                 2  /* set if the transmission complete  */
#define RF_TxFail			          7

/* Transmit states */
#define TX_IDLE                       0
#define	TX_SYNC	                      1
#define	TX_SOF	                      2
#define TX_SOF2                       9
#define TX_BEAM_ADDR                  10
#define TX_HASH                       11
#define TX_FRAME                      12

#define	TX_HEAD	                      3
#define	TX_DATA	                      4
#define	TX_CHKSUM                     5
#define	TX_EOF	                      6
#define TX_EOF2                       7
#define TX_END                        8

#define	SYNC_BYTE                     0x55  // Preamble byte
#define	SOF_BYTE                      0xF0
#define	TX_EOF_BITS                   8     /* Number of low bits to send at end of frame */

#define RF_SPEED_9_6K                 0x0001
#define RF_SPEED_40K                  0x0002
#define RF_SPEED_100K                 0x0003
#define RF_SPEED_LR_100K              0x0004

/* Transmit options used in MAC layer (RFTransmitNow) */
/* Bits 0-2 are speed, bit 3-7 are bit flags */
#define RF_OPTION_SPEED_NONE          0x0000
#define RF_OPTION_SPEED_9600          RF_SPEED_9_6K
#define RF_OPTION_SPEED_40K           RF_SPEED_40K
#define RF_OPTION_SPEED_100K          RF_SPEED_100K
#define RF_OPTION_SPEED_LR_100K       RF_SPEED_LR_100K
#define RF_OPTION_RESERVED_SPEED1     0x0005
#define RF_OPTION_RESERVED_SPEED2     0x0006
#define RF_OPTION_RESERVED_SPEED3     0x0007
#define RF_OPTION_LOW_POWER           0x0008
#define RF_OPTION_LONG_PREAMBLE       0x0010
#define RF_OPTION_SEND_BEAM_250MS     0x0020
#define RF_OPTION_SEND_BEAM_1000MS    0x0040
#define RF_OPTION_SEND_BEAM_FRAG      RF_OPTION_SEND_BEAM_1000MS  // Both beam flags would have worked here.
#define RF_OPTION_BROADCAST_HOMEID    0x0080
#define RF_OPTION_BEAM_ACK            0x0200
#define RF_OPTION_FRAGMENTED_BEAM     0x0400

#define RF_OPTION_SPEED_MASK          0x07
#define RF_OPTION_BEAM_MASK           (RF_OPTION_SEND_BEAM_1000MS | RF_OPTION_SEND_BEAM_250MS)

/* Defines to get and set the speed of a TxQueue element */
#define GET_SPEED(fr)                 ((fr->wRFoptions) & RF_OPTION_SPEED_MASK)
#define SET_SPEED(fr, sp)             (((fr)->wRFoptions) = ((fr)->wRFoptions & ~RF_OPTION_SPEED_MASK) | (sp))

#define RF_PREAMP_SHORT               0x00
#define RF_PREAMP_LONG                0x01
#define CLEAR_PREAMP_MASK             0xFE

#define ZWAVE_FRAME_LEN_OFFS          8

#define MAX_POWERLEVELS               14
#define MAX_POWERLEVELS_100           16

#define HOME_ID_HASH_2CH_MASK         0xBF /* Don't use bit 6 */
/* Illegal hash values caused by 3 bit error in 6.5x devkits */
#define HOME_ID_HASH_2CH_ILLEGAL1     SYNC_BYTE // From devkit 4.5x
#define HOME_ID_HASH_2CH_ILLEGAL2     0x4A      // From devkit 6.5x
#define HOME_ID_HASH_2CH_ILLEGAL3     0x0A      // From devkit 6.5x

#define HOME_ID_HASH_3CH_MASK         0x9F /* Don't use bit 6 and 5 */
/* Illegal hash values caused by 3 bit error in 6.5x devkits */
#define HOME_ID_HASH_3CH_ILLEGAL1     SYNC_BYTE // From devkit 6.6x
#define HOME_ID_HASH_3CH_ILLEGAL2     0x05  // From devkit 6.5x
#define HOME_ID_HASH_3CH_ILLEGAL3     0x25  // From devkit 6.5x
#define HOME_ID_HASH_3CH_ILLEGAL4     0x45  // From devkit 6.5x
#define HOME_ID_HASH_3CH_ILLEGAL5     0x65  // From devkit 6.5x
#define HOME_ID_HASH_3CH_INCREASED1   0x4A  // From devkit 6.6x
#define HOME_ID_HASH_3CH_INCREASED2   0x0A  // From devkit 6.6x

/* Number of milliseconds we pause tx to avoid collisions with repeated frames */
/* Calculation: the time needed to routed routed ack frames over to hops at 9.6K pause needed */
/*For 2 ch system TRANSMIT_PAUSE_COUNT_VALUE = (((20 bytes preamble + 16 routed ack frame) * 8)/ 9600) * 2 = 60 ms */
/*For 3 ch system TRANSMIT_PAUSE_COUNT_VALUE = (((24 bytes preamble + 17 routed ack frame) * 8)/ 100K) * 2 =6.56 ms*/
#define TRANSMIT_PAUSE_COUNT_VALUE    1 // rounding up from 4.1 ms

#define MAC_TRANSMIT_DELAY_MS         60  // Used to pause all transmission to avoid collision with a routed ACK.


#endif /* _ZW_MAC_H_ */
