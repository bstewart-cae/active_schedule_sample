/// ***************************************************************************
/// SPDX-License-Identifier: LicenseRef-TridentMSLA
/// SPDX-FileCopyrightText: 2024 Trident IoT, LLC <https://www.tridentiot.com>
/// ***************************************************************************

#include "Assert.h"
#include "zpal_defs.h"
#include "zpal_bootloader.h"
#include "zpal_misc_private.h"
#include "zpal_misc.h"
#include "zpal_misc_private.h"
#include <zpal_watchdog.h>
#include "string.h"
#include "tr_platform.h"
#include "sysfun.h"
#include <flashctl.h>
#include "flashdb_low_lvl.h"
#include "cz20_fota_define.h"
//#define DEBUGPRINT // NOSONAR
#include <DebugPrint.h>

#define bootloader_ota_information_t  fota_information_t

typedef struct
{
  app_version_info_t  app_version_info;
  uint32_t fw_crc32;
  uint32_t fw_start_addr;
  uint32_t fw_end_addr;
  uint32_t options; // 1 byte options + 3 byte binary image size
  uint8_t  decryption_iv[FOTA_AES_IV_SIZE];
} ota_header_t ;

#define OTA_UPDATE_OK                                 0
static fota_information_t m_ota_info __attribute__((__used__, section(".btl_mailbox")));
#define OTA_IMAGE_FILE_START_ADDR                     0x10099000           // the address we store ota image file
#define OTA_IMAGE_BANK_START_ADDR                     (0x10099000 + 48)    // the address the bootloader start reading the binary from
#define APP_START_ADDR                                0x10008000
#define OTA_IMAGE_BANK_LENGTH                         (400 * 1024)
#define OTA_MAGIC_WORD                                0xA55A6543

#define MULTIPLE_OF_32K(Add) ((((Add) & (0x8000-1)) == 0)?1:0)
#define MULTIPLE_OF_64K(Add) ((((Add) & (0x10000-1)) == 0)?1:0)
#define SIZE_OF_FLASH_SECTOR_ERASE                    4096          /**< Size of Flash erase Type.*/
#define  MEMBER_OFFSET(TYPE, MEMBER)  ((uint32_t)&((TYPE *)0)->MEMBER)

STATIC_ASSERT(sizeof(app_version_info_t) == 16, STATIC_ASSERT_FAILED_zpal_bootloader_app_version_info_t_wrong_size);

static uint32_t ota_crc32checksum(uint32_t flash_addr, uint32_t data_len)
{
    const uint8_t *p_buf = ((uint8_t *)flash_addr);
    uint32_t chk_sum = ~0;
    for (uint32_t i = 0; i < data_len; i ++ )
    {
        chk_sum ^= *p_buf++;
        for (uint16_t k = 0; k < 8; k ++)
        {
            chk_sum = chk_sum & 1 ? (chk_sum >> 1) ^ 0xedb88320 : chk_sum >> 1;
        }
    }
    return ~chk_sum;
}

static void ota_info_read(const uint8_t *flash_addr, uint8_t *pdata, uint32_t data_len)
{
  memcpy(pdata, flash_addr, data_len);
}

void zpal_bootloader_get_info(zpal_bootloader_info_t *info)
{
  if (NULL != info) {
    info->capabilities = ZPAL_BOOTLOADER_CAPABILITY_STORAGE;
    info->type = ZPAL_BOOTLOADER_PRESENT;
    info->version = 1<<ZPAL_BOOTLOADER_VERSION_MAJOR_SHIFT;
  }
}

zpal_status_t zpal_bootloader_init(void)
{
  flash_set_read_pagesize();
  return ZPAL_STATUS_OK;
}
extern uint32_t __flash_max_size__;

