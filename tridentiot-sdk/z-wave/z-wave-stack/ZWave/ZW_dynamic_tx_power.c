// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file
 *
 * Description: Weights and Lookup tables needed for the Dynamic Transmission Power Algorithm for Long Range
 * This holds the implementation of a Dynamic Transmission Power Algorithm for Long Range
 *
 * @copyright 2020 Silicon Laboratories Inc.
 * Author: Lucas Balling
 */
#include "ZW_lib_defines.h"
#include "ZW_dynamic_tx_power.h"
#include <ZW_node.h>
#include <string.h>
#include <zpal_radio.h>

//#define DEBUGPRINT
#include <DebugPrint.h>

#ifdef ZW_SLAVE
#include <zpal_retention_register.h>
#endif

#ifdef ZW_CONTROLLER
#include "ZW_controller_network_info_storage.h"


/*************************************************************************
 * Link data: TX Power buffer for connected links
 ************************************************************************/

#define TX_POWER_BUFFER_SIZE                    11  /// The implementation cannot handle buffer size larger than than 254!!

/**
 * Number of latest TX that can shield a tx power buffer element from being replaced.
 * This value is used to spare a number of latest accessed nodes in the TX Power buffer.
 */
#define TX_POWER_NBR_OF_LATEST_ACCESSES         5  // This is a configurable parameter.

/**
 * [dBm] A 80 meter range tx power for the initial communication to establish a link.
 * Used for devices that are NOT marked as being too far out.
 */
#define TX_POWER_DEFAULT_RANGE_SHORT            3  // [dBm]

/**
 * [dBm] A 500 to 1000 meter range tx power for the initial communication to establish a link.
 * Used for devices that are marked as being very far out. This will reduce the chance of the
 * initial transaction to fail due to the retransmission limit of 3 being hit.
 * This value is also valid for 20 dBm capable devices.
 *
 * This value is also used to determine whether a node is a node that needs the
 * TX_POWER_DEFAULT_RANGE_LONG default tx power value for initial transmission or not.
 */
#define TX_POWER_DEFAULT_RANGE_LONG             14  // [dBm] Also valid for 20dBm devices!!

/**
 * When setting a LR node in txPowerLRHighPower to be needing a high power for initial communication,
 * this value is used to determine when a node has moved closer and no longer needs to use the
 * TX_POWER_DEFAULT_RANGE_LONG default tx power value for initial use, and will be set to use
 * TX_POWER_DEFAULT_RANGE_SHORT in future link recovery.
 *
 * The hysteresis is thus between TX_POWER_DEFAULT_RANGE_LONG and TX_POWER_DEFAULT_RANGE_HYSTERISIS.
 * (Only upper value is included in the hysteresis range!)
 */
#define TX_POWER_DEFAULT_RANGE_HYSTERISIS       10  // [dBm]

#define NODE_NOT_IN_CACHE                       0xFF

/**
 * This will ensure that there will always be at least one buffer elements that is not exempt.
 */
#if (TX_POWER_NBR_OF_LATEST_ACCESSES >= TX_POWER_BUFFER_SIZE)
  #error "TX_POWER_NBR_OF_LATEST_ACCESSES cannot be equal or larger than TX_POWER_BUFFER_SIZE."
#endif // Parameter boundary check

#if (TX_POWER_BUFFER_SIZE >= UINT8_MAX)
  #error "The implementation does not support TX_POWER_BUFFER_SIZE >= UINT8_MAX."
#endif // Parameter boundary check

typedef struct __attribute__((__packed__)) {
  /**
   * The updated value of the 'bufferAccessCounter' is placed here in order to determine the order of access among other
   * tx power elements in the buffer. This is needed to determine which buffer element to replace with a new link.
   */
  uint8_t   elemAccessCounter;  /// Buffer metadata.
  /**
   * How many times this placeholder was modified holding the data of 'this' nodeID or simply read.
   */
  uint8_t   nbrOfAccesses;  /// Buffer metadata.
  node_id_t nodeID;         /// The KEY:    The nodeID of the peer with this txPowerDBm.
  int8_t    txPowerDBm;     /// The VALUE:  The tx power of this link in dBm.
}TX_Power_Buffer_Elem_t;    // 5 bytes

