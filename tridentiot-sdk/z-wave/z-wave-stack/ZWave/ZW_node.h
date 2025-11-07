// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file ZW_node.h
 * @copyright 2022 Silicon Laboratories Inc.
 */
#ifndef ZWAVE_ZW_NODE_H_
#define ZWAVE_ZW_NODE_H_

#include <stdint.h>
#include <ZW_typedefs.h>
#include <zpal_radio.h>

/**
 * @brief Function for setting node Device Options
 *
 */
void ZW_nodeDeviceOptionsSet(uint8_t newDeviceOptions);


/**
 * @brief Function for getting nodes Device Options
 *
 * @return node Device options
 */
uint8_t ZW_nodeDeviceOptionsGet(void);


/**
 * @brief Function for determining if node is a FLiRS
 *
 * @return non zero if node is a FLiRS, value reflect the FLiRS type
 */
uint8_t ZW_nodeIsFLiRS(void);


/**
 * @brief Function for determining if node is a listening node
 *
 * @return non zero if node is a listening node
 */
uint8_t ZW_nodeIsListening(void);


/**
 * @brief Function for determining if node is a long Range node.
 *
 * @return true if node is Long Range. false if the node is NOT Long Range.
 */
bool ZW_nodeIsLRNodeID(node_id_t nodeID);

/************************************************************************************
 * Node attributes parameters
 ***********************************************************************************/

/*
 * Set the globally accessible HomeID to 0.
 */
void ZW_HomeIDClear(void);

/*
 * Set the globally accessible HomeID.
 */
void ZW_HomeIDSet(uint8_t* homeID);


/*
 * Get the current HomeID of this node.
 */
uint8_t* ZW_HomeIDGet(void);

/*
 * Set the currently assigned NodeID of this node.
 * This does not write into NVM.
 */
void ZW_NodeIDSet(node_id_t assignedNodeID);

/*
 * Get the current NodeID of this node.
 */
node_id_t ZW_NodeIDGet(void);

#endif /* ZWAVE_ZW_NODE_H_ */
