/// ****************************************************************************
/// SPDX-License-Identifier: LicenseRef-TridentMSLA
/// SPDX-FileCopyrightText: 2023 Trident IoT, LLC <https://www.tridentiot.com>
/// ***************************************************************************

#include <zpal_nvm.h>
#include <zpal_init.h>
#include <zpal_watchdog.h>
#include <FreeRTOS.h>
#include <semphr.h>
#include <MfgTokens.h>
#include <flashctl.h>
#include "string.h"
#include <flashdb.h>
#include <fal_cfg.h>
#include "tr_mfg_tokens.h"

#include <flashdb_low_lvl.h>

extern bool zpal_get_lib_type(void);

#define MULTIPLE_OF_32K(Add) ((((Add) & (0x8000-1)) == 0)?1:0)
#define MULTIPLE_OF_64K(Add) ((((Add) & (0x10000-1)) == 0)?1:0)
#define SIZE_OF_FLASH_SECTOR_ERASE                    4096          /**< Size of Flash erase Type.*/

#define FAL_PART_MAGIC_WORD         0x45503130

#define CTRL_STACK_PART_SIZE       (56 * 1024)
#define CTRL_ZAF_PART_SIZE         (8  * 1024)
#define CTRL_APP_PART_SIZE         (16 * 1024)

#define SLAVE_STACK_PART_SIZE      (16 * 1024)
#define SLAVE_ZAF_PART_SIZE        (32 * 1024)
#define SLAVE_APP_PART_SIZE        (32 * 1024)

extern uint32_t __nvm_storage_start__;
#define STACK_PART_NAME     "stack_db"
#define ZAF_PART_NAME       "zaf_db"
#define APP_PART_NAME       "app_db"

#define STACK_PART_OFFSET    ((uint32_t)&__nvm_storage_start__ - 0x10000000)
#define CTRL_ZAF_PART_OFFSET      STACK_PART_OFFSET + CTRL_STACK_PART_SIZE
#define CTRL_APP_PART_OFFSET      CTRL_ZAF_PART_OFFSET + CTRL_ZAF_PART_SIZE

#define SLAVE_ZAF_PART_OFFSET     STACK_PART_OFFSET + SLAVE_STACK_PART_SIZE
#define SLAVE_APP_PART_OFFSET     SLAVE_ZAF_PART_OFFSET + SLAVE_ZAF_PART_SIZE

static uint32_t mfg_tokens_arr[5];

/*this constant is defined in the linker file*/
extern uint32_t __mfg_tokens_region_start;

static const struct fal_partition ctrl_part_tbl_def[] = {
                                                         {FAL_PART_MAGIC_WORD, STACK_PART_NAME,  NOR_FLASH_DEV_NAME, STACK_PART_OFFSET ,   CTRL_STACK_PART_SIZE, 0},
                                                         {FAL_PART_MAGIC_WORD, ZAF_PART_NAME,    NOR_FLASH_DEV_NAME, CTRL_ZAF_PART_OFFSET, CTRL_ZAF_PART_SIZE,   0},
                                                         {FAL_PART_MAGIC_WORD, APP_PART_NAME,    NOR_FLASH_DEV_NAME, CTRL_APP_PART_OFFSET, CTRL_APP_PART_SIZE,   0}
};

static const struct fal_partition slave_part_tbl_def[] = {
                                                          {FAL_PART_MAGIC_WORD, STACK_PART_NAME,  NOR_FLASH_DEV_NAME, STACK_PART_OFFSET ,   SLAVE_STACK_PART_SIZE, 0},
                                                          {FAL_PART_MAGIC_WORD, ZAF_PART_NAME,    NOR_FLASH_DEV_NAME, SLAVE_ZAF_PART_OFFSET, SLAVE_ZAF_PART_SIZE,   0},
                                                          {FAL_PART_MAGIC_WORD, APP_PART_NAME,    NOR_FLASH_DEV_NAME, SLAVE_APP_PART_OFFSET, SLAVE_APP_PART_SIZE,   0}
};

