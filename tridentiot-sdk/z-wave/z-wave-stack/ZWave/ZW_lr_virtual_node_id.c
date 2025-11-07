// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file ZW_lr_virtual_node_id.c
 * @copyright 2022 Silicon Laboratories Inc.
 */
#include "ZW_lib_defines.h"
#include "ZW_lr_virtual_node_id.h"
#include "ZW_transport.h"

#define NR_OF_VIRTUAL_LR_IDS  4

#define VIRTUAL_LR_ID_1  4002
#define VIRTUAL_LR_ID_2  4003
#define VIRTUAL_LR_ID_3  4004
#define VIRTUAL_LR_ID_4  4005


static uint8_t lr_virtual_id_enabled = 0;


void ZW_LR_EnableVirtualIDs(uint8_t bitmask){
  lr_virtual_id_enabled = bitmask;
}

LR_VIRTUAL_ID_TYPE ZW_Get_lr_virtual_id_type(node_id_t nodeID){
  //Return false on invalid virtual node IDs
  if( (VIRTUAL_LR_ID_1 > nodeID) || (VIRTUAL_LR_ID_4 < nodeID) )
  {
    return LR_VIRTUAL_ID_INVALID;
  }
  else
  {
#ifdef ZW_SLAVE
     return LR_VIRTUAL_ID_ENABLED; // slaves always accept virtual nodes
#else
    return ((1 << (nodeID - VIRTUAL_LR_ID_1)) & lr_virtual_id_enabled) ? LR_VIRTUAL_ID_ENABLED: LR_VIRTUAL_ID_DISABLED;
#endif
  }
}


bool ZW_Drop_frame_with_lr_virtual_id(uint8_t cmdClass, node_id_t node_id)
{
#ifdef ZW_SLAVE
  /*We should never handle protocol frames with LR virtual source id*/
  if ((IS_PROTOCOL_CLASS(cmdClass) || IS_PROTOCOL_LR_CLASS(cmdClass)) &&
      (LR_VIRTUAL_ID_ENABLED == ZW_Get_lr_virtual_id_type(node_id)))
  {
    return true;
  }
  return false;
#endif
#ifdef ZW_CONTROLLER
  /*We should never handle protocol frames with LR virtual destination id*/
  if ((IS_PROTOCOL_CLASS(cmdClass) || IS_PROTOCOL_LR_CLASS(cmdClass)) &&
      (LR_VIRTUAL_ID_INVALID != ZW_Get_lr_virtual_id_type(node_id)))
  {
    return true;
  }
  /*We should never handle application frames with LR virtual destination id when this id is disabled*/
  uint16_t cmdClass_ext = cmdClass;
  if (IS_APPL_CLASS(cmdClass_ext) &&
      (LR_VIRTUAL_ID_DISABLED == ZW_Get_lr_virtual_id_type(node_id)))
  {
    return true;
  }
  return false;
#endif
}
