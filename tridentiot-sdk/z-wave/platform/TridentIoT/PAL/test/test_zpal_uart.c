/// ***************************************************************************
/// SPDX-License-Identifier: LicenseRef-TridentMSLA
/// SPDX-FileCopyrightText: 2025 Trident IoT, LLC <https://www.tridentiot.com>
/// ***************************************************************************

#include "unity.h"
#include "unity_print.h"
#include "sysfun.h"
#include "zpal_uart.h"
#include "sysctrl_mock.h"
#include "sysfun_mock.h"
#include "zpal_uart_gpio.h"
#include "status.h"
#include "SizeOf.h"
#include <string.h>
#include "tr_hal_gpio_mock.h"
#include "tr_hal_uart_mock.h"

void setUpSuite(void)
{

}

void tearDownSuite(void)
{

}

#define COMM_INT_TX_BUFFER_SIZE 100
uint8_t tx_data[COMM_INT_TX_BUFFER_SIZE];
#define COMM_INT_RX_BUFFER_SIZE 100
uint8_t rx_data[COMM_INT_RX_BUFFER_SIZE];

static const tr_hal_uart_id_t tr_uart_id[] = {UART_0_ID, UART_1_ID, UART_2_ID};

void receive_callback(zpal_uart_handle_t handle, size_t length)
{
  (void)handle;
  (void)length;
}

zpal_uart_config_t default_config = {
  .receive_callback = receive_callback
};
zpal_uart_handle_t handle;

void set_default_uart_configuration(void)
{
  // Make sure to set all bytes to zero so there will be no random data causing a failed comparison later.
  memset(&default_config, 0x00, sizeof(default_config));
  default_config.id = ZPAL_UART0;
  default_config.tx_buffer = tx_data;
  default_config.tx_buffer_len = COMM_INT_TX_BUFFER_SIZE;
  default_config.rx_buffer = rx_data;
  default_config.rx_buffer_len = COMM_INT_RX_BUFFER_SIZE;
  default_config.baud_rate = 115200;
  default_config.data_bits = 8;
  default_config.parity_bit = ZPAL_UART_NO_PARITY;
  default_config.stop_bits = ZPAL_UART_STOP_BITS_1;
  default_config.ptr = NULL;
  default_config.flags = 0;
}

void setUp(void)
{
  set_default_uart_configuration();

  handle = NULL;

  tr_hal_uart_mock_Init();
}

void tearDown(void)
{
  tr_hal_uart_mock_Verify();
  tr_hal_uart_mock_Destroy();
}

typedef struct
{
  uint32_t input;
  tr_hal_baud_rate_t output;
}
io_baud_rate_t;

const io_baud_rate_t IO_BAUD_RATE[] = {
  {2400,    TR_HAL_UART_BAUD_RATE_2400},
  {4800,    TR_HAL_UART_BAUD_RATE_4800},
  {9600,    TR_HAL_UART_BAUD_RATE_9600},
  {14400,   TR_HAL_UART_BAUD_RATE_14400},
  {19200,   TR_HAL_UART_BAUD_RATE_19200},
  {28800,   TR_HAL_UART_BAUD_RATE_28800},
  {38400,   TR_HAL_UART_BAUD_RATE_38400},
  {57600,   TR_HAL_UART_BAUD_RATE_57600},
  {76800,   TR_HAL_UART_BAUD_RATE_76800},
  {115200,  TR_HAL_UART_BAUD_RATE_115200},
  {230400,  TR_HAL_UART_BAUD_RATE_230400},
  {500000,  TR_HAL_UART_BAUD_RATE_500000},
  {1000000, TR_HAL_UART_BAUD_RATE_1000000},
  {2000000, TR_HAL_UART_BAUD_RATE_2000000},
};

uint32_t get_baud_rate_index(const uint32_t baud_rate)
{
  for (uint32_t i = 0; i < sizeof_array(IO_BAUD_RATE); i++)
  {
    if (baud_rate == IO_BAUD_RATE[i].input)
    {
      return i;
    }
  }
  return sizeof_array(IO_BAUD_RATE);
}
// This is uused to check correct UART id in uart_init_CALLBACK
uint32_t expected_uart_id;
tr_hal_uart_settings_t expected_uart_config;