#define PART_TABLE_LEN    (sizeof(slave_part_tbl_def)/ sizeof(struct fal_partition))
static SemaphoreHandle_t fdbMutex = NULL;
static StaticSemaphore_t fdbMutexBuffer;

// lfs has been mounted
static bool fdb_mounted = false;

// variables used by the filesystem
//static struct fdb_kvdb kvdb = { 0 };
static uint8_t fdb_read_buffer[256];

#define flash_write_buffer fdb_read_buffer
#define FILE_SYSTEM_NAME    "ZWAVE_FS"
// Array of directory names
#define     HANDLE_NAME_MAX   5

const char area_directories[6] = "TOKEN";

typedef struct _fdb_info_t
{
  const char *db_name;
  const char* part_name;
  struct fdb_kvdb kvdb;
} fdb_info_t;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
static fdb_info_t m_fdb_info[3] = {{"APP"  , APP_PART_NAME  , {{0}}},
                                   {"ZAF"  , ZAF_PART_NAME  , {{0}}},
                                   {"STACK", STACK_PART_NAME, {{0}}}};
#pragma GCC diagnostic pop

static bool backup_first_write = true;
void zpal_block_flash_erase(uint32_t flash_addr, uint32_t image_size)
{
  uint32_t ErasedSize = 0;
  while (image_size > ErasedSize)
  {
    zpal_feed_watchdog();
    if (((image_size - ErasedSize) > 0x10000) &&
        (MULTIPLE_OF_64K(flash_addr + ErasedSize)))
    {
      flash_erase(FLASH_ERASE_64K, flash_addr + ErasedSize);
      ErasedSize += 0x10000;
    }
    else if (((image_size - ErasedSize) > 0x8000) &&
             (MULTIPLE_OF_32K(flash_addr + ErasedSize)))
    {
      flash_erase(FLASH_ERASE_32K, flash_addr + ErasedSize);
      ErasedSize += 0x8000;
    }
    else
    {
      flash_erase(FLASH_ERASE_SECTOR, flash_addr + ErasedSize);
      ErasedSize += SIZE_OF_FLASH_SECTOR_ERASE;
    }
  }
}

static void LockFs(__attribute__((unused)) fdb_db_t db)
{
  if (NULL == fdbMutex)
  {
    return;
  }
  xSemaphoreTake( fdbMutex, portMAX_DELAY );

}

static void UnlockFs(__attribute__((unused)) fdb_db_t db)
{
  if (NULL == fdbMutex)
  {
    return;
  }
  xSemaphoreGive( fdbMutex );
}

void key_2_filename(const char *dirname, zpal_nvm_object_key_t key, char *filename)
{
  uint8_t cnt = 0;
  // Copy area name from handle
  filename[cnt++] = *dirname;
  for (uint8_t i = 0; i < sizeof(uint32_t); i++)
  {
    uint8_t digit = ((uint8_t*)&key)[i];
    if (!digit)
    {
      digit = 0x30;
    }
    filename[cnt++] = digit;
  }
  filename[cnt] = 0;
}

extern int fal_partition_init(void);

