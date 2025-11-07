// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file ZW_noise_detect.h
 * @copyright 2022 Silicon Laboratories Inc.
 *
 * @brief Header file for noise detect module.
 * This is an internal header file for the protocol.
 * API functions from this module are defined in ZW_basis_api.h
 */
#ifndef ZW_NOISE_DETECT_H_
#define ZW_NOISE_DETECT_H_

/*============================   CalculateRssi   ===============================
**
**  Given a variable length array of raw rssi samples return the average rssi
**  value in dBms. This is typically used to turn a number of RSSI samples taken
**  during frame reception into a single value for the entire frame.
**
**  It is also used to convert background rssi samples into dBms.
**
**  The conversion from arbitrary units to dBms happen according
**  to a measured linear relationship of
**  f_dBm(x) = (155 * x - 16269)/ 100
**  This relationship holds for a ZW chip without Z-Wave filter.
**
**  The conversion has been shifted by ~ 7dBm to bring it as close as possible
**  to the values used for LBT threshold. Those values assume a system loss
**  including a SAW filter and also add a safety margin for variations across
**  temperature and other factors. The modified conversion formula is
**  g(x) = (155 * x - 15654) / 100
**
**  PLEASE note that the result is a SIGNED char. Typical values are in
**  the range -100 to -40.
**
**  If any sample contains one of the special values ZPAL_RADIO_RSSI_NOT_AVAILABLE or
**  RSSI_ERROR_MISSING that value is returned without averaging.
**
**  RSSI values outside the linear are are rounded up or down to
**  RSSI_MAX_POWER_SATURATED and RSSI_BELOW_SENSITIVITY respectively.
**
**-------------------------------------------------------------------------*/
signed char                            /* RET RSSI value as specified in comment */
CalculateRssi(uint8_t *rssi_array,     /* IN  RSSI sample array taken during frame
                                          reception */
    uint8_t len);                      /* IN  Length of rssi_array*/

/*=====================   NoiseDetectInit   ========================
**
**  Power-on initialization of noise detect module.
**  Protocol must call this function on power-on.
**
**-------------------------------------------------------------------------*/
void
NoiseDetectInit(void);

/*========================   SampleNoiseLevel   ============================
**
**  Sample instantaneous noise level and update noise estimate.
**  Poll this function frequently, e.g. from main loop.
**
**  Side effects: Can force RAIL into RX mode.
**
**
**-------------------------------------------------------------------------*/
void SampleNoiseLevel(void);

#endif /* ZW_NOISE_DETECT_H_ */
