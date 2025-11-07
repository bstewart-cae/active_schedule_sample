// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file
 * @brief Mocks of functions in ZAF_transport.c
 * @copyright 2021 Silicon Laboratories Inc.
 */

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/

#include "mock_control.h"

#define MOCK_FILE "ZAF_transport_mock.c"

bool
ZAF_transportSendDataAbort(void)
{
  mock_t * p_mock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(true);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(p_mock, false);

  MOCK_CALL_RETURN_VALUE(p_mock, bool);
}
