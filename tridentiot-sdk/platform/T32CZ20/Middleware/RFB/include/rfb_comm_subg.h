/**
 * @file rfb_comm_subg.h
 * @author
 * @date
 * @brief Brief single line description use for indexing
 *
 * More detailed description can go here
 *
 *
 * @see http://
 */
#ifndef _RFB_COMM_SUBG_H_
#define _RFB_COMM_SUBG_H_
/**************************************************************************************************
 *    INCLUDES
 *************************************************************************************************/
#include "rf_mcu.h"
#include "rfb.h"
/**************************************************************************************************
 *    CONSTANTS AND DEFINES
 *************************************************************************************************/

/**************************************************************************************************
 *    TYPEDEFS
 *************************************************************************************************/

/**************************************************************************************************
 *    Global Prototypes
 *************************************************************************************************/
RFB_EVENT_STATUS rfb_comm_fsk_initiate(uint8_t band_type);
RFB_EVENT_STATUS rfb_comm_fsk_modem_set(uint8_t data_rate, uint8_t modulation_index);
RFB_EVENT_STATUS rfb_comm_fsk_mac_set(uint8_t crc_type, uint8_t whitening_enable);
RFB_EVENT_STATUS rfb_comm_fsk_preamble_set(uint8_t preamble_length);
RFB_EVENT_STATUS rfb_comm_fsk_sfd_set(uint32_t sfd_content);
RFB_EVENT_STATUS rfb_comm_fsk_type_set(uint8_t filter_type);

RFB_EVENT_STATUS rfb_comm_oqpsk_initiate(uint8_t band_type);
RFB_EVENT_STATUS rfb_comm_oqpsk_modem_set(uint8_t data_rate);
RFB_EVENT_STATUS rfb_comm_oqpsk_mac_set(uint8_t crc_type, uint8_t whitening_enable);
RFB_EVENT_STATUS rfb_comm_oqpsk_preamble_set(uint8_t preamble_length);
RFB_EVENT_STATUS rfb_comm_zwave_initiate(uint8_t band_type);
RFB_EVENT_STATUS rfb_comm_zwave_modem_set(uint8_t data_rate);
RFB_EVENT_STATUS rfb_comm_zwave_scan_set(uint8_t mode, uint8_t region);
RFB_EVENT_STATUS rfb_comm_zwave_id_filter_set(uint32_t home_id, uint16_t node_id, uint8_t home_id_hash);
#endif