typedef struct __attribute__((__packed__)) {
  /**
   * An index for the first look for the TX Power for the nodeID when start searching.
   * When we access this tx power element via this index holder, we will not increment access
   * counters in order not to move the bufferAccessCounter far away from the posOfLastUsedTxPower
   * of other elements and causing them to become candidate for replacement when the need comes.
   */
  uint8_t   posOfLastUsedTxPower;
  bool      bufferFull;            /// An CPU optimization feature.
  /**
   * How many time the tx buffer has been used.
   * This value is placed inside the buffer elements when a tx buffer element is accessed,
   * before it itself is incremented. It is then used to determine which buffer to replace when a new link is created.
   * The buffer to replace will be one that has a elemAccessCounter with a difference of at least
   * TX_POWER_NBR_OF_LATEST_ACCESSES with bufferAccessCounter in order not to be exempt from replacement.
   * Other than this rule, the nbrOfAccesses count must be the least among nodes that are not exempt from replacement.
   */
  uint8_t  bufferAccessCounter;
  TX_Power_Buffer_Elem_t  buffer[TX_POWER_BUFFER_SIZE];  // 150 bytes (for TX_POWER_BUFFER_SIZE = 30)
}TX_Power_Buffer_t;  // 153 bytes

/**
 * The TX POWER BUFFER allocated to keep part of the link data for connected peers.
 */
static TX_Power_Buffer_t txPowers = {0};

/***************************************
 * TX POWER MACRO and STATIC FUNCTIION
 **************************************/

/**
 * This will calculate the distance between two index values in a circular buffer. (FILO)
 *
 * @param tailIndex     The index at which items are taken off the circular buffer.
 *                      (Old items are removed from buffer at this index)
 * @param headIndex     The index at which items are placed on the circular buffer.
 *                      (New items are added to buffer at this index)
 * @param maxValue      The maximum index value, or the size of the buffer minus 1.
 */
#define CIRCULAR_BUFFER_DIFF(tailIndex, headIndex, maxValue) \
  ((headIndex >= tailIndex) ? (headIndex - tailIndex) : ((maxValue - tailIndex + 1) + headIndex))

/**
 * Evaluated whether this tx power element is exempt from being replaced.
 */
#define TX_POWER_ELEM_IS_NOT_EXEMPT(bufferIndex)                        \
    (TX_POWER_NBR_OF_LATEST_ACCESSES <                                  \
     CIRCULAR_BUFFER_DIFF(txPowers.buffer[bufferIndex].elemAccessCounter, txPowers.bufferAccessCounter, UINT8_MAX))


/**
 * This parameter is storing a flag for each LR nodeID and is also not persistently stored.
 * With each flag it indicates whether that LR nodeID is going to need TX_POWER_DEFAULT_RANGE_LONG
 * or TX_POWER_DEFAULT_RANGE_SHORT for initial communication when recovering from a broken link.
 */
static LR_NODE_MASK_TYPE txPowerLRHighPower;  // The buffer is good for 1024 LR nodes!

#endif  // ZW_CONTROLLER

#ifdef ZW_CONTROLLER

static
uint8_t FindNodeInBuffer(node_id_t nodeID)
{
  DPRINT("FindNode : ");

  // Find the index matching the nodeID.
  for (int i = 0; i < TX_POWER_BUFFER_SIZE; i++)
  {
    if (txPowers.buffer[i].nodeID == nodeID)
    {
      DPRINTF("%d\n", i);
      return i;
    }
  }
  DPRINT("Node not found in buffer\n");
  return NODE_NOT_IN_CACHE;
}


/**
 * This macro is responsible for incrementing access counters and also handling counter overflow condition.
 *
 * In the case of overflow being imminent on the nbrOfAccesses access counter, the counters are divided by two
 * to keep their relations intact while making room for more counts.
 *
 * bufferAccessCounter is allowed to overflow as this aspect is covered by the use of CIRCULAR_BUFFER_DIFF().
 */
static void txPowerIncrementAccessCounters(TX_Power_Buffer_Elem_t *txPowerElem)
{
  // Check for imminent wrap on nbrOfAccesses.
  if (UINT8_MAX == txPowerElem->nbrOfAccesses)
  {
    for (uint8_t i = 0; i < TX_POWER_BUFFER_SIZE; i++)
    {
      txPowers.buffer[i].nbrOfAccesses /= 2;
    }
  }
  txPowerElem->nbrOfAccesses++;
  // First move the old value into element, then increment.
  txPowerElem->elemAccessCounter = txPowers.bufferAccessCounter++;
}


