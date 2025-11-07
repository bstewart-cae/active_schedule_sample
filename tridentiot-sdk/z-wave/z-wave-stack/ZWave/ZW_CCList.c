// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file ZW_CCList.c
 * @copyright 2022 Silicon Laboratories Inc.
 */
#include "ZW_CCList.h"
#include "ZW_security_api.h"
#include <ZW_system_startup_api.h>

//#define DEBUGPRINT
#include "DebugPrint.h"

static const SNetworkInfo * m_pNetworkInfo = NULL;
static const SCommandClassList_t EmptyCCList = { .iListLength = 0, .pCommandClasses = NULL };

static security_key_t GetHighestSecureLevel(void);

void CCListSetNetworkInfo(const SNetworkInfo * pNetworkInfo)
{
  m_pNetworkInfo = pNetworkInfo;
}

const SCommandClassList_t* CCListGet(security_key_t eKey)
{
  SCommandClassSet_t *pCCSet;

  pCCSet = ZW_system_startup_GetCCSet();

  if ( NULL == m_pNetworkInfo || !pCCSet)
  {
    return &EmptyCCList;
  }

  if ( m_pNetworkInfo != NULL ){
    EInclusionState_t eInclusionState = m_pNetworkInfo->eInclusionState;
  
    if (EINCLUSIONSTATE_UNSECURE_INCLUDED == eInclusionState || EINCLUSIONSTATE_EXCLUDED == eInclusionState)
    {
      if (SECURITY_KEY_NONE == eKey)
      {
        return &pCCSet->UnSecureIncludedCC;
      }
    }

    if (EINCLUSIONSTATE_SECURE_INCLUDED == eInclusionState)    
    {
      if (SECURITY_KEY_NONE == eKey)
      {
        return &pCCSet->SecureIncludedUnSecureCC;
      }
      else if(GetHighestSecureLevel() == eKey)
      {
        return &pCCSet->SecureIncludedSecureCC;
      }
    }  
  }
  return &EmptyCCList;
}


/**
* Returns the type of the highest level security key obtained.
*
* @return                           Type of highest level security key obtained.
*/
static security_key_t GetHighestSecureLevel(void)
{
  uint8_t SecurityKeys = m_pNetworkInfo->SecurityKeys;

  if (SECURITY_KEY_S2_ACCESS_BIT & SecurityKeys)
  {
    return SECURITY_KEY_S2_ACCESS;
  }
  else if (SECURITY_KEY_S2_AUTHENTICATED_BIT & SecurityKeys)
  {
    return SECURITY_KEY_S2_AUTHENTICATED;
  }
  else if (SECURITY_KEY_S2_UNAUTHENTICATED_BIT & SecurityKeys)
  {
    return SECURITY_KEY_S2_UNAUTHENTICATED;
  }
  else if (SECURITY_KEY_S0_BIT & SecurityKeys)
  {
    return SECURITY_KEY_S0;
  }

  return SECURITY_KEY_NONE;
}


bool CCListHasClass(const SCommandClassList_t* cclist , uint8_t cmdClass) {
  for(int i=0; i < cclist->iListLength; i++  ) {
    if(cclist->pCommandClasses[i] == cmdClass) return true;
  }
  return false;
}

