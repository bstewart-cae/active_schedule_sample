/**
 * @file rfb_comm_zigbee.c
 * @author
 * @date
 * @brief Apis for RFB communication subsystem and ruci format
 *
 * More detailed description can go here
 *
 *
 * @see http://
 */
/**************************************************************************************************
*    INCLUDES
*************************************************************************************************/
#include "cm33.h"
#include "ruci.h"
#include "rfb_comm_15p4Mac.h"
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

/**************************************************************************************************
 *    LOCAL FUNCTIONS
 *************************************************************************************************/

extern RF_MCU_RX_CMDQ_ERROR rfb_event_read(uint8_t *packet_length, uint8_t *event_address);
extern void rfb_send_cmd(uint8_t *cmd_address, uint8_t cmd_length);
/**************************************************************************************************
 *    GLOBAL FUNCTIONS
 *************************************************************************************************/
RFB_EVENT_STATUS  rfb_comm_zigbee_initiate(void)
{
    ruci_para_initiate_zigbee_t sZigbeeInitCmd = {0};
    ruci_para_cnf_event_t sCnfEvent = {0};
    uint8_t event_len = 0;
    RF_MCU_RX_CMDQ_ERROR event_status = RF_MCU_RX_CMDQ_ERR_INIT;

    SET_RUCI_PARA_INITIATE_ZIGBEE(&sZigbeeInitCmd);

    RUCI_ENDIAN_CONVERT((uint8_t *)&sZigbeeInitCmd, RUCI_INITIATE_ZIGBEE);

    enter_critical_section();
    rfb_send_cmd((uint8_t *)&sZigbeeInitCmd, RUCI_LEN_INITIATE_ZIGBEE);
    event_status = rfb_event_read(&event_len, (uint8_t *)&sCnfEvent);
    leave_critical_section();

    RUCI_ENDIAN_CONVERT((uint8_t *)&sCnfEvent, RUCI_CNF_EVENT);
    if (event_status != RF_MCU_RX_CMDQ_GET_SUCCESS)
    {
        return RFB_CNF_EVENT_NOT_AVAILABLE;
    }
    if (sCnfEvent.pci_cmd_subheader != RUCI_CODE_INITIATE_ZIGBEE)
    {
        return RFB_CNF_EVENT_CONTENT_ERROR;
    }
    if (sCnfEvent.status != RFB_EVENT_SUCCESS)
    {
        return (RFB_EVENT_STATUS)sCnfEvent.status;
    }

    return RFB_EVENT_SUCCESS;
}

RFB_EVENT_STATUS  rfb_comm_15p4_address_filter_set(uint8_t mac_promiscuous_mode, uint16_t short_source_address, uint32_t long_source_address_0, uint32_t long_source_address_1, uint16_t pan_id, uint8_t is_coordinator)
{
    ruci_para_set15p4_address_filter_t sAddressFielterSetCmd = {0};
    ruci_para_cnf_event_t sCnfEvent = {0};
    uint8_t event_len = 0;
    RF_MCU_RX_CMDQ_ERROR event_status = RF_MCU_RX_CMDQ_ERR_INIT;

    SET_RUCI_PARA_SET15P4_ADDRESS_FILTER(&sAddressFielterSetCmd, mac_promiscuous_mode, short_source_address, long_source_address_0,
                                         long_source_address_1, pan_id, is_coordinator);

    RUCI_ENDIAN_CONVERT((uint8_t *)&sAddressFielterSetCmd, RUCI_SET15P4_ADDRESS_FILTER);

    enter_critical_section();
    rfb_send_cmd((uint8_t *)&sAddressFielterSetCmd, RUCI_LEN_SET15P4_ADDRESS_FILTER);
    event_status = rfb_event_read(&event_len, (uint8_t *)&sCnfEvent);
    leave_critical_section();

    RUCI_ENDIAN_CONVERT((uint8_t *)&sCnfEvent, RUCI_CNF_EVENT);
    if (event_status != RF_MCU_RX_CMDQ_GET_SUCCESS)
    {
        return RFB_CNF_EVENT_NOT_AVAILABLE;
    }
    if (sCnfEvent.pci_cmd_subheader != RUCI_CODE_SET15P4_ADDRESS_FILTER)
    {
        return RFB_CNF_EVENT_CONTENT_ERROR;
    }
    if (sCnfEvent.status != RFB_EVENT_SUCCESS)
    {
        return (RFB_EVENT_STATUS)sCnfEvent.status;
    }

    return RFB_EVENT_SUCCESS;
}

