// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * Node mask manipulation function prototypes source file
 * @copyright 2019 Silicon Laboratories Inc.
 */

/****************************************************************************/
/*		                        INCLUDE FILES		                              */
/****************************************************************************/
#include <stdint.h>
#include "NodeMask.h"

/****************************************************************************
 *                          EXPORTED FUNCTIONS                              *
 ****************************************************************************/


void
ZW_NodeMaskSetBit(
  uint8_t* pMask,
  uint8_t bNodeID)
{
  bNodeID--;
  *(pMask+(bNodeID>>3)) |= (0x1 << (bNodeID & 7));
}

void
ZW_LR_NodeMaskSetBit(
  uint8_t* pMask,
  node_id_t nodeID)
{
  nodeID--;
  *(pMask + (nodeID >> 3)) |= (0x1 << (nodeID & 7));
}

void
ZW_NodeMaskClearBit(
  uint8_t* pMask,
  uint8_t bNodeID)
{
  bNodeID--;
  *(pMask+(bNodeID >> 3)) &= ~(0x1 << (bNodeID & 7));
}

void
ZW_LR_NodeMaskClearBit(
  uint8_t* pMask,
  node_id_t nodeID)
{
  nodeID--;
  *(pMask + (nodeID >> 3)) &= ~(0x1 << (nodeID & 7));
}

void
ZW_NodeMaskClear(
  uint8_t* pMask,
  uint8_t bLength)
{
  /* Clear entire node mask */
  if (bLength)
  {
    do
    {
      *pMask = 0;
      pMask++;
    } while (--bLength);
  }
}


uint8_t
ZW_NodeMaskBitsIn(
  uint8_t* pMask,
  uint8_t bLength)
{
  uint8_t t, count = 0;

  if (bLength)
  {
  	do
  	{
      for (t = 0x01; t; t += t)
      {
        if (*pMask & t)
        {
          count++;
        }
      }
      pMask++;
	  } while (--bLength);
  }
  return count;
}


uint8_t
ZW_NodeMaskNodeIn(
  uint8_t* pMask,
  uint8_t nodeID)
{
  nodeID--;
  return ((*(pMask + (nodeID >> 3)) >> (nodeID & 7)) & 0x01);
}

uint8_t
ZW_LR_NodeMaskNodeIn(
  uint8_t* pMask,
  node_id_t nodeID)
{
  nodeID--;
  return ((*(pMask + (nodeID >> 3)) >> (nodeID & 7)) & 0x01);
}

uint8_t
ZW_NodeMaskGetNextNode(
  uint8_t currentNodeId,
  uint8_t* pMask)
{
  while (currentNodeId < ZW_MAX_NODES)
  {
    if ((*(pMask + (currentNodeId >> 3)) >> (currentNodeId & 7)) & 0x01)
    {
      return (currentNodeId + 1);
    }
    currentNodeId++;
  }
  return 0;
}