static
void RefreshNodeInBuffer(__attribute__((unused)) node_id_t nodeID, int8_t tx_power, TX_Power_Buffer_Elem_t* txPowerElem)
{
  /**
   * If the new Tx power does not match the tx power in buffer, update the tx power and then
   * count this as a write access:
   * - Increment nbrOfAccesses.
   * - Store bufferAccessCounter into elemAccessCounter.
   * - Increment bufferAccessCounter.
   */
  if ((txPowerElem->txPowerDBm != tx_power))
  {
    DPRINTF("Update tx power (nodeID = %d, txPower = %d)\n", nodeID, tx_power);
    txPowerElem->txPowerDBm = tx_power;
    txPowerIncrementAccessCounters(txPowerElem);  // Counts as write access.
    DPRINTF("nodeID: %d, nbrOfAccesses: %d \n", txPowerElem->nodeID, txPowerElem->nbrOfAccesses);
  }
}


/**
 * Search for an empty array element if the buffer is not already full.
 * If it is found, update the empty element fields.
 * @return true if the tx power was inserted into buffer.
 */
static bool addToEmptyElementinTxBuffer(node_id_t nodeID, int8_t tx_power)
{
  if (false == txPowers.bufferFull)
  {
    for (int i = 0; i < TX_POWER_BUFFER_SIZE; i++)
    {
      if (0 == txPowers.buffer[i].nodeID)
      {
        DPRINTF("Added to empty buffer element: %d\n", i);
        txPowers.buffer[i].txPowerDBm = tx_power;
        txPowers.buffer[i].nodeID = nodeID;
        txPowers.buffer[i].nbrOfAccesses = 0;  // Reset
        txPowerIncrementAccessCounters(&txPowers.buffer[i]);  // Count as write access.
        return true;
      }
    }
  }

  /**
   * An optimization feature to avoid going through the loop above.
   * This state can only transition from false to true and not the other way around
   * as of its initial implemented version!!!
   * We do not remove tx power values from the buffer. (Maybe we should)
   */
  txPowers.bufferFull = true;
  DPRINT("TX Power BUFFER IS FULL\n");

  return false;
}


/**
 * Buffer was full!
 *
 * If the array is full then find the array element with the lowest nbrOfAccesses value
 * and for an elemAccessCounter that has at least TX_POWER_NBR_OF_LATEST_ACCESSES
 * difference with bufferAccessCounter and replace it with the new nodeID and tx_power
 * and set elemAccessCounter = bufferAccessCounter and nbrOfAccesses = 1.
 * Finish by incrementing bufferAccessCounter.
 */
static uint8_t findABufferElementToReplace(void)
{
  DPRINT("Find a buffer element to replace\n");
  uint8_t  buffer_elem_to_replace_idx = UINT8_MAX; // UINT8_MAX means NOT FOUND.
  uint8_t  nbrOfAccessesMin = UINT8_MAX;           // Find the link with the fewest access counts.

  /**
   * Find a non-exempt buffer element to be replaced.
   */
  for (int i = 0; i < TX_POWER_BUFFER_SIZE; i++)
  {
    /* Which of the nodes among the non-exempt nodes has the least tx activity
     * and haven't been communicated to lately (exempt)? (this one will be replaced) */
    if ((0 != txPowers.buffer[i].nodeID)                          // For all used elements in buffer.
        && (txPowers.buffer[i].nbrOfAccesses < nbrOfAccessesMin)  // Find the least accessed buffer element.
        && TX_POWER_ELEM_IS_NOT_EXEMPT(i))                        // ... only if they are NOT exempt from being replaced.
    {
      nbrOfAccessesMin = txPowers.buffer[i].nbrOfAccesses;
      buffer_elem_to_replace_idx = i;
      /*
       * Since the buffer is filled at this point, there will always be a txPower element in the buffer that
       * will meet the conditions above, hence this loop will always return an element to replace!
       *
       * When bufferAccessCounter wraps, it will protect (exempt) twice as many elements in the buffer for a
       * period of time. There is a parameter check to guaranty the statement made above.
       */
    }
  }

  /* In the extremely unlikely event that no buffer element is picked to be replaced,
   * we have a fail-safe mechanism here: */
  if (buffer_elem_to_replace_idx == UINT8_MAX)
  {
    buffer_elem_to_replace_idx = 0;
  }

  DPRINTF("Buffer element to replace: idx: %d, nodeID: %d, nbrOfAccesses: %d\n",
      buffer_elem_to_replace_idx,
      txPowers.buffer[buffer_elem_to_replace_idx].nodeID,
      nbrOfAccessesMin);

  return buffer_elem_to_replace_idx;
}

