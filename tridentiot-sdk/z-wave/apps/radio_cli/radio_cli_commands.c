/// ***************************************************************************
/// SPDX-License-Identifier: LicenseRef-TridentMSLA
/// SPDX-FileCopyrightText: 2024 Trident IoT, LLC <https://www.tridentiot.com>
/// ***************************************************************************

/**
 * @file radio_cli_commands.c
 * @brief Z-Wave radio test CLI command implementation
 */

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

//#define DEBUGPRINT // NOSONAR
#include "DebugPrint.h"
#include "DebugPrintConfig.h"

#include <embedded_cli.h>
#include <zpal_radio_utils.h>
#include "cli_uart_interface.h"
#include "radio_cli_app.h"
#include "radio_cli_commands.h"

/****************************************************************************/
/*                               PRIVATE DATA                               */
/****************************************************************************/

#define MAX_CHANNELS 4
// Frame payload must be at least 9 bytes
#define MIN_PAYLOAD_LENGTH 9

static uint8_t payloadbuffer[200];

radio_cli_tx_frame_config_t frame = {.payload_buffer = payloadbuffer,
                                     .payload_length = 0,
                                     .channel = 0,
                                     .lbt = true,
                                     .delay = DEFAULT_TX_DELAY_MS,
                                     .power = 0,};

// Function prototypes for the CLI commands
void cli_zw_init(EmbeddedCli *cli, char *args, void *context);
void cli_zw_region_set(EmbeddedCli *cli, char *args, void *context);
void cli_zw_region_list(EmbeddedCli *cli, char *args, void *context);
void cli_zw_homeid_set(EmbeddedCli *cli, char *args, void *context);
void cli_zw_nodeid_set(EmbeddedCli *cli, char *args, void *context);
void cli_zw_payload_set(EmbeddedCli *cli, char *args, void *context);
void cli_zw_tx_channel_set(EmbeddedCli *cli, char *args, void *context);
void cli_zw_tx_power_set(EmbeddedCli *cli, char *args, void *context);
void cli_zw_tx_power_index_list(EmbeddedCli *cli, char *args, void *context);
void cli_zw_classic_tx_power_enable(EmbeddedCli *cli, char *args, void *context);
void cli_zw_classic_tx_power_set(EmbeddedCli *cli, char *args, void *context);
void cli_zw_classic_tx_power_adjust_set(EmbeddedCli *cli, char *args, void *context);
void cli_zw_classic_tx_power_attenuation_set(EmbeddedCli *cli, char *args, void *context);
void cli_zw_tx(EmbeddedCli *cli, char *args, void *context);
void cli_zw_status_get(EmbeddedCli *cli, char *args, void *context);
void get_version_handler(EmbeddedCli *cli, char *args, void *context);
void cli_gpio_pulse_handler(EmbeddedCli *cli, char *args, void *context);
void cli_gpio_output_handler(EmbeddedCli *cli, char *args, void *context);
void cli_gpio_set_handler(EmbeddedCli *cli, char *args, void *context);
void cli_gpio_input_handler(EmbeddedCli *cli, char *args, void *context);
void cli_gpio_get_handler(EmbeddedCli *cli, char *args, void *context);
void cli_zw_rx_set(EmbeddedCli *cli, char *args, void *context);
void cli_zw_tx_set_lbt(EmbeddedCli *cli, char *args, void *context);
void cli_stats_get(EmbeddedCli *cli, char *args, void *context);
void cli_stats_clear(EmbeddedCli *cli, char *args, void *context);
void cli_zw_rx_channel_set(EmbeddedCli *cli, char *args, void *context);
void cli_zw_tx_delay_set(EmbeddedCli *cli, char *args, void *context);
void cli_zw_tx_config_set(EmbeddedCli *cli, char *args, void *context);
void cli_zw_tx_max_power_set(EmbeddedCli *cli, char *args, void *context);

void cli_zw_radio_tx_continues_set(EmbeddedCli *cli, char *args, void *context);
void cli_zw_radio_rf_debug_set(EmbeddedCli *cli, char *args, void *context);
void cli_zw_radio_rf_debug_reg_setting_list(EmbeddedCli *cli, char *args, void *context);
void cli_zw_radio_rssi_get(EmbeddedCli *cli, char *args, void *context);
void cli_zw_radio_rssi_get_all(EmbeddedCli *cli, char *args, void *context);
void cli_zw_radio_rssi_config_set(EmbeddedCli *cli, char *args, void *context);
void cli_zw_reset(EmbeddedCli *cli, char *args, void *context);
void cli_zw_script_entry(EmbeddedCli *cli, char *args, void *context);
void cli_zw_script_run(EmbeddedCli *cli, char *args, void *context);
void cli_zw_wait(EmbeddedCli *cli, char *args, void *context);
void cli_zw_radio_timestamp(EmbeddedCli *cli, char *args, void *context);
void cli_zw_dump(EmbeddedCli *cli, char *args, void *context);
void cli_zw_cal_xtal(EmbeddedCli *cli, char *args, void *context);