RFB_EVENT_STATUS rfb_comm_15p4_mac_pib_set(uint32_t a_unit_backoff_period, uint32_t mac_ack_wait_duration, uint8_t mac_max_be, uint8_t mac_max_csma_backoffs,
        uint32_t mac_max_frame_total_wait_time, uint8_t mac_max_frame_retries, uint8_t mac_min_be)
{
    ruci_para_set15p4_mac_pib_t s15p4Mac = {0};
    ruci_para_cnf_event_t sCnfEvent = {0};
    uint8_t event_len = 0;
    RF_MCU_RX_CMDQ_ERROR event_status = RF_MCU_RX_CMDQ_ERR_INIT;

    SET_RUCI_PARA_SET15P4_MAC_PIB(&s15p4Mac, a_unit_backoff_period, mac_ack_wait_duration, mac_max_be, mac_max_csma_backoffs, mac_max_frame_total_wait_time,
                                  mac_max_frame_retries, mac_min_be);

    RUCI_ENDIAN_CONVERT((uint8_t *)&s15p4Mac, RUCI_SET15P4_SW_MAC);

    enter_critical_section();
    rfb_send_cmd((uint8_t *)&s15p4Mac, RUCI_LEN_SET15P4_MAC_PIB);
    event_status = rfb_event_read(&event_len, (uint8_t *)&sCnfEvent);
    leave_critical_section();

    RUCI_ENDIAN_CONVERT((uint8_t *)&sCnfEvent, RUCI_CNF_EVENT);
    if (event_status != RF_MCU_RX_CMDQ_GET_SUCCESS)
    {
        return RFB_CNF_EVENT_NOT_AVAILABLE;
    }
    if (sCnfEvent.pci_cmd_subheader != RUCI_CODE_SET15P4_MAC_PIB)
    {
        return RFB_CNF_EVENT_CONTENT_ERROR;
    }
    if (sCnfEvent.status != RFB_EVENT_SUCCESS)
    {
        return (RFB_EVENT_STATUS)sCnfEvent.status;
    }

    return RFB_EVENT_SUCCESS;
}

RFB_EVENT_STATUS rfb_comm_15p4_phy_pib_set(uint16_t a_turnaround_time, uint8_t phy_cca_mode, uint8_t phy_cca_threshold, uint16_t phy_cca_duration)
{
    ruci_para_set15p4_mac_pib_t s15p4Phy = {0};
    ruci_para_cnf_event_t sCnfEvent = {0};
    uint8_t event_len = 0;
    RF_MCU_RX_CMDQ_ERROR event_status = RF_MCU_RX_CMDQ_ERR_INIT;

    SET_RUCI_PARA_SET15P4_PHY_PIB(&s15p4Phy, a_turnaround_time, phy_cca_mode, phy_cca_threshold, phy_cca_duration);

    RUCI_ENDIAN_CONVERT((uint8_t *)&s15p4Phy, RUCI_SET15P4_PHY_PIB);

    enter_critical_section();
    rfb_send_cmd((uint8_t *)&s15p4Phy, RUCI_LEN_SET15P4_PHY_PIB);
    event_status = rfb_event_read(&event_len, (uint8_t *)&sCnfEvent);
    leave_critical_section();

    RUCI_ENDIAN_CONVERT((uint8_t *)&sCnfEvent, RUCI_CNF_EVENT);
    if (event_status != RF_MCU_RX_CMDQ_GET_SUCCESS)
    {
        return RFB_CNF_EVENT_NOT_AVAILABLE;
    }
    if (sCnfEvent.pci_cmd_subheader != RUCI_CODE_SET15P4_PHY_PIB)
    {
        return RFB_CNF_EVENT_CONTENT_ERROR;
    }
    if (sCnfEvent.status != RFB_EVENT_SUCCESS)
    {
        return (RFB_EVENT_STATUS)sCnfEvent.status;
    }

    return RFB_EVENT_SUCCESS;
}


