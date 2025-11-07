/// ***************************************************************************
///
/// @file ZW_region_rf_definitions.h
///
/// SPDX-License-Identifier: LicenseRef-TridentMSLA
/// SPDX-FileCopyrightText: 2025 Trident IoT, LLC <https://www.tridentiot.com>
/// ***************************************************************************

#ifndef _ZW_REGION_RF_DEFINITIONS_H_
#define _ZW_REGION_RF_DEFINITIONS_H_

#include <zpal_radio.h>

/****************************************************************************/
/*                     EXPORTED TYPES and DEFINITIONS                       */
/****************************************************************************/

typedef struct _zw_region_rf_setup_
{
  zpal_radio_protocol_mode_t protocolMode;
  int16_t                    LBTRssiLevel;
  uint16_t                   LBTRssiTime;
  uint16_t                   LBTChannelLockTime;
  uint16_t                   LBTChannelTxTime;
  uint16_t                   channel_count;
  uint32_t                   channel_frequency_khz[4];
  uint16_t                   channel_baudrate[4];
  uint16_t                   fixed_rx_channel;
  uint16_t                   use_lr_backup_channel : 1;
  uint16_t                   useRadioReceiveBeam : 1;
  uint16_t                   useFragementedBeam : 1;
  uint16_t                   use_fixed_rx_channel : 1;
} zw_region_rf_setup_t;

/* RSSI value set according to FNC10432-12 */
// We need adjust for T32CZ20 use
#define RSSI_65DB_LEVEL (-49)   // Adjusted to match Z-Wave modules
#define RSSI_75DB_LEVEL (-75)   /* EU/US LBT level */
#define RSSI_80DB_LEVEL (-71)   // Adjusted to match Z-Wave modules

#define LBT_TIME_DONT_USE (0)   // Don't use RSSI

// Use Fragmented BEAM functionality on BEAM Rx definitions
#define RADIO_FRAGEMENTED_BEAM_USE        1
#define RADIO_FRAGEMENTED_BEAM_DONT_USE   0

// Use defined fixed channel for Rx
// This is only usefull as long as no channelscan are working
#define RADIO_FIXED_RX_CHANNEL_USE      1
#define RADIO_FIXED_RX_CHANNEL_DONT_USE 0
#endif  /* _ZW_REGION_RF_DEFINITIONS_H_ */