zpal_nvm_handle_t zpal_nvm_init(zpal_nvm_area_t area)
{
  if (ZPAL_NVM_AREA_MANUFACTURER_TOKENS == area)
  {
    nvm_init();

    // Set up token array
    mfg_tokens_arr[TOKEN_MFG_ZWAVE_COUNTRY_FREQ_ID-1] = tr_get_mfg_token_offset(TR_MFG_TOKEN_ZWAVE_COUNTRY_FREQ);
    mfg_tokens_arr[TOKEN_MFG_ZW_PRK_ID-1] = tr_get_mfg_token_offset(TR_MFG_TOKEN_ZWAVE_PRK);
    mfg_tokens_arr[TOKEN_MFG_ZW_PUK_ID-1] = tr_get_mfg_token_offset(TR_MFG_TOKEN_ZWAVE_PUK);
    mfg_tokens_arr[TOKEN_MFG_ZW_INITIALIZED_ID-1] = tr_get_mfg_token_offset(TR_MFG_TOKEN_ZWAVE_INITIALIZED);
    mfg_tokens_arr[TOKEN_MFG_ZW_QR_CODE_ID-1] = tr_get_mfg_token_offset(TR_MFG_TOKEN_ZWAVE_QR_CODE);

    return (zpal_nvm_handle_t)&__mfg_tokens_region_start;
  }
  // Check if we already mounted the file system
  if (fdb_mounted != true)
  {
    // Initialize file system mutex for locking/unlocking file system
    fdbMutex = xSemaphoreCreateMutexStatic(&fdbMutexBuffer);
    nvm_init();
    fdb_mounted = true;
    fal_partition_init();
  }
  if (ZPAL_LIBRARY_TYPE_CONTROLLER == zpal_get_library_type())
  {
    // controller libary
    fal_set_partition_table_temp((fal_partition_t)&ctrl_part_tbl_def[0], PART_TABLE_LEN);
  }
  else
  {
    fal_set_partition_table_temp((fal_partition_t)&slave_part_tbl_def[0], PART_TABLE_LEN);
  }
  fdb_kvdb_deinit(&m_fdb_info[area].kvdb);
  fdb_kvdb_control(&m_fdb_info[area].kvdb, FDB_KVDB_CTRL_SET_LOCK, (void *)LockFs);
  fdb_kvdb_control(&m_fdb_info[area].kvdb, FDB_KVDB_CTRL_SET_UNLOCK, (void *)UnlockFs);
  fdb_kvdb_init(&m_fdb_info[area].kvdb, m_fdb_info[area].db_name, m_fdb_info[area].part_name, NULL, NULL);
  return (zpal_nvm_handle_t)&m_fdb_info[area];
}

zpal_status_t zpal_nvm_read(zpal_nvm_handle_t handle, zpal_nvm_object_key_t key, void *object, size_t object_size)
{
   zpal_nvm_handle_t mfg_tokens_storage_ptr = (zpal_nvm_handle_t)&__mfg_tokens_region_start;
  if (mfg_tokens_storage_ptr == handle)
  {
    uint32_t offset = ((uint32_t)mfg_tokens_storage_ptr + mfg_tokens_arr[key -1]);
    nvm_mfg_token_read(offset, object_size, object);
    return ZPAL_STATUS_OK;
  }
  struct fdb_blob blob;
  char file_name[6];
  fdb_info_t * p_fdb_info = (fdb_info_t *)handle;
  // Add database name to file_name
  key_2_filename(p_fdb_info->db_name, key, file_name);
  // read data
  fdb_kv_get_blob(&p_fdb_info->kvdb, file_name, fdb_blob_make(&blob, object, object_size));
  /* the blob.saved.len is more than 0 when get the value successful */
  if (blob.saved.len != object_size)
  {
    return ZPAL_STATUS_FAIL;
  }
  return ZPAL_STATUS_OK;
}

zpal_status_t zpal_nvm_read_object_part(zpal_nvm_handle_t handle, zpal_nvm_object_key_t key, void *object, size_t offset, size_t object_size)
{
  struct fdb_blob blob;
  struct fdb_kv kv_obj;
  char file_name[6];
  fdb_info_t * p_fdb_info = (fdb_info_t *)handle;
  // Add database name to file_name
  key_2_filename(p_fdb_info->db_name, key, file_name);
  // read data
  fdb_blob_make(&blob, fdb_read_buffer, sizeof(fdb_read_buffer));
  fdb_kv_get_obj(&p_fdb_info->kvdb, file_name, &kv_obj);
  uint32_t read_len = fdb_blob_read((fdb_db_t)&p_fdb_info->kvdb, fdb_kv_to_blob(&kv_obj, &blob));

  /* the blob.saved.len is more than 0 when get the value successful */
  if (read_len)
  {
     memcpy((uint8_t*)object, &fdb_read_buffer[offset], object_size);
     return ZPAL_STATUS_OK;
  }
  return ZPAL_STATUS_FAIL;
}

