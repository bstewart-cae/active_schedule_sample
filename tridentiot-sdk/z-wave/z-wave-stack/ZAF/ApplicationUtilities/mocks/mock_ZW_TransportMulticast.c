// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file mock_ZW_TransportMulticast.c
 * @copyright 2022 Silicon Laboratories Inc.
 */
#include <mock_control.h>
#include "ZW_typedefs.h"
#include "ZW_TransportEndpoint.h"
#include <ZW_transport_api.h>
#include "ZW_TransportMulticast.h"

#define MOCK_FILE "mock_ZW_TransportMulticast.c"

void ZW_TransportMulticast_clearTimeout(void)
{
  mock_t * p_mock;

  MOCK_CALL_RETURN_VOID_IF_USED_AS_STUB();
  MOCK_CALL_FIND_RETURN_VOID_ON_FAILURE(p_mock);
}


enum ETRANSPORT_MULTICAST_STATUS
ZW_TransportMulticast_SendRequest(const uint8_t * const p_data,
                                  uint8_t data_length,
                                  uint8_t fSupervisionEnable,
                                  TRANSMIT_OPTIONS_TYPE_EX * p_nodelist,
                                  VOID_CALLBACKFUNC(p_callback)(TRANSMISSION_RESULT * pTransmissionResult))
{
  mock_t * p_mock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(ETRANSPORTMULTICAST_ADDED_TO_QUEUE);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(p_mock, ETRANSPORTMULTICAST_FAILED);
  MOCK_CALL_ACTUAL(p_mock, p_data, data_length, fSupervisionEnable, p_nodelist, p_callback);

  // Compares both array length and array.
  MOCK_CALL_COMPARE_INPUT_UINT8_ARRAY(
          p_mock,
          ARG0,
          p_mock->expect_arg[1].v,
          p_data,
          data_length);

  MOCK_CALL_COMPARE_INPUT_UINT8(
          p_mock,
          ARG1,
          data_length);

  MOCK_CALL_COMPARE_INPUT_UINT8(
          p_mock,
          ARG2,
          fSupervisionEnable);

  MOCK_CALL_COMPARE_INPUT_POINTER(
          p_mock,
          ARG3,
          p_nodelist);

  MOCK_CALL_COMPARE_INPUT_POINTER(
          p_mock,
          ARG4,
          p_callback);

  MOCK_CALL_RETURN_VALUE(p_mock, enum ETRANSPORT_MULTICAST_STATUS);

}

void ZW_TransportMulticast_init(void)
{
  mock_t * p_mock;

  MOCK_CALL_RETURN_VOID_IF_USED_AS_STUB();
  MOCK_CALL_FIND_RETURN_VOID_ON_FAILURE(p_mock);
}
