/**
 * @file rfb_comm_sub.c
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
#include "rfb_comm.h"
#include "rfb.h"
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
/* FSK mode */
RFB_EVENT_STATUS rfb_comm_fsk_initiate(uint8_t band_type)
{
    ruci_para_initiate_fsk_t sFskInitCmd = {0};
    ruci_para_cnf_event_t sCnfEvent = {0};
    uint8_t event_len = 0;
    RF_MCU_RX_CMDQ_ERROR event_status = RF_MCU_RX_CMDQ_ERR_INIT;

    SET_RUCI_PARA_INITIATE_FSK(&sFskInitCmd, band_type);

    RUCI_ENDIAN_CONVERT((uint8_t *)&sGfskInitCmd, RUCI_INITIATE_FSK);

    enter_critical_section();
    rfb_send_cmd((uint8_t *)&sFskInitCmd, RUCI_LEN_INITIATE_FSK);
    event_status = rfb_event_read(&event_len, (uint8_t *)&sCnfEvent);
    leave_critical_section();

    RUCI_ENDIAN_CONVERT((uint8_t *)&sCnfEvent, RUCI_CNF_EVENT);
    if (event_status != RF_MCU_RX_CMDQ_GET_SUCCESS)
    {
        return RFB_CNF_EVENT_NOT_AVAILABLE;
    }
    if (sCnfEvent.pci_cmd_subheader != RUCI_CODE_INITIATE_FSK)
    {
        return RFB_CNF_EVENT_CONTENT_ERROR;
    }
    if (sCnfEvent.status != RFB_EVENT_SUCCESS)
    {
        return (RFB_EVENT_STATUS)sCnfEvent.status;
    }
    return RFB_EVENT_SUCCESS;
}

RFB_EVENT_STATUS rfb_comm_fsk_modem_set(uint8_t data_rate, uint8_t modulation_index)
{
    ruci_para_set_fsk_modem_t sFskInitCmd = {0};
    ruci_para_cnf_event_t sCnfEvent = {0};
    uint8_t event_len = 0;
    RF_MCU_RX_CMDQ_ERROR event_status = RF_MCU_RX_CMDQ_ERR_INIT;

    SET_RUCI_PARA_SET_FSK_MODEM(&sFskInitCmd, data_rate, modulation_index);

    RUCI_ENDIAN_CONVERT((uint8_t *)&sFskInitCmd, RUCI_SET_FSK_MODEM);

    enter_critical_section();
    rfb_send_cmd((uint8_t *)&sFskInitCmd, RUCI_LEN_SET_FSK_MODEM);
    event_status = rfb_event_read(&event_len, (uint8_t *)&sCnfEvent);
    leave_critical_section();

    RUCI_ENDIAN_CONVERT((uint8_t *)&sCnfEvent, RUCI_CNF_EVENT);
    if (event_status != RF_MCU_RX_CMDQ_GET_SUCCESS)
    {
        return RFB_CNF_EVENT_NOT_AVAILABLE;
    }
    if (sCnfEvent.pci_cmd_subheader != RUCI_CODE_SET_FSK_MODEM)
    {
        return RFB_CNF_EVENT_CONTENT_ERROR;
    }
    if (sCnfEvent.status != RFB_EVENT_SUCCESS)
    {
        return (RFB_EVENT_STATUS)sCnfEvent.status;
    }
    return RFB_EVENT_SUCCESS;
}

