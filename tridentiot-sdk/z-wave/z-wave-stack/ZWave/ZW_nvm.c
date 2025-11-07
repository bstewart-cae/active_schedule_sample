// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file ZW_nvm.c
 * @copyright 2022 Silicon Laboratories Inc.
 */
#include <stdint.h>
#include <string.h> // For memset
#include "SizeOf.h"
#include "Assert.h"
#include "ZW_nvm.h"
#include <zpal_nvm.h>

static bool NVM3_StoreCb(const SSyncEvent * pCb);


// Module data
static bool               m_bIsFsInitialized = false;   /**< Keeps track of if FS has been initialized */
static const SSyncEvent * m_apResetCb[6];               /**< Storage for callbacks to
                                                             registred users on reset of FS. */
static zpal_nvm_handle_t handle;

ENvmFsInitStatus NvmFileSystemInit(void)
{
  handle = zpal_nvm_init(ZPAL_NVM_AREA_STACK);
  if (handle == NULL)
  {
    return ENVMFSINITSTATUS_FAILED;
  }

  memset(m_apResetCb, 0, sizeof(m_apResetCb));
  m_bIsFsInitialized = true;

  return ENVMFSINITSTATUS_SUCCESS;
}


bool NvmFileSystemFormat(void)
{
  if (ZPAL_STATUS_OK != zpal_nvm_erase_all(handle))
  {
    return false;
  }

  NVM3_InvokeCbs();
  return true;
}


zpal_nvm_handle_t NvmFileSystemRegister(const SSyncEvent* pFsResetCb)
{
  if (!m_bIsFsInitialized)
  {
    NvmFileSystemInit();
  }

  NVM3_StoreCb(pFsResetCb);

  return handle;
}


/**
* Stores a Cb for registered users.
*
* Looks for a null pointer in the array of SyncEvent pointers
* and stores the SyncEvent pointer when it finds an array entry that is null.
*
* @param[in]    pCb       Poitner to callback synceven to be stored.
*
* @retval       true      Cb successfully stored.
* @retval       false     Failed to store Cb, no more space.
*/
static bool NVM3_StoreCb(const SSyncEvent * pCb)
{
  for (uint32_t i = 0; i < sizeof_array(m_apResetCb); i++)
  {
    if (m_apResetCb[i] == pCb)
    {
       return true;           //callback has been already registered so return true to avoid assert
    }
    else if (!m_apResetCb[i])
    {
       m_apResetCb[i] = pCb; //callback has not been registered so assign ptr and return true
       return true;
    }
  }
  return false;   //we ran out of space all 6 callback has been registered this will trigger assert
}


/**
* Invokes all stored Cb`s for registered users.
*/
void NVM3_InvokeCbs(void)
{
  for (uint32_t i = 0; i < sizeof_array(m_apResetCb); i++)
  {
    if (m_apResetCb[i])
    {
      SyncEventInvoke(m_apResetCb[i]);
    }
  }
}
