/// ****************************************************************************
/// SPDX-License-Identifier: LicenseRef-TridentMSLA
/// SPDX-FileCopyrightText: 2023 Trident IoT, LLC <https://www.tridentiot.com>
/// ***************************************************************************

#include "zpal_uart.h"
#include "zpal_uart_gpio.h"
#include "tr_platform.h"
#include "sysctrl.h"
#include "sysfun.h"
#include "status.h"
#include "Assert.h"
#include "tr_ring_buffer.h"
#include <string.h>
#include "tr_hal_gpio.h"
#include "tr_hal_uart.h"

#define NUMBER_OF_UART_PORTS   (sizeof(uart)/sizeof(uart_t))

typedef struct
{
  tr_hal_uart_id_t id; // UART ID
  uint8_t interrupt_priority;
  bool uart_initialized;
  zpal_uart_config_t zpal_config;
  tr_ring_buffer_t ring_buffer;
}
uart_t;

typedef struct
{
  tr_hal_uart_receive_callback_t rx_data_cb;
  tr_hal_uart_event_callback_t uart_events_cb;
}
uart_cb_t;

static uart_t uart[3] = {
  [0] = {
    .id = UART_0_ID,
    .uart_initialized = false,
    .interrupt_priority = IRQ_PRIORITY_NORMAL,
    .zpal_config = {0}
  },
  [1] = {
    .id = UART_1_ID,
    .uart_initialized = false,
    .interrupt_priority = IRQ_PRIORITY_NORMAL,
    .zpal_config = {0}
  },
  [2] = {
    .id = UART_2_ID,
    .uart_initialized = false,
    .interrupt_priority = IRQ_PRIORITY_NORMAL,
    .zpal_config = {0}
  }
};

static zpal_uart_transmit_done_t zpal_uart_transmit_done;
static void uart_rx_data_handler(uart_t *p_uart, uint8_t received_byte)
{
  if (false == tr_ring_buffer_write(&p_uart->ring_buffer, received_byte))
  {
    // Buffer full, byte dropped - let the application handle situation.
  }

}
static void uart_events_handler(uart_t *p_uart, uint32_t event)
{
  if (event & TR_HAL_UART_EVENT_DMA_TX_COMPLETE)
  {
    if (NULL != zpal_uart_transmit_done)
    {
      zpal_uart_transmit_done(p_uart);
    }
  }

  if (event & (TR_HAL_UART_EVENT_RX_ERR_OVERRUN | TR_HAL_UART_EVENT_RX_ERR_PARITY |TR_HAL_UART_EVENT_RX_ERR_FRAMING | TR_HAL_UART_EVENT_RX_ERR_BREAK))
  {
    // somthing went wrong clear the receive buffer
    tr_ring_buffer_init(&p_uart->ring_buffer);
    memset(p_uart->ring_buffer.p_buffer, 0, p_uart->ring_buffer.buffer_size);
  }
  else if (event & (TR_HAL_UART_EVENT_RX_TO_USER_FX|TR_HAL_UART_EVENT_RX_ENDED_TO_USER_FX))
  {
    // Data available.
    if (NULL != p_uart->zpal_config.receive_callback)
    {
      p_uart->zpal_config.receive_callback(p_uart, tr_ring_buffer_get_available(&p_uart->ring_buffer));
    }
  }


#if 0
  if (event & UART_EVENT_RX_TIMEOUT)
  {
    // Received less bytes than expected.
    uart->zpal_config->receive_callback(uart, 1);
  }
#endif
}


static void uart0_rx_data_handler(uint8_t received_byte)
{
  uart_rx_data_handler(&uart[0], received_byte);
}

static void uart1_rx_data_handler(uint8_t received_byte)
{
  uart_rx_data_handler(&uart[1], received_byte);
}

static void uart2_rx_data_handler(uint8_t received_byte)
{
  uart_rx_data_handler(&uart[2], received_byte);
}

static void uart0_events_handler(uint32_t event)
{
  uart_events_handler(&uart[0], event);
}

static void uart1_events_handler(uint32_t event)
{
  uart_events_handler(&uart[1], event);
}

static void uart2_events_handler(uint32_t event)
{
  uart_events_handler(&uart[2], event);
}

static const uart_cb_t uart_cb_list[3] = {
  [0] = {.rx_data_cb = uart0_rx_data_handler,
         .uart_events_cb = uart0_events_handler,
        },
  [1] = {.rx_data_cb = uart1_rx_data_handler,
         .uart_events_cb = uart1_events_handler,
        },
  [2] = {.rx_data_cb = uart2_rx_data_handler,
         .uart_events_cb = uart2_events_handler,
        }
};

