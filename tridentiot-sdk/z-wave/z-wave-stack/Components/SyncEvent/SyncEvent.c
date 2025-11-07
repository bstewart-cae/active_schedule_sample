// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file SyncEvent.c
 * @copyright 2019 Silicon Laboratories Inc.
 */

#include "SyncEvent.h"


//===============================   void SyncEvent(void)   ===============================

bool SyncEventIsBound(const SSyncEvent* pThis)
{
  return (0 != pThis->uFunctor.pFunction);
}


void SyncEventUnbind(SSyncEvent* pThis)
{
  pThis->uFunctor.pClassFunction = 0;
  pThis->pObject = 0;
}


void SyncEventBind(SSyncEvent* pThis, void(*pFunction)(void))
{
  SyncEventUnbind(pThis);
  pThis->uFunctor.pFunction = pFunction;
}


void SyncEventBindObject(SSyncEvent* pThis, void(*pFunction)(void *), void* pObject)
{
  SyncEventUnbind(pThis);
  pThis->pObject = pObject;
  pThis->uFunctor.pClassFunction = pFunction;
}


void SyncEventInvoke(const SSyncEvent* pThis)
{
  if (pThis->uFunctor.pFunction)
  {
    if (pThis->pObject)
    {
      pThis->uFunctor.pClassFunction(pThis->pObject);
    }
    else
    {
      pThis->uFunctor.pFunction();
    }
  }
}


//===============================   uint32_t SyncEvent(void)   ===============================

bool SyncEventRetIsBound(const SSyncEventRet* pThis)
{
  return (0 != pThis->uFunctor.pFunction);
}


void SyncEventRetUnbind(SSyncEventRet* pThis)
{
  pThis->uFunctor.pClassFunction = 0;
  pThis->pObject = 0;
}


void SyncEventRetBind(SSyncEventRet* pThis, uint32_t(*pFunction)(void))
{
  SyncEventRetUnbind(pThis);
  pThis->uFunctor.pFunction = pFunction;
}


void SyncEventRetBindObject(SSyncEventRet* pThis, uint32_t(*pFunction)(void *), void* pObject)
{
  SyncEventRetUnbind(pThis);
  pThis->pObject = pObject;
  pThis->uFunctor.pClassFunction = pFunction;
}


uint32_t SyncEventRetInvoke(const SSyncEventRet* pThis)
{
  if (pThis->uFunctor.pFunction)
  {
    if (pThis->pObject)
    {
      return pThis->uFunctor.pClassFunction(pThis->pObject);
    }
    else
    {
      return pThis->uFunctor.pFunction();
    }
  }

  return pThis->Default;
}


void SyncEventRetSetDefault(SSyncEventRet* pThis, uint32_t DefaultReturnValue)
{
  pThis->Default = DefaultReturnValue;
}


//===============================   void SyncEvent(uint32_t)   ===============================

bool SyncEventArg1IsBound(const SSyncEventArg1* pThis)
{
  return (0 != pThis->uFunctor.pFunction);
}


void SyncEventArg1Unbind(SSyncEventArg1* pThis)
{
  pThis->uFunctor.pClassFunction = 0;
  pThis->pObject = 0;
}


void SyncEventArg1Bind(SSyncEventArg1* pThis, void(*pFunction)(uint32_t))
{
  SyncEventArg1Unbind(pThis);
  pThis->uFunctor.pFunction = pFunction;
}


void SyncEventArg1BindObject(SSyncEventArg1* pThis, void(*pFunction)(), void* pObject)
{
  SyncEventArg1Unbind(pThis);
  pThis->pObject = pObject;
  pThis->uFunctor.pClassFunction = pFunction;  
}

void SyncEventArg1Invoke(const SSyncEventArg1* pThis, uint32_t Arg1)
{
  if (pThis->uFunctor.pFunction)
  {
    if (pThis->pObject)
    {
      pThis->uFunctor.pClassFunction(pThis->pObject, Arg1);
    }
    else
    {
      pThis->uFunctor.pFunction(Arg1);
    }
  }
}


//===============================   void SyncEvent(uint32_t, uint32_t)   =====================

bool SyncEventArg2IsBound(const SSyncEventArg2* pThis)
{
  return (0 != pThis->uFunctor.pFunction);
}


void SyncEventArg2Unbind(SSyncEventArg2* pThis)
{
  pThis->uFunctor.pClassFunction = 0;
  pThis->pObject = 0;
}


void SyncEventArg2Bind(SSyncEventArg2* pThis, void(*pFunction)(uint32_t, uint32_t))
{
  SyncEventArg2Unbind(pThis);
  pThis->uFunctor.pFunction = pFunction;
}


void SyncEventArg2BindObject(SSyncEventArg2* pThis, void(*pFunction)(void *, uint32_t, uint32_t), void* pObject)
{
  SyncEventArg2Unbind(pThis);
  pThis->pObject = pObject;
  pThis->uFunctor.pClassFunction = pFunction;
}

void SyncEventArg2Invoke(const SSyncEventArg2* pThis, uint32_t Arg1, uint32_t Arg2)
{
  if (pThis->uFunctor.pFunction)
  {
    if (pThis->pObject)
    {
      pThis->uFunctor.pClassFunction(pThis->pObject, Arg1, Arg2);
    }
    else
    {
      pThis->uFunctor.pFunction(Arg1, Arg2);
    }
  }
}