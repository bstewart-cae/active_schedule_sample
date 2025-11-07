
/// ***************************************************************************
/// SPDX-License-Identifier: LicenseRef-TridentMSLA
/// SPDX-FileCopyrightText: 2024 Trident IoT, LLC <https://www.tridentiot.com>
/// ***************************************************************************

/**
 * @file s2_span_mpan_storage.c
 */

#include "s2_protocol.h"

#define FILE_SIZE_S2_SPAN            (sizeof(struct SPAN))
#define FILE_SIZE_S2_MPAN            (sizeof(struct MPAN))
static uint8_t s2_span_storage[FILE_SIZE_S2_SPAN * SPAN_TABLE_SIZE]  __attribute__((section(".ret_sram"))) __attribute__((used));
static uint8_t s2_mpan_storage[FILE_SIZE_S2_MPAN * MPAN_TABLE_SIZE]  __attribute__((section(".ret_sram"))) __attribute__((used));

bool StorageGetS2MpanTable(void * mpan_table)
{
  //Set all bytes in mpan to zero.
  memset(mpan_table, 0, MPAN_TABLE_SIZE * FILE_SIZE_S2_MPAN);
  
  //Get list of existing mpan files.
  struct MPAN * src_ptr = (struct MPAN *)&s2_mpan_storage[0];
  struct MPAN * dest_ptr = (struct MPAN *)mpan_table;
  for (uint8_t i = 0; i <MPAN_TABLE_SIZE; i++, src_ptr++ )
  {
    if (MPAN_NOT_USED != src_ptr->state)
    {
      memcpy((uint8_t *)dest_ptr, (uint8_t *)src_ptr, FILE_SIZE_S2_MPAN);
      dest_ptr++;
    }
  }
  return true;
}

bool StorageWriteS2Mpan(uint32_t id, void * mpan)
{
  if (MPAN_TABLE_SIZE <= id)
  {
    return false;
  }

  struct MPAN * mpan_ptr = (struct MPAN *)&s2_mpan_storage[0];
  const struct MPAN * pMpan = (struct MPAN *)mpan;
  if (MPAN_NOT_USED != pMpan->state)
  {
    memcpy((uint8_t *)&mpan_ptr[id], (uint8_t*)pMpan, FILE_SIZE_S2_MPAN);
  }
  return true;
}


bool StorageSetS2MpanTable(void * mpan_table)
{
  
  //Save files with valid content
  for (uint32_t i=0; i < MPAN_TABLE_SIZE; i++)
  {
    StorageWriteS2Mpan(i, (struct MPAN *)mpan_table + i);
  }
  return true;
}


bool StorageGetS2SpanTable(void * span_table)
{
  //Set all bytes in mpan to zero.
  memset(span_table, 0, SPAN_TABLE_SIZE * FILE_SIZE_S2_SPAN);
  
  //Get list of existing mpan files.
  struct SPAN * src_ptr = (struct SPAN *)&s2_span_storage[0];
  struct SPAN * dest_ptr = (struct SPAN *)span_table;
  for (uint8_t i = 0; i <SPAN_TABLE_SIZE; i++, src_ptr++ )
  {
    if (SPAN_NOT_USED != src_ptr->state)
    {
      memcpy((uint8_t *)dest_ptr, (uint8_t *)src_ptr, FILE_SIZE_S2_SPAN);
      dest_ptr++;
    }
  }
  return true;
}

bool StorageWriteS2Span(uint32_t id, void * span)
{
  if (SPAN_TABLE_SIZE <= id)
  {
    return false;
  }
  struct SPAN * span_ptr = (struct SPAN *)&s2_span_storage[0];
  const struct SPAN * pSpan = (struct SPAN *)span;
  if (SPAN_NOT_USED != pSpan->state)
  {
    memcpy((uint8_t *)&span_ptr[id], (uint8_t*)pSpan, FILE_SIZE_S2_SPAN);
  }
  return true;
}

bool StorageSetS2SpanTable(void * span_table)
{
  //Save files with valid content
  for (uint32_t i=0; i < SPAN_TABLE_SIZE; i++)
  {
    StorageWriteS2Span(i, (struct SPAN *)span_table + i);
  }
  return true;
}


void SlaveStorageDeleteMPANs(void)
{
  memset(s2_mpan_storage, 0, MPAN_TABLE_SIZE * FILE_SIZE_S2_MPAN);  
}

void SlaveStorageDeleteSPANs(void)
{
  memset(s2_span_storage, 0, SPAN_TABLE_SIZE * FILE_SIZE_S2_SPAN);  
}