// List of CLI commands
CliCommandBinding cli_command_list[] =
{
  {"zw-init", "zw-init - Initialize the Z-Wave PHY layer", false, NULL, cli_zw_init},
  {"zw-region-set", "zw-region-set <region> - Set the desired Z-Wave region (0-103)", true, NULL, cli_zw_region_set},
  {"zw-region-list", "zw-region-list - Lists current region and all supported regions", false, NULL, cli_zw_region_list},
  {"zw-homeid-set", "zw-homeid-set <homeID> - Set the desired Z-Wave HomeID in hexadecimal", true, NULL, cli_zw_homeid_set},
  {"zw-nodeid-set", "zw-nodeid-set <nodeID> - Set the desired Z-Wave nodeID in decimal", true, NULL, cli_zw_nodeid_set},
  {"zw-tx-payload-set", "zw-tx-payload-set <b1> <b2> .. <bn> - Set the frame payload in hexadecimal bytes, if less than 9 bytes then uses default payloads", true, NULL, cli_zw_payload_set},
  {"zw-tx-channel-set", "zw-tx-channel-set <channel> - Set transmit channel (0-3 according to region)", true, NULL, cli_zw_tx_channel_set},
  {"zw-tx-power-set", "zw-tx-power-set <power> [powerindex] - Set transmit power in dBm (-20-+20), if optional powerindex is specified then this will be used instead", true, NULL, cli_zw_tx_power_set},
  {"zw-tx-power-index-list", "zw-tx-power-index-list - list dynamic Tx power to radio power_index conversion table", false, NULL, cli_zw_tx_power_index_list},
  {"zw-classic-tx-power-enable", "zw-classic-tx-power-enable <on/off> - Set usage of Z-Wave Classic Tx Power setting on/off when transmitting on classic channels", true, NULL, cli_zw_classic_tx_power_enable},
  {"zw-classic-tx-power-set", "zw-classic-tx-power-set <power> - Set classic transmit power in dBm (-20-+20)", true, NULL, cli_zw_classic_tx_power_set},
  {"zw-classic-tx-power-adjust-set", "zw-tx-power-adjust-set <power_adjust> - Set transmit power adjust in dBm (-5-+5)", true, NULL, cli_zw_classic_tx_power_adjust_set},
  {"zw-classic-tx-power-attenuation-set", "zw-classic-tx-power-attenuation-set <attenuation> - Set classic transmit power attenuation in dBm (0-20)", true, NULL, cli_zw_classic_tx_power_attenuation_set},
  {"zw-tx-lbt-set", "zw-tx-lbt-set <level> - Set lbt level in dBm (-127-0), 0 equals lbt is turned off", true, NULL, cli_zw_tx_set_lbt},
  {"zw-stats-get", "zw-stats-get [0/1] - Get network statistics and optionally extended statistics", true, NULL, cli_stats_get},
  {"zw-stats-clear", "zw-stats-clear [0/1] - Clear network statistics or optionally only clear tx_time statistics", true, NULL, cli_stats_clear},
  {"zw-tx-delay-set", "zw-tx-delay-set - Set delay inbetween repeated frame transmits (2-65535) ms", true, NULL, cli_zw_tx_delay_set},
  {"zw-tx-config-set", "zw-tx-config-set <option> <on/of>- Set tx option on off, supported options: fail-crc", true, NULL, cli_zw_tx_config_set},
  {"zw-rx-channel-set",  "zw-rx-channel-set <on/off> <channel> - Set on/off to use fixed Rx channel (0-3)", true, NULL, cli_zw_rx_channel_set},
  {"zw-tx-max-power-set", "zw-tx-max-power-set <14/20> - Set Tx max power (14/20) dBm", true, NULL, cli_zw_tx_max_power_set},
  {"zw-radio-tx-continues-set", "zw-radio-tx-continues_set <on/off> [wave_type] - Set transmit continuous signal on or off, default unmodulated signal optionally set wave_type to\n\t\t\t0 - unmodulated signal\n\t\t\t1 - modulated signal using fixed byte pattern\n\t\t\t2 - modulated signal using fixed byte pattern with whitening", true, NULL, cli_zw_radio_tx_continues_set},
  {"zw-radio-rf-debug-set", "zw-radio-rf-debug-set <on/off> - Set RF state gpio out on/off", true, NULL, cli_zw_radio_rf_debug_set},
  {"zw-radio-rf-debug-reg-setting-list", "zw-radio-rf-debug-reg-setting-list [0/1] - List selected radio reg settings and optionaly all radio reg settings", true, NULL, cli_zw_radio_rf_debug_reg_setting_list},
  {"zw-radio-rssi-get",  "zw-radio-rssi-get <channel> [count [delay]] - Get radio RSSI on channel, optionally count times with optionally delay ms inbetween (defaults to 1000ms)", true, NULL, cli_zw_radio_rssi_get},
  {"zw-radio-rssi-get-all",  "zw-radio-rssi-get-all [count [delay]] - Get radio RSSI on all channels in current region, optionally count times with optionally delay ms inbetween (defaults to 1000ms)", true, NULL, cli_zw_radio_rssi_get_all},
  {"zw-radio-rssi-config-set",  "zw-radio-rssi-config-set <sample_freq> <average_count> - Set radio RSSI sample frequency sample_freq and average_count samples\n\t\t\t\tused for generating RSSI average received when doing rssi get. Valid only when doing Rx channel scanning", true, NULL, cli_zw_radio_rssi_config_set},
  {"tx", "tx <repeat> - Send <repeat> frames", true, NULL, cli_zw_tx},
  {"rx", "rx <on/off> - Set the receiver on or off", true, NULL, cli_zw_rx_set},
  {"timestamp", "timestamp <on/off> - enable/disable timestamp on Rx and Tx printout - Default is no timestamp", true, NULL, cli_zw_radio_timestamp},
  {"reset", "reset - reset radio_cli firmware", false, NULL, cli_zw_reset},
  {"script", "script <command>\n\t\tstart [1-5] - start defining active or specified script entry,\n\t\tstop - stop running script,\n\t\tautoon/autooff [1-5] - enable/disable active or specified script run on startup,\n\t\tlist [1-5] - list all or specified script,\n\t\tclear [1-5] - clear active or specified script", true, NULL, cli_zw_script_entry},
  {"run", "run [1-5] - run active script or specified script", true, NULL, cli_zw_script_run},
  {"wait", "wait <time> - wait time milliseconds before doing scriptline transitioning - valid range 1-86400000 ms", true, NULL, cli_zw_wait},
  {"status",  "status - Get status", false, NULL, cli_zw_status_get},
  {"dump", "dump <ft/mp> - Dump flash sector", true, NULL, cli_zw_dump},
  {"cal-xtal", "cal-xtal <try/store> <cal value> - Set crystal calibration to value (0-63)", true, NULL, cli_zw_cal_xtal},
  {"gpio-pulse", "gpio-pulse <pin_number> <width-in-ms> - Create a GPIO pulse", true, NULL, cli_gpio_pulse_handler},
  {"gpio-output", "gpio-output <pin_number> <high/low> - Set GPIO pin output level, default low", true, NULL, cli_gpio_output_handler},
  {"gpio-input", "gpio-input <pin_number> - Read GPIO pin input level", true, NULL, cli_gpio_input_handler},
  {"gpio-set", "gpio-set <pin_number> - Set GPIO pin to high/low", true, NULL, cli_gpio_set_handler},
  {"gpio-get", "gpio-get <pin_number> - Get GPIO pin level", true, NULL, cli_gpio_get_handler},
  {"version",  "version - Get version", false, NULL, get_version_handler},
  {"", "", false, NULL}
};

/****************************************************************************/
/*                                 FUNCTIONS                                */
/****************************************************************************/

/*
 * Validate a range and print an error if out of range
 */
bool validate_integer_range(int value, int min, int max, uint8_t parameter)
{
  if ((value > max) || (value < min ))
  {
    if (parameter)
    {
      cli_uart_printf("Parameter %u must be in the range (%i..%i)\n", parameter, min, max);
    }
    return false;
  }
  return true;
}

/*
 * Check the number of arguments for a command and print error message
 */
bool check_argument_count(uint8_t count, uint8_t required, char* parms)
{
  if (count == required)
  {
    return true;
  }
  else if (count < required)
  {
    cli_uart_printf("** Missing argument(s) %s\n", parms);
  }
  else
  {
    cli_uart_print("** Wrong number of arguments\n");
  }
  return false;
}

/*
 * Parse an signed 32-bit base 10 integer argument from the CLI input
 */
static bool parse_arg_int(void *args, int arg_index, int *out_value)
{
  const char *arg = embeddedCliGetToken(args, arg_index);
  if (arg == NULL || *arg == '\0') {
    cli_uart_printf("** Argument %d is missing or empty\n", arg_index);
    return false;
  }

  char *endptr;
  long temp = strtol(arg, &endptr, 10);
  if (*endptr != '\0') {
    cli_uart_printf("** Conversion error at argument %d, non-numeric (base10) characters found: %s\n", arg_index, endptr);
    return false;
  }

  *out_value = (int)temp;
  return true;
}

/*
 * Parse an unsigned 32-bit integer argument in either base 10 or 16 from the CLI input
 */
static bool parse_arg_uint32_base(void *args, int arg_index, uint32_t *out_value, int base)
{
  const char *arg = embeddedCliGetToken(args, arg_index);
  if (arg == NULL || *arg == '\0') {
    cli_uart_printf("** Argument %d is missing or empty\n", arg_index);
    return false;
  }

  char *endptr;
  uint64_t temp = strtoul(arg, &endptr, base);
  if (*endptr != '\0') {
    cli_uart_printf("** Conversion error at argument %d, non-numeric (base%d) characters found: %s\n", arg_index, base, endptr);
    return false;
  }

  *out_value = (uint32_t)temp;
  return true;
}

void cli_zw_status_get(EmbeddedCli *cli, char *args, void *context)
{
  cli_radio_status_get(&frame);
  cli_radio_script_state_transition_event();
}

/*
 * Handler for version command
 */
void get_version_handler(EmbeddedCli *cli, char *args, void *context)
{
  cli_radio_version_print();
  cli_radio_script_state_transition_event();
}

__attribute__((weak)) void radio_cli_hw_gpio_pulse(
  __attribute__((unused)) uint32_t pin_number,
  __attribute__((unused)) uint32_t width)
{
  cli_uart_printf("** gpio-pulse is not implemented for the current platform/hardware.\n");
}

__attribute__((weak)) void radio_cli_hw_gpio_output(
  __attribute__((unused)) uint32_t pin_number,
  __attribute__((unused)) bool level)
{
  cli_uart_printf("** gpio-output is not implemented for the current platform/hardware.\n");
}

