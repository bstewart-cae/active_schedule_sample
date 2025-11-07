// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file
 * @copyright 2022 Silicon Laboratories Inc.
 */
#ifndef _ZW_RECEIVEFILTER_LEARNMODE_H_
#define _ZW_RECEIVEFILTER_LEARNMODE_H_

#include <stdint.h>
#include <ZW_DataLinkLayer.h>
#include <ZW_basis_api.h>

#ifndef ZW_SET_LEARN_MODE_DISABLE
/* Mode parameters to ZW_SetLearnMode */
#define ZW_SET_LEARN_MODE_DISABLE 0x00
#define ZW_SET_LEARN_MODE_CLASSIC 0x01
#define ZW_SET_LEARN_MODE_NWI     0x02
#define ZW_SET_LEARN_MODE_NWE     0x03
#endif

/**
 * @defgroup zwrflm Z-Wave Receive Filter LearnMode
 *
 * Module for setting Receive filters for Z-Wave frames when node enters/exits LearnMode.
 *
 * This module handles pausing existing Receive Filters and set Receive Filters needed when specific LearnMode.
 *
 * @section learnmode receivefilters
 *
 * Module dependencies for learnmode receivefilters can be seen in the following graph.
 *
 * @startuml
 * object "Received Frame" as rf
 * object "normal mode Receive Filter" as nrf
 * object "Classic LearnMode Receive Filter" as clrf
 * object "NWI LearnMode Receive Filter" as nwirf
 * object "NWE LearnMode Receive Filter" as nwerf
 * object "ReceiveHandler" as fh
 * object "Protocol frame received" as pfr
 * object "Application frame received" as afr
 * object "Frame filter rejected" as ffr
 * object "Frame ReceiveHandler rejected" as frr
 *
 * rf --> nrf
 * nrf --> fh
 * nrf --> ffr
 * rf --> nwirf
 * nwirf --> fh
 * nwirf --> ffr
 * rf --> nwerf
 * nwerf --> fh
 * nwerf --> ffr
 * fh --> pfr
 * fh --> afr
 * fh --> frr
 *
 * rf --> clrf
 * clrf --> fh
 * clrf --> ffr
 *
 * @enduml
 *
 *
 *
 * @{
 */

/**Function for setting specific LearnMode receive filters.
 *
 * When the function returns SUCCESS the previous active receive filters has been put in pause mode and
 * the needed number of LearnMode filters has been added to list of active filters.
 * \note If Specified LearnMode equals E_NETWORK_LEARN_MODE_DISABLE then any LearnMode filter
 * active is removed and previous active filters are made active again.
 *
 *
 * @param[in] bMode E_NETWORK_LEARN_MODE_ACTION mode receive filters to enable
 * @param[in] nodeId NodeId on the node filter should be active on - NOT ACTIVE- USEFULL?
 * @param[in] pHomeId array of HOME_ID_LENGTH uint8_t comprising the HomeId the filter should be active on - NOT ACTIVE- USEFULL?
 *
 * @retval SUCCESS             The bMode filters was successfully add to the list of active filters
 * @retval UNSUPPORTED         Unsupported LearnMode requested
 */
ZW_ReturnCode_t rfLearnModeFilter_Set(E_NETWORK_LEARN_MODE_ACTION bMode, node_id_t nodeId, uint8_t const pHomeId[HOME_ID_LENGTH]);


/**
 * @}
 */

#endif // _ZW_RECEIVEFILTER_LEARNMODE_H_


