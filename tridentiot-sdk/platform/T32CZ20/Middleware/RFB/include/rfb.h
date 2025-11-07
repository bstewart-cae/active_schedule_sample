/**
 * @file rfb.h
 * @author
 * @date 20 Jan. 2021
 * @brief The baseband control commands and events for RF MCU
 *
 */
#ifndef _RFB_H_
#define _RFB_H_
/**************************************************************************************************
 *    INCLUDES
 *************************************************************************************************/


/**************************************************************************************************
 *    CONSTANTS AND DEFINES
 *************************************************************************************************/
typedef enum
{
    RFB_EVENT_SUCCESS = 0,
    RFB_CNF_EVENT_INVALID_CMD,
    RFB_CNF_EVENT_RX_BUSY,
    RFB_CNF_EVENT_TX_BUSY,
    RFB_CNF_EVENT_CMD_DISALLOWED,
    RFB_CNF_EVENT_NOT_AVAILABLE,
    RFB_CNF_EVENT_CONTENT_ERROR,
    RFB_RSP_EVENT_NOT_AVAILABLE,
    RFB_RSP_EVENT_CONTENT_ERROR
} RFB_EVENT_STATUS;

typedef enum
{
    RFB_CMN_EVENT_SUCCESS = 0,
    RFB_CMN_CNF_EVENT_INVALID_CMD,
    RFB_CMN_CNF_EVENT_RX_BUSY,
    RFB_CMN_CNF_EVENT_TX_BUSY,
    RFB_CMN_CNF_EVENT_UNSUPPORT_CMD,
    RFB_CMN_CNF_EVENT_NOT_AVAILABLE,
    RFB_CMN_CNF_EVENT_CONTENT_ERROR,
    RFB_CMN_RSP_EVENT_NOT_AVAILABLE,
    RFB_CMN_RSP_EVENT_CONTENT_ERROR
} RFB_CMN_EVENT_STATUS;

typedef enum
{
    RFB_WRITE_TXQ_SUCCESS = 0,
    RFB_WRITE_TXQ_FULL
} RFB_WRITE_TXQ_STATUS;

typedef enum
{
    RFB_KEYING_FSK               = 1,
    RFB_KEYING_OQPSK             = 2,
    RFB_KEYING_ZWAVE             = 3,
} rfb_keying_type_t;

typedef enum
{
    RFB_MODEM_FSK                = 1,
    RFB_MODEM_ZIGBEE             = 2,
    RFB_MODEM_BLE                = 3,
    RFB_MODEM_OQPSK              = 4,
    RFB_MODEM_ZWAVE              = 5,
} rfb_modem_type_t;

typedef void (*RFB_INIT_CB)(void);

/**
 * @brief A structure to hold information about a RF baseband interrupt events.
 *
 * This structure contains details about the interrupt callback events triggered by
 * RF MCU
 */
typedef struct rfb_interrupt_event_s
{
    /**
     * @brief Callback to notify application layer when RF RX packet is received
     *
     * @param ruci_packet_length The packet length of the received signal
     * @param rx_data_address The payload address of received signal
     * @param crc_status CRC status of received signal [0: CRC pass, 1: CRC fail]
     * @param rssi RSSI value of received signal [dBm]
     * @param snr SNR of received signal [dB]
     * @return None
     */
    void (*rx_done)(uint16_t packet_length, uint8_t *rx_data_address, uint8_t crc_status, uint8_t rssi, uint8_t snr);
    /**
     * @brief Callback to notify application layer when RF TX activity is done
     *
     * @param tx_status Status of TX activity [0x00: TX success, 0x10: CSMA-CA fail]
     * @return None
     */
    void (*tx_done)(uint8_t tx_status);
    /**
     * @brief Callback to notify application layer when
     * 1. no packet is received for 5ms after receiving a beam frame
     * or
     * 2. receiving a normal frame within 5ms after receiving a beam frame,
     * which is equivalent to RX beam end
     *
     * @param None
     * @return None
     */
    void (*rx_timeout)(void);
    void (*rtc)(void); /**< Reserved for future use */
    /**
     * @brief Callback to notify application layer when Z-Wave RX beam frame is received
     *
     * @param None
     * @return None
     */
    void (*rx_beam)(void);
} rfb_interrupt_event_t;

typedef enum
{
    BAND_SUBG_915M   = 0,
    BAND_2P4G        = 1,
    BAND_SUBG_868M   = 2,
    BAND_SUBG_433M   = 3,
    BAND_SUBG_315M   = 4
} band_type_t;

typedef enum
{
    BAND_OQPSK_SUBG_915M   = 0,
    BAND_OQPSK_SUBG_868M   = 1,
    BAND_OQPSK_SUBG_433M   = 2,
    BAND_OQPSK_SUBG_315M   = 3
} oqpsk_band_type_t;

typedef enum
{
    TX_POWER_20dBm     = 0,
    TX_POWER_14dBm     = 1,
    TX_POWER_0dBm      = 2
} tx_power_level_t;

typedef enum
{
    FSK_PREAMBLE_TYPE_0     = 0,  /*01010101*/
    FSK_PREAMBLE_TYPE_1     = 1   /*10101010*/
} fsk_preamble_type_t;

typedef enum
{
    SFD_4BYTE           = 0,
    SFD_2BYTE           = 1
} sfd_type_t;

