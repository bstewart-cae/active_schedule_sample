/// ***************************************************************************
/// SPDX-License-Identifier: LicenseRef-TridentMSLA
/// SPDX-FileCopyrightText: 2025 Trident IoT, LLC <https://www.tridentiot.com>
/// ***************************************************************************

#include "unity.h"
#include "unity_print.h"
#include "SizeOf.h"
#include <string.h>
#include "zpal_retention_register.h"
#include "sysfun_mock.h"

void setUpSuite(void)
{

}

void tearDownSuite(void)
{

}

void zpal_retention_register_clear(void);

void setUp(void)
{
  Enter_Critical_Section_Ignore();
  Leave_Critical_Section_Ignore();
  zpal_retention_register_clear();
}

void tearDown(void)
{

}

typedef struct
{
  uint32_t index;
  uint32_t value;
  zpal_status_t expected_return_value;
} retention_register_t;

void test_zpal_retention_register_write_read(void)
{
  retention_register_t test_vectors[] = {{0, 14532, ZPAL_STATUS_OK},
                                         {3, 764, ZPAL_STATUS_OK},
                                         {5, 3940, ZPAL_STATUS_OK},
                                         {5, 77777, ZPAL_STATUS_OK},
                                         {16, 55555, ZPAL_STATUS_OK},
                                         {19, 23451, ZPAL_STATUS_OK},
                                         {30, 3333, ZPAL_STATUS_OK},
                                         {31, 22222, ZPAL_STATUS_OK},
                                         {zpal_retention_register_count(), 8, ZPAL_STATUS_INVALID_ARGUMENT},
                                         {zpal_retention_register_count() + 14, 3452, ZPAL_STATUS_INVALID_ARGUMENT},
                                        };

  for (uint8_t i = 0; i < sizeof_array(test_vectors); i++)
  {
    zpal_status_t status = zpal_retention_register_write(test_vectors[i].index, test_vectors[i].value);
    TEST_ASSERT_EQUAL(test_vectors[i].expected_return_value, status);

    uint32_t value = ~test_vectors[i].value;
    status = zpal_retention_register_read(test_vectors[i].index, &value);
    TEST_ASSERT_EQUAL(test_vectors[i].expected_return_value, status);
    if (ZPAL_STATUS_OK == status)
    {
      TEST_ASSERT_EQUAL(test_vectors[i].value, value);
    }
    else if (ZPAL_STATUS_INVALID_ARGUMENT == status)
    {
      TEST_ASSERT_EQUAL(~test_vectors[i].value, value);
    }
  }
}