RFB_EVENT_STATUS rfb_comm_fsk_mac_set(uint8_t crc_type, uint8_t whitening_enable)
{
    ruci_para_set_fsk_mac_t sFskMacSetCmd = {0};
    ruci_para_cnf_event_t sCnfEvent = {0};
    uint8_t event_len = 0;
    RF_MCU_RX_CMDQ_ERROR event_status = RF_MCU_RX_CMDQ_ERR_INIT;

    SET_RUCI_PARA_SET_FSK_MAC(&sFskMacSetCmd, crc_type, whitening_enable);

    RUCI_ENDIAN_CONVERT((uint8_t *)&sGfskMacSetCmd, RUCI_SET_FSK_MAC);

    enter_critical_section();
    rfb_send_cmd((uint8_t *)&sFskMacSetCmd, RUCI_LEN_SET_FSK_MAC);
    event_status = rfb_event_read(&event_len, (uint8_t *)&sCnfEvent);
    leave_critical_section();

    RUCI_ENDIAN_CONVERT((uint8_t *)&sCnfEvent, RUCI_CNF_EVENT);
    if (event_status != RF_MCU_RX_CMDQ_GET_SUCCESS)
    {
        return RFB_CNF_EVENT_NOT_AVAILABLE;
    }
    if (sCnfEvent.pci_cmd_subheader != RUCI_CODE_SET_FSK_MAC)
    {
        return RFB_CNF_EVENT_CONTENT_ERROR;
    }
    if (sCnfEvent.status != RFB_EVENT_SUCCESS)
    {
        return (RFB_EVENT_STATUS)sCnfEvent.status;
    }
    return RFB_EVENT_SUCCESS;
}

RFB_EVENT_STATUS rfb_comm_fsk_preamble_set(uint8_t preamble_length)
{
    ruci_para_set_fsk_preamble_t sFskPreambleSetCmd = {0};
    ruci_para_cnf_event_t sCnfEvent = {0};
    uint8_t event_len = 0;
    RF_MCU_RX_CMDQ_ERROR event_status = RF_MCU_RX_CMDQ_ERR_INIT;

    SET_RUCI_PARA_SET_FSK_PREAMBLE(&sFskPreambleSetCmd, preamble_length);

    RUCI_ENDIAN_CONVERT((uint8_t *)&sGfskPreambleSetCmd, RUCI_SET_FSK_PREAMBLE);

    enter_critical_section();
    rfb_send_cmd((uint8_t *)&sFskPreambleSetCmd, RUCI_LEN_SET_FSK_PREAMBLE);
    event_status = rfb_event_read(&event_len, (uint8_t *)&sCnfEvent);
    leave_critical_section();

    RUCI_ENDIAN_CONVERT((uint8_t *)&sCnfEvent, RUCI_CNF_EVENT);
    if (event_status != RF_MCU_RX_CMDQ_GET_SUCCESS)
    {
        return RFB_CNF_EVENT_NOT_AVAILABLE;
    }
    if (sCnfEvent.pci_cmd_subheader != RUCI_CODE_SET_FSK_PREAMBLE)
    {
        return RFB_CNF_EVENT_CONTENT_ERROR;
    }
    if (sCnfEvent.status != RFB_EVENT_SUCCESS)
    {
        return (RFB_EVENT_STATUS)sCnfEvent.status;
    }
    return RFB_EVENT_SUCCESS;
}

RFB_EVENT_STATUS rfb_comm_fsk_sfd_set(uint32_t sfd_content)
{
    ruci_para_set_fsk_sfd_t sFskSfdSetCmd = {0};
    ruci_para_cnf_event_t sCnfEvent = {0};
    uint8_t event_len = 0;
    RF_MCU_RX_CMDQ_ERROR event_status = RF_MCU_RX_CMDQ_ERR_INIT;

    SET_RUCI_PARA_SET_FSK_SFD(&sFskSfdSetCmd, sfd_content)

    RUCI_ENDIAN_CONVERT((uint8_t *)&sFskSfdSetCmd, RUCI_SET_FSK_SFD);

    enter_critical_section();
    rfb_send_cmd((uint8_t *)&sFskSfdSetCmd, RUCI_LEN_SET_FSK_SFD);
    event_status = rfb_event_read(&event_len, (uint8_t *)&sCnfEvent);
    leave_critical_section();

    RUCI_ENDIAN_CONVERT((uint8_t *)&sCnfEvent, RUCI_CNF_EVENT);
    if (event_status != RF_MCU_RX_CMDQ_GET_SUCCESS)
    {
        return RFB_CNF_EVENT_NOT_AVAILABLE;
    }
    if (sCnfEvent.pci_cmd_subheader != RUCI_CODE_SET_FSK_SFD)
    {
        return RFB_CNF_EVENT_CONTENT_ERROR;
    }
    if (sCnfEvent.status != RFB_EVENT_SUCCESS)
    {
        return (RFB_EVENT_STATUS)sCnfEvent.status;
    }
    return RFB_EVENT_SUCCESS;
}