static tr_hal_baud_rate_t zpal_to_hal_baud_rate(uint32_t zpal_baud_rate)
{
    switch (zpal_baud_rate)
    {
    case 2400:    return TR_HAL_UART_BAUD_RATE_2400;
    case 4800:    return TR_HAL_UART_BAUD_RATE_4800;
    case 9600:    return TR_HAL_UART_BAUD_RATE_9600;
    case 14400:   return TR_HAL_UART_BAUD_RATE_14400;
    case 19200:   return TR_HAL_UART_BAUD_RATE_19200;
    case 28800:   return TR_HAL_UART_BAUD_RATE_28800;
    case 38400:   return TR_HAL_UART_BAUD_RATE_38400;
    case 57600:   return TR_HAL_UART_BAUD_RATE_57600;
    case 76800:   return TR_HAL_UART_BAUD_RATE_76800;
    case 115200:  return TR_HAL_UART_BAUD_RATE_115200;
    case 230400:  return TR_HAL_UART_BAUD_RATE_230400;
    case 500000:  return TR_HAL_UART_BAUD_RATE_500000;
    case 1000000: return TR_HAL_UART_BAUD_RATE_1000000;
    case 2000000: return TR_HAL_UART_BAUD_RATE_2000000;
    default:
      return TR_HAL_UART_BAUD_RATE_115200;
    }
}

static tr_hal_data_bits_t zpal_to_hal_data_bits(uint8_t zpal_data_bits)
{
  switch (zpal_data_bits)
  {
  case 5:    return LCR_DATA_BITS_5_VALUE;
  case 6:    return LCR_DATA_BITS_6_VALUE;
  case 7:    return LCR_DATA_BITS_7_VALUE;
  case 8:    return LCR_DATA_BITS_8_VALUE;
  default:
    return LCR_DATA_BITS_8_VALUE;
  }
}

static tr_hal_parity_t zpal_to_hal_parity_bit(zpal_uart_parity_bit_t zpal_parity_bit)
{
  switch (zpal_parity_bit)
  {
  case ZPAL_UART_NO_PARITY:   return LCR_PARITY_NONE_VALUE;
  case ZPAL_UART_EVEN_PARITY: return LCR_PARITY_EVEN_VALUE;
  case ZPAL_UART_ODD_PARITY:  return LCR_PARITY_ODD_VALUE;
  default:
    return LCR_PARITY_NONE_VALUE;
  }
}

static tr_hal_stop_bits_t zpal_to_hal_stop_bits(zpal_uart_stop_bits_t zpal_stop_bits)
{
  switch (zpal_stop_bits)
  {
  case ZPAL_UART_STOP_BITS_0P5: return LCR_STOP_BITS_ONE_VALUE;
  case ZPAL_UART_STOP_BITS_1:   return LCR_STOP_BITS_ONE_VALUE;
  case ZPAL_UART_STOP_BITS_1P5: return LCR_STOP_BITS_ONE_VALUE;
  case ZPAL_UART_STOP_BITS_2:   return LCR_STOP_BITS_TWO_VALUE;
  default:
    return LCR_STOP_BITS_ONE_VALUE;
  }
}

static tr_hal_uart_id_t zpal_to_hal_uart_id(zpal_uart_id_t uart_id)
{
  switch (uart_id)
  {
  case ZPAL_UART0: return UART_0_ID ;
  case ZPAL_UART1: return UART_1_ID ;
  case ZPAL_UART2: return UART_2_ID ;
  default:
    ASSERT(0);
    return UART_0_ID;
  }
}

STATIC_ASSERT((ZPAL_UART0 == 0), ASSERT_ZPAL_UART0_ID_mismatch);
STATIC_ASSERT((ZPAL_UART1 == 1), ASSERT_ZPAL_UART1_ID_mismatch);
STATIC_ASSERT((ZPAL_UART2 == 2), ASSERT_ZPAL_UART2_ID_mismatch);

STATIC_ASSERT((UART_0_ID == 0), ASSERT_HAL_UART0_ID_mismatch);
STATIC_ASSERT((UART_1_ID == 1), ASSERT_HAL_UART1_ID_mismatch);
STATIC_ASSERT((UART_2_ID == 2), ASSERT_HAL_UART2_ID_mismatch);

