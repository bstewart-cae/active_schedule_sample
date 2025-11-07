/**
 * @file rfb.c
 * @author
 * @date
 * @brief To control rfb
 *
 * More detailed description can go here
 *
 *
 * @see http://
 */
/**************************************************************************************************
*    INCLUDES
*************************************************************************************************/
#include <string.h>
#include <stdio.h>
#include <math.h>

#include "radio.h"
#include "rfb.h"
#include "rfb_port.h"


/**************************************************************************************************
 *    MACROS
 *************************************************************************************************/


/**************************************************************************************************
 *    CONSTANTS AND DEFINES
 *************************************************************************************************/


/**************************************************************************************************
 *    TYPEDEFS
 *************************************************************************************************/


/**************************************************************************************************
 *    GLOBAL VARIABLES
 *************************************************************************************************/

#if (defined RFB_ZIGBEE_ENABLED && RFB_ZIGBEE_ENABLED == 1)
/* Register Rfb control Apis*/
rfb_zb_ctrl_t const rfb_zb_ctrl =
{
    rfb_port_zb_init,
    rfb_port_frequency_set,
    rfb_port_zb_is_channel_free,
    rfb_port_data_send,
    rfb_port_tx_continuous_wave_set,
    rfb_port_rssi_read,
    rfb_port_15p4_address_filter_set,
    rfb_port_15p4_mac_pib_set,
    rfb_port_15p4_phy_pib_set,
    rfb_port_15p4_auto_ack_set,
    rfb_port_15p4_pending_bit_set,
    rfb_port_auto_state_set,
    rfb_port_version_get,
    rfb_port_15p4_src_addr_match_ctrl,
    rfb_port_15p4_short_addr_ctrl,
    rfb_port_15p4_extend_addr_ctrl,
    rfb_port_key_set,
    rfb_port_csl_receiver_ctrl,
    rfb_port_csl_accuracy_get,
    rfb_port_csl_uncertainty_get,
    rfb_port_csl_sample_time_update,
    rfb_port_rtc_time_read,
    rfb_port_ack_packet_read,
    rfb_port_rx_rtc_time_get,
    rfb_port_current_channel_get,
    rfb_port_frame_counter_get,
};
#endif

#if (defined RFB_SUBG_ENABLED && RFB_SUBG_ENABLED == 1)
/* Register Rfb control Apis*/
rfb_subg_ctrl_t const rfb_ctrl =
{
    rfb_port_subg_init,
    rfb_port_modem_set,
    rfb_port_frequency_set,
    rfb_port_subg_is_channel_free,
    rfb_port_subg_rx_config_set,
    rfb_port_subg_tx_config_set,
    rfb_port_data_send,
    rfb_port_sleep_set,
    rfb_port_idle_set,
    rfb_port_rx_start,
    rfb_port_tx_continuous_wave_set,
    rfb_port_tx_continuous_modulated_wave_set,
    rfb_port_rssi_read,
    rfb_port_version_get,
    rfb_port_15p4_address_filter_set,
    rfb_port_15p4_mac_pib_set,
    rfb_port_15p4_phy_pib_set,
    rfb_port_15p4_auto_ack_set,
    rfb_port_15p4_pending_bit_set,
    rfb_port_auto_state_set,
    rfb_port_15p4_src_addr_match_ctrl,
    rfb_port_15p4_short_addr_ctrl,
    rfb_port_15p4_extend_addr_ctrl,
    rfb_port_key_set,
    rfb_port_csl_receiver_ctrl,
    rfb_port_csl_accuracy_get,
    rfb_port_csl_uncertainty_get,
    rfb_port_csl_sample_time_update,
    rfb_port_rtc_time_read,
    rfb_port_ack_packet_read,
    rfb_port_rx_rtc_time_get,
    rfb_port_current_channel_get,
    rfb_port_frame_counter_get,
    rfb_port_zwave_scan_set,
    rfb_port_zwave_id_filter_set,
    rfb_port_zwave_beam_get,
    rfb_port_zwave_scan_stage_get,
    rfb_port_zwave_scan_rssi_read,
    rfb_port_zwave_all_channel_average_rssi_read,
    rfb_port_zwave_wake_on_radio_set,
    rfb_port_zwave_scan_rssi_set,
    rfb_port_init_cb_register,
};
#endif

#if (defined RFB_BLE_ENABLED && RFB_BLE_ENABLED == 1)
/* Register Rfb control Apis*/
rfb_ble_ctrl_t const rfb_ctrl =
{
    rfb_port_ble_init,
    rfb_port_modem_set,
    rfb_port_frequency_set,
    rfb_port_data_send,
    rfb_port_ble_modem_set,
    rfb_port_ble_mac_set,
    rfb_port_rx_start
};
#endif
/**************************************************************************************************
 *    LOCAL FUNCTIONS
 *************************************************************************************************/


/**************************************************************************************************
 *    GLOBAL FUNCTIONS
 *************************************************************************************************/
#if (defined RFB_ZIGBEE_ENABLED && RFB_ZIGBEE_ENABLED == 1)
rfb_zb_ctrl_t *rfb_zb_init(void)
{
    return (rfb_zb_ctrl_t *)&rfb_zb_ctrl;
}
#endif

#if (defined RFB_SUBG_ENABLED && RFB_SUBG_ENABLED == 1)
rfb_subg_ctrl_t *rfb_subg_init(void)
{
    return (rfb_subg_ctrl_t *)&rfb_ctrl;
}
#endif

#if (defined RFB_BLE_ENABLED && RFB_BLE_ENABLED == 1)
rfb_ble_ctrl_t *rfb_ble_init(void)
{
    return (rfb_ble_ctrl_t *)&rfb_ctrl;
}
#endif