typedef enum
{
    FSK_200K             = 3,
    FSK_100K             = 4,
    FSK_50K              = 5,
    FSK_300K             = 6,
    FSK_150K             = 7
} fsk_bw_t;

typedef enum
{
    OQPSK_25K            = 3,
    OQPSK_12P5K          = 4,
    OQPSK_6P25K          = 5,
} oqpsk_bw_t;

typedef enum
{
    ZWAVE_100K          = 0,
    ZWAVE_40K,
    ZWAVE_9P6K,
    ZWAVE_LR,
} zwave_bw_t;

typedef enum
{
    ZWAVE_SCANNING_R3,
    ZWAVE_SCANNING_R2,
    ZWAVE_SCANNING_R1,
    ZWAVE_SCANNING_LR,
    ZWAVE_SCANNING_SWITCH_LR,
    ZWAVE_SCANNING_SWITCH_R3_2,
    ZWAVE_SCANNING_SWITCH_R3_3,
} zwave_scan_stage_t;

typedef enum
{
    ZWAVE_SCANNING_DISABLE,                   //[Disable Rx scan]              (mode 0, available already)
    ZWAVE_SCANNING_NORMAL,                    //[R1 - R2 - R3 - LR]
    ZWAVE_SCANNING_R1_ONLY,                   //[R1]
    ZWAVE_SCANNING_R2_ONLY,                   //[R2]
    ZWAVE_SCANNING_R3_ONLY,                   //[R3]
    ZWAVE_SCANNING_LR_ONLY,                   //[LR]
    ZWAVE_SCANNING_LEGACY,                    //[R1 - R2 - R3]
    ZWAVE_SCANNING_LR_SWITCH,                 //[LR - LR_2]                    (mode 7, available already)
    ZWAVE_SCANNING_R3_SWITCH,                 //[R3 - R3_2 - R3_3]
    ZWAVE_SCANNING_LR2_ONLY,                  //[LR_2]
    ZWAVE_SCANNING_NORMAL_LR2,                //[R1 - R2 - R3 - LR_2]
    ZWAVE_SCANNING_NORMAL_R2_ENHANCE,         //[R1 - R2 - R3 - LR - R2]       (mode 11, available already)
    ZWAVE_SCANNING_LEGACY_R2_ENHANCE,         //[R1 - R2 - R3 - R2]            (mode 12, available already)
    ZWAVE_SCANNING_NORMAL_LR2_R2_ENHANCE,     //[R1 - R2 - R3 - LR_2 - R2]     (mode 13, available already)
} zwave_scan_mode_t;

typedef enum
{
    ZWAVE_SCANNING_US,     //United States, including Long Range(USLR)
    ZWAVE_SCANNING_EU,     //European Union, including Long Range(EULR)
    ZWAVE_SCANNING_ANZ,    //Australia/New Zealand
    ZWAVE_SCANNING_HK,     //Hong Kong
    ZWAVE_SCANNING_IL,     //Israel
    ZWAVE_SCANNING_IN,     //India
    ZWAVE_SCANNING_RU,     //Russia
    ZWAVE_SCANNING_CN,     //China
    ZWAVE_SCANNING_JP,     //Japan
    ZWAVE_SCANNING_KR,     //Korea
} zwave_scan_region_t;

typedef enum
{
    MOD_0P5             = 0,
    MOD_1               = 1,
    MOD_UNDEF           = 2,
} fsk_mod_t;

typedef enum
{
    CRC_16              = 0,
    CRC_32              = 1
} crc_type_t;

typedef enum
{
    FSK              = 0,
    GFSK             = 1,
    OQPSK            = 2,
} fsk_filter_type_t;

typedef enum
{
    WHITEN_DISABLE     = 0,
    WHITEN_ENABLE      = 1
} whiten_enable_t;

typedef enum
{
    CLEAR_ALL          = 0,
    ADD_AN_ADDRESS     = 1,
    REMOVE_AN_ADDRESS  = 2,
} src_match_ctrl_t;