uint32_t tr_hal_uart_init_CALLBACK(tr_hal_uart_id_t  uart_id, tr_hal_uart_settings_t* uart_settings, int cmock_num_calls)
{
  TEST_ASSERT_EQUAL_UINT32(expected_uart_id, uart_id);

  TEST_ASSERT_EQUAL_MESSAGE(expected_uart_config.baud_rate,            uart_settings->baud_rate, "baudrate");
  TEST_ASSERT_EQUAL_MESSAGE(expected_uart_config.data_bits,            uart_settings->data_bits, "databits");
  TEST_ASSERT_EQUAL_MESSAGE(expected_uart_config.stop_bits,             uart_settings->stop_bits, "stopbits");
  TEST_ASSERT_EQUAL_MESSAGE(expected_uart_config.parity,              uart_settings->parity,"parity");
  TEST_ASSERT_EQUAL_MESSAGE(expected_uart_config.hardware_flow_control_enabled,   uart_settings->hardware_flow_control_enabled,"hwfc");
  TEST_ASSERT_EQUAL_MESSAGE(expected_uart_config.interrupt_priority,  uart_settings->interrupt_priority, "interrupt_priority");

  return STATUS_SUCCESS;
}


void test_zpal_uart_init_baud_rate(void)
{
  // Checked in uart_init_CALLBACK
  expected_uart_id = 0;

  tr_hal_uart_init_Stub(tr_hal_uart_init_CALLBACK);
  tr_hal_gpio_set_mode_IgnoreAndReturn(TR_HAL_SUCCESS);
  tr_hal_gpio_set_direction_IgnoreAndReturn(TR_HAL_SUCCESS);
  tr_hal_gpio_set_pull_mode_IgnoreAndReturn(TR_HAL_SUCCESS);
  tr_hal_uart_uninit_IgnoreAndReturn(TR_HAL_SUCCESS);

  Enter_Critical_Section_Ignore();
  Leave_Critical_Section_Ignore();


  for (uint32_t i = 0; i < 2000001; i++)
  {
    default_config.baud_rate = i;

    uint32_t index = get_baud_rate_index(i);
    tr_hal_baud_rate_t expected_baud_rate = ((index < sizeof_array(IO_BAUD_RATE)) ? IO_BAUD_RATE[index].output : TR_HAL_UART_BAUD_RATE_115200);
    if (index < sizeof_array(IO_BAUD_RATE))
    {
      printf("%u %u\n", i, expected_baud_rate);
    }
    else
    {
      //printf("*");
    }

    memset(&expected_uart_config, 0, sizeof(expected_uart_config));
    expected_uart_config.baud_rate = expected_baud_rate;
    expected_uart_config.data_bits = LCR_DATA_BITS_8_VALUE;
    expected_uart_config.stop_bits = LCR_STOP_BITS_ONE_VALUE;
    expected_uart_config.parity = LCR_PARITY_NONE_VALUE;
    expected_uart_config.hardware_flow_control_enabled = false;
    expected_uart_config.interrupt_priority = 3; // IRQ_PRIORITY_NORMAL

    tr_hal_uart_set_power_mode_ExpectAndReturn(UART_0_ID, TR_HAL_POWER_MODE_0, TR_HAL_SUCCESS);
    zpal_uart_init(&default_config, &handle);

    TEST_ASSERT_NOT_NULL(handle);
  }
  tr_hal_gpio_set_mode_StopIgnore();
  tr_hal_gpio_set_direction_StopIgnore();
  tr_hal_gpio_set_pull_mode_StopIgnore();
  tr_hal_uart_uninit_StopIgnore();
  Enter_Critical_Section_StopIgnore();
  Leave_Critical_Section_StopIgnore();

}

typedef struct
{
  uint8_t input;
  tr_hal_data_bits_t output;
}
data_bits_map_t;

static const data_bits_map_t DATA_BITS_MAP[] = {
  {5, LCR_DATA_BITS_5_VALUE},
  {6, LCR_DATA_BITS_6_VALUE},
  {7, LCR_DATA_BITS_7_VALUE},
  {8, LCR_DATA_BITS_8_VALUE},
};

uint32_t get_data_bits_index(const uint8_t data_bits)
{
  for (uint32_t i = 0; i < sizeof_array(DATA_BITS_MAP); i++)
  {
    if (data_bits == DATA_BITS_MAP[i].input)
    {
      return i;
    }
  }
  return sizeof_array(DATA_BITS_MAP);
}