__attribute__((weak)) void radio_cli_hw_gpio_set(
  __attribute__((unused)) uint32_t pin_number,
  __attribute__((unused)) bool level)
{
  cli_uart_printf("** gpio-set is not implemented for the current platform/hardware.\n");
}

__attribute__((weak)) bool radio_cli_hw_gpio_get(
  __attribute__((unused)) uint32_t pin_number,
  bool *level,
  bool *isOutput)
{
  cli_uart_printf("** gpio-get is not implemented for the current platform/hardware.\n");
  *level = false;
  *isOutput = false;
  return false;
}

__attribute__((weak)) bool radio_cli_hw_gpio_input(
  __attribute__((unused)) uint32_t pin_number,
  __attribute__((unused)) bool pullup)
{
  cli_uart_printf("** gpio-input is not implemented for the current platform/hardware.\n");
  return false;
}

void cli_gpio_pulse_handler(EmbeddedCli *cli, char *args, void *context)
{
  uint16_t arg_count = embeddedCliGetTokenCount(args);

  if (2 != arg_count)
  {
    cli_uart_printf("** Incorrect number of arguments.\n");
    return;
  }

  uint32_t pin_number = 0;
  uint32_t pulse_width = 0;

  if (!parse_arg_uint32_base(args, 1, &pin_number, 10) ||
      !parse_arg_uint32_base(args, 2, &pulse_width, 10)) {
    return;
  }

  cli_uart_printf("Pin number: %u\n", pin_number);
  cli_uart_printf("Pulse width: %u\n", pulse_width);

  radio_cli_hw_gpio_pulse(pin_number, pulse_width);

  cli_radio_script_state_transition_event();
}

void cli_gpio_output_handler(EmbeddedCli *cli, char *args, void *context)
{
  uint16_t arg_count = embeddedCliGetTokenCount(args);

  if (arg_count < 1)
  {
    cli_uart_printf("** Incorrect number of arguments.\n");
    return;
  }

  // Process the pin number argument
  uint32_t pin_number = 0;
  if (!parse_arg_uint32_base(args, 1, &pin_number, 10)) {
    return;
  }

  // Process the value argument (optional, default to "low")
  bool level = false;
  if (arg_count >= 2)
  {
    const char * arg2 = embeddedCliGetToken(args, 2);
    if (strcmp(arg2, "high") == 0)
    {
      level = true; // High level
    }
    else if (strcmp(arg2, "low") == 0)
    {
      level = false; // Low level
    }
    else
    {
      cli_uart_printf("** Invalid value '%s', expected 'high' or 'low'.\n", arg2);
      return;
    }
  }

  radio_cli_hw_gpio_output(pin_number, level);

  cli_radio_script_state_transition_event();
}

void cli_gpio_set_handler(EmbeddedCli *cli, char *args, void *context)
{
  uint16_t arg_count = embeddedCliGetTokenCount(args);

  if (arg_count < 2)
  {
    cli_uart_printf("** Incorrect number of arguments.\n");
    return;
  }

  // Process the pin number argument
  uint32_t pin_number = 0;
  if (!parse_arg_uint32_base(args, 1, &pin_number, 10)) {
    return;
  }

  // Process the value argument
  bool level = false;
  const char * arg2 = embeddedCliGetToken(args, 2);
  if (strcmp(arg2, "high") == 0)
  {
    level = true; // High level
  }
  else if (strcmp(arg2, "low") == 0)
  {
    level = false; // Low level
  }
  else
  {
    cli_uart_printf("** Invalid value '%s', expected 'high' or 'low'.\n", arg2);
    return;
  }

  radio_cli_hw_gpio_set(pin_number, level);

  cli_radio_script_state_transition_event();
}

void cli_gpio_input_handler(EmbeddedCli *cli, char *args, void *context)
{
  uint16_t arg_count = embeddedCliGetTokenCount(args);

  if (arg_count < 1) {
    cli_uart_printf("** Incorrect number of arguments.\n");
    return;
  }

  // Process the pin number argument
  uint32_t pin_number = 0;
  if (!parse_arg_uint32_base(args, 1, &pin_number, 10)) {
    return;
  }

  // Default pull-up is ON
  bool pullup = true;

  // Optional: parse pull-up argument
  if (arg_count >= 2) {
    const char *arg2 = embeddedCliGetToken(args, 2);
    if (strncmp(arg2, "pull-up=", 8) == 0) {
      const char *value = arg2 + 8;
      if (strcmp(value, "on") == 0) {
        pullup = true;
      } else if (strcmp(value, "off") == 0) {
        pullup = false;
      } else {
        cli_uart_printf("** Invalid pull-up value '%s', expected 'on' or 'off'.\n", value);
        cli_radio_script_state_transition_event();
        return;
      }
    } else {
      cli_uart_printf("** Invalid argument '%s', expected 'pull-up=on' or 'pull-up=off'.\n", arg2);
      cli_radio_script_state_transition_event();
      return;
    }
  }

  bool ok = radio_cli_hw_gpio_input(pin_number, pullup);
  if (!ok) {
    cli_uart_printf("Failed to set GPIO %u input\n", pin_number);
  }

  cli_radio_script_state_transition_event();
}

void cli_gpio_get_handler(EmbeddedCli *cli, char *args, void *context)
{
  uint16_t arg_count = embeddedCliGetTokenCount(args);

  if (arg_count < 1)
  {
    cli_uart_printf("** Incorrect number of arguments.\n");
    return;
  }

  // Process the pin number argument
  uint32_t pin_number = 0;
  if (!parse_arg_uint32_base(args, 1, &pin_number, 10)) {
    return;
  }

  bool pin_status = false;
  bool is_output = false;
  bool ok = radio_cli_hw_gpio_get(pin_number, &pin_status, &is_output);

  if (ok) {
    cli_uart_printf("GPIO %u: %s %s\n", pin_number, pin_status ? "high" : "low", is_output ? "output" : "input");
  } else {
    cli_uart_printf("Failed to read GPIO %u\n", pin_number);
  }

  cli_radio_script_state_transition_event();
}

bool radio_is_not_initialized(void)
{
  bool status = false;
  if (!cli_radio_initialized())
  {
    cli_radio_script_state_transition_event();
    status = true;
  }
  return status;
}

/*
 * Handler for Z-Wave init function
 */
void cli_zw_init(EmbeddedCli *cli, char *args, void *context)
{
  if (REGION_UNDEFINED == cli_radio_region_current_get())
  {
    cli_uart_printf("** Undefined region, use %s to set the region\n", cli_command_list[1].name);
  }
  else
  {
    cli_radio_setup(cli_radio_region_current_get());
    cli_uart_printf("Z-Wave Radio initialized to Region %s (%i), Tx channel %i\n", cli_radio_region_current_description_get(), cli_radio_region_current_get(), frame.channel);
  }
  cli_radio_script_state_transition_event();
}

/*
 * Handler for Z-Wave z-wave region list function
 */
void cli_zw_region_list(EmbeddedCli *cli, char *args, void *context)
{
  cli_radio_region_list(cli_radio_region_current_get());
  cli_radio_script_state_transition_event();
}

/*
 * Handler for Z-Wave region set function
 */
void cli_zw_region_set(EmbeddedCli *cli, char *args, void *context)
{
  uint8_t count = embeddedCliGetTokenCount(args);

  if (check_argument_count(count, 1, "Region"))
  {
    const char * arg = embeddedCliGetToken(args, 1); // args are counted from 1 (not from 0)
    zpal_radio_region_t new_region = atoi(arg);

    if (false == cli_radio_change_region(new_region))
    {
      cli_uart_print("** Changing region failed\n");
    }
    else
    {
      uint8_t region_channel_count = cli_radio_region_channel_count_get();
      frame.channel = (region_channel_count - 1 < frame.channel) ? 0 : frame.channel;
    }
    cli_uart_printf("Region %i, Tx channel %i\n", cli_radio_region_current_get(), frame.channel);
  }
  else
  {
    cli_radio_region_list(cli_radio_region_current_get());
  }
  cli_radio_script_state_transition_event();
}