#if (defined RFB_ZIGBEE_ENABLED && RFB_ZIGBEE_ENABLED == 1)
typedef struct rfb_zb_ctrl_s
{
    /**
    * @brief Initiate RFB, and register interrupt event.
    *
    * @param Rfb interrupt event struct [rx_done, tx_done, rx_timeout]
    *
    * @return command setting result
    * @date 14 Dec. 2020
    * @image
    */
    RFB_EVENT_STATUS (*init)(rfb_interrupt_event_t *_rfb_interrupt_event);

    /**
    * @brief Set RF frequency.
    *
    * @param RF frequency [2402~2480 (MHz)]
    *
    * @return command setting result
    * @date 14 Dec. 2020
    * @image
    */
    RFB_EVENT_STATUS (*frequency_set)(uint32_t rf_frequency);

    /**
    * @brief Check channel is free or not.
    *
    * @param RF frequency [2402~2480 (MHz)]
    * @param Rssi_threshold [0->0dbm, 1->-1dBm, 2->-2dBm, ..., etc.]
    * @return Is channel free [1: channel is free 0: channel is not free]
    * @date 14 Dec. 2020
    * @image
    */
    bool (*is_channel_free)(uint32_t rf_frequency, uint8_t rssi_threshold);

    /**
    * @brief Send data to RFB buffer and RFB will transmit this data automatically
    *
    * @param Data address
    * @param Packet length
    *         Zigbee 1~127
    * @param TX control
    *        BIT1: [0: DIRECT_TRANSMISSION, 1:NONBEACON_MODE_CSMACA]
    *        BIT0: [0: ACK request = false, ACK request = true] (auto ack feature must enable)
    * @param Data sequence number (auto ack feature must enable)
    * @return Write Tx Queue result
    * @date 20 Jan. 2021
    * @image
    */
    RFB_WRITE_TXQ_STATUS (*data_send)(uint8_t *tx_data_address, uint16_t packet_length, uint8_t tx_control, uint8_t dsn);

    /**
    * @brief Set TX continuous wave (for testing, tx timeout is not supported)
    *
    * @param tx_enable [1: start, 0: stop]
    * @return command setting result
    * @date 14 Dec. 2020
    * @image
    */
    RFB_EVENT_STATUS (*tx_continuous_wave_set)(bool tx_enable);

    /**
    * @brief Read RSSI
    *
    * @param is_rssi_valid [true: is valid RSSI, false: the RSSi result is unreliable]
    * @return RSSI value [0->0dbm, 1->-1dBm, 2->-2dBm, ..., etc.]
    * @date 21 Nov. 2024
    * @image
    */
    uint8_t (*rssi_read)(bool *is_rssi_valid);

    /**
    * @brief Set 15.4 address filter
    * @param Activate peomiscuous mode by set mac_promiscuous_mode to true
    * @param 16bits short address.
    * @param 32bits long address[0]
    * @param 32bits long address[1]
    *        long address[0] and [1] contain 64 bits extended address
    * @param 16bits PAN ID.
    * @param Whether the device id coordinator or mot [0:false, 1:true]
    * @return command setting result
    * @date 17 Dec. 2021
    * @image
    */
    RFB_EVENT_STATUS (*address_filter_set)(uint8_t mac_promiscuous_mode, uint16_t short_source_address, uint32_t long_source_address_0, uint32_t long_source_address_1, uint16_t pan_id, uint8_t isCoordinator);

    /**
    * @brief Set 15.4 MAC PIB
    * @param The time forming the basic time period used by the CSMA-CA algorithm, specified in us.
    * @param The maximum time to wait for an acknowledgment frame to arrive following a transmitted data frame, specified in us.
    * @param The maximum value of the backoff exponent, BE, in the CSMA-CA algorithm
    * @param The maximum number of backoffs the CSMA-CA algorithm will attempt before declaring a channel access failure
    * @param The maximum time to wait either for a frame intended as a response to a data request frame, specified in us.
    * @param The maximum number of retries allowed after a transmission failure.
    * @param The minimum value of the backoff exponent (BE) in the CSMA-CA algorithm
    * @return command setting result
    * @date 17 Dec. 2021
    * @image
    */
    RFB_EVENT_STATUS (*mac_pib_set)(uint32_t a_unit_backoff_period, uint32_t mac_ack_wait_duration, uint8_t mac_max_BE, uint8_t mac_max_CSMA_backoffs,
                                    uint32_t mac_max_frame_total_wait_time, uint8_t mac_max_frame_retries, uint8_t mac_min_BE);

    /**
    * @brief Set 15.4 PHY PIB
    * @param RX-to-TX or TX-to-RX turnaround time, specified in us.
    * @param 0: Energy above threshold, 1: Carrier sense only, 2: Carrier sense with energy above threshold, where the logical operator is AND.
    *        3: Carrier sense with energy above threshold, where the logical operator is OR.
    * @param The received power threshold of the, energy above threshold, algorithm.
    * @param The duration for CCA, specified in us.
    * @return command setting result
    * @date 17 Dec. 2021
    * @image
    */
    RFB_EVENT_STATUS (*phy_pib_set)(uint16_t a_turnaround_time, uint8_t phy_cca_mode, uint8_t phy_cca_threshold, uint16_t phy_cca_duration);

    /**
    * @brief Enable auto ACK
    *
    * @param Enable auto ACK flag
    * @return command setting result
    * @date 20 Jan. 2021
    * @image
    */
    RFB_EVENT_STATUS (*auto_ack_set)(uint8_t auto_ack);

    /**
    * @brief Set frame pending bit
    *
    * @param frame pending bit flag [0:false, 1:true]
    * @return command setting result
    * @date 20 Jan. 2021
    * @image
    */
    RFB_EVENT_STATUS (*frame_pending_set)(uint8_t frame_pending);

    /**
    * @brief Set RX on when IDLW
    *
    * @param the flag for transfering to RX state automatically when RFB is idle [0:false, 1:true]; Th
    * @return command setting result
    * @date 20 Jan. 2021
    * @image
    */
    RFB_EVENT_STATUS (*auto_state_set)(bool rx_on_when_idle);

    /**
    * @brief Get RFB firmware version
    *
    * @param None
    * @return None
    * @date 20 Jan. 2021
    * @image
    */
    uint32_t (*fw_version_get)(void);

    /**
    * @brief Enable/ disable source address match feature
    *
    * @param Enable flag [0:disable, 1:enable]
    * @return command setting result
    * @date 17 Aug. 2022
    * @image
    */
    RFB_EVENT_STATUS (*src_addr_match_ctrl)(uint8_t ctrl_type);

    /**
    * @brief Control the short source address table
    *
    * @param Control type [0: clear src matching table,
    *                      1: add/set address entry - ack pending bit,
    *                      2: clear address entry   - ack pending bit,
    *                      3: add/set address entry - link metrics LQI (ehn-ack),
    *                      4: clear address entry   - link metrics LQI (ehn-ack),
    *                      5: add/set address entry - link metrics link margin (ehn-ack),
    *                      6: clear address entry   - link metrics link margin (ehn-ack),
    *                      7: add/set address entry - link metrics RSSI (ehn-ack),
    *                      8: clear address entry   - link metrics RSSI (ehn-ack),
    *                      9: find address entry]
    * @param The pointer for short address
    * @return None
    * @date 17 Aug. 2022
    * @image
    */
    RFB_EVENT_STATUS (*short_addr_ctrl)(uint8_t ctrl_type, uint8_t *short_addr);

    /**
    * @brief Control the short source address table
    *
    * @param Control type [0: clear src matching table,
    *                      1: add/set address entry - ack pending bit,
    *                      2: clear address entry   - ack pending bit,
    *                      3: add/set address entry - link metrics LQI (ehn-ack),
    *                      4: clear address entry   - link metrics LQI (ehn-ack),
    *                      5: add/set address entry - link metrics link margin (ehn-ack),
    *                      6: clear address entry   - link metrics link margin (ehn-ack),
    *                      7: add/set address entry - link metrics RSSI (ehn-ack),
    *                      8: clear address entry   - link metrics RSSI (ehn-ack),
    *                      9: find address entry]
    * @param The pointer for short address
    * @return None
    * @date 17 Aug. 2022
    * @image
    */
    RFB_EVENT_STATUS (*extend_addr_ctrl)(uint8_t ctrl_type, uint8_t *extend_addr);

    /**
    * @brief Configure 128bits encryption key
    *
    * @param The pointer for key
    * @return command setting result
    * @date 30 Aug. 2022
    * @image
    */
    RFB_EVENT_STATUS (*key_set)(uint8_t *key_addr);
    /**
    * @brief Enable/ disable the CSL receiver
    *
    * @param Enable flag [0:disable, 1:enable]
    * @param CSL period
    * @return command setting result
    * @date 8 Sep. 2022
    * @image
    */
    RFB_EVENT_STATUS (*csl_receiver_ctrl)(uint8_t csl_receiver_ctrl, uint16_t csl_period);
    /**
    * @brief Get the current accuracy
    *
    * @param None
    * @return The current accuracy
    * @date 8 Sep. 2022
    * @image
    */
    uint8_t (*csl_accuracy_get)(void);
    /**
    * @brief Get the current uncertainty
    *
    * @param None
    * @return The current uncertainty
    * @date 8 Sep. 2022
    * @image
    */
    uint8_t (*csl_uncertainty_get)(void);
    /**
    * @brief Update CSL sample time
    *
    * @param None
    * @return command setting result
    * @date 13 Sep. 2022
    * @image
    */
    RFB_EVENT_STATUS (*csl_sample_time_update)(uint32_t csl_sample_time);
    /**
    * @brief Read RFB RTC time
    *
    * @param None
    * @return The current RTC time
    * @date 20 Sep. 2022
    * @image
    */
    uint32_t (*rtc_time_read)(void);
    /**
    * @brief Read ack pkt data
    *
    * @param rx_data_address
    * @param rx_time_address
    * @return The length of ack pkt
    * @date 18 Dec. 2022
    * @image
    */
    uint8_t (*ack_packet_read)(uint8_t *rx_data_address, uint8_t *rx_time_address, bool is2bytephr);
    /**
    * @brief Read RFB RX RTC time
    *
    * @param rx_cnt: total_rx_done_cnt%5 (start from 0)
    * @return The RX RTC time stored in local data q
    * @date 18 Dec. 2022
    * @image
    */
    uint32_t (*rx_rtc_time_get)(uint8_t rx_cnt);
    /**
    * @brief Read RFB current channel
    *
    * @param rx_cnt: total_rx_done_cnt%5 (start from 0)
    * @return The channel number (for 2.4G)
    * @date 6 Mar. 2023
    * @image
    */
    uint8_t (*current_channel_get)(uint8_t rx_cnt);
    /**
    * @brief Read RFB current frame counter
    *
    * @param None
    * @return The frame counter value
    * @date 6 Mar. 2023
    * @image
    */
    uint32_t (*frame_counter_get)(void);
} rfb_zb_ctrl_t;
#endif