zpal_status_t zpal_nvm_write(zpal_nvm_handle_t handle, zpal_nvm_object_key_t key, const void *object, size_t object_size)
{
  zpal_nvm_handle_t mfg_tokens_storage_ptr = (zpal_nvm_handle_t)&__mfg_tokens_region_start;
  if (mfg_tokens_storage_ptr == handle)
  {
    uint32_t mfg_token_addr = ((uint32_t)mfg_tokens_storage_ptr + mfg_tokens_arr[key-1]);
    nvm_mfg_token_write(mfg_token_addr, object_size, (uint8_t*)object );
    return ZPAL_STATUS_OK;
  }
  fdb_info_t * p_fdb_info = (fdb_info_t *)handle;
  char file_name[6];
  // Add database to file_name
  key_2_filename(p_fdb_info->db_name, key, file_name);

  struct fdb_blob blob;
  uint8_t temp_buf[64];
  /*
   * We need to read and compare the new data with the stored data.
   * We should only write the new data if it is different from what is already stored.
   * If the new data exceeds the size of the local buffer, we will read the stored data in chunks and compare it with the new data.
   * This approach helps reduce FLASH writes and prolong the lifespan of the FLASH memory.
  */
  if (object_size > sizeof(temp_buf))
  {
    uint32_t idx = 0;
    struct fdb_kv kv_obj;
    bool new_data = false;
    // Retrieve the address of the stored data using its name.
    fdb_kv_get_obj(&p_fdb_info->kvdb, file_name, &kv_obj);
    fdb_blob_make(&blob, temp_buf, sizeof(temp_buf));
    fdb_kv_to_blob(&kv_obj, &blob);
    const uint8_t *obj_ptr = (const uint8_t *)object;
    uint8_t read_size = sizeof(temp_buf);
    uint8_t count = object_size / sizeof(temp_buf);
    do
    {
      /*
       * Read the stored data chunks and compare them with the new data.
       * If the data is different, flag it as new and stop reading the chunks.
       * If the data is the same, increment the stored data pointer's offset by the size of the local buffer.
       */

      fdb_blob_read((fdb_db_t)&p_fdb_info->kvdb, &blob);
      if (memcmp(temp_buf, &obj_ptr[idx], read_size))
      {
        new_data = true;
        break;
      }
      idx += sizeof(temp_buf);
      blob.saved.addr += sizeof(temp_buf);
    } while (--count);
    /*
     * If the length of the new data is not a multiple of the local buffer length,
     * and all the read data matches the new data chunks,
     * then read the remaining stored data and compare it with the new data.
    */
    read_size = object_size % sizeof(temp_buf);
    if (read_size && !new_data)
    {
      blob.size = read_size;
      fdb_blob_read((fdb_db_t)&p_fdb_info->kvdb, &blob);
      if (memcmp(temp_buf, &obj_ptr[idx], read_size))
      {
        new_data = true;
      }
    }
    // If the new data matches the stored data, then skip the write operation.
    if (!new_data)
    {
      return ZPAL_STATUS_OK;
    }
  }
  else
  {
    // Open file and read the data
    memset(temp_buf, 0xFF, sizeof(temp_buf));
    fdb_kv_get_blob(&p_fdb_info->kvdb, file_name, fdb_blob_make(&blob, temp_buf, object_size));
    // If the new data matches the stored data, then skip the write operation.
    if ((blob.saved.len == object_size) && !memcmp(temp_buf, object, object_size))
    {
      return ZPAL_STATUS_OK;
    }
  }
  fdb_err_t res = fdb_kv_set_blob(&p_fdb_info->kvdb, file_name, fdb_blob_make(&blob, object, object_size));
  if (FDB_NO_ERR == res)
  {
    return ZPAL_STATUS_OK;
  }
  return ZPAL_STATUS_FAIL;
}

zpal_status_t zpal_nvm_erase_all(zpal_nvm_handle_t handle)
{
  if (handle ==(zpal_nvm_handle_t)&__mfg_tokens_region_start)
  {
    return ZPAL_STATUS_OK;
  }
  fdb_info_t * p_fdb_info = (fdb_info_t *)handle;
  if (FDB_NO_ERR != fdb_kv_set_default(&p_fdb_info->kvdb))
  {
    return ZPAL_STATUS_FAIL;
  }
  return ZPAL_STATUS_OK;
}