void test_zpal_uart_init_data_bits(void)
{
  // Checked in uart_init_CALLBACK
  expected_uart_id = 0;

  tr_hal_uart_init_Stub(tr_hal_uart_init_CALLBACK);
  tr_hal_gpio_set_mode_IgnoreAndReturn(TR_HAL_SUCCESS);
  tr_hal_gpio_set_direction_IgnoreAndReturn(TR_HAL_SUCCESS);
  tr_hal_gpio_set_pull_mode_IgnoreAndReturn(TR_HAL_SUCCESS);
  tr_hal_uart_uninit_IgnoreAndReturn(TR_HAL_SUCCESS);
  Enter_Critical_Section_Ignore();
  Leave_Critical_Section_Ignore();


  for (uint8_t i = 0; i < UINT8_MAX; i++)
  {
    default_config.data_bits = i;

    uint32_t index = get_data_bits_index(i);
    tr_hal_data_bits_t expected_data_bits = ((index < sizeof_array(DATA_BITS_MAP)) ? DATA_BITS_MAP[index].output : LCR_DATA_BITS_8_VALUE);

    memset(&expected_uart_config, 0, sizeof(expected_uart_config));

    expected_uart_config.baud_rate = TR_HAL_UART_BAUD_RATE_115200;
    expected_uart_config.data_bits = expected_data_bits;
    expected_uart_config.stop_bits = LCR_STOP_BITS_ONE_VALUE;
    expected_uart_config.parity = LCR_PARITY_NONE_VALUE;
    expected_uart_config.hardware_flow_control_enabled = false;
    expected_uart_config.interrupt_priority = 3; // IRQ_PRIORITY_NORMAL

    tr_hal_uart_set_power_mode_ExpectAndReturn(UART_0_ID, TR_HAL_POWER_MODE_0, TR_HAL_SUCCESS);
    zpal_uart_init(&default_config, &handle);
  }

  tr_hal_gpio_set_mode_StopIgnore();
  tr_hal_gpio_set_direction_StopIgnore();
  tr_hal_gpio_set_pull_mode_StopIgnore();
  tr_hal_uart_uninit_StopIgnore();
  Enter_Critical_Section_StopIgnore();
  Leave_Critical_Section_StopIgnore();

}

typedef struct
{
  zpal_uart_parity_bit_t input;
  tr_hal_parity_t output;
}
parity_bit_map_t;

static const parity_bit_map_t PARITY_BIT_MAP[] = {
  {ZPAL_UART_NO_PARITY,   LCR_PARITY_NONE_VALUE},
  {ZPAL_UART_EVEN_PARITY, LCR_PARITY_EVEN_VALUE},
  {ZPAL_UART_ODD_PARITY,  LCR_PARITY_ODD_VALUE},
};

void test_zpal_uart_init_parity_bit(void)
{
  // Checked in uart_init_CALLBACK
  expected_uart_id = 0;

  tr_hal_uart_init_Stub(tr_hal_uart_init_CALLBACK);
  tr_hal_gpio_set_mode_IgnoreAndReturn(TR_HAL_SUCCESS);
  tr_hal_gpio_set_direction_IgnoreAndReturn(TR_HAL_SUCCESS);
  tr_hal_gpio_set_pull_mode_IgnoreAndReturn(TR_HAL_SUCCESS);
  tr_hal_uart_uninit_IgnoreAndReturn(TR_HAL_SUCCESS);
  Enter_Critical_Section_Ignore();
  Leave_Critical_Section_Ignore();


  for (uint8_t i = 0; i < sizeof_array(PARITY_BIT_MAP); i++)
  {
    default_config.parity_bit = PARITY_BIT_MAP[i].input;

    memset(&expected_uart_config, 0, sizeof(expected_uart_config));

    expected_uart_config.baud_rate = TR_HAL_UART_BAUD_RATE_115200;
    expected_uart_config.data_bits = LCR_DATA_BITS_8_VALUE;
    expected_uart_config.stop_bits = LCR_STOP_BITS_ONE_VALUE;
    expected_uart_config.parity = PARITY_BIT_MAP[i].output;
    expected_uart_config.hardware_flow_control_enabled = false;
    expected_uart_config.interrupt_priority = 3; // IRQ_PRIORITY_NORMAL

    tr_hal_uart_set_power_mode_ExpectAndReturn(UART_0_ID, TR_HAL_POWER_MODE_0, TR_HAL_SUCCESS);
    zpal_uart_init(&default_config, &handle);
  }

  tr_hal_gpio_set_mode_StopIgnore();
  tr_hal_gpio_set_direction_StopIgnore();
  tr_hal_gpio_set_pull_mode_StopIgnore();
  tr_hal_uart_uninit_StopIgnore();
  Enter_Critical_Section_StopIgnore();
  Leave_Critical_Section_StopIgnore();

}

typedef struct
{
  zpal_uart_stop_bits_t input;
  tr_hal_stop_bits_t output;
}
stop_bit_map_t;

static const stop_bit_map_t STOP_BIT_MAP[] = {
  {ZPAL_UART_STOP_BITS_0P5, LCR_STOP_BITS_ONE_VALUE},
  {ZPAL_UART_STOP_BITS_1,   LCR_STOP_BITS_ONE_VALUE},
  {ZPAL_UART_STOP_BITS_1P5, LCR_STOP_BITS_ONE_VALUE},
  {ZPAL_UART_STOP_BITS_2,   LCR_STOP_BITS_TWO_VALUE},
};

