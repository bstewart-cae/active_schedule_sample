// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file
 * @copyright 2022 Silicon Laboratories Inc.
 */
#ifndef _ZW_RECEIVEFILTER_TRANSPORT_H_
#define _ZW_RECEIVEFILTER_TRANSPORT_H_

#include <stdint.h>

/**
 * @defgroup zwrf Z-Wave Receive Filter Transport
 *
 * Module for setting Receive filters for Z-Wave frames for transport module.
 *
 * This module removes any active receive filters and adds needed receive filters for transport module.
 *
 * @section receiving Z-Wave Frame receiving
 *
 * Module dependencies for frame receiving can be seen in the following graph.
 *
 * @startuml
 * object "Received Frame" as rf
 * object "transport receive filter" as trf
 * object "ReceiveHandler" as fh
 * object "Protocol frame received" as pfr
 * object "Application frame received" as afr
 * object "Frame filter rejected" as ffr
 * object "Frame ReceiveHandler rejected" as frr
 *
 * rf --> trf
 * trf --> fh
 * trf --> ffr
 * fh --> pfr
 * fh --> afr
 * fh --> frr
 *
 * @enduml
 *
 *
 *
 * @{
 */

/**Function for setting default transport receive filters.
 *
 * When the function returns SUCCESS the default transport receive filters have been added to list of active filters.
 *
 * @retval SUCCESS             The default transport receive filters was successfully set as the active filters
 * @retval UNSUPPORTED         Unsupported LearnMode requested
 */
ZW_ReturnCode_t rfTransportFilter_Set(void);


/**
 * @}
 */

#endif // _ZW_TRANSPORT_RECEIVEFILTER_H_


