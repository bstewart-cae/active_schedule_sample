/// ****************************************************************************
/// SPDX-License-Identifier: LicenseRef-TridentMSLA
/// SPDX-FileCopyrightText: 2025 Trident IoT, LLC <https://www.tridentiot.com>
/// ***************************************************************************

#include <stdbool.h>
#include <string.h>
#include "zpal_entropy.h"
#include "zpal_init.h"
#include "zpal_defs.h"
#include "Assert.h"
#include "tr_platform.h"
#include "sysctrl.h"
#include "trng.h"

static bool entropy_init = false;

static uint32_t entropy_seed  __attribute__((section(".ret_sram")));

// Function to get random numbers (uint32_t) from the platform-specific random number generator.
static inline uint32_t tr_get_random_number(uint32_t * p_value, uint32_t length)
{
  uint32_t random_number_generated = 0;
  uint32_t status = get_random_number(p_value, length);
  if (status == STATUS_SUCCESS)
  {
    random_number_generated = length;
  }
  return random_number_generated;
}

void zpal_entropy_init(void)
{
  ASSERT(!entropy_init); // Catch multiple calls to zpal_entropy_init().
  entropy_init = true;
  zpal_reset_reason_t reason =  zpal_get_reset_reason();

  if ((ZPAL_RESET_REASON_POWER_ON == reason) || (ZPAL_RESET_REASON_PIN == reason))
  {
    tr_get_random_number(&entropy_seed, 1);
  }
}

uint8_t zpal_get_pseudo_random(void)
{
  ASSERT(entropy_init); // Catch missing call to zpal_entropy_init().
  entropy_seed = (entropy_seed ^ 0xAA) + 0x11;  /* XOR with 170 and add 17 */
  return entropy_seed;
}

static void read_random_words(uint32_t* rand_words, uint8_t rand_count)
{
  uint32_t rand_word;
  while(rand_count--)
  {
    rand_word = 0;
    rand_word |= zpal_get_pseudo_random();
    rand_word <<=8;
    rand_word |= zpal_get_pseudo_random();
    rand_word <<=8;
    rand_word |= zpal_get_pseudo_random();
    rand_word <<=8;
    rand_word |= zpal_get_pseudo_random();
    *rand_words++ = rand_word;
  }
}

zpal_status_t zpal_get_random_data(uint8_t *data, size_t len)
{
  ASSERT(entropy_init); // Catch missing call to zpal_entropy_init().
  uint32_t rand_buf[8]; // Buffer to hold random numbers.
  uint8_t random_words_generated = 0;
  uint8_t total_words = len / 4;
  while (0 != total_words)
  {
    uint8_t words = total_words;
    uint8_t try = 0;
    if (words > 8)
    {
      words = 8; // Limit to 8 words at a time.
    }
    do
    {
      random_words_generated = tr_get_random_number(rand_buf, words);
      try ++;
    } while ((0 == random_words_generated) && (try <= 3));

    if (!random_words_generated)
    {
      read_random_words(rand_buf, words);
      random_words_generated = words;
    }
    memcpy(data, rand_buf, random_words_generated * 4);
    data += random_words_generated * 4;
    len -= random_words_generated * 4;
    total_words -= random_words_generated;
  }
  if (len > 0)
  {
    uint8_t try = 0;
    do
    {
      random_words_generated = tr_get_random_number(rand_buf, 1);
      try++;
    } while ((0 == random_words_generated) && (try <= 3));

    if (!random_words_generated)
    {
      read_random_words(rand_buf, 1);
      random_words_generated = 1;
    }
    memcpy(data, &rand_buf, len);
  }
  return ZPAL_STATUS_OK;
}