/*
 * Handler for Z-Wave homeID set function
 */
void cli_zw_homeid_set(EmbeddedCli *cli, char *args, void *context)
{
  uint32_t homeid;

  uint8_t count = embeddedCliGetTokenCount(args);

  if (check_argument_count(count, 1, "HomeID"))
  {
    if (!parse_arg_uint32_base(args, 1, &homeid, 16))
    {
      cli_radio_script_state_transition_event();
      return;
    }
    cli_uart_printf("Setting homeID to %08X\n", homeid);
    cli_radio_set_homeid(homeid);
  }
  cli_radio_script_state_transition_event();
}

/*
 * Handler for Z-Wave nodeID set function
 */
void cli_zw_nodeid_set(EmbeddedCli *cli, char *args, void *context)
{
  uint8_t count = embeddedCliGetTokenCount(args);

  if (check_argument_count(count, 1, "nodeID"))
  {
    uint32_t node_id;
    if (!parse_arg_uint32_base(args, 1, &node_id, 10))
    {
      cli_radio_script_state_transition_event();
      return;
    }
    if (validate_integer_range(node_id, 0, 239, 0) || validate_integer_range(node_id, 256, 1024, 0))
    {
      cli_uart_printf("Setting nodeID to %u\n", node_id);
      cli_radio_set_nodeid((uint16_t)node_id);
    }
    else
    {
      cli_uart_print("NodeID must be in the range (0..239) or (256..1024)\n");
    }
  }
  cli_radio_script_state_transition_event();
}

/*
 * Handler for Z-Wave Tx payload set function
 */
void cli_zw_payload_set(EmbeddedCli *cli, char *args, void *context)
{
  uint8_t count = embeddedCliGetTokenCount(args);

  frame.payload_length = count;

  if (count >= MIN_PAYLOAD_LENGTH)
  {
    int i = 0;
    uint8_t tmp_buffer[200];
    for (i = 1; i <= count; i++)
    {
      uint32_t tmp_value;
      if (!parse_arg_uint32_base(args, i, &tmp_value, 16))
      {
        cli_radio_script_state_transition_event();
        return;
      }
      if (tmp_value > 0xFF)
      {
        cli_uart_printf("** Payload byte %u value %u out of range (00..FF)\n", i, tmp_value);
        cli_radio_script_state_transition_event();
        return;
      }
      tmp_buffer[i-1] = (uint8_t)tmp_value;
    }
    cli_uart_print("Setting payload ");
    for (i = 0; i < count; i++)
    {
      frame.payload_buffer[i] = tmp_buffer[i];
      cli_uart_printf("%02x",frame.payload_buffer[i-1]);
    }
    cli_uart_printf("\n");
  }
  else
  {
    cli_uart_printf("** Payload must be at least %u bytes\n", MIN_PAYLOAD_LENGTH);
    cli_uart_printf("** Using default payload\n");
    cli_radio_set_payload_default(&frame);
  }
  cli_radio_script_state_transition_event();
}

/**
 * Handler for setting Tx max power command
 */
void cli_zw_tx_max_power_set(EmbeddedCli *cli, char *args, void *context)
{
  uint8_t max_tx_power;

  uint8_t count = embeddedCliGetTokenCount(args);

  if (check_argument_count(count, 1, "14/20"))
  {
    const char * arg = embeddedCliGetToken(args, 1); // args are counted from 1 (not from 0)
    max_tx_power = atoi(arg);

    if ((max_tx_power == 14) || (max_tx_power == 20))
    {
      cli_uart_printf("Setting Tx max power to %udBm\n", max_tx_power);
      cli_radio_set_tx_max_power_20dbm((max_tx_power == 20));
      cli_uart_printf("NOTE: The allowed Max Tx power depends of the board layout so, make sure that the value set is supported by this board.\n");
    }
    else
    {
      cli_uart_print("Tx max power must 14 or 20\n");
    }
  }
  cli_radio_script_state_transition_event();
}

/*
 * Callback function for Z-Wave transmit command
 */
void cli_zw_tx_complete(uint16_t success, uint16_t failed, uint16_t failed_lbt)
{
  cli_uart_printf("Transmit complete, %u success, %u failed, %u lbt_failed\n", success, failed, failed_lbt);
  cli_radio_script_state_transition_event();
}

/*
 * Handler for Z-Wave transmit function
 */
void cli_zw_tx(EmbeddedCli *cli, char *args, void *context)
{
  if (REGION_UNDEFINED == cli_radio_region_current_get())
  {
    cli_uart_printf("** Undefined region, use %s to set the region\n", cli_command_list[1].name);
    cli_radio_script_state_transition_event();
    return;
  }

  uint8_t count = embeddedCliGetTokenCount(args);

  if (count == 1)
  {
    if (0 == frame.payload_length)
    {
      cli_uart_print("** No payload set\n");
    }
    else if (MAX_CHANNELS == frame.channel)
    {
      cli_uart_print("** No channel set\n");
    }
    else
    {
      uint32_t tmp_repeat;
      if (!parse_arg_uint32_base(args, 1, &tmp_repeat, 10))
      {
        cli_radio_script_state_transition_event();
        return;
      }
      frame.frame_repeat = tmp_repeat;
      frame.channel = frame.channel;
      frame.tx_callback = cli_zw_tx_complete;
      cli_radio_transmit_frame(&frame);
    }
  }
  else
  {
    cli_uart_print("** Invalid number of arguments\n");
  }
  cli_radio_script_state_transition_event();
}

/*
 * Handler for Z-Wave tx channel set function
 */
void cli_zw_tx_channel_set(EmbeddedCli *cli, char *args, void *context)
{
  if (radio_is_not_initialized())
  {
    return;
  }

  uint8_t count = embeddedCliGetTokenCount(args);

  if (check_argument_count(count, 1, "channel"))
  {
    uint32_t tmp_channel;

    if (!parse_arg_uint32_base(args, 1, &tmp_channel, 10))
    {
      cli_radio_script_state_transition_event();
      return;
    }

    uint8_t region_channel_count = cli_radio_region_channel_count_get();
    if (0 == region_channel_count)
    {
      // If radio not initialized - we will only allow Tx channel to become 0
      region_channel_count++;
    }
    if (validate_integer_range(tmp_channel, 0, region_channel_count - 1, 1))
    {
      frame.channel = tmp_channel;
      cli_uart_printf("Tx channel %i\n", frame.channel);
      cli_uart_printf("** Using default payload\n");
      cli_radio_set_payload_default(&frame);
    }
    else
    {
      cli_uart_printf("Current Tx channel %i\n", frame.channel);
    }
  }
  cli_radio_script_state_transition_event();
}

bool validate_radio_power_index(uint8_t power_index)
{
  bool valid = false;

  if (cli_radio_get_tx_max_power_20dbm())
  {
    if ((power_index < 201) && validate_integer_range(power_index, 76, 127, 2))
    {
      valid = true;
    }
    else if (validate_integer_range(power_index, 201, 255, 2))
    {
      valid = true;
    }
  }
  else
  {
    if ((power_index < 143) && validate_integer_range(power_index, 15, 63, 2))
    {
      valid = true;
    }
    else if (validate_integer_range(power_index, 143, 191, 2))
    {
      valid = true;
    }
  }
  return valid;
}