RFB_EVENT_STATUS rfb_comm_fsk_type_set(uint8_t filter_type)
{
    ruci_para_set_fsk_type_t sfskTypeSetCmd = {0};
    ruci_para_cnf_event_t sCnfEvent = {0};
    uint8_t event_len = 0;
    RF_MCU_RX_CMDQ_ERROR event_status = RF_MCU_RX_CMDQ_ERR_INIT;

    SET_RUCI_PARA_SET_FSK_TYPE(&sfskTypeSetCmd, filter_type)

    RUCI_ENDIAN_CONVERT((uint8_t *)&sfskTypeSetCmd, RUCI_SET_FSK_TYPE);

    enter_critical_section();
    rfb_send_cmd((uint8_t *)&sfskTypeSetCmd, RUCI_LEN_SET_FSK_TYPE);
    event_status = rfb_event_read(&event_len, (uint8_t *)&sCnfEvent);
    leave_critical_section();

    RUCI_ENDIAN_CONVERT((uint8_t *)&sCnfEvent, RUCI_CNF_EVENT);
    if (event_status != RF_MCU_RX_CMDQ_GET_SUCCESS)
    {
        return RFB_CNF_EVENT_NOT_AVAILABLE;
    }
    if (sCnfEvent.pci_cmd_subheader != RUCI_CODE_SET_FSK_TYPE)
    {
        return RFB_CNF_EVENT_CONTENT_ERROR;
    }
    if (sCnfEvent.status != RFB_EVENT_SUCCESS)
    {
        return (RFB_EVENT_STATUS)sCnfEvent.status;
    }
    return RFB_EVENT_SUCCESS;
}
/* FSK mode end */

/* OQPSK mode */
RFB_EVENT_STATUS rfb_comm_oqpsk_initiate(uint8_t band_type)
{
    ruci_para_initiate_oqpsk_t sOqpskInitCmd = {0};
    ruci_para_cnf_event_t sCnfEvent = {0};
    uint8_t event_len = 0;
    RF_MCU_RX_CMDQ_ERROR event_status = RF_MCU_RX_CMDQ_ERR_INIT;

    SET_RUCI_PARA_INITIATE_OQPSK(&sOqpskInitCmd, band_type);

    RUCI_ENDIAN_CONVERT((uint8_t *)&sGoqpskInitCmd, RUCI_INITIATE_OQPSK);

    enter_critical_section();
    rfb_send_cmd((uint8_t *)&sOqpskInitCmd, RUCI_LEN_INITIATE_OQPSK);
    event_status = rfb_event_read(&event_len, (uint8_t *)&sCnfEvent);
    leave_critical_section();

    RUCI_ENDIAN_CONVERT((uint8_t *)&sCnfEvent, RUCI_CNF_EVENT);
    if (event_status != RF_MCU_RX_CMDQ_GET_SUCCESS)
    {
        return RFB_CNF_EVENT_NOT_AVAILABLE;
    }
    if (sCnfEvent.pci_cmd_subheader != RUCI_CODE_INITIATE_OQPSK)
    {
        return RFB_CNF_EVENT_CONTENT_ERROR;
    }
    if (sCnfEvent.status != RFB_EVENT_SUCCESS)
    {
        return (RFB_EVENT_STATUS)sCnfEvent.status;
    }
    return RFB_EVENT_SUCCESS;
}

