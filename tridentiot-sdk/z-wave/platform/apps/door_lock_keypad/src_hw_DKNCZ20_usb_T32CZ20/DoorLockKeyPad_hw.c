/// ****************************************************************************
/// @file DoorLockKeypad_hw.c
///
/// @brief Door Lock Keypad Hardware setup for the DKNCZ20 board with UART on USB
///
///
/// SPDX-License-Identifier: LicenseRef-TridentMSLA
/// SPDX-FileCopyrightText: 2025 Trident IoT, LLC <https://www.tridentiot.com>
/// ***************************************************************************

#include <apps_hw.h>
#include <DebugPrint.h>
#include "CC_BinarySwitch.h"
#include "SizeOf.h"
#include "events.h"
#include <inttypes.h>
#ifdef TR_CLI_ENABLED
#include "tr_cli.h"
#endif
#include <zaf_event_distributor_soc.h>
#include "tr_board_DKNCZ20.h"
#include "zpal_uart.h"
#include "zpal_misc.h"
#include "zpal_misc_private.h"
#include "setup_common_cli.h"
#include "MfgTokens.h"
#include "zpal_uart_gpio.h"
#include "ZAF_Common_interface.h"
#include "tr_hal_gpio.h"
#include "CC_UserCode.h"
#include "cc_user_code_config.h"
#include "CC_UserCredential.h"
#include "cc_user_credential_validation.h"
#include "CC_DoorLock.h"

/*defines what gpios ids are used*/
const uint8_t PB_LEARN_MODE = (uint8_t)TR_BOARD_BTN_LEARN_MODE;

/*Defines if the gpio support low power mode or nots*/
const  low_power_wakeup_cfg_t PB_LEARN_MODE_LP    = LOW_POWER_WAKEUP_GPIO4;

/*Defines the on state for the gpios*/
const uint8_t PB_LEARN_MODE_ON  = 0;

static gpio_info_t m_gpio_info[] = {
  GPIO_INFO()
};

static const gpio_config_t m_gpio_config[] = {
  GPIO_CONFIG(PB_LEARN_MODE, PB_LEARN_MODE_LP, PB_LEARN_MODE_ON, EVENT_SYSTEM_LEARNMODE_TOGGLE, EVENT_SYSTEM_EMPTY, EVENT_SYSTEM_RESET, EVENT_SYSTEM_EMPTY)
};

#define BUTTONS_SIZE    sizeof_array(m_gpio_config)

/*Setup for debug and remote CLI uart*/

static const zpal_uart_config_ext_t ZPAL_UART_CONFIG_GPIO = {
  .txd_pin = TR_BOARD_UART0_TX,
  .rxd_pin = TR_BOARD_UART0_RX,
  .cts_pin = 0, // Not used.
  .rts_pin = 0, // Not used.
  .uart_wakeup = true
};

static zpal_uart_config_t UART_CONFIG =
{
  .id = ZPAL_UART1, // Use UART1 because UART2 is disabled when the Door Lock sleeps.
  .baud_rate = 115200,
  .data_bits = 8,
  .parity_bit = ZPAL_UART_NO_PARITY,
  .stop_bits = ZPAL_UART_STOP_BITS_1,
  .receive_callback = NULL,
  .ptr = &ZPAL_UART_CONFIG_GPIO,
  .flags = 0
};

zpal_debug_config_t debug_port_cfg = &UART_CONFIG;


#ifdef TR_CLI_ENABLED

// Hard-coded user credential data, to validate the local handle
// state triggering event.
static uint8_t user_credential_data[] = { 0x33, 0x34, 0x39, 0x34 };
static u3c_credential_t credential = {
  .metadata = {
    .uuid = 1,
    .slot = 1,
    .modifier_node_id = 0,
    .length = sizeof(user_credential_data),
    .modifier_type = MODIFIER_TYPE_LOCALLY,
    .type = CREDENTIAL_TYPE_PIN_CODE
  },
  .data = (uint8_t *) &user_credential_data
};
static u3c_event_data_validate_t user_credential_event_validate_data = {
  .credential = &credential,
  .is_unlocked = false
};

// Battery command implementation
static int cli_cmd_app_battery(int  argc, char *argv[]);
// User Credential command implementation
static int user_credential_handle_set(int  argc, char *argv[]);


// User credential commands cli table. to access them run:
// application user_credential <command>
TR_CLI_COMMAND_TABLE(user_credentials_specific_sub_commands) =
{
  { "handle_state",           user_credential_handle_set,   "handle_state <1/0>. <lock/unlock> door handle with User Code"},
  TR_CLI_COMMAND_TABLE_END
};

// Adding battery, user code and user credential command groups to the application specific table
TR_CLI_COMMAND_TABLE(app_specific_commands) =
{
  { "battery",         cli_cmd_app_battery, "Triggers a battery report"},
  { "user_credential", TR_CLI_SUB_COMMANDS, TR_CLI_SUB_COMMAND_TABLE(user_credentials_specific_sub_commands) },
  TR_CLI_COMMAND_TABLE_END
};

// Battery command implementation
static int cli_cmd_app_battery(__attribute__((unused)) int  argc,__attribute__((unused))  char *argv[])
{
  zaf_event_distributor_enqueue_app_event_from_isr(EVENT_APP_BATTERY_REPORT);
  return 0;
}


static int user_credential_handle_set(int  argc, char *argv[])
{
  uint64_t status = 0;

  if(argc > 0)
  {
    status = tr_dec_or_hex_string_to_int(argv[1]);
    door_lock_mode_t door_operation;
    if( status != 0 && status != 1)
    {
      tr_cli_common_printf("Status detected not 0 nor 1, set <0/1> to <unlock/lock> door handle\n");
      return 0;
    }
    if(status == 1)
    {
      door_operation = DOOR_MODE_UNSECURE;
    }
    else
    {
      door_operation = DOOR_MODE_SECURED;
    }

    user_credential_event_validate_data.is_unlocked = door_lock_hw_bolt_is_unlocked();
    void *cc_data = &user_credential_event_validate_data;
    zaf_event_distributor_enqueue_cc_event(COMMAND_CLASS_USER_CREDENTIAL, CC_USER_CREDENTIAL_EVENT_VALIDATE, cc_data);
    // Check if the credential is correct before unlocking the DoorLock
    // In this case is always correct, however in the future we may allow for non-correct credentials
    if(validate_new_credential_data(&credential, NULL))
    {
      cc_door_lock_mode_hw_change(door_operation);
    }
    return 0;
  }
  tr_cli_common_printf("Handle status not included, set <1/0> to <lock/unlock> door handle\n");
  return 0;
}


#endif // #ifdef TR_CLI_ENABLED

void app_hw_init(void)
{
  apps_hw_init(m_gpio_config, m_gpio_info, BUTTONS_SIZE);

#ifdef TR_CLI_ENABLED
  // Enable wake up from UART0 upon reception of some data.
  Lpm_Enable_Low_Power_Wakeup(LOW_POWER_WAKEUP_UART0_DATA);

  setup_cli(&UART_CONFIG);
#endif
}

uint8_t board_indicator_gpio_get( void)
{
  return TR_BOARD_LED_LEARN_MODE;
}

uint8_t board_indicator_led_off_gpio_state(void)
{
  return TR_HAL_GPIO_LEVEL_HIGH;
}
