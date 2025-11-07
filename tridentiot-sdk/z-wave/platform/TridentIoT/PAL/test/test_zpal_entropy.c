/// ***************************************************************************
/// SPDX-License-Identifier: LicenseRef-TridentMSLA
/// SPDX-FileCopyrightText: 2025 Trident IoT, LLC <https://www.tridentiot.com>
/// ***************************************************************************

#include "unity.h"
#include "unity_print.h"
#include <string.h>
#include "zpal_init_mock.h"
#include "zpal_entropy.h"
#include "Assert_mock.h"

void setUpSuite(void)
{

}

void tearDownSuite(void)
{

}

void setUp(void)
{
  Assert_Ignore();
}

void tearDown(void)
{

}

#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
  #define htonl(ui32) (uint32_t)ui32
#else
  // T32CZ20 is little-endian
  #define htonl(ui32) ((((uint32_t)ui32 & 0x000000ffUL) << 24) | (((uint32_t)ui32 & 0x0000ff00UL) << 8) | (((uint32_t)ui32 & 0x00ff0000UL) >> 8) | (((uint32_t)ui32 & 0xff000000UL) >> 24))
#endif

#define RAND ((uint32_t) 0x11223344)

uint32_t Get_Random_Number(uint32_t *p_buffer, uint32_t number)
{
  *p_buffer = htonl(RAND);
  return 0;
}

void test_zpal_entropy_init(void)
{
  zpal_get_reset_reason_ExpectAndReturn(ZPAL_RESET_REASON_POWER_ON);
  zpal_entropy_init();
}

void test_zpal_entropy_get_random_data(void)
{
  uint8_t test_buf[8] = {0};

  zpal_status_t status = zpal_get_random_data(&test_buf[0], 7);
  printf("\r\nRandom data - ");
  for (int i=0; i<7; i++)
  {
    printf("%02X", test_buf[i]);
  }
  printf("\r\n");
  TEST_ASSERT_EQUAL(ZPAL_STATUS_OK, status);
  TEST_ASSERT_EQUAL(test_buf[0], 0x11);
  TEST_ASSERT_EQUAL(test_buf[1], 0x22);
  TEST_ASSERT_EQUAL(test_buf[2], 0x33);
  TEST_ASSERT_EQUAL(test_buf[3], 0x44);
  TEST_ASSERT_EQUAL(test_buf[4], 0x11);
  TEST_ASSERT_EQUAL(test_buf[5], 0x22);
  TEST_ASSERT_EQUAL(test_buf[6], 0x33);

  memset(test_buf, 0, sizeof(test_buf));
  status = zpal_get_random_data(&test_buf[0], 4);
  TEST_ASSERT_EQUAL(ZPAL_STATUS_OK, status);
  TEST_ASSERT_EQUAL(test_buf[0], 0x11);
  TEST_ASSERT_EQUAL(test_buf[1], 0x22);
  TEST_ASSERT_EQUAL(test_buf[2], 0x33);
  TEST_ASSERT_EQUAL(test_buf[3], 0x44);
}

#define test_vectors 64

void test_zpal_entropy_get_pseudo_random(void)
{
  uint8_t test_rnd[test_vectors], last_rnd;

  // Generate test_vectors pseudo random numbers
  for (int i = 0; i < test_vectors; i++)
  {
    test_rnd[i] = zpal_get_pseudo_random();
  }
  // Generate cur_rnd pseudo random number
  last_rnd = zpal_get_pseudo_random();
  // Test if any of [test_vectors + cur_rnd] pseudo random numbers are equal
  printf("%02X", last_rnd);
  for (int i = 0; i < test_vectors; i++)
  {
    printf(",%02X", test_rnd[i]);
    TEST_ASSERT_NOT_EQUAL(test_rnd[i], last_rnd);
    if (i > 0)
    {
      for (int j = 0; j < i; j++)
      {
        TEST_ASSERT_NOT_EQUAL(test_rnd[j], test_rnd[i]);
      }
    }
  }
  printf("\n");
}