RFB_EVENT_STATUS rfb_comm_15p4_auto_ack_set(uint8_t auto_ack_enable_flag)
{
    ruci_para_set15p4_auto_ack_t sAutoAck = {0};
    ruci_para_cnf_event_t sCnfEvent = {0};
    uint8_t event_len = 0;
    RF_MCU_RX_CMDQ_ERROR event_status = RF_MCU_RX_CMDQ_ERR_INIT;

    SET_RUCI_PARA_SET15P4_AUTO_ACK(&sAutoAck, auto_ack_enable_flag);

    RUCI_ENDIAN_CONVERT((uint8_t *)&sAutoAck, RUCI_SET15P4_AUTO_ACK);

    enter_critical_section();
    rfb_send_cmd((uint8_t *)&sAutoAck, RUCI_LEN_SET15P4_AUTO_ACK);
    event_status = rfb_event_read(&event_len, (uint8_t *)&sCnfEvent);
    leave_critical_section();

    RUCI_ENDIAN_CONVERT((uint8_t *)&sCnfEvent, RUCI_CNF_EVENT);
    if (event_status != RF_MCU_RX_CMDQ_GET_SUCCESS)
    {
        return RFB_CNF_EVENT_NOT_AVAILABLE;
    }
    if (sCnfEvent.pci_cmd_subheader != RUCI_CODE_SET15P4_AUTO_ACK)
    {
        return RFB_CNF_EVENT_CONTENT_ERROR;
    }
    if (sCnfEvent.status != RFB_EVENT_SUCCESS)
    {
        return (RFB_EVENT_STATUS)sCnfEvent.status;
    }

    return RFB_EVENT_SUCCESS;
}



RFB_EVENT_STATUS rfb_comm_15p4_pending_bit_set(uint8_t frame_pending_bit)
{
    ruci_para_set15p4_pending_bit_t sFramePending = {0};
    ruci_para_cnf_event_t sCnfEvent = {0};
    uint8_t event_len = 0;
    RF_MCU_RX_CMDQ_ERROR event_status = RF_MCU_RX_CMDQ_ERR_INIT;

    SET_RUCI_PARA_SET15P4_PENDING_BIT(&sFramePending, frame_pending_bit);

    RUCI_ENDIAN_CONVERT((uint8_t *)&sFramePending, RUCI_SET15P4_PENDING_BIT);

    enter_critical_section();
    rfb_send_cmd((uint8_t *)&sFramePending, RUCI_LEN_SET15P4_PENDING_BIT);
    event_status = rfb_event_read(&event_len, (uint8_t *)&sCnfEvent);
    leave_critical_section();

    RUCI_ENDIAN_CONVERT((uint8_t *)&sCnfEvent, RUCI_CNF_EVENT);
    if (event_status != RF_MCU_RX_CMDQ_GET_SUCCESS)
    {
        return RFB_CNF_EVENT_NOT_AVAILABLE;
    }
    if (sCnfEvent.pci_cmd_subheader != RUCI_CODE_SET15P4_PENDING_BIT)
    {
        return RFB_CNF_EVENT_CONTENT_ERROR;
    }
    if (sCnfEvent.status != RFB_EVENT_SUCCESS)
    {
        return (RFB_EVENT_STATUS)sCnfEvent.status;
    }

    return RFB_EVENT_SUCCESS;
}

RFB_EVENT_STATUS rfb_comm_15p4_src_match_ctrl(uint8_t enable)
{
    ruci_para_set_src_match_enable_t sSrcMatEn = {0};
    ruci_para_cnf_event_t sCnfEvent = {0};
    uint8_t event_len = 0;
    RF_MCU_RX_CMDQ_ERROR event_status = RF_MCU_RX_CMDQ_ERR_INIT;

    SET_RUCI_PARA_SET_SRC_MATCH_ENABLE(&sSrcMatEn, enable);

    RUCI_ENDIAN_CONVERT((uint8_t *)&sSrcMatEn, RUCI_SET_SRC_MATCH_ENABLE);

    enter_critical_section();
    rfb_send_cmd((uint8_t *)&sSrcMatEn, RUCI_LEN_SET_SRC_MATCH_ENABLE);
    event_status = rfb_event_read(&event_len, (uint8_t *)&sCnfEvent);
    leave_critical_section();

    RUCI_ENDIAN_CONVERT((uint8_t *)&sCnfEvent, RUCI_CNF_EVENT);
    if (event_status != RF_MCU_RX_CMDQ_GET_SUCCESS)
    {
        return RFB_CNF_EVENT_NOT_AVAILABLE;
    }
    if (sCnfEvent.pci_cmd_subheader != RUCI_CODE_SET_SRC_MATCH_ENABLE)
    {
        return RFB_CNF_EVENT_CONTENT_ERROR;
    }
    if (sCnfEvent.status != RFB_EVENT_SUCCESS)
    {
        return (RFB_EVENT_STATUS)sCnfEvent.status;
    }

    return RFB_EVENT_SUCCESS;
}

