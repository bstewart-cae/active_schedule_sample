// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file
 * Stores association data discovered by the Security Discovery module.
 *
 * It stores the following for any given node discovered:
 * - Support for CC Supervision
 * - Support for CC Multicommand
 * - Highest security class
 * @copyright 2018 Silicon Laboratories Inc.
 */

#ifndef ZAF_APPLICATIONUTILITIES_ZAF_ASSOCIATIONDATASTORE_H_
#define ZAF_APPLICATIONUTILITIES_ZAF_ASSOCIATIONDATASTORE_H_

#include <stdbool.h>
#include <stdint.h>
#include <ZW_security_api.h>

/**
 * @addtogroup ZAF
 * @{
 * @addtogroup ADS Association Data Store
 * @{
 */


/**
 * Type to use when defining a handle for the ADS functions.
 *
 * The handle is assigned by calling ZAF_ADS_Init.
 */
typedef void * ZAF_ADS_Handle_t;

// These following defines must be aligned with the structs in the private implementation
// (made as individual defines in order to easier assert on each of them)
#define ZAF_ADS_HEADER_SIZE 4
#define ZAF_ADS_ELEMENT_SIZE 20

// Calculate size of storage
#define ZAF_ADS_STORAGE_SIZE(num_elements) (ZAF_ADS_HEADER_SIZE + (num_elements) * ZAF_ADS_ELEMENT_SIZE)
/**
 * Allocates storage at a word boundary (or any other boundary if needed)
 *
 * Please see ZAF_ADS_Init.
 */
#define ZAF_ADS_STORAGE(name, num_elements) union { void *align; uint8_t storage[ZAF_ADS_STORAGE_SIZE(num_elements)]; } name

/**
 * Defines the states of a highest security class discovery.
 */
typedef enum
{
  SECURITY_HIGHEST_CLASS_NOT_DISCOVERED,              //!< Not discovered yet
  SECURITY_HIGHEST_CLASS_DISCOVERED_NOT_GRANTED_TO_ME,//!< The security class has been discovered but was not granted to me
  SECURITY_HIGHEST_CLASS_DISCOVERED                   //!< The Security class has been discovered and is granted to me
} e_ZAF_ADS_SecurityHighestClassState_t;

/**
 * Pairs a security with the discovery state of it.
 */
typedef struct
{
  security_key_t securityKey;
  e_ZAF_ADS_SecurityHighestClassState_t state;
} s_ZAF_ADS_SecurityHighestClass_t;

/**
 * Defines the states of a command class discovery.
 */
typedef enum
{
  CC_CAPABILITY_NOT_DISCOVERED,//!< Support of the CC has not yet been discovered.
  CC_CAPABILITY_SUPPORTED,     //!< The CC is supported.
  CC_CAPABILITY_NOT_SUPPORTED  //!< The CC is NOT supported.
} e_ZAF_ADS_CC_Capability_t;

/**
 * Defines the list of command classes for which the support can be stored.
 */
typedef enum
{
    ZAF_ADS_CC_SUPERVISION,  //!< Command Class Supervision
    ZAF_ADS_CC_MULTI_COMMAND,//!< Command Class Multi Command
    ZAF_ADS_MAX_CC           //!< Must be the last element in this enum.
} e_ZAF_ADS_CommandClass_t;

/**
 * Initializes the Association Data Store.
 *
 * This function must be invoked before all the other ADS functions.
 *
 * @param pStorage     Pointer to allocated storage by ZAF_ADS_STORAGE. The name given in
 *                     ZAF_ADS_STORAGE is what must be supplied as this argument.
 * @param MaxNodeCount MaxNodeCount should be set to the max number of nodes that can be associated
 *                     in controlling association groups. A controlling group is one that can
 *                     transmit set or get commands.
 *                     If an application has one controlling group that sends Basic Set, and the
 *                     group can contain 5 associations, the MaxNodeCount should be set to 5.
 *
 *                     This value must match the number of elements given in ZAF_ADS_STORAGE.
 * @return             Returns a handle to the allocated Association Data Store.
 */
ZAF_ADS_Handle_t ZAF_ADS_Init (void* pStorage, uint8_t MaxNodeCount);

/**
 * Removes all information related to a NodeID in the data store.
 * @param handle ADS handle returned from ZAF_ADS_Init.
 * @param NodeID Node ID to remove.
 * @return       Returns true if the node was removed, and false otherwise.
 */
bool ZAF_ADS_Delete(ZAF_ADS_Handle_t handle, uint8_t NodeID);

/**
 * Returns the highest security class for a given NodeID.
 * @param handle        ADS handle returned from ZAF_ADS_Init.
 * @param NodeID        Node ID of which the highest security class must be read.
 * @param pHighestClass Pointer to variable where the highest security class must be written.
 * @return              Returns the state of the discovery for the given node ID.
 */
e_ZAF_ADS_SecurityHighestClassState_t ZAF_ADS_GetNodeHighestSecurityClass(
    ZAF_ADS_Handle_t handle,
    uint8_t NodeID,
    security_key_t * pHighestClass);

/**
 * Sets the highest security class of a node.
 * @param handle       ADS handle returned from ZAF_ADS_Init.
 * @param NodeID       Node ID of which the highest security class must be set.
 * @param HighestClass Security class to be set as the highest.
 * @return             Returns true if the class was set and false if there were no free slots in
 *                     the ADS (increase the MaxNodeCount for the storage space used with
 *                     ZAF_ADS_Init)
 */
bool ZAF_ADS_SetNodeHighestSecurityClass(
    ZAF_ADS_Handle_t handle,
    uint8_t NodeID,
    security_key_t HighestClass);

/**
 * Returns whether a node supports a given CC or not.
 * @param handle       ADS handle returned from ZAF_ADS_Init.
 * @param NodeID       Node ID of which the check is made.
 * @param commandClass Command class to check support for.
 * @return             Returns the state of the CC support discovery.
 */
e_ZAF_ADS_CC_Capability_t ZAF_ADS_IsNodeSupportingCC(
    ZAF_ADS_Handle_t handle,
    uint8_t NodeID,
    e_ZAF_ADS_CommandClass_t commandClass);

/**
 * Sets whether a given node supports a given CC.
 * @param handle        ADS handle returned from ZAF_ADS_Init.
 * @param NodeID        Node ID of which the given CC must be set.
 * @param commandClass  CC that is discovered.
 * @param CC_Capability State of the support.
 * @return              Returns true if the state was set and false if there were no free slots in
 *                      the ADS (increase the MaxNodeCount for the storage space used with
 *                      ZAF_ADS_Init)
 */
bool ZAF_ADS_SetNodeIsSupportingCC(
    ZAF_ADS_Handle_t handle,
    uint8_t NodeID,
    e_ZAF_ADS_CommandClass_t commandClass,
    e_ZAF_ADS_CC_Capability_t CC_Capability);


/**
 * @} // ADS Association Data Store
 * @} // ZAF
 */


#endif /* ZAF_APPLICATIONUTILITIES_ZAF_ASSOCIATIONDATASTORE_H_ */