zpal_status_t zpal_nvm_erase_object(zpal_nvm_handle_t handle, zpal_nvm_object_key_t key)
{
  if (handle ==(zpal_nvm_handle_t)&__mfg_tokens_region_start)
  {
    return ZPAL_STATUS_OK;
  }

  fdb_info_t * p_fdb_info = (fdb_info_t *)handle;
  char file_name[6];
  key_2_filename(p_fdb_info->db_name, key, file_name);
  fdb_err_t res = fdb_kv_del(&p_fdb_info->kvdb, file_name);
  if (FDB_NO_ERR == res)
  {
    return ZPAL_STATUS_OK;
  }
  return ZPAL_STATUS_FAIL;
}

zpal_status_t zpal_nvm_get_object_size(zpal_nvm_handle_t handle, zpal_nvm_object_key_t key, size_t *len)
{
  char file_name[6];
  // Add database name to file_name
  struct fdb_kv kv_obj;
  struct fdb_blob blob;
  fdb_info_t * p_fdb_info = (fdb_info_t *)handle;
  key_2_filename(p_fdb_info->db_name, key, file_name);
  fdb_blob_make(&blob, fdb_read_buffer, sizeof(fdb_read_buffer));
  if (NULL != fdb_kv_get_obj(&p_fdb_info->kvdb, file_name, &kv_obj))
  {
    uint32_t read_len = fdb_blob_read((fdb_db_t)&p_fdb_info->kvdb, fdb_kv_to_blob(&kv_obj, &blob));
    *len = read_len;
    return ZPAL_STATUS_OK;
  }
  return ZPAL_STATUS_FAIL;
}

size_t zpal_nvm_enum_objects(zpal_nvm_handle_t handle,
                             zpal_nvm_object_key_t *key_list,
                             size_t key_list_size,
                             zpal_nvm_object_key_t key_min,
                             zpal_nvm_object_key_t key_max)
{
  (void)key_max;

  char file_name[6];
  fdb_info_t * p_fdb_info = (fdb_info_t *)handle;

  size_t keys_count = 0;
  struct fdb_kv kv_obj;
  for (zpal_nvm_object_key_t obj_key = key_min; obj_key < (key_min + key_list_size) ; obj_key++ )
  {
    key_2_filename(p_fdb_info->db_name, obj_key, file_name);
    if (NULL != fdb_kv_get_obj(&p_fdb_info->kvdb, file_name, &kv_obj))
    {
       keys_count++;
       key_list[obj_key - key_min] = obj_key;
    }
  }
  return keys_count;
}

zpal_status_t zpal_nvm_backup_open(void)
{
  backup_first_write = true;
  return ZPAL_STATUS_OK;
}

void zpal_nvm_backup_close(void)
{
  // TODO: implemnet this
}

#define NVM_STORAGE_OFFSET (uint32_t)&__nvm_storage_start__
zpal_status_t zpal_nvm_backup_read(uint32_t offset, void *data, size_t data_length)
{
  nvm_read((offset + NVM_STORAGE_OFFSET) , data_length, data);
  return ZPAL_STATUS_OK;
}

zpal_status_t zpal_nvm_backup_write(uint32_t offset, const void *data, size_t data_length)
{
  if (backup_first_write)
  {
    backup_first_write = false;
    zpal_block_flash_erase(NVM_STORAGE_OFFSET, NVM_STORAGE_SIZE );
  }
  nvm_write((offset + NVM_STORAGE_OFFSET), data_length, (uint8_t *)data);
  return ZPAL_STATUS_OK;
}

size_t zpal_nvm_backup_get_size(void)
{
  return NVM_STORAGE_SIZE;
}

zpal_status_t zpal_nvm_lock(zpal_nvm_handle_t handle)
{
  (void)(handle);
  return ZPAL_STATUS_OK;
}

zpal_status_t zpal_nvm_migrate_legacy_app_file_system(void)
{
  return ZPAL_STATUS_FAIL;
}
