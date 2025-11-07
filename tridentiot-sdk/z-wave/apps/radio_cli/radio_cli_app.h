/// ***************************************************************************
/// SPDX-License-Identifier: LicenseRef-TridentMSLA
/// SPDX-FileCopyrightText: 2024 Trident IoT, LLC <https://www.tridentiot.com>
/// ***************************************************************************

/**
 * @file radio_cli_app.h
 * @brief Radio CLI application
 */
#ifndef _CLI_APP_H_
#define _CLI_APP_H_

#include <zpal_power_manager.h>
#include <zpal_radio.h>
#include <zpal_radio_private.h>
#include <zpal_init.h>

/****************************************************************************/
/*                             EXPORTED DEFINES                             */
/****************************************************************************/
#define CLI_MAJOR_VERSION       0 ///< CLI major version
#define CLI_MINOR_VERSION       7 ///< CLI minor version
#define CLI_PATCH_VERSION       7 ///< CLI patch version

#define RADIO_CLI_SCRIPT_VERSION  1 ///< RadioCLI script version

/****************************************************************************/
/*                              EXPORTED DATA                               */
/****************************************************************************/

/**
 * @brief Allocated memory for the application task stack.
 *
 */
#define APPLICATION_TASK_STACK              4*1024
#define APPLICATION_TASK_PRIORITY_STACK     ( TASK_PRIORITY_MAX - 10 )  ///< High, due to time critical protocol activity.

/**
 * @brief Default transmit delay in milliseconds.
 *
 */
#define DEFAULT_TX_DELAY_MS                 100

/**
 * @brief Transmit callback.
 *
 */
typedef void (*tx_callback_t)(uint16_t success, uint16_t failed, uint16_t failed_lbt);

/**
 * @brief Receive callback.
 *
 */
typedef void (*rx_callback_t)(uint16_t success, uint16_t failed);

/**
 * Script callback called when scripted functionality is done.
 */
typedef void (*event_handler_script_callback_t)(void);

/**
 * @brief Frame configuration
 *
 */
typedef struct {
  uint8_t destid;                     ///< Destination ID
  uint32_t frame_repeat;              ///< Frame repeat
  bool wait_ack;                      ///< Wait ack
  bool lbt;                           ///< Use LBT on transmit
  int8_t power;                       ///< Power
  zpal_radio_zwave_channel_t channel; ///< Channel
  zpal_radio_wave_type_t wave_type;   ///< Wave type, only used for 'tx continues wave'
  uint32_t delay;                     ///< Delay
  uint8_t *payload_buffer;            ///< Payload
  uint8_t payload_length;             ///< Payload length
  tx_callback_t tx_callback;          ///< Transmit callback
} radio_cli_tx_frame_config_t;

/**
 * Script line definition
 */
typedef struct _scriptline_t_ {
  char entry[128];                    ///< Buffer for contents of a scriptline
} scriptline_t;

/****************************************************************************/
/*                            EXPORTED FUNCTIONS                            */
/****************************************************************************/
/**
 * Main task of the CLI application
 *
 * @param[in] unused_prt
 */
void ZwaveCliTask(void* unused_prt);

/**
 * Print Radio CLI version information on uart
 */
void cli_radio_version_print(void);

/**
 * Print current status on uart
 */
void cli_radio_status_get(const radio_cli_tx_frame_config_t *const tx_frame_config);

/**
 * Initialize the radio for the given region
 *
 * @param[in] eRegion
 */
void cli_radio_setup(zpal_radio_region_t eRegion);

/**
 * Change the region.
 * This function works both before and after cli_radio_setup
 *
 * @param[in] new_region Region to set.
 * @return true
 * @return false
 */
bool cli_radio_change_region(zpal_radio_region_t new_region);

/**
 * Print out current region and print list of supported regions
 *
 * @param[in] region
 */
void cli_radio_region_list(zpal_radio_region_t region);

/**
 * Set the homeID of the receiver filter
 *
 * @param[in] home_id Home ID
 */
void cli_radio_set_homeid(uint32_t home_id);

/**
 * Set the nodeID of the receiver filter
 *
 * @param[in] node_id Node ID
 */
void cli_radio_set_nodeid(node_id_t node_id);

/**
 * Transmit a frame
 *
 * @param[in] tx_frame_config
 * @return true
 * @return false
 */