static void update_tx_power(char *args, uint8_t count)
{
  int tmp_power;
  if (!parse_arg_int(args, 1, &tmp_power))
  {
    return;
  }

  if (validate_integer_range(tmp_power, cli_radio_get_tx_min_power(frame.channel), cli_radio_get_tx_max_power(frame.channel), 0))
  {
    if (2 == count)
    {
      uint32_t power_index;
      if (!parse_arg_uint32_base(args, 2, &power_index, 10))
      {
        return;
      }
      else if (power_index > 255)
      {
        cli_uart_printf("** Power index out of range %d\n", power_index);
        return;
      }
      bool valid = validate_radio_power_index((uint8_t)power_index);
      if (valid && cli_radio_tx_power_index_set(frame.channel, frame.power, (uint8_t)power_index))
      {
        cli_uart_printf("Tx Power on channel %d, set to %idBm, power_index %d\n", frame.channel, tmp_power, power_index);
      }
      else
      {
        cli_uart_printf("** Power setting %idBm and power_index %d not valid for this board\n", tmp_power, power_index);
        return;
      }
    }
    else
    {
      cli_uart_printf("Tx Power on channel %d, set to %idBm\n", frame.channel, tmp_power);
    }
    frame.power = (int8_t)tmp_power;
  }
  else
  {
    if ((tmp_power > 14) && (!cli_radio_get_tx_max_power_20dbm()))
    {
      cli_uart_printf("Tx power is above this boards allowed Max Tx power\n");
      cli_uart_printf("The allowed Max Tx power can be set with the zw-tx-max-power-set command\n");
    }
  }
}

/*
 * Handler for Z-Wave tx power set function
 */
void cli_zw_tx_power_set(EmbeddedCli *cli, char *args, void *context)
{
  if (radio_is_not_initialized())
  {
    return;
  }

  uint8_t count = embeddedCliGetTokenCount(args);

  if ((count >= 1) && (count < 3))
  {
    update_tx_power(args, count);
  }
  else
  {
    check_argument_count(count, 1, "power");
  }
  cli_radio_script_state_transition_event();
}

void cli_zw_tx_power_index_list(EmbeddedCli *cli, char *args, void *context)
{
  if (radio_is_not_initialized())
  {
    return;
  }

  cli_radio_tx_power_index_list(frame.channel);
  cli_radio_script_state_transition_event();
}

void cli_zw_classic_tx_power_enable(EmbeddedCli *cli, char *args, void *context)
{
  uint8_t count = embeddedCliGetTokenCount(args);
  if (check_argument_count(count, 1, "on/off"))
  {
    bool classic_tx_power_enable = false;
    const char * arg = embeddedCliGetToken(args, 1); // args are counted from 1 (not from 0)
    if (!strcmp(arg, "on"))
    {
      classic_tx_power_enable = true;
    }
    else if (!strcmp(arg, "off"))
    {
      classic_tx_power_enable = false;
    }
    else
    {
      cli_uart_print("** argument must be on or off\n");
      cli_radio_script_state_transition_event();
      return;
    }
    cli_radio_zw_classic_tx_power_enable(classic_tx_power_enable);
  }
  cli_uart_printf("Z-Wave Classic Tx Power setting is %s\n", cli_radio_zw_classic_tx_power_is_enabled() ? "Enabled" : "Disabled");
  cli_uart_printf("Use %s to set the Z-Wave Classic Tx Power        - %3d deci dBm\n",
                  cli_command_list[9].name, cli_radio_classic_tx_power_get()); // zw-classic-tx-power-set
  cli_uart_printf("Use %s to set the Z-Wave Classic Tx Power adjust - %3d deci dBm\n",
                  cli_command_list[10].name, cli_radio_classic_tx_power_adjust_get()); // zw-classic-tx-power-adjust_set
  cli_uart_printf("Use %s to list the Z-Wave Classic Tx Power attenuation setting - %3d dBm\n",
                  cli_command_list[11].name, cli_radio_classic_tx_power_attenuation_get()); // zw-classic-tx-power-attenuation_set
  cli_radio_script_state_transition_event();
}

void cli_zw_classic_tx_power_set(EmbeddedCli *cli, char *args, void *context)
{
  if (radio_is_not_initialized())
  {
    return;
  }

  uint8_t count = embeddedCliGetTokenCount(args);

  if (check_argument_count(count, 1, "power"))
  {
    int tmp_power;
    if (!parse_arg_int(args, 1, &tmp_power))
    {
      cli_radio_script_state_transition_event();
      return;
    }

    if (validate_integer_range(tmp_power, -20, 20, 0))
    {
      tmp_power = cli_radio_classic_tx_power_set(tmp_power * 10);
      cli_uart_printf("Z-Wave Classic Tx Power set to %d dBm. Used when transmitting on classic channel\n", tmp_power/10);
    }
    else
    {
      cli_uart_print("Z-Wave Classic Tx Power must be in the range (-20..20)\n");
    }
  }
  else
  {
    cli_uart_printf("Z-Wave Classic Tx Power is set to %d dBm\n", cli_radio_classic_tx_power_get()/10);
  }
  cli_radio_script_state_transition_event();
}

void cli_zw_classic_tx_power_adjust_set(EmbeddedCli *cli, char *args, void *context)
{
  if (radio_is_not_initialized())
  {
    return;
  }

  uint8_t count = embeddedCliGetTokenCount(args);

  if (check_argument_count(count, 1, "power_adjust"))
  {
    int tmp_power_adjust;
    if (!parse_arg_int(args, 1, &tmp_power_adjust))
    {
      cli_radio_script_state_transition_event();
      return;
    }

    if (validate_integer_range(tmp_power_adjust, -5, 5, 0))
    {
      tmp_power_adjust = cli_radio_classic_tx_power_adjust_set(tmp_power_adjust * 10);
      cli_uart_printf("Z-Wave Classic Tx Power max adjust set to %d dBm\n", tmp_power_adjust/10);
    }
    else
    {
      cli_uart_print("Z-Wave Classic Tx Power max adjust must be in the range (-5..5)\n");
    }
  }
  else
  {
    cli_uart_printf("Z-Wave Classic Tx Power max adjust is set to %d dBm\n", cli_radio_classic_tx_power_adjust_get()/10);
  }
  cli_radio_script_state_transition_event();
}

void cli_zw_classic_tx_power_attenuation_set(EmbeddedCli *cli, char *args, void *context)
{
  if (radio_is_not_initialized())
  {
    return;
  }

  uint8_t count = embeddedCliGetTokenCount(args);

  if (check_argument_count(count, 1, "attenuation"))
  {
    int tmp_attenuation;
    if (!parse_arg_int(args, 1, &tmp_attenuation))
    {
      cli_radio_script_state_transition_event();
      return;
    }

    if (validate_integer_range(tmp_attenuation, 0, 10, 0))
    {
      tmp_attenuation = cli_radio_classic_tx_power_attenuation_set(tmp_attenuation);
      cli_uart_printf("Z-Wave Classic Tx Power attenuation set to %d dBm\n", tmp_attenuation);
    }
    else
    {
      cli_uart_print("Z-Wave Classic Tx Power attenuation must be in the range (0..10)\n");
    }
  }
  else
  {
    cli_uart_printf("Z-Wave Classic Tx Power attenuation is set to %d dBm\n", cli_radio_classic_tx_power_attenuation_get());
  }
  cli_radio_script_state_transition_event();
}

/**
 * Handler for setting transmission delay between transmit repeats
 */
void cli_zw_tx_delay_set(EmbeddedCli *cli, char *args, void *context)
{
  uint8_t count = embeddedCliGetTokenCount(args);

  if (check_argument_count(count, 1, "delay"))
  {
    uint32_t tmp_delay;
    if (!parse_arg_uint32_base(args, 1, &tmp_delay, 10))
    {
      cli_radio_script_state_transition_event();
      return;
    }

    if (validate_integer_range(tmp_delay, 2, 65535, 1))
    {
      frame.delay = tmp_delay;
      cli_uart_printf("Tx repeat delay %ums\n", frame.delay);
    }
    else
    {
      cli_uart_printf("Current Tx repeat delay %ums\n", frame.delay);
    }
  }
  cli_radio_script_state_transition_event();
}

