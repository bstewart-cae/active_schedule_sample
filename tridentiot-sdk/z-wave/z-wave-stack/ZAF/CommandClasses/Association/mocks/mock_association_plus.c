/**
 * @file mock_association_plus.c
 * @copyright 2022 Silicon Laboratories Inc.
 */

#include "association_plus_base.h"
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <mock_control.h>

#define MOCK_FILE "mock_association_plus.c"

/**
 * @brief handleAssociationGetnodeList
 * Deliver group number node list
 * @param[in] groupIden  Grouping Identifier
 * @param[in] ep endpoint
 * @param[out] ppList is out double-pointer of type MULTICHAN_NODE_ID deliver node list
 * @param[out] pListLen length of list
 * @return enum type NODE_LIST_STATUS
 */
NODE_LIST_STATUS handleAssociationGetnodeList(
  uint8_t groupIden,
  uint8_t ep,
  MULTICHAN_NODE_ID** ppList,
  uint8_t* pListLen)
{
	  mock_t * p_mock;

	  MOCK_CALL_RETURN_IF_USED_AS_STUB(NODE_LIST_STATUS_SUCCESS);
	  MOCK_CALL_FIND_RETURN_ON_FAILURE(p_mock, NODE_LIST_STATUS_ERROR_LIST);
	  MOCK_CALL_RETURN_IF_ERROR_SET(p_mock, NODE_LIST_STATUS);
	  MOCK_CALL_ACTUAL(p_mock, groupIden, ep, ppList, pListLen);

	  MOCK_CALL_COMPARE_INPUT_UINT8(p_mock, ARG0, groupIden);
	  MOCK_CALL_COMPARE_INPUT_UINT8(p_mock, ARG1, ep);

	  *ppList = p_mock->output_arg[2].p;
	  *pListLen = (uint8_t)p_mock->output_arg[3].v;
	  MOCK_CALL_RETURN_VALUE(p_mock, NODE_LIST_STATUS);
}

void
CC_Association_Init(void)
{
  mock_t * p_mock;

  MOCK_CALL_RETURN_VOID_IF_USED_AS_STUB();
  MOCK_CALL_FIND_RETURN_VOID_ON_FAILURE(p_mock);
}

void
CC_Association_Reset(void)
{
  mock_t * p_mock;

  MOCK_CALL_RETURN_VOID_IF_USED_AS_STUB();
  MOCK_CALL_FIND_RETURN_VOID_ON_FAILURE(p_mock);
}

e_cmd_handler_return_code_t
AssociationRemove(
  uint8_t groupIden,
  uint8_t ep,
  ZW_MULTI_CHANNEL_ASSOCIATION_REMOVE_1BYTE_V2_FRAME* pCmd,
  uint8_t cmdLength)
{
  mock_t * p_mock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(E_CMD_HANDLER_RETURN_CODE_HANDLED);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(p_mock, E_CMD_HANDLER_RETURN_CODE_FAIL);

  MOCK_CALL_COMPARE_INPUT_UINT8(p_mock, ARG0, groupIden);
  MOCK_CALL_COMPARE_INPUT_UINT8(p_mock, ARG1, ep);
  MOCK_CALL_COMPARE_INPUT_POINTER(p_mock, ARG2, pCmd);
  MOCK_CALL_COMPARE_INPUT_UINT8(p_mock, ARG3, cmdLength);

  MOCK_CALL_RETURN_VALUE(p_mock, e_cmd_handler_return_code_t);
}

void
AssociationGet(
  uint8_t endpoint,
  uint8_t * incomingFrame,
  uint8_t * outgoingFrame,
  uint8_t * outgoingFrameLength)
{
  mock_t * p_mock;

  MOCK_CALL_RETURN_VOID_IF_USED_AS_STUB();
  MOCK_CALL_FIND_RETURN_VOID_ON_FAILURE(p_mock);

  MOCK_CALL_COMPARE_INPUT_UINT8(p_mock, ARG0, endpoint);
  MOCK_CALL_COMPARE_INPUT_POINTER(p_mock, ARG1, incomingFrame);
  MOCK_CALL_COMPARE_INPUT_POINTER(p_mock, ARG2, outgoingFrame);
  MOCK_CALL_COMPARE_INPUT_POINTER(p_mock, ARG3, outgoingFrameLength);
}


e_cmd_handler_return_code_t
handleAssociationSet(
    uint8_t ep,
    ZW_MULTI_CHANNEL_ASSOCIATION_SET_1BYTE_V2_FRAME* pCmd,
    uint8_t cmdLength)
{
  mock_t * p_mock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(E_CMD_HANDLER_RETURN_CODE_HANDLED);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(p_mock, E_CMD_HANDLER_RETURN_CODE_FAIL);

  MOCK_CALL_COMPARE_INPUT_UINT8(p_mock, ARG0, ep);
  MOCK_CALL_COMPARE_INPUT_POINTER(p_mock, ARG1, pCmd);
  MOCK_CALL_COMPARE_INPUT_UINT8(p_mock, ARG2, cmdLength);

  MOCK_CALL_RETURN_VALUE(p_mock, e_cmd_handler_return_code_t);
}

void AssociationGetDestinationInit(destination_info_t * pFirstDestination)
{
  mock_t * p_mock;

  MOCK_CALL_RETURN_VOID_IF_USED_AS_STUB();
  MOCK_CALL_FIND_RETURN_VOID_ON_FAILURE(p_mock);
  MOCK_CALL_ACTUAL(p_mock, pFirstDestination);

  MOCK_CALL_COMPARE_INPUT_POINTER(p_mock, ARG0, pFirstDestination);
}

destination_info_t * AssociationGetNextSinglecastDestination(void)
{
  mock_t * p_mock;

  static destination_info_t node;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(&node);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(p_mock, NULL);

  MOCK_CALL_RETURN_POINTER(p_mock, destination_info_t *);
}

uint8_t AssociationGetSinglecastEndpointDestinationCount(void)
{
  mock_t * p_mock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(1);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(p_mock, 0);

  MOCK_CALL_RETURN_VALUE(p_mock, uint8_t);
}

uint32_t AssociationGetSinglecastNodeCount(void)
{
  mock_t * p_mock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(1);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(p_mock, 0);

  MOCK_CALL_RETURN_VALUE(p_mock, uint32_t);
}

bool AssociationGetBitAdressingDestination(destination_info_t ** ppNodeList,
                                           uint8_t * pListLength,
                                           destination_info_t * pNode)
{
  mock_t * p_mock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(false);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(p_mock, false);
  MOCK_CALL_ACTUAL(p_mock, ppNodeList, pListLength, pNode);

  MOCK_CALL_COMPARE_INPUT_POINTER(p_mock, ARG0, ppNodeList);
  MOCK_CALL_COMPARE_INPUT_POINTER(p_mock, ARG1, pListLength);
  MOCK_CALL_COMPARE_INPUT_POINTER(p_mock, ARG2, pNode);

  *ppNodeList = p_mock->output_arg[0].p;
  *pListLength = p_mock->output_arg[1].v;
  memcpy((uint8_t *)pNode, p_mock->output_arg[2].p, sizeof(destination_info_t));

  MOCK_CALL_RETURN_VALUE(p_mock, bool);
}

uint8_t
ApplicationGetLastActiveGroupId(void)
{
  mock_t * p_mock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(0x01);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(p_mock, 0x00);

  MOCK_CALL_RETURN_VALUE(p_mock, uint8_t);
}
