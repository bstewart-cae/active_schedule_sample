/**
 * @file CC_WakeUp_mock.c
 * @copyright 2022 Silicon Laboratories Inc.
 */
#include <CC_WakeUp.h>
#include <ZW_basis_api.h>
#include <CC_Common.h>
#include <ZAF_types.h>
#include <mock_control.h>

#define MOCK_FILE "CC_WakeUp_mock.c"

void CC_WakeUp_notification_tx(void (*pCallback)(uint8_t txStatus, TX_STATUS_TYPE* pExtendedTxStatus))
{

}
