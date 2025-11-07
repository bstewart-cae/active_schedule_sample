/// ****************************************************************************
/// SPDX-License-Identifier: LicenseRef-TridentMSLA
/// SPDX-FileCopyrightText: 2023 Trident IoT, LLC <https://www.tridentiot.com>
/// ***************************************************************************

#include "tr_platform.h"
#include "sysfun.h"
#include <string.h>
#include "flashctl.h"
#include <fal.h>
#include <zpal_watchdog.h>
#include <flashdb_low_lvl.h>

#define NVM_PAGE_CACHED

#define FLASH_START_ADDR      0x10000000
static inline uint32_t _min(uint32_t a, uint32_t b) {
    return (a < b) ? a : b;
}

// Align to nearest multiple of a size
static inline uint32_t _aligndown(uint32_t a, uint32_t alignment) {
    return a - (a % alignment);
}
static uint8_t zw_nvm_storage[NVM_STORAGE_SIZE] __attribute__((section(".nvm_storage"))) __attribute__((used));

#define NVM_PAGE_BUFFER_SIZE  2
static uint8_t nvm_page_buf[NVM_PAGE_BUFFER_SIZE][NVM_PAGE_SIZE] __attribute__((aligned(4)));  // FLASH controller require the buffer to be 4-bytes aligned

#ifdef NVM_PAGE_CACHED
typedef struct _nvm_page_cache
{
  uint32_t page_addr;
  bool dirty;
} nvm_page_cache_t;

static nvm_page_cache_t nvm_page_cache[NVM_PAGE_BUFFER_SIZE] = {{0, true}, {0, true}};
#endif

static uint8_t nvm_page_buf_index = 1;
static uint8_t nvm_page_buf_index_lru = 0;

static int init(void)
{
  /* do nothing now */

  return 0;
}

void nvm_write(uint32_t nvmAddress, uint32_t Len, uint8_t *pSrcBuffer)
{
  uint32_t uiAddr = nvmAddress;
  while (Len)
  {
    uint32_t uiPageOffset = uiAddr % NVM_PAGE_SIZE;
    uint32_t uiWriteLen = _min(Len, (NVM_PAGE_SIZE - uiPageOffset));
    uint32_t pageAddr = _aligndown(uiAddr, NVM_PAGE_SIZE);
    memset(nvm_page_buf[nvm_page_buf_index_lru], 0xFF, sizeof(nvm_page_buf[0]));
    memcpy(&nvm_page_buf[nvm_page_buf_index_lru][uiPageOffset], pSrcBuffer, uiWriteLen);
    zpal_feed_watchdog();
    flash_write_page((uint32_t)&nvm_page_buf[nvm_page_buf_index_lru], pageAddr);
    while (flash_check_busy());
    Len -= uiWriteLen;
    pSrcBuffer += uiWriteLen;
    uiAddr += uiWriteLen;
#ifdef NVM_PAGE_CACHED
    nvm_page_cache[nvm_page_buf_index_lru].dirty = true;
#endif
  }
}

static int _write(long offset, const uint8_t *buf, size_t size)
{
  uint32_t uiAddr = offset + FLASH_START_ADDR;
  nvm_write(uiAddr, size, (uint8_t *)buf);
  return size;
}

void nvm_read(uint32_t nvmAddress, uint32_t Len, uint8_t *pDestBuffer)
{
  uint32_t uiAddr = nvmAddress;
  while (Len)
  {
    uint32_t uiPageOffset = uiAddr % NVM_PAGE_SIZE;
    uint32_t uiReadLen = _min(Len, (NVM_PAGE_SIZE - uiPageOffset));
    uint32_t pageAddr = _aligndown(uiAddr, NVM_PAGE_SIZE);
#ifdef NVM_PAGE_CACHED
    uint32_t i = 0;
    for (; i < NVM_PAGE_BUFFER_SIZE; i++)
    {
      if (!nvm_page_cache[i].dirty && (pageAddr == nvm_page_cache[i].page_addr))
      {
        nvm_page_buf_index = i;
        nvm_page_buf_index_lru = (i + 1) % NVM_PAGE_BUFFER_SIZE;
        break;
      }
    }
    if (i >= NVM_PAGE_BUFFER_SIZE)
    {
      nvm_page_buf_index = nvm_page_buf_index_lru;
      nvm_page_buf_index_lru = (nvm_page_buf_index_lru + 1) % NVM_PAGE_BUFFER_SIZE;
      nvm_page_cache[nvm_page_buf_index].dirty = true;
    }
    if (nvm_page_cache[nvm_page_buf_index].dirty || (pageAddr != nvm_page_cache[nvm_page_buf_index].page_addr))
    {
#endif
      zpal_feed_watchdog();
      flash_read_page((uint32_t)nvm_page_buf[nvm_page_buf_index], pageAddr);
      while (flash_check_busy());
#ifdef NVM_PAGE_CACHED
      nvm_page_cache[nvm_page_buf_index].page_addr = pageAddr;
      nvm_page_cache[nvm_page_buf_index].dirty = false;
    }
#endif
    memcpy(pDestBuffer, &nvm_page_buf[nvm_page_buf_index][uiPageOffset], uiReadLen);
    Len -= uiReadLen;
    pDestBuffer += uiReadLen;
    uiAddr += uiReadLen;
  }
}

static int _read(long offset, uint8_t *buf, size_t size)
{
  // ensure we do not cross page boundaries
  uint32_t uiAddr = offset + FLASH_START_ADDR;
  nvm_read(uiAddr, size, buf);
  return size;
}

static int nvm_erase(long offset, size_t size)
{
  size_t erased_size = 0;
  uint32_t addr = offset + FLASH_START_ADDR;
  while(erased_size < size)
  {
    zpal_feed_watchdog();
    flash_erase(FLASH_ERASE_SECTOR, addr + erased_size);
    while (flash_check_busy());
    erased_size += NVM_ERASE_SIZE;
  }
#ifdef NVM_PAGE_CACHED
  // Just mark all cache pages as dirty
  for (uint32_t i = 0; i < NVM_PAGE_BUFFER_SIZE; i++)
  {
    nvm_page_cache[i].dirty = true;
  }
#endif
  return size;
}

static bool nvm_initialized = false;

void nvm_init(void)
{
  if (nvm_initialized)
  {
    return;
  }
  nvm_initialized = true;
  /*Here we default read page is 256 bytes.*/
  flash_set_read_pagesize();
  Flash_Timing_Init();
}

uint32_t nvm_get_start_address(void)
{
  return (uint32_t)&zw_nvm_storage[0];
}

void nvm_mfg_token_read(uint32_t nvmAddress, uint32_t Len, uint8_t *pDestBuffer)
{
  zpal_feed_watchdog();
  while(Len--)
  {
    *pDestBuffer = flash_read_byte(nvmAddress);
    nvmAddress++;
    pDestBuffer++;
  }
}

void nvm_mfg_token_write(uint32_t nvmAddress, uint32_t Len, uint8_t *pSrcBuffer)
{
  zpal_feed_watchdog();
  while(Len--)
  {
    while (flash_check_busy());
    flash_write_byte(nvmAddress, *pSrcBuffer);
    nvmAddress++;
    pSrcBuffer++;
  }
}

const struct fal_flash_dev t32cz20_onchip_flash =
{
    .name       = "t32cz20_flash",
    .addr       = 0x10000000,
    .len        = 1024 * 1024,
    .blk_size   = 4*1024,
    .ops        = {init, _read, _write, nvm_erase},
    .write_gran = 1
};
