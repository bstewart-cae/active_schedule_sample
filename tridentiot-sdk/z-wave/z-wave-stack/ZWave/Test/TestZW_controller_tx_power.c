// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file TestZW_controller_tx_power.c
 * @copyright 2022 Silicon Laboratories Inc.
 */
#include <stdbool.h>
#include <stdint.h>
#include <ZW_controller.h>
#include <string.h>
#include <mock_control.h>
#include <ZW_dynamic_tx_power.h>

void setUpSuite(void) {

}

void tearDownSuite(void) {

}

void setUp(void) {

}

void tearDown(void) {

}

#define TX_POWER_BUFFER_SIZE_TEST     (11 + 4)  /* (11 = TX_POWER_BUFFER_SIZE)
                                                 * We need to test for more than we have buffers for. */

#define DYNAMIC_TX_POWR_MAX           14  // This needs to be updated each time the max power is raised. Exists also in mocks.

/*
 * Global variables required for the test to build.
 */
/*
 * Verifies that ProtocolInterfacePassToAppMultiFrame() is invoked with the correct arguments for
 * command ID 0x20-0xFF.
 */

  uint16_t node_id_list[] = {(LOWEST_LONG_RANGE_NODE_ID + 3),
                             (LOWEST_LONG_RANGE_NODE_ID + 4),
                             (LOWEST_LONG_RANGE_NODE_ID + 9),
                             (LOWEST_LONG_RANGE_NODE_ID + 11),
                             (LOWEST_LONG_RANGE_NODE_ID + 20),
                             (LOWEST_LONG_RANGE_NODE_ID + 50),
                             (LOWEST_LONG_RANGE_NODE_ID + 16),
                             (LOWEST_LONG_RANGE_NODE_ID + 25),
                             (LOWEST_LONG_RANGE_NODE_ID + 65),
                             (LOWEST_LONG_RANGE_NODE_ID + 23),
                             (LOWEST_LONG_RANGE_NODE_ID + 43),
                             (LOWEST_LONG_RANGE_NODE_ID + 122),
                             (LOWEST_LONG_RANGE_NODE_ID + 200),
                             (LOWEST_LONG_RANGE_NODE_ID + 8),
                             (LOWEST_LONG_RANGE_NODE_ID + 19),
                             (LOWEST_LONG_RANGE_NODE_ID + 0),
                             (LOWEST_LONG_RANGE_NODE_ID + 1000),
                             (LOWEST_LONG_RANGE_NODE_ID + 1023),  // Max LR nodeID range (for when LR node counts is 1024 at max)
                             (LOWEST_LONG_RANGE_NODE_ID + 433),
                             (LOWEST_LONG_RANGE_NODE_ID + 324)};

  int8_t node_tx_pwr_wr_list[] = {-10, -9, -8, -7,
                                   -6, -5, -4, -3,
                                   -2, -1,  0,  1,
                                    2,  3,  4,  5,
                                    6,  7,  8,  9,
                                   10, 11, 12, 13,
                                   14};

  // When the 8-bit tx power was implemented, we started to read exactly what we were writing to the cache!
  int8_t node_tx_pwr_rd_list[] = {  3,  3,  3,  3,  // idx = 0, value = 3 => default tx power
                                   -6, -5, -4, -3,
                                   -2, -1,  0,  1,
                                    2,  3,  4,  5,
                                    6,  7,  8,  9,
                                   10, 11, 12, 13,
                                   14};

  // test we can write to the cache array and read from it.
void test_write_to_tx_power_cache(void)
{
//  TEST_IGNORE();
  TxPowerBufferInit();
  for (int i = 0; i < TX_POWER_BUFFER_SIZE_TEST; i++)
  {
    SetTXPowerforLRNode(node_id_list[i], node_tx_pwr_wr_list[i]);
  }

  printf(" START TESTING =========================\n");

  for (int i = 0; i < TX_POWER_BUFFER_SIZE_TEST; i++)
  {
    printf("i: %d \n", i);
    int8_t txPower;
    txPower = GetTXPowerforLRNode(node_id_list[i]);
    printf("i: %d, node_id_list[i]: %d, txPower: %d, node_tx_pwr_rd_list[i]: %d \n",
        i, node_id_list[i], txPower, node_tx_pwr_rd_list[i]);
    TEST_ASSERT_EQUAL_INT8(node_tx_pwr_rd_list[i], txPower);
  }
}


// we test that the caching system will write the cache value to the flash after it was changed more than 10 times
void test_cache_flushing(void)
{
  mock_calls_clear();  
  TxPowerBufferInit();

  printf(" START TESTING ==============================\n");

  SetTXPowerforLRNode(node_id_list[1], node_tx_pwr_wr_list[2]);        // Set the tx power

  // write changing tx power value more than 10 times for the same node
  for (int i = 0; i < 10; i++)
  {
    SetTXPowerforLRNode(node_id_list[1], i);      // [i = value in dBm] Change the tx power to something random
  }

  /*
   * The test will fail if the mock remains unused!
   */

  mock_calls_verify();
}

#define TX_POWER_DEFAULT_RANGE_SHORT    3
#define TX_POWER_DEFAULT_RANGE_LONG     14