#if (defined RFB_SUBG_ENABLED && RFB_SUBG_ENABLED == 1)
typedef struct rfb_subg_ctrl_s
{
    /**
    * @brief Initiate RFB, and register interrupt event.
    *
    * @param Rfb interrupt event struct [rx_done, tx_done, rx_timeout, rx_beam]
    * @param keying_mode [RFB_KEYING_ZWAVE]
    * @param band_type [BAND _SUBG_915M / BAND _SUBG_868M] for ZWave
    *
    * @return Results of initialization
    * @date 07 Dec. 2022
    * @image
    */
    RFB_EVENT_STATUS (*init)(rfb_interrupt_event_t *_rfb_interrupt_event, rfb_keying_type_t keying_mode, uint8_t band_type);

    /**
    * @brief Set RFB modem type.
    *
    * @param Modem [RFB_MODEM_ZWAVE]
    * @param band_type [BAND _SUBG_915M / BAND _SUBG_868M] for ZWave
    *
    * @return command setting result
    * @date 17 Dec. 2020
    * @image
    */
    RFB_EVENT_STATUS (*modem_set)(rfb_modem_type_t modem, uint8_t band_type);

    /**
    * @brief Set RF frequency.
    *
    * @param RF frequency [116250~930000 (kHz)]
    *
    * @return Results of configuration
    * @date 14 Dec. 2020
    * @image
    */
    RFB_EVENT_STATUS (*frequency_set)(uint32_t rf_frequency);

    /**
    * @brief Check channel is free or not.
    *
    * @param rf_frequency [116250~930000 (kHz) ]
    * @param rssi_threshold [0->0dbm, 1->-1dBm, 2->-2dBm, ..., etc.]
    * @return Is channel free [1: channel is free 0: channel is not free]
    * @date 14 Dec. 2020
    * @image
    */
    bool (*is_channel_free)(uint32_t rf_frequency, uint8_t rssi_threshold);

    /**
    * @brief Set RX configurations.
    *
    * @param data_rate Z-WAVE  [ZWAVE_9P6K = 1, ZWAVE_40K = 2, ZWAVE_100K = 3, ZWAVE_LR = 4]
    * @return Results of configuration
    * @date 17 Dec. 2020
    * @image
    */
    RFB_EVENT_STATUS (*rx_config_set)(uint8_t data_rate, uint16_t preamble_len, fsk_mod_t mod_idx, crc_type_t crc_type,
                                      whiten_enable_t whiten_enable, uint32_t rx_timeout, bool rx_continuous, uint8_t filter_type);

    /**
    * @brief Set TX configurations.
    *
    * @param tx_power [Reserved for future use]
    * @param data_rate Z-WAVE  [ZWAVE_9P6K = 1, ZWAVE_40K = 2, ZWAVE_100K = 3, ZWAVE_LR = 4]
    * @return Results of configuration
    * @date 14 Dec. 2020
    * @image
    */
    RFB_EVENT_STATUS (*tx_config_set)(tx_power_level_t tx_power, uint8_t data_rate, uint16_t preamble_len, fsk_mod_t mod_idx,
                                      crc_type_t crc_type, whiten_enable_t whiten_enable, uint8_t filter_type);

    /**
     * @brief Send data to RFB buffer and RFB will transmit this data automatically
     *
     * @param tx_data_address The payload address of TX data
     * @param packet_length Packet length [Z-Wave]
     * @param tx_control TX control setting
     * @param dsn Data sequence number  [0] (Not applicable for Z-Wave mode)
     * @return The result of TX data send
     * @date 20 Jan. 2021
      * @image
     */
    RFB_WRITE_TXQ_STATUS (*data_send)(uint8_t *tx_data_address, uint16_t packet_length, uint8_t tx_control, uint8_t dsn);

    /**
     * @brief Set RFB to sleep state
     *
     * @param None
     * @return command setting result
     * @date 14 Dec. 2020
      * @image
     */
    RFB_EVENT_STATUS (*sleep_set)(void);

    /**
     * @brief Set RFB to idle state
     *
     * @param None
     * @return command setting result
     * @date 14 Dec. 2020
      * @image
     */
    RFB_EVENT_STATUS (*idle_set)(void);

    /**
     * @brief Set RFB to RX state
     *
     * @param None
     * @return command setting result
     * @date 14 Dec. 2020
      * @image
     */
    RFB_EVENT_STATUS (*rx_start)(void);

    /**
     * @brief Set TX continuous wave (for testing, tx timeout is not supported)
     *
     * @param tx_enable [1: start, 0: stop]
     * @return command setting result
     * @date 14 Dec. 2020
      * @image
     */
    RFB_EVENT_STATUS (*tx_continuous_wave_set)(bool tx_enable);

    /**
     * @brief Set TX continuous modulated wave (for testing, tx timeout is not supported)
     *
     * @param tx_enable [1: start, 0: stop]
     * @param randomness [1: continuous modulated wave using random data, 0: continuous modulated wave using fixed data]
     * @return command setting result
     */
    RFB_EVENT_STATUS (*tx_continuous_modulated_wave_set)(bool tx_enable, bool randomness);

    /**
    * @brief Read RSSI
    *
    * @param is_rssi_valid [true: is valid RSSI, false: the RSSi result is unreliable]
    * @return RSSI value [0->0dbm, 1->-1dBm, 2->-2dBm, ..., etc.]
    * @date 21 Nov. 2024
    * @image
    */
    uint8_t (*rssi_read)(bool *is_rssi_valid);

    /**
    * @brief Get RFB firmware version
    *
    * @param None
    * @return The version of FW
    * @date 20 Jan. 2021
    * @image
    */
    uint32_t (*fw_version_get)(void);

    /**
     * @brief Set 15.4 address filter
     * @param mac_promiscuous_mode Activate peomiscuous mode by set mac_promiscuous_mode to true
     * @param short_source_address 16bits short address.
     * @param long_source_address_0 32bits long address[0]
     * @param long_source_address_1 32bits long address[1]
     * @param pan_id 16bits PAN ID.
     * @param isCoordinator Whether the device id coordinator or mot [0:false, 1:true]
     * @return command setting result
     * @date 17 Dec. 2021
      * @image
     */
    RFB_EVENT_STATUS (*address_filter_set)(uint8_t mac_promiscuous_mode, uint16_t short_source_address, uint32_t long_source_address_0, uint32_t long_source_address_1, uint16_t pan_id, uint8_t isCoordinator);

    /**
    * @brief Set 15.4 MAC PIB
    * @param a_unit_backoff_period The time forming the basic time period used by the CSMA-CA algorithm, specified in us.
    * @param mac_ack_wait_duration The maximum time to wait for an acknowledgment frame to arrive following a transmitted data frame, specified in us.
    * @param mac_max_BE The maximum value of the backoff exponent, BE, in the CSMA-CA algorithm
    * @param mac_max_CSMA_backoffs The maximum number of backoffs the CSMA-CA algorithm will attempt before declaring a channel access failure
    * @param mac_max_frame_total_wait_time The maximum time to wait either for a frame intended as a response to a data request frame, specified in us.
    * @param mac_max_frame_retries The maximum number of retries allowed after a transmission failure.
    * @param mac_min_BE The minimum value of the backoff exponent (BE) in the CSMA-CA algorithm
    * @return command setting result
    * @date 17 Dec. 2021
    * @image
    */
    RFB_EVENT_STATUS (*mac_pib_set)(uint32_t a_unit_backoff_period, uint32_t mac_ack_wait_duration, uint8_t mac_max_BE, uint8_t mac_max_CSMA_backoffs,
                                    uint32_t mac_max_frame_total_wait_time, uint8_t mac_max_frame_retries, uint8_t mac_min_BE);

    /**
    * @brief Set 15.4 PHY PIB
    * @param a_turnaround_time RX-to-TX or TX-to-RX turnaround time, specified in us.
    * @param phy_cca_mode [0: Energy above threshold only,  1: Carrier sense only,  2: Carrier sense with energy above threshold, where the logical operator is AND.  3: Carrier sense with energy above threshold, where the logical operator is OR.]
    * @param phy_cca_threshold The received power threshold of the above threshold algorithm
    * @param phy_cca_duration The duration for CCA [us].
    * @return command setting result
    * @date 17 Dec. 2021
    * @image
    */
    RFB_EVENT_STATUS (*phy_pib_set)(uint16_t a_turnaround_time, uint8_t phy_cca_mode, uint8_t phy_cca_threshold, uint16_t phy_cca_duration);

    /**
    * @brief Enable auto ACK
    *
    * @param auto_ack Enable auto ACK flag
    * @return command setting result
    * @date 20 Jan. 2021
    * @image
    */
    RFB_EVENT_STATUS (*auto_ack_set)(uint8_t auto_ack);

    /**
    * @brief Set frame pending bit
    *
    * @param frame_pending frame pending bit flag [0:false, 1:true]
    * @return command setting result
    * @date 20 Jan. 2021
    * @image
    */
    RFB_EVENT_STATUS (*frame_pending_set)(uint8_t frame_pending);

    /**
    * @brief Set RX on when IDLE
    *
    * @param rx_on_when_idle the flag for transfering to RX state automatically when RFB is idle [0:false, 1:true];
    * @return Results of configuration
    * @date 20 Jan. 2021
    * @image
    */
    RFB_EVENT_STATUS (*auto_state_set)(bool rx_on_when_idle);

    /**
    * @brief Enable/ disable source address match feature
    *
    * @param ctrl_type Enable flag [0:disable, 1:enable]
    * @return Results of configuration
    * @date 17 Aug. 2022
    * @image
    */
    RFB_EVENT_STATUS (*src_addr_match_ctrl)(uint8_t ctrl_type);

    /**
    * @brief Control the short source address table
    *
    * @param ctrl_type [0: clear src matching table,
    *                      1: add/set address entry - ack pending bit,
    *                      2: clear address entry   - ack pending bit,
    *                      3: add/set address entry - link metrics LQI (ehn-ack),
    *                      4: clear address entry   - link metrics LQI (ehn-ack),
    *                      5: add/set address entry - link metrics link margin (ehn-ack),
    *                      6: clear address entry   - link metrics link margin (ehn-ack),
    *                      7: add/set address entry - link metrics RSSI (ehn-ack),
    *                      8: clear address entry   - link metrics RSSI (ehn-ack),
    *                      9: find address entry]
    * @param short_addr The pointer for short address
    * @return Results of configuration
    * @date 17 Aug. 2022
    * @image
    */
    RFB_EVENT_STATUS (*short_addr_ctrl)(uint8_t ctrl_type, uint8_t *short_addr);

    /**
    * @brief Control the extended source address table
    *
    * @param ctrl_type [0: clear src matching table,
    *                      1: add/set address entry - ack pending bit,
    *                      2: clear address entry   - ack pending bit,
    *                      3: add/set address entry - link metrics LQI (ehn-ack),
    *                      4: clear address entry   - link metrics LQI (ehn-ack),
    *                      5: add/set address entry - link metrics link margin (ehn-ack),
    *                      6: clear address entry   - link metrics link margin (ehn-ack),
    *                      7: add/set address entry - link metrics RSSI (ehn-ack),
    *                      8: clear address entry   - link metrics RSSI (ehn-ack),
    *                      9: find address entry]
    * @param extend_addr The pointer for extended address
    * @return Results of configuration
    * @date 17 Aug. 2022
    * @image
    */
    RFB_EVENT_STATUS (*extend_addr_ctrl)(uint8_t ctrl_type, uint8_t *extend_addr);

    /**
    * @brief Configure 128bits encryption key
    *
    * @param key_addr The pointer for key
    * @return Results of configuration
    * @date 30 Aug. 2022
    * @image
    */
    RFB_EVENT_STATUS (*key_set)(uint8_t *key_addr);

    /**
    * @brief Enable/ disable the CSL receiver
    *
    * @param csl_receiver_ctrl Enable flag [0:disable, 1:enable]
    * @param csl_period CSL period
    * @return Results of configuration
    * @date 8 Sep. 2022
    * @image
    */
    RFB_EVENT_STATUS (*csl_receiver_ctrl)(uint8_t csl_receiver_ctrl, uint16_t csl_period);

    /**
    * @brief Get the current accuracy
    *
    * @param None
    * @return The current accuracy of CSL
    * @date 8 Sep. 2022
    * @image
    */
    uint8_t (*csl_accuracy_get)(void);

    /**
    * @brief Get the current uncertainty
    *
    * @param None
    * @return The current uncertainty of CSL
    * @date 8 Sep. 2022
    * @image
    */
    uint8_t (*csl_uncertainty_get)(void);

    /**
    * @brief Update CSL sample time
    *
    * @param csl_sample_time The CSL sample time
    * @return Results of configuration
    * @date 13 Sep. 2022
    * @image
    */
    RFB_EVENT_STATUS (*csl_sample_time_update)(uint32_t csl_sample_time);

    /**
    * @brief Read RFB RTC time
    *
    * @param None
    * @return The current RTC time
    * @date 20 Sep. 2022
    * @image
    */
    uint32_t (*rtc_time_read)(void);

    /**
    * @brief Read ack pkt data
    *
    * @param rx_data_address
    * @param rx_time_address
    * @return The length of ack pkt
    * @date 18 Dec. 2022
    * @image
    */
    uint8_t (*ack_packet_read)(uint8_t *rx_data_address, uint8_t *rx_time_address, bool is2bytephr);

    /**
    * @brief Read RFB RX RTC time
    *
    * @param rx_cnt: total_rx_done_cnt%5 (start from 0)
    * @return The RX RTC time stored in local data q
    * @date 18 Dec. 2022
    * @image
    */
    uint32_t (*rx_rtc_time_get)(uint8_t rx_cnt);

    /**
    * @brief Read RFB current channel
    *
    * @param rx_cnt: total_rx_done_cnt%5 (start from 0)
    * @return The channel number (for 2.4G)
    * @date 6 Mar. 2023
    * @image
    */
    uint8_t (*current_channel_get)(uint8_t rx_cnt);

    /**
    * @brief Read RFB current frame counter
    *
    * @param None
    * @return The frame counter value
    * @date 6 Mar. 2023
    * @image
    */
    uint32_t (*frame_counter_get)(void);

    /**
    * @brief Set zwave scan
    * @param scan_mode Mode of Z-Wave RX scanning
    * @param scan_region Region of Z-Wave RX scanning
    * @return Result of configuration
    * @date 29 Apr. 2024
    * @image
    */
    RFB_EVENT_STATUS (*zwave_scan_set)(uint8_t scan_mode, uint8_t scan_region);

    /**
    * @brief Set zwave id filter
    * @param home_id 32bits Home ID
    * @param node_id 16bits Node ID.
    * @param home_id_hash 8bits  Home ID hash.
    * @return Results of setting
    * @date 12 Apr. 2024
    * @image
    */
    RFB_EVENT_STATUS (*zwave_id_filter_set)(uint32_t homd_id, uint16_t node_id, uint8_t home_id_hash);

    /**
    * @brief Get received beam data
    *
    * @param beam_data_address The payload address of RX beam data
    * @param beam_rssi_address The RSSI address of RX beam
    * @return None
    * @date 12 Apr. 2024
    * @image
    */
    void (*zwave_beam_get)(uint8_t *beam_data_address, uint8_t *beam_rssi_addr);

    /**
    * @brief Read Z-Wave scan stage of the latest RX frame
    *
    * @param None
    * @return Z-Wave scan stage [ZWAVE_SCANNING_R3 = 0, ZWAVE_SCANNING_R2 = 1,  ZWAVE_SCANNING_R1 = 2,  ZWAVE_SCANNING_LR = 3,]
    * @date 8 May. 2024
    * @image
    */
    uint8_t (*zwave_scan_stage_get)(void);

    /**
     * @brief Read RSSI under Z-wave scan RX
     *
     * @param scan_stage To get RSSI at which scan stage (see zwave_scan_stage_t)
     * @param is_rssi_valid [true: the RSSI result is valid, false: the RSSi result is unreliable]
     * @param average_mode [0: get RSSI in normal scan RX, 1: get RSSI in WOR(FLIRS) scan RX]
     * @return RSSI value [0->0dbm, 1->-1dBm, 2->-2dBm, ..., etc.]
     * @date 21 Nov. 2024
      * @image
     */
    uint8_t (*zwave_scan_rssi_read)(uint8_t scan_stage, bool *is_rssi_valid, bool average_mode);

    /**
     * @brief Read average RSSI over all channels under Z-wave scan RX
     *
     * @param average_rssi average RSSI values of all channels
     * @return Results of configuration
     * @date 7 May. 2025
      * @image
     */
    uint8_t (*zwave_all_channel_average_rssi_read)(uint8_t *average_rssi);

    /**
     * @brief Z-wave wake on radio
     *
     * @param rx_on_time RX on time in unit of ms per operation cycle
     * @param sleep_time RX off (sleep) time in unit of ms per operation cycle
     * @param scan_cycle Scan cycle during RX on time per operation cycle (only effective in RX scan mode and when rx_on_time is set to 0)
     * @param crc_filter [0: Commit all RX normal packets; 1: Only commit RX normal packets with correct CRC status]
     * @param rssi_read_period [0: no RSSI measurement; others: do RSSI measurement every rssi_read_period WOR cycles]
     * @param rssi_average_number [calculate average RSSI over rssi_average_number measurement samples]
     * @param scan_cycle_for_rssi_read Scan cycle during RX on time per operation cycle in which RSSI measurement is done
     * @return Results of configuration
     * @date 3 Jan. 2025
      * @image
     */
    uint8_t (*zwave_wake_on_radio_set)(uint16_t rx_on_time, uint32_t sleep_time, uint8_t scan_cycle, uint8_t crc_filter,
                                       uint8_t rssi_read_period, uint8_t rssi_average_number, uint8_t scan_cycle_for_rssi_read);

    /**
     * @brief Set RSSI measurement under Z-wave scan RX
     *
     * @param rssi_read_period [0: no RSSI measurement; others: measuring RSSI at each channel every rssi_read_period scan cycles]
     * @param rssi_average_number Calculating average RSSI over rssi_average_number RSSI values
     * @return None
     * @date 8 Apr. 2025
      * @image
     */
    void (*zwave_scan_rssi_set)(uint16_t rssi_read_period, uint8_t rssi_average_number);

    /**
    * @brief Set initialization callback
    *
    * @param the registration callback
    * @return None
    * @date 9 Sep. 2024
    * @image
    */
    bool (*init_cb_register)(RFB_INIT_CB cb);

} rfb_subg_ctrl_t;
#endif