void test_zpal_uart_init_stop_bit(void)
{
  // Checked in uart_init_CALLBACK
  expected_uart_id = 0;

  tr_hal_uart_init_Stub(tr_hal_uart_init_CALLBACK);
  tr_hal_gpio_set_mode_IgnoreAndReturn(TR_HAL_SUCCESS);
  tr_hal_gpio_set_direction_IgnoreAndReturn(TR_HAL_SUCCESS);
  tr_hal_gpio_set_pull_mode_IgnoreAndReturn(TR_HAL_SUCCESS);
  tr_hal_uart_uninit_IgnoreAndReturn(TR_HAL_SUCCESS);
  Enter_Critical_Section_Ignore();
  Leave_Critical_Section_Ignore();

  for (uint8_t i = 0; i < sizeof_array(STOP_BIT_MAP); i++)
  {
    default_config.stop_bits = STOP_BIT_MAP[i].input;

    memset(&expected_uart_config, 0, sizeof(expected_uart_config));
    expected_uart_config.baud_rate = TR_HAL_UART_BAUD_RATE_115200;
    expected_uart_config.data_bits = LCR_DATA_BITS_8_VALUE;
    expected_uart_config.stop_bits = STOP_BIT_MAP[i].output;
    expected_uart_config.parity = LCR_PARITY_NONE_VALUE;
    expected_uart_config.hardware_flow_control_enabled = false;
    expected_uart_config.interrupt_priority = 3; // IRQ_PRIORITY_NORMAL

    tr_hal_uart_set_power_mode_ExpectAndReturn(UART_0_ID, TR_HAL_POWER_MODE_0, TR_HAL_SUCCESS);
    zpal_uart_init(&default_config, &handle);
  }

  tr_hal_gpio_set_mode_StopIgnore();
  tr_hal_gpio_set_direction_StopIgnore();
  tr_hal_gpio_set_pull_mode_StopIgnore();
  tr_hal_uart_uninit_StopIgnore();
  Enter_Critical_Section_StopIgnore();
  Leave_Critical_Section_StopIgnore();

}

const zpal_uart_config_ext_t default_pins[3] = {{17,16},{28,29},{30,31}};
const zpal_uart_config_ext_t custom_pins[3] = {{1,2},{3,4},{5,6}};
const zpal_uart_config_ext_t custom_pins_flow_ctrl[3] = {{1,2,3,4},{5,6,7,8},{9,10,11,12}};

typedef struct
{
  uint32_t tx;
  uint32_t rx;
  uint32_t cts;
  uint32_t rts;
}
pin_mode_t;

static const pin_mode_t default_mode[3] = {
  {
    .tx  = TR_HAL_GPIO_MODE_UART_0_TX,
    .rx  = TR_HAL_GPIO_MODE_UART_0_RX,
    .cts = 0,
    .rts = 0
  },
  {
    .tx  = TR_HAL_GPIO_MODE_UART_1_TX,
    .rx  = TR_HAL_GPIO_MODE_UART_1_RX,
    .cts = TR_HAL_GPIO_MODE_UART_1_CTS,
    .rts = TR_HAL_GPIO_MODE_UART_1_RTSN
  },
  {
    .tx  = TR_HAL_GPIO_MODE_UART_2_TX,
    .rx  = TR_HAL_GPIO_MODE_UART_2_RX,
    .cts = TR_HAL_GPIO_MODE_UART_2_CTS,
    .rts = TR_HAL_GPIO_MODE_UART_2_RTSN
  }
};