// we test we can remove node from thr cache array
void test_remove_from_cache(void)
{
  mock_calls_clear();
  TxPowerBufferInit();

  printf(" START ADDING TX POWER TO THE BUFFER ==============================\n");

  for (int i = 0; i < 15; i++)
  {
    SetTXPowerforLRNode(node_id_list[i], node_tx_pwr_wr_list[i]);
  }


  printf(" TEST INVALID INPUT ===============================================\n");

  printf("             About to set an invalid tx power!\n");
  SetTXPowerforLRNode(node_id_list[5], DYNAMIC_TX_POWR_INVALID);
  printf("             Setting of tx power finished!\n");

  int8_t txPower = GetTXPowerforLRNode(node_id_list[5]);
  /**
   * This shows that invalid inputs are not accepted
   * and the old value of -5 is still in the buffer.
   */
  TEST_ASSERT_EQUAL(-5, txPower);


  printf(" TEST GETTING A TX POWER ==========================================\n");

  txPower = GetTXPowerforLRNode(node_id_list[6]);
  TEST_ASSERT_EQUAL(node_tx_pwr_rd_list[6], txPower);

  mock_calls_verify();
  mock_calls_clear();  // This

  /**
   * Try to remove non existing node.
   */

  printf(" TEST GETTING A NON EXISTING NODE =================================\n");

  txPower = GetTXPowerforLRNode(node_id_list[17]);  // See above, node_id_list[17] is not set in the buffer.
  TEST_ASSERT_EQUAL(TX_POWER_DEFAULT_RANGE_SHORT, txPower);


  printf(" TEST GETTING A NON EXISTING NODE that is learned to be far away ===\n");

  SetTXPowerforLRNode(node_id_list[17], 14);  // Far away node is set!
  printf("             Setting of tx power finished!\n");

  TxPowerBufferErase();  // This erases all tx power data from buffer, but not the long range flags.

  txPower = GetTXPowerforLRNode(node_id_list[17]);  // Get a long range node.
  /**
   * This shows that the learned long range node is still set in the flags and working.
   */
  TEST_ASSERT_EQUAL(TX_POWER_DEFAULT_RANGE_LONG, txPower);


  printf(" TEST GETTING A NON EXISTING NODE that is learned to be close or nearby ===\n");

  SetTXPowerforLRNode(node_id_list[16], 13);  // Nearby node is set! (still 13 dBm)
  printf("             Setting of tx power finished!\n");

  TxPowerBufferErase();  // This erases all tx power data from buffer, but not the long range flags.

  txPower = GetTXPowerforLRNode(node_id_list[16]);  // Get a long range node.
  /**
   * This shows that the learned short range node is still set in the flags and working.
   * However, we get 3 instead of the input being 13, this is because the data is lost and the default value of 3 is returned.
   * In previous test the input value was the same as the default value for far away devices.
   */
  TEST_ASSERT_EQUAL(TX_POWER_DEFAULT_RANGE_SHORT, txPower);

  printf("========== TEST COMPLETE ============\n");

  mock_calls_verify();
 }

// we test we can replace a cache element having a low cache_hit value with new one
void test_replace_cache_element(void)
{
  mock_calls_clear();  
  TxPowerBufferInit();
  for (int i = 0; i < 20; i++)
  {
    SetTXPowerforLRNode(node_id_list[i], node_tx_pwr_wr_list[i]);
  }

  printf("             Setting of tx power finished!\n");

  // write 5 times the same tx power value to the cache elm at index 4. (this raises the cache hit value)
  for (int i = 0; i < 5; i++)
  {
    SetTXPowerforLRNode(node_id_list[4], node_tx_pwr_wr_list[4]);
  }

  printf("             Rose the cache hit value of node_id_list[4] with tx power node_tx_pwr_wr_list[4]! \n");

  // fake that the saved tx power of the old node is not the same as the one in the cache to trigger
  // call to CtrlStorageSetTxPower

  int8_t newTxPwr = 5;  // The tx power of the new node that do not exist in the cache.

  // now try to add new node to the full cache array
  SetTXPowerforLRNode(LOWEST_LONG_RANGE_NODE_ID + 500, newTxPwr);

  int8_t txPower = GetTXPowerforLRNode(LOWEST_LONG_RANGE_NODE_ID + 500);
  // tx_power 4 and 5 dBm is converted to 4-bit value of 7
  // and when reading tx_power back then 4-bit value 7 is converted back to 4dBm
  // the lowest power value of 4 and 5.
  TEST_ASSERT_EQUAL(newTxPwr, txPower);

  mock_calls_verify();
 }

void test_get_tx_power_for_node_zero(void)
{
  mock_t * pMock;
  mock_calls_clear();

  //Tx power must be set to max at first tx of inclusion when nodeID is zero
  mock_call_expect(TO_STR(zpal_radio_get_maximum_lr_tx_power), &pMock);
  pMock->return_code.value = 14;

  int8_t txPower = GetTXPowerforLRNode(0);
  TEST_ASSERT_EQUAL(txPower, 14);
  mock_calls_verify();
}