bool cli_radio_transmit_frame(const radio_cli_tx_frame_config_t *const tx_frame_config);

/**
 * Enable or disable the receiver
 *
 * @param[in] start_receive
 * @return true
 * @return false
 */
bool cli_radio_start_receive(bool start_receive);

/**
 * Get the number of received frames
 *
 * @return Number of received frames.
 */
uint16_t cli_radio_get_rx_count(void);

/**
 * Set LBT level for a given channel
 *
 * @param[in] channel_id
 * @param[in] level
 * @return true
 * @return false
 */
bool cli_radio_set_lbt_level(uint8_t channel_id, int8_t level);

/**
 * Set Tx max power 20dBm = true, 14dBm = false
 *
 * @param[in] max_tx_power_20dbm
 */
void cli_radio_set_tx_max_power_20dbm(bool max_tx_power_20dbm);

/**
 * Get the current Tx max power, 20dBm = true, 14dBm = false
 *
 * @return max_tx_power_20dbm
 */
bool cli_radio_get_tx_max_power_20dbm(void);

/**
 * Get current minimum Tx power in dBm on channel_id
 *
 * @param channel_id
 * @return Minimum Tx Power
 */
int8_t cli_radio_get_tx_min_power(uint8_t channel_id);

/**
 * Get current maximum Tx power in dBm on channel_id
 *
 * @param channel_id
 * @return Maximum Tx Power
 */
int8_t cli_radio_get_tx_max_power(uint8_t channel_id);

/**
 * Set power_index for specified Tx Power
 *
 * @param channel_id
 * @param txpower
 * @param power_index
 * @return true, if power_index set. false, if power_index not set
 */
bool cli_radio_tx_power_index_set(uint8_t channel_id, int8_t txpower, uint8_t power_index);

/**
 * List current Tx power to radio power_index conversion table for channel_id
 *
 * @param channel_id
 */
void cli_radio_tx_power_index_list(uint8_t channel_id);

/**
 * Enable or disable fixed Rx channel
 *
 * @param[in] enable_fixed_channel
 * @param[in] channel_id
 */
void cli_radio_set_fixed_channel(uint8_t enable_fixed_channel, uint8_t channel_id);

/**
 * Get channel count for current Region
 *
 * @return Channel count
 */
uint8_t cli_radio_region_channel_count_get(void);

/**
 * Get radio statistics
 *
 * @return Network statistics
 */
zpal_radio_network_stats_t* cli_radio_get_stats(void);

/**
 * Clear Network statitics
 *  if clear_tx_time_stat equals false then clear current network statistics
 *  if clear_tx_time_stat equals true then clear tx_time statistics
 *
 * @param[in] clear_tx_timers_stat
 */
void cli_radio_clear_stats(bool clear_tx_timers_stat);

/**
 * Returns whether the radio is initialized.
 *
 * @return true
 * @return false
 */
bool cli_radio_initialized(void);

/**
 * Script command definitions
 */
typedef enum {
                SCRIPT_LIST = 0,      ///< List current defined script
                SCRIPT_START,         ///< Start entering new script
                SCRIPT_STOP,          ///< Stop current running script
                SCRIPT_RUN,           ///< Run current defined script
                SCRIPT_AUTORUN_ON,    ///< Enable auto run of script on start-up
                SCRIPT_AUTORUN_OFF,   ///< Disable auto run of script on start-up
                SCRIPT_CLEAR          ///< Clear current defined script
} radio_cli_script_cmd_t;

/**
 * Script state request.
 *  if script_state_request equals SCRIPT_LIST (0) then script is listed
 *  if script_state_request equals SCRIPT_START (1) then script entry mode is started and previous script entries are removed
 *  if script_state_request equals SCRIPT_STOP (2) then script entry mode is stopped and the entered script is saved in nvm
 *  if script_state_request equals SCRIPT_RUN (3) then current script is run
 *  if script_state_request equals SCRIPT_AUTORUN_ON (4) then autorun on start-up is set to on
 *  if script_state_request equals SCRIPT_AUTORUN_OFF (5) then autorun on start-up is set to off
 *  if script_state_request equals SCRIPT_CLEAR (6) then current script is emptied
 *
 * @param script_state_request
 * @param script_number
 */
void cli_radio_script(radio_cli_script_cmd_t script_state_request, int8_t script_number);

/**
 * Script signal state transition
 */