RFB_EVENT_STATUS rfb_comm_15p4_src_match_short_entry(uint8_t control_type, uint8_t *short_addr)
{
    ruci_para_set_src_match_short_entry_ctrl_t sSrcMatShortEntry = {0};
    ruci_para_cnf_event_t sCnfEvent = {0};
    uint8_t event_len = 0;
    RF_MCU_RX_CMDQ_ERROR event_status = RF_MCU_RX_CMDQ_ERR_INIT;

    SET_RUCI_PARA_SET_SRC_MATCH_SHORT_ENTRY_CTRL(&sSrcMatShortEntry, control_type, *short_addr, *(short_addr + 1));

    RUCI_ENDIAN_CONVERT((uint8_t *)&sSrcMatShortEntry, RUCI_SET_SRC_MATCH_SHORT_ENTRY_CTRL);

    enter_critical_section();
    rfb_send_cmd((uint8_t *)&sSrcMatShortEntry, RUCI_LEN_SET_SRC_MATCH_SHORT_ENTRY_CTRL);
    event_status = rfb_event_read(&event_len, (uint8_t *)&sCnfEvent);
    leave_critical_section();

    RUCI_ENDIAN_CONVERT((uint8_t *)&sCnfEvent, RUCI_CNF_EVENT);
    if (event_status != RF_MCU_RX_CMDQ_GET_SUCCESS)
    {
        return RFB_CNF_EVENT_NOT_AVAILABLE;
    }
    if (sCnfEvent.pci_cmd_subheader != RUCI_CODE_SET_SRC_MATCH_SHORT_ENTRY_CTRL)
    {
        return RFB_CNF_EVENT_CONTENT_ERROR;
    }
    if (sCnfEvent.status != RFB_EVENT_SUCCESS)
    {
        return (RFB_EVENT_STATUS)sCnfEvent.status;
    }

    return RFB_EVENT_SUCCESS;
}

RFB_EVENT_STATUS rfb_comm_15p4_src_match_extended_entry(uint8_t control_type, uint8_t *extended_addr)
{
    ruci_para_set_src_match_extended_entry_ctrl_t sSrcMatExtendEntry = {0};
    ruci_para_cnf_event_t sCnfEvent = {0};
    uint8_t event_len = 0;
    RF_MCU_RX_CMDQ_ERROR event_status = RF_MCU_RX_CMDQ_ERR_INIT;

    SET_RUCI_PARA_SET_SRC_MATCH_EXTENDED_ENTRY_CTRL(&sSrcMatExtendEntry, control_type
            , *extended_addr, *(extended_addr + 1), *(extended_addr + 2), *(extended_addr + 3), *(extended_addr + 4)
            , *(extended_addr + 5), *(extended_addr + 6), *(extended_addr + 7));

    RUCI_ENDIAN_CONVERT((uint8_t *)&sSrcMatExtendEntry, RUCI_SET_SRC_MATCH_EXTENDED_ENTRY_CTRL);

    enter_critical_section();
    rfb_send_cmd((uint8_t *)&sSrcMatExtendEntry, RUCI_LEN_SET_SRC_MATCH_EXTENDED_ENTRY_CTRL);
    event_status = rfb_event_read(&event_len, (uint8_t *)&sCnfEvent);
    leave_critical_section();

    RUCI_ENDIAN_CONVERT((uint8_t *)&sCnfEvent, RUCI_CNF_EVENT);
    if (event_status != RF_MCU_RX_CMDQ_GET_SUCCESS)
    {
        return RFB_CNF_EVENT_NOT_AVAILABLE;
    }
    if (sCnfEvent.pci_cmd_subheader != RUCI_CODE_SET_SRC_MATCH_EXTENDED_ENTRY_CTRL)
    {
        return RFB_CNF_EVENT_CONTENT_ERROR;
    }
    if (sCnfEvent.status != RFB_EVENT_SUCCESS)
    {
        return (RFB_EVENT_STATUS)sCnfEvent.status;
    }

    return RFB_EVENT_SUCCESS;
}