RFB_EVENT_STATUS rfb_comm_oqpsk_modem_set(uint8_t data_rate)
{
    ruci_para_set_oqpsk_modem_t sOqpskInitCmd = {0};
    ruci_para_cnf_event_t sCnfEvent = {0};
    uint8_t event_len = 0;
    RF_MCU_RX_CMDQ_ERROR event_status = RF_MCU_RX_CMDQ_ERR_INIT;

    SET_RUCI_PARA_SET_OQPSK_MODEM(&sOqpskInitCmd, data_rate);

    RUCI_ENDIAN_CONVERT((uint8_t *)&sGoqpskInitCmd, RUCI_SET_OQPSK_MODEM);

    enter_critical_section();
    rfb_send_cmd((uint8_t *)&sOqpskInitCmd, RUCI_LEN_SET_OQPSK_MODEM);
    event_status = rfb_event_read(&event_len, (uint8_t *)&sCnfEvent);
    leave_critical_section();

    RUCI_ENDIAN_CONVERT((uint8_t *)&sCnfEvent, RUCI_CNF_EVENT);
    if (event_status != RF_MCU_RX_CMDQ_GET_SUCCESS)
    {
        return RFB_CNF_EVENT_NOT_AVAILABLE;
    }
    if (sCnfEvent.pci_cmd_subheader != RUCI_CODE_SET_OQPSK_MODEM)
    {
        return RFB_CNF_EVENT_CONTENT_ERROR;
    }
    if (sCnfEvent.status != RFB_EVENT_SUCCESS)
    {
        return (RFB_EVENT_STATUS)sCnfEvent.status;
    }
    return RFB_EVENT_SUCCESS;
}

RFB_EVENT_STATUS rfb_comm_oqpsk_mac_set(uint8_t crc_type, uint8_t whitening_enable)
{
    ruci_para_set_oqpsk_mac_t sOqpskMacSetCmd = {0};
    ruci_para_cnf_event_t sCnfEvent = {0};
    uint8_t event_len = 0;
    RF_MCU_RX_CMDQ_ERROR event_status = RF_MCU_RX_CMDQ_ERR_INIT;

    SET_RUCI_PARA_SET_OQPSK_MAC(&sOqpskMacSetCmd, crc_type, whitening_enable);

    RUCI_ENDIAN_CONVERT((uint8_t *)&sGoqpskMacSetCmd, RUCI_SET_OQPSK_MAC);

    enter_critical_section();
    rfb_send_cmd((uint8_t *)&sOqpskMacSetCmd, RUCI_LEN_SET_OQPSK_MAC);
    event_status = rfb_event_read(&event_len, (uint8_t *)&sCnfEvent);
    leave_critical_section();

    RUCI_ENDIAN_CONVERT((uint8_t *)&sCnfEvent, RUCI_CNF_EVENT);
    if (event_status != RF_MCU_RX_CMDQ_GET_SUCCESS)
    {
        return RFB_CNF_EVENT_NOT_AVAILABLE;
    }
    if (sCnfEvent.pci_cmd_subheader != RUCI_CODE_SET_OQPSK_MAC)
    {
        return RFB_CNF_EVENT_CONTENT_ERROR;
    }
    if (sCnfEvent.status != RFB_EVENT_SUCCESS)
    {
        return (RFB_EVENT_STATUS)sCnfEvent.status;
    }
    return RFB_EVENT_SUCCESS;
}