void cli_radio_script_state_transition_event(void);

/**
 * Wait time_ms milliseconds before signaling script state transition
 *
 * @param time_ms
 */
void cli_radio_wait(uint32_t time_ms);

/**
 * Enable or disable timestamp on Rx and TX printout
 *
 * @param enable
 */
void cli_radio_timestamp_set(bool enable);

/**
 * Return if timestamp is enabled or disabled on Rx and Tx dump
 *
 * @return true/false
 */
bool cli_radio_timestamp_get(void);

/**
 * Enable or disable continues unmodulated signal transmission on selected tx channel
 *  if enable equals true device will start transmitting a continues unmodulated signal on selected tx channel
 *  if enable equals false device will stop transmitting a continues unmodulated signal on selected tx channel
 *
 * @param enable
 * @param frame
 */
void cli_radio_tx_continues_set(bool enable, radio_cli_tx_frame_config_t *frame);

/**
 * Set/clear Tx option
 *
 * @param option
 * @param enable
 * @return Tx option set to enable
 */
bool cli_radio_tx_option_set(uint8_t option, uint8_t enable);

/**
 * Enable/disable Classic Z-Wave Application Tx Power setting
 * when transmitting on classic channels
 *
 * @param enable
 * @return true if Classic Z-Wave Tx Power setting is enabled,
 *         false if not
 */
bool cli_radio_zw_classic_tx_power_enable(bool enable);

/**
 * Check if Classic Z-Wave Application Tx Power setting is enabled
 *
 * @return true if Classic Z-Wave Tx Power setting is enabled,
 *         false if not
 */
bool cli_radio_zw_classic_tx_power_is_enabled(void);

/**
 * Set Classic Z-Wave Application Tx Power setting
 * when transmitting on classic channels and
 * Classic Z-Wave Tx Power setting is enabled
 *
 * @param power - deci dBm value
 * @return Tx Power set in deci dBm
 */
int16_t cli_radio_classic_tx_power_set(int16_t power);

/**
 * Get Classic Z-Wave Application Tx Power setting
 * when transmitting on classic channels and
 * Classic Z-Wave Tx Power setting is enabled
 *
 * @return Tx Power in deci dBm
 */
int16_t cli_radio_classic_tx_power_get(void);

/**
 * Set Classic Z-Wave Application Tx Power adjust setting
 * when transmitting on classic channels and
 * Classic Z-Wave Tx Power setting is enabled
 *
 * @param power_adjust - deci dBm value
 * @return Tx Power adjust set in deci dBm
 */
int16_t cli_radio_classic_tx_power_adjust_set(int16_t power_adjust);

/**
 * Get Classic Z-Wave Application Tx Power adjust setting
 * when transmitting on classic channels and
 * Classic Z-Wave Tx Power setting is enabled
 *
 * @return Tx Power adjust in deci dBm
 */
int16_t cli_radio_classic_tx_power_adjust_get(void);

/**
 * Set Classic Z-Wave Application Tx Power attenuation
 * when transmitting on classic channels and
 * Classic Z-Wave Tx Power setting is enabled
 *
 * @param tx_power_attenuation - deci dBm value
 * @return Tx Power attenuation set in deci dBm
 */
uint8_t cli_radio_classic_tx_power_attenuation_set(uint8_t tx_power_attenuation);

/**
 * Get Classic Z-Wave Application Tx Power attenuation
 * when transmitting on classic channels and
 * Classic Z-Wave Tx Power setting is enabled
 *
 * @return Tx Power attenuation in deci dBm
 */
uint8_t cli_radio_classic_tx_power_attenuation_get(void);

/**
 * Set radio rf state on gpios
 *  if rf_state_enable equals true device will make Radio show RF state on gpios
 *   gpio 0  = rf cpu state (1: cpu awake,  0: sleep)
 *   gpio 21 = rf rx        (1: rx on,      0: rx off)
 *   gpio 28 = rf tx        (1: tx on,      0: tx off)
 *  if rf_state_enable equals false device will make Radio stop showing RF state on gpios
 *
 * @param rf_state_enable
 */
void cli_radio_rf_debug_set(bool rf_state_enable);

/**
 * List current active calibration radio register settings
 *  if listallreg equals true more radio register values are listed
 *
 * @param listallreg
 */
void cli_radio_rf_debug_reg_setting_list(bool listallreg);