extern bool xmodem_receive(void);
void zpal_bootloader_reboot_and_install(void)
{
/**
 * For T32CZ20
 * The information should be written in the data structure (bootloader_ota_information_t)
 *  located at the start of the RAM.
 * First, read the OTA image header from flash.
 * If it is initialized, then this indicates an OTA update.
 * In this case, we initialize the bootloader mailbox and reboot the system.
 * Otherwise, if it is OTW, we begin receiving the image through UART.
 * Once the transfer is complete, we initialize the bootloader mailbox and reboot.
 */
   ota_header_t  ota_header;
   ota_info_read((uint8_t*)(OTA_IMAGE_FILE_START_ADDR), (uint8_t*)&ota_header, sizeof(ota_header_t));
   uint32_t length = (ota_header.fw_end_addr - ota_header.fw_start_addr);
   memset(&m_ota_info, 0xff, sizeof(bootloader_ota_information_t));
   uint8_t *ptr = (uint8_t*)&ota_header;
   if (ptr[0] != 0xFF && ptr[1] != 0xFF &&
       ptr[2] != 0xFF && ptr[3] != 0xFF && ptr[4] != 0xFF)
   {
     // The OTA header has been initialized, which indicates that the OTA has triggered the update.
     m_ota_info.fotabank_ready = OTA_MAGIC_WORD;
   }

   if (OTA_MAGIC_WORD != m_ota_info.fotabank_ready)
   {
     // The OTA header has not been initialized, so the update must be triggered by the OTW process.
     // receives the image through xmodem
     bool image_rx_ok = xmodem_receive();
     // check the image verion for roll-back protection
     if (image_rx_ok && (ZPAL_STATUS_OK == zpal_bootloader_verify_image()))
     {
       ota_info_read((uint8_t*)(OTA_IMAGE_FILE_START_ADDR), (uint8_t*)&ota_header, sizeof(ota_header_t));
       length = (ota_header.fw_end_addr - ota_header.fw_start_addr);
       m_ota_info.fotabank_ready = OTA_MAGIC_WORD;
     }
     else
     {
       memset(&ota_header, 0, sizeof(ota_header_t));
     }
   }
   m_ota_info.fotabank_crc = ota_header.fw_crc32;
   m_ota_info.fotabank_datalen = length;
   m_ota_info.fotabank_startaddr = OTA_IMAGE_BANK_START_ADDR ;
   m_ota_info.target_startaddr = APP_START_ADDR;
   m_ota_info.fota_image_info = ota_header.options;
   memcpy(m_ota_info.fota_encryption_iv, ota_header.decryption_iv, sizeof(m_ota_info.fota_encryption_iv));
   m_ota_info.signature_len = *(int32_t *)(OTA_IMAGE_BANK_START_ADDR + length - 252);
   m_ota_info.max_image_size = (uint32_t)&__flash_max_size__; // the maximum length of the app image
   m_ota_info.image_binary_size = ota_header.options >> 8;
   zpal_reset_soft();
}

static bool ota_version_check(app_version_t *app_version)
{
  uint32_t new_version = (((uint32_t)(app_version->app_version_major<<16)) | ((uint32_t)(app_version->app_version_minor<<8))  |   ((uint32_t)app_version->app_version_patch));
  if (new_version > zpal_get_app_version())
  {
    return true;
  }
  return false;
}

zpal_status_t zpal_bootloader_verify_image(void)
{
  ota_header_t  ota_header;

  ota_info_read((uint8_t*)(OTA_IMAGE_FILE_START_ADDR), (uint8_t*)&ota_header, sizeof(ota_header_t));
  if (false == ota_version_check(&ota_header.app_version_info.app_version))
  {
    return ZPAL_STATUS_BOOTLOADER_DOWNGRADE_NOT_SUPPORTED;
  }
  uint32_t crc32_val = ota_crc32checksum(OTA_IMAGE_BANK_START_ADDR , (ota_header.fw_end_addr - ota_header.fw_start_addr));
  if (crc32_val != ota_header.fw_crc32)
  {
    return ZPAL_STATUS_BOOTLOADER_INVALID_CHECKSUM;
  }
  return ZPAL_STATUS_OK;
}

zpal_status_t zpal_bootloader_write_data(uint32_t offset, uint8_t *data, uint16_t length)
{
  if (OTA_IMAGE_BANK_LENGTH <= (offset + length))
  {
    return ZPAL_STATUS_FAIL;
  }
  nvm_write(OTA_IMAGE_FILE_START_ADDR + offset, length, data);
  return ZPAL_STATUS_OK;
}

bool zpal_bootloader_is_first_boot(bool *updated_successfully)
{
   *updated_successfully = false;
   if (m_ota_info.fotabank_ready == OTA_MAGIC_WORD)
   {
     if (OTA_UPDATE_OK == m_ota_info.fota_result)
     {
       *updated_successfully = true;
     }
     memset((uint8_t*)&m_ota_info, 0xFF, sizeof(bootloader_ota_information_t));
     return true;
   }
   return false;

}

void zpal_bootloader_reset_page_counters(void)
{
  ota_header_t ota_header;
  ota_info_read((uint8_t*)(OTA_IMAGE_FILE_START_ADDR), (uint8_t*)&ota_header, sizeof(ota_header_t));
  uint8_t *ptr = (uint8_t*)&ota_header;
  if (ptr[0] != 0xFF && ptr[1] != 0xFF &&
      ptr[2] != 0xFF && ptr[3] != 0xFF && ptr[4] != 0xFF)
  {
    zpal_block_flash_erase(OTA_IMAGE_FILE_START_ADDR, OTA_IMAGE_BANK_LENGTH);
  }
}
