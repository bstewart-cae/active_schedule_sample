// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file
 * Z-Wave NVM Caretaker
 * @copyright 2020 Silicon Laboratories Inc.
 */
#ifndef _ZW_NVM_CARETAKER_H_
#define _ZW_NVM_CARETAKER_H_

#include <stdint.h>
#include <zpal_nvm.h>

// Description of an NVM object for the caretaker.
typedef struct SObjectDescriptor
{
  zpal_nvm_object_key_t ObjectKey;
  size_t   iDataSize;
} SObjectDescriptor;

// Set of object descriptors
typedef struct SObjectSet
{
  zpal_nvm_handle_t       pFileSystem;
  uint32_t                iObjectCount;
  const SObjectDescriptor*  pObjectDescriptors;
} SObjectSet;


// Error codes returned from NVMCaretaker Interface
typedef enum ECaretakerStatus
{
  ECTKR_STATUS_SUCCESS = 0,
  ECTKR_STATUS_FILESYS_MISMATCH,
  
  // Object errors
  ECTKR_STATUS_UNABLE_TO_AQUIRE_HANDLE,  // Most likely caused by object not existing
  ECTKR_STATUS_SIZE_MISMATCH,            // Current object size does not match size specified in
  
  // Could not create or modify misfit objects
  ECTKR_STATUS_WRITE_ERROR

} ECaretakerStatus;


/**
* \brief Verify the objects in the objectset
* \details Verifies that objects exists and that they have the expected size. Creates missing files. Modifies files with wrong size.
*
* \param pObjectSet Pointer for ObjectSet to verify.
* \param pObjectSetStatus Output vector containing status values for all files.
* \return ECaretakerStatus. Success or error code.
*/
ECaretakerStatus NVMCaretakerVerifySet(const SObjectSet* pObjectSet, ECaretakerStatus * pObjectSetStatus);

/**
* \brief Verify object in NVM file system
* \details Verifies that the object exists and that it has the expected size.
*
* \param pFileSystem Pointer for FileSystem which contains object.
* \param pObjectDescriptor Pointer for object descriptor of object to verify.
* \return ECaretakerStatus. Success or error code.
*/
ECaretakerStatus NVMCaretakerVerifyObject(zpal_nvm_handle_t pFileSystem, const SObjectDescriptor* pObjectDescriptor);


#endif  // _ZW_NVM_CARETAKER_H_
