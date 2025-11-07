// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file ZW_lr_virtual_node_id.h
 * @copyright 2022 Silicon Laboratories Inc.
 */
#ifndef ZWAVE_ZW_LR_VIRTUAL_NODE_ID_H_
#define ZWAVE_ZW_LR_VIRTUAL_NODE_ID_H_


#include <zpal_radio.h>

typedef enum 
{
  LR_VIRTUAL_ID_ENABLED = 0,  /*< The id is virtual and it is enabled*/
  LR_VIRTUAL_ID_DISABLED,     /*< The id is virtual and it is disabled*/
  LR_VIRTUAL_ID_INVALID      /*< The id is not isnot valid virtual id (Normal LongeRange ID)*/
} LR_VIRTUAL_ID_TYPE;

/**
*   Enables LongRange virtual nodeIDs
* @param[in]    bitmask   The lowest 4 bits enables nodeID 4002, 4003, 4004 and 4005.
*
*/
void ZW_LR_EnableVirtualIDs(uint8_t bitmask);


/**
*   Check the type of LoneRange node ID
* @param[in]    nodeID
*
* @return       the type of the node ID
*/
LR_VIRTUAL_ID_TYPE ZW_Get_lr_virtual_id_type(node_id_t nodeID);

/**
*   Drop frame that don't comply with LR virtual id rules
*
*  In slaves we drop protocol commands frames with the source ID is LR virtual id
*  In controllers we drop protocol commands frames with the destination ID is LR virtual id
*  In controllers we drop application commands frames with the destination ID is LR virtual id
*  and the id is not enabled
*  
* @param[in]    cmdClass      the command class
* @param[in]    nodeID
*
* @return       true to drop the frame, else false
*/
bool ZW_Drop_frame_with_lr_virtual_id(uint8_t cmdClass, node_id_t node_id);

#endif /*ZWAVE_ZW_LR_VIRTUAL_NODE_ID_H_*/