RFB_EVENT_STATUS rfb_comm_15p4_csl_receiver_ctrl(uint8_t csl_receiver_ctrl, uint16_t csl_period)
{
    ruci_para_set_csl_receiver_ctrl_t sCslReceiverCtrl = {0};
    ruci_para_cnf_event_t sCnfEvent = {0};
    uint8_t event_len = 0;
    RF_MCU_RX_CMDQ_ERROR event_status = RF_MCU_RX_CMDQ_ERR_INIT;

    SET_RUCI_PARA_SET_CSL_RECEIVER_CTRL(&sCslReceiverCtrl, csl_receiver_ctrl, csl_period);

    RUCI_ENDIAN_CONVERT((uint8_t *)&sCslReceiverCtrl, RUCI_SET_CSL_RECEIVER_CTRL);

    enter_critical_section();
    rfb_send_cmd((uint8_t *)&sCslReceiverCtrl, RUCI_LEN_SET_CSL_RECEIVER_CTRL);
    event_status = rfb_event_read(&event_len, (uint8_t *)&sCnfEvent);
    leave_critical_section();

    RUCI_ENDIAN_CONVERT((uint8_t *)&sCnfEvent, RUCI_CNF_EVENT);
    if (event_status != RF_MCU_RX_CMDQ_GET_SUCCESS)
    {
        return RFB_CNF_EVENT_NOT_AVAILABLE;
    }
    if (sCnfEvent.pci_cmd_subheader != RUCI_CODE_SET_CSL_RECEIVER_CTRL)
    {
        return RFB_CNF_EVENT_CONTENT_ERROR;
    }
    if (sCnfEvent.status != RFB_EVENT_SUCCESS)
    {
        return (RFB_EVENT_STATUS)sCnfEvent.status;
    }

    return RFB_EVENT_SUCCESS;
}

RFB_EVENT_STATUS rfb_comm_15p4_csl_accuracy_get(uint8_t *csl_accuracy)
{
    ruci_para_get_csl_accuracy_t sGetCslAccuracyCmd = {0};
    ruci_para_get_csl_accuracy_event_t sGetCslAccuracyEvent = {0};
    ruci_para_cnf_event_t sCnfEvent = {0};
    uint8_t event_len = 0;
    RF_MCU_RX_CMDQ_ERROR event_status = RF_MCU_RX_CMDQ_ERR_INIT;
    uint8_t get_csl_accuracy_event_len = 0;
    RF_MCU_RX_CMDQ_ERROR get_rssi_event_status = RF_MCU_RX_CMDQ_ERR_INIT;

    SET_RUCI_PARA_GET_CSL_ACCURACY(&sGetCslAccuracyCmd);

    RUCI_ENDIAN_CONVERT((uint8_t *)&sGetCslAccuracyCmd, RUCI_GET_CSL_ACCURACY);

    enter_critical_section();
    rfb_send_cmd((uint8_t *)&sGetCslAccuracyCmd, RUCI_LEN_GET_CSL_ACCURACY);
    event_status = rfb_event_read(&event_len, (uint8_t *)&sCnfEvent);
    get_rssi_event_status = rfb_event_read(&get_csl_accuracy_event_len, (uint8_t *)&sGetCslAccuracyEvent);
    leave_critical_section();

    RUCI_ENDIAN_CONVERT((uint8_t *)&sCnfEvent, RUCI_CNF_EVENT);
    if (event_status != RF_MCU_RX_CMDQ_GET_SUCCESS)
    {
        return RFB_CNF_EVENT_NOT_AVAILABLE;
    }
    if (sCnfEvent.pci_cmd_subheader != RUCI_CODE_GET_CSL_ACCURACY)
    {
        return RFB_CNF_EVENT_CONTENT_ERROR;
    }
    if (sCnfEvent.status != RFB_EVENT_SUCCESS)
    {
        return (RFB_EVENT_STATUS)sCnfEvent.status;
    }

    RUCI_ENDIAN_CONVERT((uint8_t *)&sGetCslAccuracyEvent, RUCI_GET_CSL_ACCURACY_EVENT);
    if (get_rssi_event_status != RF_MCU_RX_CMDQ_GET_SUCCESS)
    {
        return RFB_RSP_EVENT_NOT_AVAILABLE;
    }
    if (sGetCslAccuracyEvent.sub_header != RUCI_CODE_GET_CSL_ACCURACY_EVENT)
    {
        return RFB_RSP_EVENT_CONTENT_ERROR;
    }
    *csl_accuracy = sGetCslAccuracyEvent.get_csl_accuracy;

    return RFB_EVENT_SUCCESS;
}