static
void AddNodeToBuffer(node_id_t nodeID, int8_t tx_power)
{
  DPRINT("Adding TX Power to buffer\n");

  if (addToEmptyElementinTxBuffer(nodeID, tx_power))
  {
    return;  // Added to empty buffer element
  }

  uint8_t buffer_elem_to_replace_idx = findABufferElementToReplace();

  /**
   * REPLACE:
   * At this point we have identified a link to a peer to remove its tx power data
   * and replace it with the given new connection.
   *
   * - The tx power to be removed belongs to the least active link.
   * - It is not one of the last TX_POWER_NBR_OF_LATEST_ACCESSES number of active links.
   *   -- This avoids new links to replace an almost newest link in the buffer with yet few activities.
   */

  // (Replace) Switch out the data of the old link with the new.
  txPowers.buffer[buffer_elem_to_replace_idx].nodeID = nodeID;
  txPowers.buffer[buffer_elem_to_replace_idx].txPowerDBm = tx_power;
  txPowers.buffer[buffer_elem_to_replace_idx].nbrOfAccesses = 0;  // Reset
  txPowerIncrementAccessCounters(&txPowers.buffer[buffer_elem_to_replace_idx]);  // Counts as write access.
}

static
void TxPowerBufferUpdate(node_id_t nodeID, int8_t txPower)
{
   uint8_t node_in_buffer = FindNodeInBuffer(nodeID);

   DPRINTF("Update tx power value for nodeID: %d\n", nodeID);
   if (NODE_NOT_IN_CACHE != node_in_buffer)
   {
     DPRINT("Refresh node in buffer\n");
     // Node already exists in buffer, updated it the tx power value.
     RefreshNodeInBuffer(nodeID, txPower, &txPowers.buffer[node_in_buffer]);
   }
   else
   {
     // Just established a new connection to this node. Add this node to the tx power buffer.
     AddNodeToBuffer(nodeID, txPower);  // This will always make room for the last node to be inserted.
   }
}


/**
 * This function return one of two default values for the initial communication.
 * (The default values are not experimentally deduced, but agreed upon.)
 *
 * @param nodeID  The node ID of the long range node to get a default tx power value for.
 * @return TX_POWER_DEFAULT_RANGE_LONG or TX_POWER_DEFAULT_RANGE_SHORT.
 */
static int8_t TxPowerLongRangeDefaultPowerGet(node_id_t nodeID)
{
  uint16_t index = nodeID - LOWEST_LONG_RANGE_NODE_ID + 1;
  return ZW_LR_NodeMaskNodeIn(txPowerLRHighPower, index) ? TX_POWER_DEFAULT_RANGE_LONG : TX_POWER_DEFAULT_RANGE_SHORT;
}

/**
 * Alongside the stored TX Power in RAM, we maintain a Default Power Flag (also in RAM) for all LR nodes to quickly determine,
 * whether the node needs a TX ower to be reached in the high end of the scale.
 *
 * (The RAM overhead is 129 bytes.)
 *
 * @param nodeID   The node ID of the long range node to get a default tx power value for.
 * @param txPower  The latest txPower, put here to be examined for setting the Persistent Default Power Flag.
 */
static void TxPowerLongRangeDefaultPowerSet(node_id_t nodeID, int8_t txPower)
{
  uint16_t index = nodeID - LOWEST_LONG_RANGE_NODE_ID + 1;
  if (txPower >= TX_POWER_DEFAULT_RANGE_LONG)               // Also valid for a 20 dBm node!
  {
    DPRINT("Set flag bit\n");
    ZW_LR_NodeMaskSetBit(txPowerLRHighPower, index);        // This node is far away.
  }
  else if (txPower <= TX_POWER_DEFAULT_RANGE_HYSTERISIS)    // TX_POWER_DEFAULT_RANGE_HYSTERISIS is not in the hysteresis range.
  {
    DPRINT("Clear flag bit\n");
    ZW_LR_NodeMaskClearBit(txPowerLRHighPower, index);      // This node is close by.
  }

}


/**
 * Read the Tx power for a long range node from buffer.
 *
 * @param[in] nodeID    The node ID of the long range node to read its tx power.
 * @return The tx power of the node in dBm.
 */