void test_zpal_uart_init_pins(void)
{
  set_default_uart_configuration();

  tr_hal_uart_init_Stub(tr_hal_uart_init_CALLBACK);
  tr_hal_uart_uninit_IgnoreAndReturn(TR_HAL_SUCCESS);
  Enter_Critical_Section_Ignore();
  Leave_Critical_Section_Ignore();

  memset(&expected_uart_config, 0, sizeof(expected_uart_config));
  expected_uart_config.baud_rate = TR_HAL_UART_BAUD_RATE_115200;
  expected_uart_config.data_bits = LCR_DATA_BITS_8_VALUE;
  expected_uart_config.stop_bits = LCR_STOP_BITS_ONE_VALUE;
  expected_uart_config.parity = LCR_PARITY_NONE_VALUE;
  expected_uart_config.hardware_flow_control_enabled = false;
  expected_uart_config.interrupt_priority = 3; // IRQ_PRIORITY_NORMAL


  default_config.ptr = NULL;
  tr_hal_gpio_pin_t _pin;
  // Loop through all UARTS and check default pins
  for (size_t i = 0; i < 3; i++)
  {
    // Set uart number
    default_config.id = i;
    // Checked in uart_init_CALLBACK
    expected_uart_id = tr_uart_id[i];

    _pin.pin = default_pins[i].txd_pin;
    tr_hal_gpio_set_mode_ExpectAndReturn(_pin, default_mode[i].tx, TR_HAL_SUCCESS );
    tr_hal_gpio_set_pull_mode_ExpectAndReturn(_pin, TR_HAL_PULLOPT_PULL_NONE, TR_HAL_SUCCESS );
    tr_hal_gpio_set_direction_ExpectAndReturn(_pin, TR_HAL_GPIO_DIRECTION_OUTPUT, TR_HAL_SUCCESS );

    _pin.pin = default_pins[i].rxd_pin;
    tr_hal_gpio_set_mode_ExpectAndReturn(_pin, default_mode[i].rx, TR_HAL_SUCCESS );
    tr_hal_gpio_set_direction_ExpectAndReturn(_pin, TR_HAL_GPIO_DIRECTION_INPUT, TR_HAL_SUCCESS );

    tr_hal_uart_set_power_mode_ExpectAndReturn(expected_uart_id, TR_HAL_POWER_MODE_0, TR_HAL_SUCCESS);
    zpal_uart_init(&default_config, &handle);
  }

  set_default_uart_configuration();

  // Test custom UART pins with flow ctrl
  for (size_t i = 0; i < 1; i++)
  {
    default_config.ptr = &custom_pins_flow_ctrl[i];
    // Set uart number
    default_config.id = i;
    // Checked in uart_init_CALLBACK
    expected_uart_id = tr_uart_id[i];

    _pin.pin = custom_pins_flow_ctrl[i].txd_pin;
    tr_hal_gpio_set_mode_ExpectAndReturn(_pin, default_mode[i].tx, TR_HAL_SUCCESS );
    tr_hal_gpio_set_pull_mode_ExpectAndReturn(_pin, TR_HAL_PULLOPT_PULL_NONE, TR_HAL_SUCCESS );
    tr_hal_gpio_set_direction_ExpectAndReturn(_pin, TR_HAL_GPIO_DIRECTION_OUTPUT, TR_HAL_SUCCESS );

    _pin.pin = custom_pins_flow_ctrl[i].rxd_pin;
    tr_hal_gpio_set_mode_ExpectAndReturn(_pin, default_mode[i].rx, TR_HAL_SUCCESS );
    tr_hal_gpio_set_direction_ExpectAndReturn(_pin, TR_HAL_GPIO_DIRECTION_INPUT, TR_HAL_SUCCESS );

    _pin.pin = custom_pins_flow_ctrl[i].cts_pin;
    tr_hal_gpio_set_mode_ExpectAndReturn(_pin, default_mode[i].cts, TR_HAL_SUCCESS );
    tr_hal_gpio_set_direction_ExpectAndReturn(_pin, TR_HAL_GPIO_DIRECTION_INPUT, TR_HAL_SUCCESS );

    _pin.pin = custom_pins_flow_ctrl[i].rts_pin;
    tr_hal_gpio_set_mode_ExpectAndReturn(_pin, default_mode[i].rts, TR_HAL_SUCCESS );
    tr_hal_gpio_set_pull_mode_ExpectAndReturn(_pin, TR_HAL_PULLOPT_PULL_NONE, TR_HAL_SUCCESS );
    tr_hal_gpio_set_direction_ExpectAndReturn(_pin, TR_HAL_GPIO_DIRECTION_OUTPUT, TR_HAL_SUCCESS );

    tr_hal_uart_set_power_mode_ExpectAndReturn(expected_uart_id, TR_HAL_POWER_MODE_0, TR_HAL_SUCCESS);
    zpal_uart_init(&default_config, &handle);
  }

  set_default_uart_configuration();

  // Test custom UART pins without flow ctrl
  for (size_t i = 0; i < 3; i++)
  {
    default_config.ptr = &custom_pins[i];
    // Set uart number
    default_config.id = i;
    // Checked in uart_init_CALLBACK
    expected_uart_id = tr_uart_id[i];

    _pin.pin = custom_pins[i].txd_pin;
    tr_hal_gpio_set_mode_ExpectAndReturn(_pin, default_mode[i].tx, TR_HAL_SUCCESS );
    tr_hal_gpio_set_pull_mode_ExpectAndReturn(_pin, TR_HAL_PULLOPT_PULL_NONE, TR_HAL_SUCCESS );
    tr_hal_gpio_set_direction_ExpectAndReturn(_pin, TR_HAL_GPIO_DIRECTION_OUTPUT, TR_HAL_SUCCESS );

    _pin.pin = custom_pins[i].rxd_pin;
    tr_hal_gpio_set_mode_ExpectAndReturn(_pin, default_mode[i].rx, TR_HAL_SUCCESS );
    tr_hal_gpio_set_direction_ExpectAndReturn(_pin, TR_HAL_GPIO_DIRECTION_INPUT, TR_HAL_SUCCESS );

    tr_hal_uart_set_power_mode_ExpectAndReturn(expected_uart_id, TR_HAL_POWER_MODE_0, TR_HAL_SUCCESS);
    zpal_uart_init(&default_config, &handle);
  }

  Enter_Critical_Section_StopIgnore();
  Leave_Critical_Section_StopIgnore();
  tr_hal_uart_uninit_StopIgnore();
}