RFB_EVENT_STATUS rfb_comm_oqpsk_preamble_set(uint8_t preamble_length)
{
    ruci_para_set_oqpsk_extra_preamble_t sOqpskPreambleSetCmd = {0};
    ruci_para_cnf_event_t sCnfEvent = {0};
    uint8_t event_len = 0;
    RF_MCU_RX_CMDQ_ERROR event_status = RF_MCU_RX_CMDQ_ERR_INIT;

    SET_RUCI_PARA_SET_OQPSK_EXTRA_PREAMBLE(&sOqpskPreambleSetCmd, preamble_length);

    RUCI_ENDIAN_CONVERT((uint8_t *)&sGoqpskPreambleSetCmd, RUCI_SET_OQPSK_PREAMBLE);

    enter_critical_section();
    rfb_send_cmd((uint8_t *)&sOqpskPreambleSetCmd, RUCI_LEN_SET_OQPSK_EXTRA_PREAMBLE);
    event_status = rfb_event_read(&event_len, (uint8_t *)&sCnfEvent);
    leave_critical_section();

    RUCI_ENDIAN_CONVERT((uint8_t *)&sCnfEvent, RUCI_CNF_EVENT);
    if (event_status != RF_MCU_RX_CMDQ_GET_SUCCESS)
    {
        return RFB_CNF_EVENT_NOT_AVAILABLE;
    }
    if (sCnfEvent.pci_cmd_subheader != RUCI_CODE_SET_OQPSK_EXTRA_PREAMBLE)
    {
        return RFB_CNF_EVENT_CONTENT_ERROR;
    }
    if (sCnfEvent.status != RFB_EVENT_SUCCESS)
    {
        return (RFB_EVENT_STATUS)sCnfEvent.status;
    }
    return RFB_EVENT_SUCCESS;
}

/* OQPSK mode end */



/* ZWAVE */

RFB_EVENT_STATUS rfb_comm_zwave_initiate(uint8_t band_type)
{
    ruci_para_initiate_zwave_t sZwaveInitCmd = {0};
    ruci_para_cnf_event_t sCnfEvent = {0};
    uint8_t event_len = 0;
    RF_MCU_RX_CMDQ_ERROR event_status = RF_MCU_RX_CMDQ_ERR_INIT;

    SET_RUCI_PARA_INITIATE_ZWAVE(&sZwaveInitCmd, band_type);

    RUCI_ENDIAN_CONVERT((uint8_t *)&sZwaveInitCmd, RUCI_INITIATE_ZWAVE);

    enter_critical_section();
    rfb_send_cmd((uint8_t *)&sZwaveInitCmd, RUCI_LEN_INITIATE_ZWAVE);
    event_status = rfb_event_read(&event_len, (uint8_t *)&sCnfEvent);
    leave_critical_section();

    RUCI_ENDIAN_CONVERT((uint8_t *)&sCnfEvent, RUCI_CNF_EVENT);
    if (event_status != RF_MCU_RX_CMDQ_GET_SUCCESS)
    {
        return RFB_CNF_EVENT_NOT_AVAILABLE;
    }
    if (sCnfEvent.pci_cmd_subheader != RUCI_CODE_INITIATE_ZWAVE)
    {
        return RFB_CNF_EVENT_CONTENT_ERROR;
    }
    if (sCnfEvent.status != RFB_EVENT_SUCCESS)
    {
        return (RFB_EVENT_STATUS)sCnfEvent.status;
    }
    return RFB_EVENT_SUCCESS;
}

RFB_EVENT_STATUS rfb_comm_zwave_modem_set(uint8_t data_rate)
{
    ruci_para_set_zwave_modem_t sZwaveInitCmd = {0};
    ruci_para_cnf_event_t sCnfEvent = {0};
    uint8_t event_len = 0;
    RF_MCU_RX_CMDQ_ERROR event_status = RF_MCU_RX_CMDQ_ERR_INIT;

    SET_RUCI_PARA_SET_ZWAVE_MODEM(&sZwaveInitCmd, data_rate);

    RUCI_ENDIAN_CONVERT((uint8_t *)&sZwaveInitCmd, RUCI_SET_ZWAVE_MODEM);

    enter_critical_section();
    rfb_send_cmd((uint8_t *)&sZwaveInitCmd, RUCI_LEN_SET_ZWAVE_MODEM);
    event_status = rfb_event_read(&event_len, (uint8_t *)&sCnfEvent);
    leave_critical_section();

    RUCI_ENDIAN_CONVERT((uint8_t *)&sCnfEvent, RUCI_CNF_EVENT);
    if (event_status != RF_MCU_RX_CMDQ_GET_SUCCESS)
    {
        return RFB_CNF_EVENT_NOT_AVAILABLE;
    }
    if (sCnfEvent.pci_cmd_subheader != RUCI_CODE_SET_ZWAVE_MODEM)
    {
        return RFB_CNF_EVENT_CONTENT_ERROR;
    }
    if (sCnfEvent.status != RFB_EVENT_SUCCESS)
    {
        return (RFB_EVENT_STATUS)sCnfEvent.status;
    }
    return RFB_EVENT_SUCCESS;
}