RFB_EVENT_STATUS rfb_comm_15p4_csl_uncertainty_get(uint8_t *csl_uncertainty)
{
    ruci_para_get_csl_uncertainty_t sGetCslUncertaintyCmd = {0};
    ruci_para_get_csl_uncertainty_event_t sGetCslUncertaintyEvent = {0};
    ruci_para_cnf_event_t sCnfEvent = {0};
    uint8_t event_len = 0;
    RF_MCU_RX_CMDQ_ERROR event_status = RF_MCU_RX_CMDQ_ERR_INIT;
    uint8_t get_csl_uncertainty_event_len = 0;
    RF_MCU_RX_CMDQ_ERROR get_rssi_event_status = RF_MCU_RX_CMDQ_ERR_INIT;

    SET_RUCI_PARA_GET_CSL_UNCERTAINTY(&sGetCslUncertaintyCmd);

    RUCI_ENDIAN_CONVERT((uint8_t *)&sGetCslUncertaintyCmd, RUCI_GET_CSL_UNCERTAINTY);

    enter_critical_section();
    rfb_send_cmd((uint8_t *)&sGetCslUncertaintyCmd, RUCI_LEN_GET_CSL_UNCERTAINTY);
    event_status = rfb_event_read(&event_len, (uint8_t *)&sCnfEvent);
    get_rssi_event_status = rfb_event_read(&get_csl_uncertainty_event_len, (uint8_t *)&sGetCslUncertaintyEvent);
    leave_critical_section();

    RUCI_ENDIAN_CONVERT((uint8_t *)&sCnfEvent, RUCI_CNF_EVENT);
    if (event_status != RF_MCU_RX_CMDQ_GET_SUCCESS)
    {
        return RFB_CNF_EVENT_NOT_AVAILABLE;
    }
    if (sCnfEvent.pci_cmd_subheader != RUCI_CODE_GET_CSL_UNCERTAINTY)
    {
        return RFB_CNF_EVENT_CONTENT_ERROR;
    }
    if (sCnfEvent.status != RFB_EVENT_SUCCESS)
    {
        return (RFB_EVENT_STATUS)sCnfEvent.status;
    }

    RUCI_ENDIAN_CONVERT((uint8_t *)&sGetCslAccuracyEvent, RUCI_GET_CSL_UNCERTAINTY_EVENT);
    if (get_rssi_event_status != RF_MCU_RX_CMDQ_GET_SUCCESS)
    {
        return RFB_RSP_EVENT_NOT_AVAILABLE;
    }
    if (sGetCslUncertaintyEvent.sub_header != RUCI_CODE_GET_CSL_UNCERTAINTY_EVENT)
    {
        return RFB_RSP_EVENT_CONTENT_ERROR;
    }
    *csl_uncertainty = sGetCslUncertaintyEvent.csl_uncertainty;

    return RFB_EVENT_SUCCESS;
}

RFB_EVENT_STATUS rfb_comm_15p4_csl_sample_time_update(uint32_t csl_sample_time)
{
    ruci_para_update_csl_sample_time_t sCslSampleTimeUpdate = {0};
    ruci_para_cnf_event_t sCnfEvent = {0};
    uint8_t event_len = 0;
    RF_MCU_RX_CMDQ_ERROR event_status = RF_MCU_RX_CMDQ_ERR_INIT;

    SET_RUCI_PARA_UPDATE_CSL_SAMPLE_TIME(&sCslSampleTimeUpdate, csl_sample_time);

    RUCI_ENDIAN_CONVERT((uint8_t *)&sCslSampleTimeUpdate, RUCI_UPDATE_CSL_SAMPLE_TIME);

    enter_critical_section();
    rfb_send_cmd((uint8_t *)&sCslSampleTimeUpdate, RUCI_LEN_UPDATE_CSL_SAMPLE_TIME);
    event_status = rfb_event_read(&event_len, (uint8_t *)&sCnfEvent);
    leave_critical_section();

    RUCI_ENDIAN_CONVERT((uint8_t *)&sCnfEvent, RUCI_CNF_EVENT);
    if (event_status != RF_MCU_RX_CMDQ_GET_SUCCESS)
    {
        return RFB_CNF_EVENT_NOT_AVAILABLE;
    }
    if (sCnfEvent.pci_cmd_subheader != RUCI_CODE_UPDATE_CSL_SAMPLE_TIME)
    {
        return RFB_CNF_EVENT_CONTENT_ERROR;
    }
    if (sCnfEvent.status != RFB_EVENT_SUCCESS)
    {
        return (RFB_EVENT_STATUS)sCnfEvent.status;
    }

    return RFB_EVENT_SUCCESS;
}