void test_zpal_uart_init_fail(void)
{
  tr_hal_gpio_set_mode_IgnoreAndReturn(TR_HAL_SUCCESS);
  tr_hal_gpio_set_direction_IgnoreAndReturn(TR_HAL_SUCCESS);
  tr_hal_gpio_set_pull_mode_IgnoreAndReturn(TR_HAL_SUCCESS);
  tr_hal_uart_uninit_IgnoreAndReturn(TR_HAL_SUCCESS);
  tr_hal_uart_init_IgnoreAndReturn(TR_HAL_ERROR_NOT_INITIALIZED);
  Enter_Critical_Section_Ignore();
  Leave_Critical_Section_Ignore();

  zpal_status_t status = zpal_uart_init(&default_config, &handle);
  TEST_ASSERT_EQUAL(ZPAL_STATUS_FAIL, status);
  TEST_ASSERT_NULL(handle);
  tr_hal_gpio_set_mode_StopIgnore();
  tr_hal_gpio_set_direction_StopIgnore();
  tr_hal_gpio_set_pull_mode_StopIgnore();
  tr_hal_uart_uninit_StopIgnore();
  tr_hal_uart_init_StopIgnore();
  Enter_Critical_Section_StopIgnore();
  Leave_Critical_Section_StopIgnore();

}

void test_zpal_uart_enable_disable(void)
{
  tr_hal_gpio_set_mode_IgnoreAndReturn(TR_HAL_SUCCESS);
  tr_hal_gpio_set_direction_IgnoreAndReturn(TR_HAL_SUCCESS);
  tr_hal_gpio_set_pull_mode_IgnoreAndReturn(TR_HAL_SUCCESS);
  tr_hal_uart_uninit_IgnoreAndReturn(TR_HAL_SUCCESS);
  tr_hal_uart_init_IgnoreAndReturn(TR_HAL_SUCCESS);
  Enter_Critical_Section_Ignore();
  Leave_Critical_Section_Ignore();

  tr_hal_uart_set_power_mode_ExpectAndReturn(UART_0_ID, TR_HAL_POWER_MODE_0, TR_HAL_SUCCESS);
  zpal_status_t status = zpal_uart_init(&default_config, &handle);
  TEST_ASSERT_EQUAL(ZPAL_STATUS_OK, status);

  // Success
  tr_hal_uart_power_on_ExpectAndReturn(UART_0_ID, TR_HAL_SUCCESS);
  status = zpal_uart_enable(handle);
  TEST_ASSERT_EQUAL(ZPAL_STATUS_OK, status);

  tr_hal_uart_power_off_ExpectAndReturn(UART_0_ID, TR_HAL_SUCCESS);
  status = zpal_uart_disable(handle);
  TEST_ASSERT_EQUAL(ZPAL_STATUS_OK, status);

  // Failure
  tr_hal_uart_power_on_ExpectAndReturn(UART_0_ID, TR_HAL_ERROR_INVALID_UART_ID);
  status = zpal_uart_enable(handle);
  TEST_ASSERT_EQUAL(ZPAL_STATUS_FAIL, status);

  tr_hal_uart_power_off_ExpectAndReturn(UART_0_ID, TR_HAL_ERROR_INVALID_UART_ID);
  status = zpal_uart_disable(handle);
  TEST_ASSERT_EQUAL(ZPAL_STATUS_FAIL, status);

  tr_hal_gpio_set_mode_StopIgnore();
  tr_hal_gpio_set_direction_StopIgnore();
  tr_hal_gpio_set_pull_mode_StopIgnore();
  tr_hal_uart_uninit_StopIgnore();
  tr_hal_uart_init_StopIgnore();
  Enter_Critical_Section_StopIgnore();
  Leave_Critical_Section_StopIgnore();

}

static uint32_t zpal_uart_transmit_done_num_calls = 0;

void zpal_uart_transmit_done(zpal_uart_handle_t handle_)
{
  TEST_ASSERT_EQUAL_PTR(handle, handle_);

  zpal_uart_transmit_done_num_calls++;
}

tr_hal_uart_event_callback_t uart_event_handler;

uint32_t my_uart_init_stub(
  tr_hal_uart_id_t  uart_id,
  tr_hal_uart_settings_t* uart_settings,
   int cmock_num_calls)