RFB_EVENT_STATUS rfb_comm_zwave_scan_set(uint8_t mode, uint8_t region)
{
    ruci_para_set_zwave_scan_t sZwaveInitCmd = {0};
    ruci_para_cnf_event_t sCnfEvent = {0};
    uint8_t event_len = 0;
    RF_MCU_RX_CMDQ_ERROR event_status = RF_MCU_RX_CMDQ_ERR_INIT;

    SET_RUCI_PARA_SET_ZWAVE_SCAN(&sZwaveInitCmd, mode, region);

    RUCI_ENDIAN_CONVERT((uint8_t *)&sZwaveInitCmd, RUCI_SET_ZWAVE_SCAN);

    enter_critical_section();
    rfb_send_cmd((uint8_t *)&sZwaveInitCmd, RUCI_LEN_SET_ZWAVE_SCAN);
    event_status = rfb_event_read(&event_len, (uint8_t *)&sCnfEvent);
    leave_critical_section();

    RUCI_ENDIAN_CONVERT((uint8_t *)&sCnfEvent, RUCI_CNF_EVENT);
    if (event_status != RF_MCU_RX_CMDQ_GET_SUCCESS)
    {
        return RFB_CNF_EVENT_NOT_AVAILABLE;
    }
    if (sCnfEvent.pci_cmd_subheader != RUCI_CODE_SET_ZWAVE_SCAN)
    {
        return RFB_CNF_EVENT_CONTENT_ERROR;
    }
    if (sCnfEvent.status != RFB_EVENT_SUCCESS)
    {
        return (RFB_EVENT_STATUS)sCnfEvent.status;
    }
    return RFB_EVENT_SUCCESS;
}

RFB_EVENT_STATUS rfb_comm_zwave_id_filter_set(uint32_t home_id, uint16_t node_id, uint8_t home_id_hash)
{
    ruci_para_set_zwave_id_filter_t sZwaveIdFilterCmd = {0};
    ruci_para_cnf_event_t sCnfEvent = {0};
    uint8_t event_len = 0;
    RF_MCU_RX_CMDQ_ERROR event_status = RF_MCU_RX_CMDQ_ERR_INIT;

    SET_RUCI_PARA_SET_ZWAVE_ID_FILTER(&sZwaveIdFilterCmd, home_id, node_id, home_id_hash);

    RUCI_ENDIAN_CONVERT((uint8_t *)&sZwaveIdFilterCmd, RUCI_SET_ZWAVE_ID_FILTER);

    enter_critical_section();
    rfb_send_cmd((uint8_t *)&sZwaveIdFilterCmd, RUCI_LEN_SET_ZWAVE_ID_FILTER);
    event_status = rfb_event_read(&event_len, (uint8_t *)&sCnfEvent);
    leave_critical_section();

    RUCI_ENDIAN_CONVERT((uint8_t *)&sCnfEvent, RUCI_CNF_EVENT);
    if (event_status != RF_MCU_RX_CMDQ_GET_SUCCESS)
    {
        return RFB_CNF_EVENT_NOT_AVAILABLE;
    }
    if (sCnfEvent.pci_cmd_subheader != RUCI_CODE_SET_ZWAVE_ID_FILTER)
    {
        return RFB_CNF_EVENT_CONTENT_ERROR;
    }
    if (sCnfEvent.status != RFB_EVENT_SUCCESS)
    {
        return (RFB_EVENT_STATUS)sCnfEvent.status;
    }
    return RFB_EVENT_SUCCESS;
}
/* ZWAVE end */
