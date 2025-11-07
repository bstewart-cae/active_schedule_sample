// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file ZW_NVMCaretaker.c
 * @copyright 2022 Silicon Laboratories Inc.
 */
#include <ZW_NVMCaretaker.h>
//#define DEBUGPRINT
#include <DebugPrint.h>
#include "Assert.h"


ECaretakerStatus NVMCaretakerVerifySet(const SObjectSet* pObjectSet, ECaretakerStatus * pObjectSetStatus)
{
  ECaretakerStatus eStatusReturn = ECTKR_STATUS_SUCCESS;

  for (uint32_t i = 0; i < pObjectSet->iObjectCount; i++)
  {
    ECaretakerStatus eStatus = NVMCaretakerVerifyObject(pObjectSet->pFileSystem, &pObjectSet->pObjectDescriptors[i]);
    *(pObjectSetStatus + i) = eStatus;

    if(ECTKR_STATUS_SUCCESS != eStatus)
    {
      eStatusReturn = ECTKR_STATUS_FILESYS_MISMATCH;
    }
  }

  return eStatusReturn;
}

ECaretakerStatus NVMCaretakerVerifyObject(zpal_nvm_handle_t pFileSystem, const SObjectDescriptor* pObjectDescriptor)
{
  size_t   dataLen = 0;

  if (NULL == pFileSystem)
  {
    return ECTKR_STATUS_FILESYS_MISMATCH;
  }

  const zpal_status_t status = zpal_nvm_get_object_size(pFileSystem, pObjectDescriptor->ObjectKey, &dataLen);

  if (ZPAL_STATUS_OK != status)
  {
    return ECTKR_STATUS_UNABLE_TO_AQUIRE_HANDLE;
  }

  if (dataLen != pObjectDescriptor->iDataSize)
  {    
    return ECTKR_STATUS_SIZE_MISMATCH;
  }
  
  return ECTKR_STATUS_SUCCESS; // verification succesful
}