{
  (void)uart_id;
  (void)cmock_num_calls;

  // Save the event handler passed to uart_init() so we can invoke it later.
  uart_event_handler = uart_settings->event_handler_fx;

  return STATUS_SUCCESS;
}

void test_zpal_uart_transmit(void)
{
  tr_hal_gpio_set_mode_IgnoreAndReturn(TR_HAL_SUCCESS);
  tr_hal_gpio_set_direction_IgnoreAndReturn(TR_HAL_SUCCESS);
  tr_hal_gpio_set_pull_mode_IgnoreAndReturn(TR_HAL_SUCCESS);
  tr_hal_uart_uninit_IgnoreAndReturn(TR_HAL_SUCCESS);
  tr_hal_uart_init_IgnoreAndReturn(TR_HAL_SUCCESS);
  Enter_Critical_Section_Ignore();
  Leave_Critical_Section_Ignore();

  zpal_uart_transmit_done_num_calls = 0;

  tr_hal_uart_init_Stub(my_uart_init_stub);

  tr_hal_uart_set_power_mode_ExpectAndReturn(UART_0_ID, TR_HAL_POWER_MODE_0, TR_HAL_SUCCESS);
  zpal_status_t status = zpal_uart_init(&default_config, &handle);
  TEST_ASSERT_EQUAL(ZPAL_STATUS_OK, status);

  const uint8_t some_data[50];
  tr_hal_uart_dma_tx_bytes_in_buffer_ExpectAndReturn(UART_0_ID, some_data, sizeof(some_data), TR_HAL_SUCCESS);
  status = zpal_uart_transmit(handle, some_data, sizeof(some_data), zpal_uart_transmit_done);
  TEST_ASSERT_EQUAL(ZPAL_STATUS_OK, status);

  // Invoke event handler with TX done event.
  uart_event_handler(TR_HAL_UART_EVENT_DMA_TX_COMPLETE);

  TEST_ASSERT_EQUAL_UINT32(1, zpal_uart_transmit_done_num_calls);

  tr_hal_gpio_set_mode_StopIgnore();
  tr_hal_gpio_set_direction_StopIgnore();
  tr_hal_gpio_set_pull_mode_StopIgnore();
  tr_hal_uart_uninit_StopIgnore();
  tr_hal_uart_init_StopIgnore();
  Enter_Critical_Section_StopIgnore();
  Leave_Critical_Section_StopIgnore();

}

void test_zpal_uart_transmit_failure(void)
{
  tr_hal_gpio_set_mode_IgnoreAndReturn(TR_HAL_SUCCESS);
  tr_hal_gpio_set_direction_IgnoreAndReturn(TR_HAL_SUCCESS);
  tr_hal_gpio_set_pull_mode_IgnoreAndReturn(TR_HAL_SUCCESS);
  tr_hal_uart_uninit_IgnoreAndReturn(TR_HAL_SUCCESS);
  tr_hal_uart_init_IgnoreAndReturn(TR_HAL_SUCCESS);
  Enter_Critical_Section_Ignore();
  Leave_Critical_Section_Ignore();

  tr_hal_uart_set_power_mode_ExpectAndReturn(UART_0_ID, TR_HAL_POWER_MODE_0, TR_HAL_SUCCESS);
  zpal_uart_init(&default_config, &handle);

  const uint8_t some_data[50];

  tr_hal_uart_dma_tx_bytes_in_buffer_ExpectAndReturn(UART_0_ID, some_data, sizeof(some_data), TR_HAL_ERROR_DMA_NOT_ENABLED);
  zpal_status_t status = zpal_uart_transmit(handle, some_data, sizeof(some_data), zpal_uart_transmit_done);
  TEST_ASSERT_EQUAL(ZPAL_STATUS_FAIL, status);

  tr_hal_gpio_set_mode_StopIgnore();
  tr_hal_gpio_set_direction_StopIgnore();
  tr_hal_gpio_set_pull_mode_StopIgnore();
  tr_hal_uart_uninit_StopIgnore();
  tr_hal_uart_init_StopIgnore();
  Enter_Critical_Section_StopIgnore();
  Leave_Critical_Section_StopIgnore();
}

static bool tx_is_active = true;
static tr_hal_status_t tr_hal_uart_tx_active_cb(
  tr_hal_uart_id_t uart_id,
  bool* tx_active,
  int cmock_num_calls)
{
  (void)uart_id;
  (void)cmock_num_calls;

  *tx_active = tx_is_active? true:false;
  return TR_HAL_SUCCESS;
}