void cli_zw_tx_config_set(EmbeddedCli *cli, char *args, void *context)
{
  const char* tmp;
  uint8_t count = embeddedCliGetTokenCount(args);

  if (check_argument_count(count, 2, "option, on/off"))
  {
    uint8_t option = 0;
    uint8_t enable = false;

    tmp = embeddedCliGetToken(args, 1);
    if (!strcmp(tmp, "fail-crc"))
    {
      option = 1;
    }
    else
    {
      cli_uart_print("** first argument must be a tx option - fail-crc\n");
      cli_radio_script_state_transition_event();
      return;
    }
    tmp = embeddedCliGetToken(args, 2);
    if (!strcmp(tmp, "on"))
    {
      enable = 1;
    }
    else if (!strcmp(tmp, "off"))
    {
      enable = 0;
    }
    else
    {
      cli_uart_print("** second argument must be on or off\n");
      cli_radio_script_state_transition_event();
      return;
    }
    if (0 != option)
    {
      cli_radio_tx_option_set(option, enable);
    }
  }
  cli_radio_script_state_transition_event();
}

void cli_zw_rx_set(EmbeddedCli *cli, char *args, void *context)
{
  if (REGION_UNDEFINED == cli_radio_region_current_get())
  {
    cli_uart_printf("** Undefined region, use %s to set the region\n", cli_command_list[1].name);
    cli_radio_script_state_transition_event();
    return;
  }

  bool start_receive = false;
  uint8_t count = embeddedCliGetTokenCount(args);

  if (check_argument_count(count, 1, "on/off"))
  {
    const char * arg = embeddedCliGetToken(args, 1); // args are counted from 1 (not from 0)
    if (!strcmp(arg, "on"))
    {
      start_receive = true;
    }
    else if (!strcmp(arg, "off"))
    {
      start_receive = false;
      cli_uart_printf("Rx off - Received %u frames\n", cli_radio_get_rx_count());
    }
    else
    {
      cli_uart_print("** argument must be on or off\n");
      cli_radio_script_state_transition_event();
      return;
    }
    cli_radio_start_receive(start_receive);
  }
  cli_radio_script_state_transition_event();
}

void cli_zw_tx_set_lbt(EmbeddedCli *cli, char *args, void *context)
{
  uint8_t count = embeddedCliGetTokenCount(args);

  if (check_argument_count(count, 2, "channel, power"))
  {
    uint32_t channel;
    if (!parse_arg_uint32_base(args, 1, &channel, 10))
    {
      cli_radio_script_state_transition_event();
      return;
    }
    if (validate_integer_range(channel, 0, 3, 0))
    {
      int lbt_level;
      if (!parse_arg_int(args, 2, &lbt_level))
      {
        cli_radio_script_state_transition_event();
        return;
      }
      if (validate_integer_range(lbt_level, -127, 0, 0))
      {
        // If lbt level equal 0 then turn off LBT on transmit on specified channel
        frame.lbt = (0 != lbt_level);
        cli_radio_set_lbt_level((uint8_t)channel, (int8_t)lbt_level);
        cli_uart_printf("Setting lbt level %idBm for channel %i, lbt %s\n", lbt_level, channel, (frame.lbt ? "on" : "off"));
      }
    }
  }
  cli_radio_script_state_transition_event();
}

void cli_stats_get(EmbeddedCli *cli, char *args, void *context)
{
  zpal_radio_network_stats_t *stats;
  stats = cli_radio_get_stats();

  if (NULL != stats)
  {
    cli_uart_print("Network statistics\n");
    cli_uart_print("----------------------\n");
    cli_uart_printf("Tx frame        = %i\n",stats->tx_frames);
    cli_uart_printf("Tx lbt errors   = %i\n",stats->tx_lbt_back_offs);
    cli_uart_printf("Rx frame        = %i\n",stats->rx_frames);
    cli_uart_printf("Rx lrc errors   = %i\n",stats->rx_lrc_errors);
    cli_uart_printf("Rx crc errors   = %i\n",stats->rx_crc_errors);
    cli_uart_printf("HomeID mismatch = %i\n",stats->rx_foreign_home_id);
    cli_uart_printf("Total Tx time 0 = %i\n",stats->tx_time_channel_0);
    cli_uart_printf("Total Tx time 1 = %i\n",stats->tx_time_channel_1);
    cli_uart_printf("Total Tx time 2 = %i\n",stats->tx_time_channel_2);
    cli_uart_printf("Total Tx time 3 = %i\n",stats->tx_time_channel_3);
    cli_uart_printf("Total Tx time 4 = %i\n",stats->tx_time_channel_4);
    cli_uart_print("----------------------\n");

    bool print_extended = false;
    uint8_t count = embeddedCliGetTokenCount(args);
    if (count > 1)
    {
      cli_uart_print("** Invalid number of arguments\n");
      cli_radio_script_state_transition_event();
      return;
    }
    if (count == 1)
    {
      uint32_t param_print_extended;
      if (!parse_arg_uint32_base(args, 1, &param_print_extended, 10))
      {
        cli_radio_script_state_transition_event();
        return;
      }
      if (validate_integer_range(param_print_extended, 0, 1, 1))
      {
        print_extended = (param_print_extended == 1);
      }
      else
      {
        cli_radio_script_state_transition_event();
        return;
      }
    }
    cli_radio_print_statistics(print_extended);
  }
  cli_radio_script_state_transition_event();
}

void cli_stats_clear(EmbeddedCli *cli, char *args, void *context)
{
  uint8_t count = embeddedCliGetTokenCount(args);
  if (2 > count)
  {
    uint32_t clear = 0;
    if (1 == count)
    {
      if (!parse_arg_uint32_base(args, 1, &clear, 10))
      {
        cli_radio_script_state_transition_event();
        return;
      }
      if (!validate_integer_range(clear, 0, 1, 1))
      {
        cli_radio_script_state_transition_event();
        return;
      }
    }
    cli_radio_clear_stats((clear == 1));
  }
  else
  {
    cli_uart_print("** Invalid number of arguments\n");
  }
  cli_radio_script_state_transition_event();
}

void cli_zw_rx_channel_set(EmbeddedCli *cli, char *args, void *context)
{
  const char* tmp;
  uint8_t channel;
  int8_t enable = -1;
  uint8_t count = embeddedCliGetTokenCount(args);

  if (REGION_UNDEFINED == cli_radio_region_current_get())
  {
    cli_uart_printf("** Undefined region, use %s to set the region\n", cli_command_list[1].name);
    return;
  }

  if ((0 < count) && (3 > count))
  {
    tmp = embeddedCliGetToken(args, 1); // args are counted from 1 (not from 0)
    if (!strcmp(tmp, "on"))
    {
      enable = 1;
    }
    else if (!strcmp(tmp, "off"))
    {
      enable = 0;
    }
    else
    {
      cli_uart_print("** argument must be on or off\n");
      cli_radio_script_state_transition_event();
      return;
    }
    if (1 == enable)
    {
      if (count == 1)
      {
        cli_uart_print("** Missing channel parameter\n");
        cli_radio_script_state_transition_event();
        return;
      }
      tmp = embeddedCliGetToken(args, 2);
      channel = atoi(tmp);
      if (strcmp(tmp, "0") && (channel == 0))
      {
        cli_uart_print("** Invalid channel\n");
        cli_radio_script_state_transition_event();
        return;
      }
      uint8_t region_channel_count = cli_radio_region_channel_count_get();
      if (validate_integer_range(channel, 0, region_channel_count-1, 1))
      {
        cli_uart_printf("Rx fixed channel enabled using channel %u \n", channel);
      }
      else
      {
        cli_uart_print("** Invalid channel\n");
        cli_radio_script_state_transition_event();
        return;
      }
    }
    else if (0 == enable)
    {
      if (count == 2)
      {
        cli_uart_print("** Ignoring extra parameter\n");
      }
      cli_uart_print("Rx fixed channel disabled\n");
    }
    cli_radio_set_fixed_channel(enable, channel);
  }
  else
  {
    cli_uart_print("** Invalid number of arguments\n");
  }
  cli_radio_script_state_transition_event();
}