zpal_status_t zpal_uart_init(zpal_uart_config_t const * const config_, zpal_uart_handle_t *handle)
{
  uint8_t tmp_txd_pin = 0;
  uint8_t tmp_rxd_pin = 0;
  uint8_t tmp_cts_pin = 0;
  uint8_t tmp_rts_pin = 0;
  const zpal_uart_config_ext_t *ext_uart_cfg = config_->ptr;

  ASSERT(NULL != config_);
  if (NULL == config_)
  {
    return ZPAL_STATUS_INVALID_ARGUMENT;
  }
  uart_t *uart_ptr = &uart[config_->id];
  ASSERT(NULL != uart_ptr);
  if (NULL == uart_ptr)
  {
    return ZPAL_STATUS_FAIL;
  }
  if (uart_ptr->uart_initialized)
  {
    // Uart already initialized
    if (NULL == uart_ptr->zpal_config.receive_callback)
    {
      // As initial uart init had no receive callback we can reinitialize with the new uart configuration
      tr_hal_uart_uninit(uart_ptr->id);
    }
    else
    {
      // There can only be one receiver of data
      *handle = (zpal_uart_handle_t)uart_ptr; // Pass handle to caller of zpal_uart_init().
      return ZPAL_STATUS_OK;
    }
  }
  enter_critical_section();
  tr_hal_uart_settings_t uart_config;
  // The config contains receive_callback() required by uart_event_handler().
  memcpy(&uart_ptr->zpal_config, config_, sizeof(zpal_uart_config_t));
  zpal_uart_config_t * const config = &uart_ptr->zpal_config;

  if (ext_uart_cfg == NULL)
  {
    tmp_cts_pin = 0;
    tmp_rts_pin = 0;
    uart_config.run_when_sleeping = false;
    // Set up default pins for UARTx
    switch (config->id)
    {
      case ZPAL_UART0:
      {
        tmp_txd_pin = 17;
        tmp_rxd_pin = 16;
      }
      break;

      case ZPAL_UART1:
      {
        tmp_txd_pin = 28;
        tmp_rxd_pin = 29;
      }
      break;

      case ZPAL_UART2:
      {
        tmp_txd_pin = 30;
        tmp_rxd_pin = 31;
      }
      break;

    default:
      break;
    }
  }
  else
  {
    // Custom set UART pins
    tmp_rxd_pin = ext_uart_cfg->rxd_pin;
    tmp_txd_pin = ext_uart_cfg->txd_pin;
    tmp_cts_pin = ext_uart_cfg->cts_pin;
    tmp_rts_pin = ext_uart_cfg->rts_pin;

    uart_config.run_when_sleeping = ext_uart_cfg->uart_wakeup;
  }
  uart_config.baud_rate = zpal_to_hal_baud_rate(config->baud_rate);
  // Set internal UART configuration structure
  if (uart_config.run_when_sleeping)
  {
    uart_config.sleep_baud_rate = uart_config.baud_rate;
    uart_config.sleep_clock_to_use = TR_HAL_CLOCK_1M;
    uart_config.clock_to_use = TR_HAL_CLOCK_1M;
  }
  else
  {
    uart_config.clock_to_use = TR_HAL_CLOCK_32M;
  }

  uart_config.data_bits = zpal_to_hal_data_bits(config->data_bits);
  uart_config.stop_bits = zpal_to_hal_stop_bits(config->stop_bits);
  uart_config.parity = zpal_to_hal_parity_bit(config->parity_bit);
  uart_config.hardware_flow_control_enabled = false;
  uart_config.interrupt_priority = TR_HAL_INTERRUPT_PRIORITY_3; //IRQ_PRIORITY_NORMAL
  uart_config.rx_dma_enabled = false;
  uart_config.rx_dma_buffer = NULL;
  uart_config.rx_dma_buff_length = 0;
  uart_config.tx_pin.pin =  tmp_txd_pin;
  uart_config.rx_pin.pin = tmp_rxd_pin;
  uart_config.rx_bytes_before_trigger = FCR_TRIGGER_1_BYTE;
  uart_ptr->ring_buffer.p_buffer    = config->rx_buffer;
  uart_ptr->ring_buffer.buffer_size = config->rx_buffer_len;
  tr_ring_buffer_init(&uart_ptr->ring_buffer);

  if (tmp_cts_pin)
  {
    uart_config.cts_pin.pin = tmp_cts_pin;
  }
  if (tmp_rts_pin)
  {
    uart_config.rts_pin.pin = tmp_rts_pin;
  }
  // Initialize uart
  if (config->flags & ZPAL_UART_CONFIG_FLAG_BLOCKING)
  {
    uart_config.tx_dma_enabled = false;
    uart_config.enable_chip_interrupts = false;
    uart_config.event_handler_fx = NULL;
    uart_config.rx_handler_function = NULL;
  }
  else
  {
    tr_hal_uart_id_t _id = zpal_to_hal_uart_id(config->id);
    uart_config.tx_dma_enabled = true;
    uart_config.enable_chip_interrupts = true;
    uart_config.rx_handler_function = uart_cb_list[_id].rx_data_cb;
    uart_config.event_handler_fx = uart_cb_list[_id].uart_events_cb;
  }

  tr_hal_status_t retval = tr_hal_uart_init(uart_ptr->id, &uart_config);
  if (TR_HAL_SUCCESS != retval)
  {
    leave_critical_section();
    return ZPAL_STATUS_FAIL;
  }
  if (uart_config.run_when_sleeping)
  {
    retval = tr_hal_uart_set_power_mode(uart_ptr->id, TR_HAL_POWER_MODE_1);
  }
  else
  {
    retval = tr_hal_uart_set_power_mode(uart_ptr->id, TR_HAL_POWER_MODE_0 );
  }
  if (TR_HAL_SUCCESS != retval)
  {
    leave_critical_section();
    return ZPAL_STATUS_FAIL;
  }
  *handle = (zpal_uart_handle_t)uart_ptr; // Pass handle to caller of zpal_uart_init().
  uart_ptr->uart_initialized = true;
  leave_critical_section();
  return ZPAL_STATUS_OK;
}