void test_zpal_uart_transmit_in_progress(void)
{
  tr_hal_gpio_set_mode_IgnoreAndReturn(TR_HAL_SUCCESS);
  tr_hal_gpio_set_direction_IgnoreAndReturn(TR_HAL_SUCCESS);
  tr_hal_gpio_set_pull_mode_IgnoreAndReturn(TR_HAL_SUCCESS);
  tr_hal_uart_uninit_IgnoreAndReturn(TR_HAL_SUCCESS);
  tr_hal_uart_init_IgnoreAndReturn(TR_HAL_SUCCESS);
  Enter_Critical_Section_Ignore();
  Leave_Critical_Section_Ignore();

  tr_hal_uart_tx_active_StubWithCallback(tr_hal_uart_tx_active_cb);

  tr_hal_uart_set_power_mode_ExpectAndReturn(UART_0_ID, TR_HAL_POWER_MODE_0, TR_HAL_SUCCESS);
  zpal_uart_init(&default_config, &handle);

  tx_is_active = true;
  bool retval = zpal_uart_transmit_in_progress(handle);
  TEST_ASSERT_TRUE(retval);

  tx_is_active = false;
  retval = zpal_uart_transmit_in_progress(handle);
  TEST_ASSERT_FALSE(retval);

  tr_hal_gpio_set_mode_StopIgnore();
  tr_hal_gpio_set_direction_StopIgnore();
  tr_hal_gpio_set_pull_mode_StopIgnore();
  tr_hal_uart_uninit_StopIgnore();
  tr_hal_uart_init_StopIgnore();
  Enter_Critical_Section_StopIgnore();
  Leave_Critical_Section_StopIgnore();
}


void test_zpal_uart_init_blocking(void)
{
  tr_hal_gpio_set_mode_IgnoreAndReturn(TR_HAL_SUCCESS);
  tr_hal_gpio_set_direction_IgnoreAndReturn(TR_HAL_SUCCESS);
  tr_hal_gpio_set_pull_mode_IgnoreAndReturn(TR_HAL_SUCCESS);
  tr_hal_uart_uninit_IgnoreAndReturn(TR_HAL_SUCCESS);
  tr_hal_uart_init_IgnoreAndReturn(TR_HAL_SUCCESS);
  Enter_Critical_Section_Ignore();
  Leave_Critical_Section_Ignore();

  default_config.flags = ZPAL_UART_CONFIG_FLAG_BLOCKING;

  tr_hal_uart_set_power_mode_ExpectAndReturn(UART_0_ID, TR_HAL_POWER_MODE_0, TR_HAL_SUCCESS);
  zpal_status_t status = zpal_uart_init(&default_config, &handle);
  TEST_ASSERT_EQUAL(ZPAL_STATUS_OK, status);
  TEST_ASSERT_NOT_NULL(handle);

  tr_hal_gpio_set_mode_StopIgnore();
  tr_hal_gpio_set_direction_StopIgnore();
  tr_hal_gpio_set_pull_mode_StopIgnore();
  tr_hal_uart_uninit_StopIgnore();
  tr_hal_uart_init_StopIgnore();
  Enter_Critical_Section_StopIgnore();
  Leave_Critical_Section_StopIgnore();
}

typedef struct
{
  zpal_uart_config_t config;
  zpal_uart_handle_t handle;
}
test_uart_t;

void test_zpal_uart_0_and_1_and_2(void)
{
  tr_hal_gpio_set_mode_IgnoreAndReturn(TR_HAL_SUCCESS);
  tr_hal_gpio_set_direction_IgnoreAndReturn(TR_HAL_SUCCESS);
  tr_hal_gpio_set_pull_mode_IgnoreAndReturn(TR_HAL_SUCCESS);
  tr_hal_uart_uninit_IgnoreAndReturn(TR_HAL_SUCCESS);
  tr_hal_uart_init_IgnoreAndReturn(TR_HAL_SUCCESS);
  Enter_Critical_Section_Ignore();
  Leave_Critical_Section_Ignore();

  test_uart_t uarts[3];
  for (size_t i = 0; i < 3; i++)
  {
    uarts[i].config = default_config;
    uarts[i].config.id = tr_uart_id[i];
    uarts[i].config.flags = ZPAL_UART_CONFIG_FLAG_BLOCKING;

    tr_hal_uart_set_power_mode_ExpectAndReturn(tr_uart_id[i], TR_HAL_POWER_MODE_0, TR_HAL_SUCCESS);
    zpal_uart_init(&uarts[i].config, &uarts[i].handle);


    tr_hal_uart_power_on_ExpectAndReturn(tr_uart_id[i], TR_HAL_SUCCESS);
    zpal_uart_enable(uarts[i].handle);
  }

  tr_hal_gpio_set_mode_StopIgnore();
  tr_hal_gpio_set_direction_StopIgnore();
  tr_hal_gpio_set_pull_mode_StopIgnore();
  tr_hal_uart_uninit_StopIgnore();
  tr_hal_uart_init_StopIgnore();
  Enter_Critical_Section_StopIgnore();
  Leave_Critical_Section_StopIgnore();
}