void cli_zw_radio_tx_continues_set(EmbeddedCli *cli, char *args, void *context)
{
  if (radio_is_not_initialized())
  {
    return;
  }

  uint8_t count = embeddedCliGetTokenCount(args);

  if (count >= 1)
  {
    const char * arg = embeddedCliGetToken(args, 1);
    bool enable;
    if (!strcmp(arg, "on"))
    {
      enable = true;
    }
    else if (!strcmp(arg, "off"))
    {
      enable = false;
    }
    else
    {
      cli_uart_print("** argument must be on or off\n");
      cli_radio_script_state_transition_event();
      return;
    }
    if (count >= 2)
    {
      arg = embeddedCliGetToken(args, 2);
      uint8_t wave_type = atoi(arg);
      if (validate_integer_range(wave_type, 0, ZPAL_RADIO_WAVE_TYPE_COUNT - 1, 1))
      {
        frame.wave_type = wave_type;
      }
      else
      {
        cli_uart_printf("** Invalid wave type %s, must be 0 (unmodulated), 1 (modulated with fixed byte pattern) or 2 (modulated with fixed pattern with whitening)\n", arg);
        cli_radio_script_state_transition_event();
        return;
      }
    }
    cli_radio_tx_continues_set(enable, &frame);
  }
  cli_radio_script_state_transition_event();
}

void cli_zw_radio_rf_debug_set(EmbeddedCli *cli, char *args, void *context)
{
  uint8_t count = embeddedCliGetTokenCount(args);

  if (check_argument_count(count, 1, "on/off"))
  {
    const char * arg;
    bool rf_state_enable;

    arg = embeddedCliGetToken(args, 1);
    if (!strcmp(arg, "on"))
    {
      rf_state_enable = true;
    }
    else if (!strcmp(arg, "off"))
    {
      rf_state_enable = false;
    }
    else
    {
      cli_uart_printf("rf ** argument must be on or off %s\n", arg);
      cli_radio_script_state_transition_event();
      return;
    }

    cli_radio_rf_debug_set(rf_state_enable);
  }
  cli_radio_script_state_transition_event();
}

void cli_zw_radio_rf_debug_reg_setting_list(EmbeddedCli *cli, char *args, void *context)
{
  uint8_t count = embeddedCliGetTokenCount(args);
  bool listallreg = false;

  if (count == 1)
  {
    const char * arg = embeddedCliGetToken(args, 1);
    int16_t listreg = atoi(arg);
    if (validate_integer_range(listreg, 0, 1, 0))
    {
      listallreg = (listreg == 1);
    }
  }
  cli_radio_rf_debug_reg_setting_list(listallreg);
  cli_radio_script_state_transition_event();
}

void cli_zw_radio_rssi_get(EmbeddedCli *cli, char *args, void *context)
{
  if (radio_is_not_initialized())
  {
    return;
  }

  uint8_t count = embeddedCliGetTokenCount(args);
  uint8_t channel = 0;
  uint32_t repeats = 1;
  uint32_t delay = 1000;

  if (count >= 1)
  {
    uint32_t value;
    if (!parse_arg_uint32_base(args, 1, &value, 10))
    {
      cli_radio_script_state_transition_event();
      return;
    }
    if (validate_integer_range(value, 0, cli_radio_region_channel_count_get() - 1, 0))
    {
      channel = (uint8_t)value;
    }
    if (count >= 2)
    {
      if (!parse_arg_uint32_base(args, 2, &value, 10))
      {
        cli_radio_script_state_transition_event();
        return;
      }
      if (value)
      {
        repeats = value;
      }
    }
    if (count == 3)
    {
      if (!parse_arg_uint32_base(args, 3, &value, 10))
      {
        cli_radio_script_state_transition_event();
        return;
      }
      if (value)
      {
        delay = value;
      }
    }
    else if (count > 3)
    {
      cli_uart_print("Usage: zw-radio-rssi-get [channel] [repeats] [delay]\n\n");
      cli_radio_script_state_transition_event();
      return;
    }
  }
  cli_radio_rssi_get(channel, repeats, delay);
  cli_radio_script_state_transition_event();
}

void cli_zw_radio_rssi_get_all(EmbeddedCli *cli, char *args, void *context)
{
  if (radio_is_not_initialized())
  {
    return;
  }

  uint8_t count = embeddedCliGetTokenCount(args);
  uint32_t repeats = 1;
  uint32_t delay = 1000;

  if (count >= 1)
  {
    uint32_t value;
    if (!parse_arg_uint32_base(args, 1, &value, 10))
    {
      cli_radio_script_state_transition_event();
      return;
    }
    if (value)
    {
      repeats = value;
    }
    if (count == 2)
    {
      if (!parse_arg_uint32_base(args, 2, &value, 10))
      {
        cli_radio_script_state_transition_event();
        return;
      }
      if (value)
      {
        delay = value;
      }
    }
    else if (count > 2)
    {
      cli_uart_print("Usage: zw-radio-rssi-get-all [repeats] [delay]\n\n");
      cli_radio_script_state_transition_event();
      return;
    }
  }

  cli_radio_rssi_get_all(repeats, delay);
  cli_radio_script_state_transition_event();
}

void cli_zw_radio_rssi_config_set(EmbeddedCli *cli, char *args, void *context)
{
  if (radio_is_not_initialized())
  {
    return;
  }

  uint8_t count = embeddedCliGetTokenCount(args);
  uint16_t sample_frequency = 0;
  uint8_t sample_count_average = 0;

  if (count == 2)
  {
    const char * arg = embeddedCliGetToken(args, 1);
    sample_frequency = (uint16_t)atoi(arg);
    if (validate_integer_range(sample_frequency, 0, 65535, 0))
    {
      arg = embeddedCliGetToken(args, 2);
      sample_count_average = atoi(arg);
      if (!validate_integer_range(sample_count_average, 0, 255, 1))
      {
        sample_count_average = 0;
      }
    }
    cli_radio_rssi_config_set(sample_frequency, sample_count_average);
  }
  else
  {
    cli_uart_print("Usage: zw-radio-rssi-config-set <sample_frequency> <sample_count_average>\n\n");
    cli_radio_script_state_transition_event();
    return;
  }
  cli_radio_rssi_config_get(&sample_frequency, &sample_count_average);
  cli_uart_printf("Current RSSI sample configuration: sample_frequency %u, sample_count_average %u\n", sample_frequency, sample_count_average);
  cli_radio_script_state_transition_event();
}

void cli_zw_reset(EmbeddedCli *cli, char *args, void *context)
{
  cli_radio_reset();
}

void cli_zw_script_entry(EmbeddedCli *cli, char *args, void *context)
{
  uint8_t count = embeddedCliGetTokenCount(args);
  int8_t script_number = -1;

  if ((0 < count) && (count < 3))
  {
    const char * arg;
    radio_cli_script_cmd_t script_state_request = 0;

    arg = embeddedCliGetToken(args, 1);
    if (!strcmp(arg, "start"))
    {
      script_state_request = SCRIPT_START;
    }
    else if (!strcmp(arg, "stop"))
    {
      script_state_request = SCRIPT_STOP;
    }
    else if (!strcmp(arg, "autoon"))
    {
      script_state_request = SCRIPT_AUTORUN_ON;
    }
    else if (!strcmp(arg, "autooff"))
    {
      script_state_request = SCRIPT_AUTORUN_OFF;
    }
    else if (!strcmp(arg, "list"))
    {
      script_state_request = SCRIPT_LIST;
    }
    else if (!strcmp(arg, "clear"))
    {
      script_state_request = SCRIPT_CLEAR;
    }
    else
    {
      cli_uart_printf(" ** argument must be start, stop, autoon, autooff, list or clear - entered %s\n", arg);
      return;
    }
    if (count == 2)
    {
      arg = embeddedCliGetToken(args, 2);
      int number = atoi(arg);
      if (strcmp(arg, "0") && (number == 0))
      {
        cli_uart_printf("** Invalid script number %s\n", arg);
        return;
      }
      if (validate_integer_range(number, 1, 5, 1))
      {
        script_number = number;
      }
      else
      {
        return;
      }
    }
    cli_radio_script(script_state_request, script_number);
  }
  else
  {
    cli_uart_print("** Invalid number of arguments\n");
    return;
  }
  cli_radio_script_state_transition_event();
}