zpal_status_t zpal_uart_enable(zpal_uart_handle_t handle)
{
  uart_t * p_uart = (uart_t *)handle;
  tr_hal_status_t retval = tr_hal_uart_power_on(p_uart->id);
  if (TR_HAL_SUCCESS   != retval)
  {
    return ZPAL_STATUS_FAIL;
  }
  return ZPAL_STATUS_OK;
}

zpal_status_t zpal_uart_disable(zpal_uart_handle_t handle)
{
  uart_t * p_uart = (uart_t *)handle;
  tr_hal_status_t retval = tr_hal_uart_power_off(p_uart->id);
  if (TR_HAL_SUCCESS   != retval)
  {
    return ZPAL_STATUS_FAIL;
  }
  return ZPAL_STATUS_OK;
}

zpal_status_t zpal_uart_transmit(zpal_uart_handle_t handle, const uint8_t *data, size_t length,
                                 zpal_uart_transmit_done_t tx_cb)
{
  tr_hal_status_t retval;
  uart_t * p_uart = (uart_t *)handle;
  enter_critical_section();
  zpal_uart_transmit_done = tx_cb;
  if (p_uart->zpal_config.flags  & ZPAL_UART_CONFIG_FLAG_BLOCKING)
  {

    bool is_tx_active = false;
    do{
      tr_hal_uart_tx_active(p_uart->id, &is_tx_active);
    } while(is_tx_active);
     retval = tr_hal_uart_raw_tx_buffer(p_uart->id, data, length);
     is_tx_active = false;
    do{
      tr_hal_uart_tx_active(p_uart->id, &is_tx_active);
    } while(is_tx_active);

  }
  else
  {
    retval = tr_hal_uart_dma_tx_bytes_in_buffer(p_uart->id, data, length);
  }
  leave_critical_section();

  if (TR_HAL_SUCCESS  != retval)
  {
    return ZPAL_STATUS_FAIL;
  }
  return ZPAL_STATUS_OK;
}

bool zpal_uart_transmit_in_progress(zpal_uart_handle_t handle)
{
  uart_t * p_uart = (uart_t *)handle;
  bool tx_is_active = false;
  tr_hal_uart_tx_active(p_uart->id, &tx_is_active);
  return tx_is_active;
}

size_t zpal_uart_get_available(zpal_uart_handle_t handle)
{
  size_t retval;
  uart_t * p_uart = (uart_t *)handle;
  enter_critical_section();
  retval = tr_ring_buffer_get_available(&p_uart->ring_buffer);
  leave_critical_section();
  return retval;
}

size_t zpal_uart_receive(zpal_uart_handle_t handle, uint8_t *data, size_t length)
{
  size_t retval;
  uart_t * p_uart = (uart_t *)handle;
  enter_critical_section();
  retval = tr_ring_buffer_read(&p_uart->ring_buffer, data, length);
  leave_critical_section();
  return retval;
}

zpal_status_t zpal_uart_uninit(zpal_uart_id_t uart_id)
{
  uart_t *ptr = &uart[uart_id];
  if (NULL != ptr)
  {
    tr_hal_uart_power_off(ptr->id);
    tr_hal_uart_uninit(ptr->id);
    ptr->uart_initialized = false;
    return ZPAL_STATUS_OK;
  }
  return ZPAL_STATUS_FAIL;
}