int8_t
GetTXPowerforLRNode(node_id_t nodeID)
{
   DPRINTF("GetTXPowerforLRNode %d\n", nodeID);

   //special case for first transmission at inclusion
   if (0 == nodeID)
   {
     return zpal_radio_get_maximum_lr_tx_power();
   }

   /**
    * A quick measure to retrieve the last accessed tx power. (without incrementing counters)
    */
   if (txPowers.buffer[txPowers.posOfLastUsedTxPower].nodeID == nodeID)
   {
     DPRINT("Retrieving last tx power\n");
     txPowers.buffer[txPowers.posOfLastUsedTxPower].nbrOfAccesses++;
     return txPowers.buffer[txPowers.posOfLastUsedTxPower].txPowerDBm;
   }

   /**
    * Return the correct tx power and increment access counters.
    */
   uint8_t node_in_buffer = FindNodeInBuffer(nodeID);
   if (NODE_NOT_IN_CACHE != node_in_buffer)
   {
     DPRINTF("IN CACHE TX_PWR: %d\n", txPowers.buffer[node_in_buffer].txPowerDBm);
     txPowerIncrementAccessCounters(&txPowers.buffer[node_in_buffer]);  // Count as read access.
     txPowers.posOfLastUsedTxPower = node_in_buffer;
     return txPowers.buffer[node_in_buffer].txPowerDBm;
   }
   else
   {
     // The node is not in the tx power buffer, return one of two default values.
     int8_t tx_power = TxPowerLongRangeDefaultPowerGet(nodeID);
     DPRINT("No link established with peer yet. Returning default tx power.\n");
     return tx_power;
   }
}

/**
 * Save the tx power in buffer.
 *
 * @param[in] nodeID    The node id of the long range node to buffer its tx power.
 * @param[in] txPower   The tx power of the node-link to keep in the buffer.
 *                      tx power value range -10 to 14
 * @return nothing
*/
void
SetTXPowerforLRNode(node_id_t nodeID, int8_t txPower)
{
  DPRINTF("SetTXPowerforLRNode (nodeID:%d, txPower:%d)\n", nodeID, txPower);

  /**
   * A quick measure to set the last accessed tx power. (without incrementing counters)
   */
  if (txPowers.buffer[txPowers.posOfLastUsedTxPower].nodeID == nodeID)
  {
    DPRINT("Set to last tx power element\n");
    txPowers.buffer[txPowers.posOfLastUsedTxPower].nbrOfAccesses++;
    txPowers.buffer[txPowers.posOfLastUsedTxPower].txPowerDBm = txPower;
  }

  // Set the tx power into the buffer
  if (DYNAMIC_TX_POWR_INVALID != txPower)
  {
    TxPowerLongRangeDefaultPowerSet(nodeID, txPower);  /* This parameter can be set independently
                                                        * of the tx power set in the next call. */
    TxPowerBufferUpdate(nodeID, txPower);
  }
}

void
TxPowerBufferInit(void)
{
  memset(&txPowers, 0, sizeof(TX_Power_Buffer_t));
  memset(&txPowerLRHighPower, 0, sizeof(LR_NODE_MASK_TYPE));  // All nodes are initialized to use TX_POWER_DEFAULT_RANGE_SHORT.
}

void
TxPowerBufferErase(void)
{
  memset(&txPowers, 0, sizeof(TX_Power_Buffer_t));
}

#endif  // ZW_CONTROLLER

#ifdef ZW_SLAVE

//Struct for saving data to retention RAM.
//Must not be more than 4 bytes long to fit one word in retention RAM.
typedef struct StxPowerAndRssi_t
{
  int8_t txPower;
  int8_t rssi;
  int8_t unused_1;
  int8_t unused_2;
}StxPowerAndRssi_t;

//Union for saving data to retention RAM.
//Must be 32bit long to fit one word in retention RAM.
typedef union UDataWord_t
{
  uint32_t ram_word;
  StxPowerAndRssi_t data;
} UDataWord_t;

void
SaveTxPowerAndRSSI(int8_t txPower, int8_t rssi)
{
  UDataWord_t dataWord;
  dataWord.ram_word = 0;

  dataWord.data.txPower = txPower;
  dataWord.data.rssi = rssi;

  zpal_retention_register_write(ZPAL_RETENTION_REGISTER_TXPOWER_RSSI_LR, dataWord.ram_word);
}

void
ReadTxPowerAndRSSI(int8_t * txPower, int8_t * rssi)
{
  UDataWord_t dataWord;
  dataWord.ram_word = 0;

  zpal_retention_register_read(ZPAL_RETENTION_REGISTER_TXPOWER_RSSI_LR, &(dataWord.ram_word));

  *txPower = dataWord.data.txPower;
  *rssi = dataWord.data.rssi;
}

#endif  // ZW_SLAVE
