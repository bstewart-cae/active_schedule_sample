/// ****************************************************************************
/// SPDX-License-Identifier: LicenseRef-TridentMSLA
/// SPDX-FileCopyrightText: 2023 Trident IoT, LLC <https://www.tridentiot.com>
/// ***************************************************************************

#include <zpal_misc.h>
#include <zpal_uart.h>
#include <zpal_uart_gpio.h>
#include "zpal_retention_register.h"
#include "zpal_defs.h"
#include "zpal_misc_private.h"
#include <stdlib.h>
#include <Assert.h>
#include <unistd.h>
#include <string.h>
#include "tr_platform.h"
#include "sysfun.h"
#include "flashctl.h"
#include <FreeRTOS.h>

#include <DebugPrint.h>

static uint32_t m_gpio_status __attribute__((section(".ret_sram"))) __attribute__((used));
static uint32_t m_internal_soft_reset __attribute__((section(".ret_sram"))) __attribute__((used));
static uint32_t m_reset_counter __attribute__((section(".ret_counter"))) __attribute__((used));

#define INTERNAL_SOFT_RESET   0xdeadbeef

#define UNUSED(x) (void)x
extern const app_version_info_t app_version_info;
extern const uint16_t ZPAL_PRODUCT_ID_INSTALLER_ICON_TYPE;
extern const uint8_t ZPAL_PRODUCT_ID_GENERIC_TYPE;
extern const uint8_t ZPAL_PRODUCT_ID_SPECIFIC_TYPE;
extern const uint8_t ZPAL_PRODUCT_ID_REQUESTED_SECURITY_KEYS;

static zpal_uart_handle_t uart_handle = NULL;
#define COMM_INT_RX_BUFFER_SIZE   16
#define COMM_INT_TX_BUFFER_SIZE   32

// 0x14(20) and 0x00 are chosen to indicate that this is the T32CZ20 Trident IoT chip
static const uint8_t TR_ZW_CHIP_TYPE     = 0x14;
static const uint8_t TR_ZW_CHIP_REVISION = 0x00;

void zpal_reboot(void)
{
  #pragma message "notice: zpal_reboot is deprecated in favor of zpal_reboot_with_info and may be removed in the future"
  zpal_reboot_with_info(0x462, ZPAL_RESET_INFO_DEFAULT);
}

void zpal_reboot_with_info(const zpal_soft_reset_mfid_t manufacturer_id,
                           const zpal_soft_reset_info_t reset_info)
{
  uint32_t all_reset_info = (manufacturer_id << 16) | reset_info;
  if (ZPAL_STATUS_OK != zpal_retention_register_write(ZPAL_RETENTION_REGISTER_RESET_INFO, all_reset_info)){
    DPRINTF("Error while storing reset information (%hu)", all_reset_info);
  }
  Sys_Software_Reset();
}

void zpal_initiate_shutdown_handler(void)
{

}

void zpal_shutdown_handler(void)
{

}

typedef struct
{
  uint8_t id[8];
} serial_number_t;

size_t zpal_get_serial_number_length(void)
{
  return sizeof(serial_number_t);
}

void zpal_get_serial_number(uint8_t *serial_number)
{
  serial_number_t uuid = {.id = {0x12, 0x23, 0x45, 0x78, 0x90, 0xAB, 0xCD, 0xEF}};
  while (flash_check_busy()) {;}
  flash_get_unique_id((uint32_t)uuid.id, sizeof(serial_number_t)); // NOSONAR
  memcpy(serial_number, uuid.id, sizeof(serial_number_t));
}

bool zpal_in_isr(void)
{
  return (bool)(SCB->ICSR & SCB_ICSR_VECTACTIVE_Msk);
}

uint8_t zpal_get_chip_type(void)
{
  return TR_ZW_CHIP_TYPE;
}

uint8_t zpal_get_chip_revision(void)
{
  return TR_ZW_CHIP_REVISION;
}

uint32_t zpal_get_app_version(void)
{
  app_version_t const *app_version = &app_version_info.app_version;
  return ((app_version->app_version_major << 16) | (app_version->app_version_minor << 8) | app_version->app_version_patch) ;
}

uint8_t zpal_get_app_version_major(void)
{
  return app_version_info.app_version.app_version_major;
}

uint8_t zpal_get_app_version_minor(void)
{
  return app_version_info.app_version.app_version_minor;
}

uint8_t zpal_get_app_version_patch(void)
{
  return app_version_info.app_version.app_version_patch;
}