void cli_zw_script_run(EmbeddedCli *cli, char *args, void *context)
{
  uint8_t count = embeddedCliGetTokenCount(args);
  int8_t script_number = -1;

  if (count)
  {
    const char * arg;

    arg = embeddedCliGetToken(args, 1);
    int number = atoi(arg);
    if (validate_integer_range(number, 0, 5, 0))
    {
      script_number = number;
    }
  }
  cli_radio_script(SCRIPT_RUN, script_number);
  cli_radio_script_state_transition_event();
}

void cli_zw_wait(EmbeddedCli *cli, char *args, void *context)
{
  uint8_t count = embeddedCliGetTokenCount(args);

  if (check_argument_count(count, 1, "time ms"))
  {
    const char * arg;
    arg = embeddedCliGetToken(args, 1);
    uint32_t waittime_ms = atol(arg);
    if (validate_integer_range(waittime_ms, 1, 86400000, 0))
    {
      cli_radio_wait(waittime_ms);
      return;
    }
    else
    {
      cli_uart_print("Wait time in ms must in the range (1..86400000)\n");
    }
  }
  cli_radio_script_state_transition_event();
}

void cli_zw_radio_timestamp(EmbeddedCli *cli, char *args, void *context)
{
  uint8_t count = embeddedCliGetTokenCount(args);

  if (check_argument_count(count, 1, "on/off"))
  {
    const char * arg;
    bool timestamp_enable = false;

    arg = embeddedCliGetToken(args, 1);
    if (!strcmp(arg, "on"))
    {
      timestamp_enable = true;
    }
    else if (!strcmp(arg, "off"))
    {
      timestamp_enable = false;
    }
    else
    {
      cli_uart_printf("** argument must be on or off - %s\n", arg);
      cli_uart_printf("\nTimestamp currently %s on Rx and Tx dump\n", cli_radio_timestamp_get() ? "Enabled" : "Disabled");
      cli_radio_script_state_transition_event();
      return;
    }

    cli_radio_timestamp_set(timestamp_enable);
  }
  else
  {
    cli_uart_printf("\nTimestamp currently %s on Rx and Tx dump\n", cli_radio_timestamp_get() ? "Enabled" : "Disabled");
  }
  cli_radio_script_state_transition_event();
}

static EmbeddedCli *cmd_cli;

void cli_command_execute(char * cmdStr, size_t length)
{
  if (NULL != cmdStr)
  {
    size_t i = 0;
    while (i < length)
    {
      embeddedCliReceiveChar(cmd_cli, cmdStr[i++]);
    }
    embeddedCliReceiveChar(cmd_cli, '\n');
  }
  embeddedCliProcess(cmd_cli);
}

/*
 * Add all defined commands to the CLI
 */
void cli_commands_init(EmbeddedCli *cli)
{
  uint16_t command = 0;
  cmd_cli = cli;
  while (cli_command_list[command].binding != NULL)
  {
    embeddedCliAddBinding(cli, cli_command_list[command]);
    command++;
  }
}

void cli_zw_dump(EmbeddedCli *cli, char *args, void *context)
{
  uint8_t count = embeddedCliGetTokenCount(args);

  if (check_argument_count(count, 1, "Sector"))
  {
    const char * tmp = embeddedCliGetToken(args, 1); // args are counted from 1 (not from 0)
    if (!strcmp(tmp, "ft"))
    {
      cli_uart_print("\nDumping FT sector (Security page 0)\n");
      cli_system_dumpft();
    }
    else if (!strcmp(tmp, "mp"))
    {
      cli_uart_print("\nDumping MP sector\n");
      cli_system_dumpmp();
    }
    else if (!strcmp(tmp, "uft"))
    {
      cli_uart_print("\nDumping user FT sector (Security Page 1, offset 512.)\n");
      cli_system_dumpuft();
    }
  }
  cli_radio_script_state_transition_event();
}

typedef enum {
  read = 0,
  store,
  try
} XTAL_CMD;

static void xtal_internal_handler(char *args, XTAL_CMD xtal_cmd)
{
  uint16_t xtal_cal = 0xffff;

  if ((xtal_cmd == store) || (xtal_cmd == try))
  {
    uint8_t arg_count = embeddedCliGetTokenCount(args);

    if (arg_count == 3)
    {
      const char* string_u16 = embeddedCliGetToken(args, 3);
      xtal_cal = (uint16_t)strtoul(string_u16, NULL, 10);

      if (!validate_integer_range(xtal_cal, 0, 63, 2))
      {
        cli_uart_printf("** Invalid xtal value %u\n", xtal_cal);
        return;
      }
    }
    else
    {
      cli_uart_print("** xtal value missing\n");
      return;
    }
  }

  if (xtal_cmd == try)
  {
    cli_calibration_change_xtal(xtal_cal);
  }
  else if (xtal_cmd == store)
  {
    cli_calibration_store_xtal_sec_reg(xtal_cal);
  }
  else if (xtal_cmd == read)
  {
    cli_calibration_read_xtal_sec_reg(&xtal_cal);
    cli_uart_printf("\nxtal trim value %u (0x%02X)\n", xtal_cal, xtal_cal);
  }
  else
  {
    cli_uart_print("** Invalid parameter\n");
  }
}

static void xtal_token_handler(char *args, XTAL_CMD xtal_cmd)
{
  uint16_t xtal_cal = 0xffff;

  if ((xtal_cmd == store) || (xtal_cmd == try))
  {
    uint8_t arg_count = embeddedCliGetTokenCount(args);

    if (arg_count == 2)
    {
      const char* string_u16 = embeddedCliGetToken(args, 2);
      xtal_cal = (uint16_t)strtoul(string_u16, NULL, 10);

      if (!validate_integer_range(xtal_cal, 0, 63, 1))
      {
        cli_uart_printf("** Invalid xtal value %u\n", xtal_cal);
        return;
      }
    }
    else
    {
      cli_uart_print("** xtal value missing\n");
      return;
    }
  }

  if (xtal_cmd == try)
  {
    cli_calibration_change_xtal(xtal_cal);
  }
  else if (xtal_cmd == store)
  {
    cli_calibration_store_xtal(xtal_cal);
  }
  else if (xtal_cmd == read)
  {
    cli_calibration_read_xtal(&xtal_cal);
    cli_uart_printf("\nxtal trim value %u (0x%02X)\n", xtal_cal, xtal_cal);
  }
  else
  {
    cli_uart_print("** Invalid parameter\n");
  }
}

void cli_zw_cal_xtal(EmbeddedCli *cli, char *args, void *context)
{
  XTAL_CMD xtal_cmd = read;
  bool internal = false;

  uint8_t arg_count = embeddedCliGetTokenCount(args);

  for (uint8_t i = 0; i < arg_count; i++)
  {
    const char * arg = embeddedCliGetToken(args, i + 1);
    if (!strcmp(arg, "store"))
    {
      xtal_cmd = store;
    }
    else if (!strcmp(arg, "read"))
    {
      xtal_cmd = read;
    }
    else if (!strcmp(arg, "try"))
    {
      xtal_cmd = try;
    }
    else if (!strcmp(arg, "internal"))
    {
      internal = true;
    }
  }

  if (internal)
  {
    xtal_internal_handler(args, xtal_cmd);
  }
  else
  {
    xtal_token_handler(args, xtal_cmd);
  }
  cli_radio_script_state_transition_event();
}