/**
 * Get RSSI measurement on specified channel
 *  if repeats equals 0 or 1 then only one RSSI value is read
 *  if repeats is greater than 1 then RSSI value is read repeats times with delay ms in between
 *
 * @param channel_id
 * @param repeats
 * @param delay
 */
void cli_radio_rssi_get(uint8_t channel_id, uint32_t repeats, uint32_t delay);

/**
 * Get RSSI measurement on all channels in current region
 *  if repeats equals 0 or 1 then only one RSSI value is read
 *  if repeats is greater than 1 then RSSI value is read repeats times with delay ms in between
 *
 * @param repeats
 * @param delay
 */
void cli_radio_rssi_get_all(uint32_t repeats, uint32_t delay);

/**
 * Congigure RSSI measurement
 *  configure radio to do RSSI sample every rssi_sample_frequency channel scan cycles
 *  configure radio to use rssi_sample_count_average RSSI samples for generating resulting RSSI average
 *  rssi_sample_frequency and rssi_sample_count_average are only used when doing Rx channel scanning
 *
 *  @param rssi_sample_frequency
 *  @param rssi_sample_count_average
 */
void cli_radio_rssi_config_set(uint16_t rssi_sample_frequency, uint8_t rssi_sample_count_average);

/**
 * Get RSSI measurement configuration
 *  *rssi_sample_frequency will be updated with current RSSI sample frequency
 *  *rssi_sample_count_average will be updated with current RSSI sample count average
 *
 * @param rssi_sample_frequency
 * @param rssi_sample_count_average
 */
void cli_radio_rssi_config_get(uint16_t *rssi_sample_frequency, uint8_t *rssi_sample_count_average);

/**
 * List current radio statistic values through cli_uart
 *  if print_extended equals true more statistic values are listed through cli_uart
 *
 * @param print_extended
 */
void cli_radio_print_statistics(bool print_extended);

/**
 * Set Tx payload to a default valid frame for current selected region and tx channel
 *  frame points to structure where default valid frame is to be copied to
 *
 * @param frame
 */
void cli_radio_set_payload_default(radio_cli_tx_frame_config_t *frame);

/**
 * Get channel configuration for a region
 *
 * @param region
 * @return Long Range channel configuration
 */
zpal_radio_lr_channel_config_t internal_region_to_channel_config(zpal_radio_region_t region);

/**
 * Get current selected Z-Wave region
 *
 * @return zpal_radio_region_t
 */
zpal_radio_region_t cli_radio_region_current_get(void);

/**
 * Get zero terminated Z-Wave Region name of current Region
 *
 * @return char*
 */
char *cli_radio_region_current_description_get(void);

/**
 * Convert zpal Z-Wave channel_id to internal phy channel_id
 *
 * @param zwave_channel_id
 * @return internal_channel_id
 */
uint8_t cli_radio_convert_zpal_channel_to_internal(zpal_radio_zwave_channel_t zwave_channel_id);

/**
 * Get last System Reset Reason
 *
 * @return zpal_reset_reason
 */
zpal_reset_reason_t cli_radio_reset_reason_get(void);

/**
 * Set current System Reset Reason
 *
 * @param reset_reason
 */
void cli_radio_reset_reason_set(zpal_reset_reason_t reset_reason);

/**
 * System reset
 * Uses watchdog to reset system
 * Resetcounter is set to 1
 */
void cli_radio_reset(void);

/**
 * Dump the FT sector in secure page 0
 */
void cli_system_dumpft(void);

/**
 * Dump the MP sector in main flash
 */
void cli_system_dumpmp(void);

/**
 * Dump the user FT sector in secure page 1
 */
void cli_system_dumpuft();

/**
 * Change the crystal calibration register
 */
void cli_calibration_change_xtal(uint16_t xtal_value);

/**
 * Store the crystal calibration in a token
 */
void cli_calibration_store_xtal(uint16_t xtal_cal);

/**
 * Read the crystal calibration from a token
 */
void cli_calibration_read_xtal(uint16_t *xtal_cal);

/**
 * Store the crystal calibration in a FLASH security registers
 */
void cli_calibration_store_xtal_sec_reg(uint16_t xtal_cal);

/**
 * Read the crystal calibration from the FLASH security registers
 */
void cli_calibration_read_xtal_sec_reg(uint16_t *xtal_cal);
#endif  /* _CLI_APP_H_ */