#if (defined RFB_BLE_ENABLED && RFB_BLE_ENABLED == 1)
typedef struct rfb_ble_ctrl_s
{
    /**
    * @brief Initiate RFB, and register interrupt event.
    *
    * @param Rfb interrupt event struct [rx_done, tx_done]
    *
    * @return Command setting result
    * @date 14 Dec. 2020
    * @image
    */
    RFB_EVENT_STATUS (*init)(rfb_interrupt_event_t *_rfb_interrupt_event);

    /**
    * @brief Set RFB modem type.
    *
    * @param Modem [RFB_MODEM_BLE]
    * @param band_type [BAND_2P4G]
    *
    * @return Command setting result
    * @date 17 Dec. 2020
    * @image
    */
    RFB_EVENT_STATUS (*modem_set)(rfb_modem_type_t modem, uint8_t band_type);


    /**
    * @brief Set RF frequency.
    *
    * @param RF frequency [2400~2480]
    *
    * @return Command setting result
    * @date 14 Dec. 2020
    * @image
    */

    RFB_EVENT_STATUS (*frequency_set)(uint32_t rf_frequency);

    /**
     * @brief Send data to RFB buffer and RFB will transmit this data automatically
     *
     * @param Data address
     * @param Packet length
     *         FSK [1~2047] / OQPSK [1~127]
     * @param TX control [0](Not apply for 15p4g mode)
     * @param Data sequence number [0] (Not apply for 15.4g mode)
     * @return Write Tx queue result
     * @date 20 Jan. 2021
      * @image
     */
    RFB_WRITE_TXQ_STATUS (*data_send)(uint8_t *tx_data_address, uint16_t packet_length, uint8_t tx_control, uint8_t dsn);
    RFB_EVENT_STATUS (*ble_modem_set)(uint8_t data_rate, uint8_t coded_scheme);
    RFB_EVENT_STATUS (*ble_mac_set)(uint32_t sfd_content, uint8_t whitening_en, uint8_t whitening_init_value, uint32_t crc_init_value);

    /**
     * @brief Set RFB to RX state
     *
     * @param None
     * @return command setting result
     * @date 14 Dec. 2020
      * @image
     */
    RFB_EVENT_STATUS (*rx_start)(void);

} rfb_ble_ctrl_t;
#endif
/**************************************************************************************************
 *    TYPEDEFS
 *************************************************************************************************/
#if (defined RFB_SUBG_ENABLED && RFB_SUBG_ENABLED == 1)
#define FSK_MAX_RF_LEN 2063 //2047+16
#define OQPSK_MAX_RF_LEN 142 //127+15
#define MAX_RF_LEN FSK_MAX_RF_LEN
#elif (defined RFB_BLE_ENABLED && RFB_BLE_ENABLED == 1)
#define MAX_RF_LEN 268 //255+13
#elif (defined RFB_ZIGBEE_ENABLED && RFB_ZIGBEE_ENABLED == 1)
#define MAX_RF_LEN 140 //127+13
#endif

/**************************************************************************************************
 *    Global Prototypes
 *************************************************************************************************/
#if (defined RFB_ZIGBEE_ENABLED && RFB_ZIGBEE_ENABLED == 1)
rfb_zb_ctrl_t *rfb_zb_init(void);
#endif
#endif
#if (defined RFB_SUBG_ENABLED && RFB_SUBG_ENABLED == 1)
rfb_subg_ctrl_t *rfb_subg_init(void);
#endif
#if (defined RFB_BLE_ENABLED && RFB_BLE_ENABLED == 1)
rfb_ble_ctrl_t *rfb_ble_init(void);
#endif