void zpal_get_product_id(zpal_product_id_t *product_id)
{
  product_id->app_manufacturer_id = app_version_info.manufacturer_id;
  product_id->app_product_id = app_version_info.product_id;
  product_id->app_product_type = app_version_info.product_type_id;

  // Installer icon type (not user icon type) according to the
  // Node Provisioning QR Code Format (S2, Smart Start).
  product_id->app_icon_type = ZPAL_PRODUCT_ID_INSTALLER_ICON_TYPE;

  product_id->generic_type = ZPAL_PRODUCT_ID_GENERIC_TYPE;
  product_id->specyfic_type = ZPAL_PRODUCT_ID_SPECIFIC_TYPE;
  product_id->requested_security_key = ZPAL_PRODUCT_ID_REQUESTED_SECURITY_KEYS;
}

uint16_t zpal_get_hardware_manufacturer_id(void)
{
  return 0x0462; // Trident IoT manufacturer ID.
}

bool zpal_get_manufacturer_specific_chip_info(
  uint8_t *p_info,
  uint8_t *p_info_length,
  uint8_t info_length_max
)
{
  if ((NULL == p_info) || (NULL == p_info_length)) {
    ASSERT(false);
    return false;
  }
  const uint8_t INFO_LENGTH = 2;
  if (info_length_max < INFO_LENGTH) {
    ASSERT(false);
    *p_info_length = 0;
    return false;
  }
  *(p_info++) = TR_ZW_CHIP_TYPE;
  *(p_info++) = TR_ZW_CHIP_REVISION;
  *p_info_length = INFO_LENGTH;
  return true;
}

void zpal_disable_interrupts(void)
{
  __disable_irq();
}

__attribute__((weak)) void zpal_debug_init(zpal_debug_config_t debug_uart)
{

  zpal_uart_config_t *uart_cfg = (zpal_uart_config_t*) debug_uart;
  if (NULL == uart_cfg)
  {
    // Allocate UART structure
    uart_cfg = pvPortMalloc(sizeof(zpal_uart_config_t));
    ASSERT(NULL != uart_cfg);
    // Allocate UART Rx buffer
    uart_cfg->rx_buffer = pvPortMalloc(sizeof(COMM_INT_RX_BUFFER_SIZE));
    ASSERT(NULL != uart_cfg->rx_buffer);
    // Allocate UART Tx buffer
    uart_cfg->tx_buffer = pvPortMalloc(sizeof(COMM_INT_TX_BUFFER_SIZE));
    ASSERT(NULL != uart_cfg->tx_buffer);

    uart_cfg->id               = ZPAL_UART0;
    uart_cfg->baud_rate        = 115200;
    uart_cfg->tx_buffer_len    = COMM_INT_TX_BUFFER_SIZE;
    uart_cfg->rx_buffer_len    = COMM_INT_RX_BUFFER_SIZE;
    uart_cfg->parity_bit       = ZPAL_UART_NO_PARITY;
    uart_cfg->stop_bits        = ZPAL_UART_STOP_BITS_1;
    uart_cfg->ptr              = NULL;
    uart_cfg->receive_callback = NULL;
    uart_cfg->flags            = ZPAL_UART_CONFIG_FLAG_BLOCKING;
  }

  zpal_status_t ret = zpal_uart_init(uart_cfg, &uart_handle);

  ASSERT(ZPAL_STATUS_OK == ret);
}

__attribute__((weak)) void zpal_debug_output(const uint8_t *data, uint32_t length)
{
  while (zpal_uart_transmit_in_progress(uart_handle));
  zpal_uart_transmit(uart_handle, data, length, NULL);
  while (zpal_uart_transmit_in_progress(uart_handle));
}

// Unused function.
zpal_chip_se_type_t zpal_get_secure_element_type(void)
{
  return ZPAL_CHIP_SE_UNKNOWN;
}

// Unused function.
void zpal_psa_set_location_persistent_key(const void *attributes)
{
  UNUSED(attributes);
}

// Unused function.
void zpal_psa_set_location_volatile_key(const void *attributes)
{
  UNUSED(attributes);
}


void zpal_gpio_status_store(uint32_t gpio_status)
{
  m_gpio_status =  gpio_status;
}

uint32_t zpal_gpio_status_get(void)
{
  return m_gpio_status;
}

uint32_t zpal_get_restarts(void)
{
  return m_reset_counter;
}

void zpal_clear_restarts(void)
{
  m_reset_counter = 0;
}

void zpal_increase_restarts(void)
{
  m_reset_counter++;
}

void zpal_reset_soft(void)
{
  m_internal_soft_reset = INTERNAL_SOFT_RESET;
  NVIC_SystemReset();
}

bool zpal_was_internal_soft_reset(void)
{
  if (INTERNAL_SOFT_RESET == m_internal_soft_reset)
  {
    m_internal_soft_reset = 0;
    return true;
  }
  return false;
}