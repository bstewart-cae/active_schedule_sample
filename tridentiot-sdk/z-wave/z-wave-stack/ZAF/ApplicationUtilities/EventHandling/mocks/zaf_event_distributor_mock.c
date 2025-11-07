/*
 * zaf_event_distributor_mock.c
 *
 *  Created on: 24. apr. 2018
 *      Author: COlsen
 */
#include <stdbool.h>
#include <stdint.h>
#include <mock_control.h>

#define MOCK_FILE "zaf_event_distributor.c"

void zaf_event_distributor_init(void)
{
  mock_t * p_mock;

  MOCK_CALL_RETURN_VOID_IF_USED_AS_STUB();
  MOCK_CALL_FIND_RETURN_VOID_ON_FAILURE(p_mock);
}

uint32_t zaf_event_distributor_distribute(void)
{
  mock_t * p_mock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(true);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(p_mock, false);

  MOCK_CALL_RETURN_VALUE(p_mock, uint32_t);
}
