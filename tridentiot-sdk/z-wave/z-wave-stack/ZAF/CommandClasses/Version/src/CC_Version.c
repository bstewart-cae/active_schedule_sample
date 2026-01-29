/*
 * SPDX-FileCopyrightText: 2018 Silicon Laboratories Inc. <https://www.silabs.com/>
 * SPDX-FileCopyrightText: 2025 Z-Wave Alliance <https://z-wavealliance.org/>
 * SPDX-FileCopyrightText: 2025 Card Access Engineering, LLC <https://www.caengineering.com/>
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

/**
 * @file
 * Handler for Command Class Version.
 *
 * @copyright 2018 Silicon Laboratories Inc.
 */

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/
#include "association_plus_base.h"
#include "CC_Version.h"
#include "cc_version_migration_config_api.h"
#include "cc_version_migration_api.h"
#include "ZAF_TSE.h"
#include <ZW_TransportEndpoint.h>
#include <ZW_TransportSecProtocol.h>
#include "ZW_application_transport_interface.h"
#include <string.h>
#include "ZAF_Common_interface.h"
#include "zaf_event_distributor_soc.h"
#include <zaf_config_api.h>
#include <zpal_bootloader.h>
#include <zpal_misc.h>
#include "zw_build_no.h"
//#define DEBUGPRINT
#include "DebugPrint.h"

/****************************************************************************/
/*                      PRIVATE TYPES and DEFINITIONS                       */
/****************************************************************************/
static void CC_Version_Migration_ReportTX(
  RECEIVE_OPTIONS_TYPE_EX *rx_opts,
  const cc_version_migration_operation_t operation,
  const cc_version_migration_operation_state_t * const state,
  const bool lifeline
);

/****************************************************************************/
/*                             INLINE HELPERS                               */
/****************************************************************************/

/**
 * This packs a given migration report frame with given constant values
 *
 * @param[in] pOperation Migration Operation
 * @param[in] pStatus    Status of the migration operation
 * @param[in] pRemainingTime Remaining time of the operation, if in progress.
 * @param[out] pOutFrame Frame buffer into which to pack the information
 */
static inline void pack_migration_report(
  const cc_version_migration_operation_t pOperation,
  const cc_version_migration_status_t pStatus,
  const uint16_t pRemainingTime,
  ZW_VERSION_MIGRATION_REPORT_V4_FRAME * pOutFrame)
{
  ASSERT(pOutFrame != NULL);

  pOutFrame->cmdClass = COMMAND_CLASS_VERSION_V4;
  pOutFrame->cmd = VERSION_MIGRATION_REPORT_V4;
  pOutFrame->migrationOperationId = (uint8_t)pOperation;
  pOutFrame->migrationStatus = pStatus;
  /*
   * CC:0086.04.1D.11.002 - Field must be set to zero if the status isn't in progress
   */
  uint16_t time = (pStatus == MIGRATION_STATUS_IN_PROGRESS) ? pRemainingTime : 0;
  pOutFrame->estimatedTimeOfCompletionSeconds1 = (uint8_t)((time >> 8) & 0xFF);
  pOutFrame->estimatedTimeOfCompletionSeconds2 = (uint8_t)((time) & 0xFF);
}

/****************************************************************************/
/*                              PRIVATE DATA                                */
/****************************************************************************/
static RECEIVE_OPTIONS_TYPE_EX * m_current_rx_opts;
static RECEIVE_OPTIONS_TYPE_EX m_report_rx_opts;
static ZW_APPLICATION_TX_BUFFER m_report_tse_buffer;
/****************************************************************************/
/*                              EXPORTED DATA                               */
/****************************************************************************/

/****************************************************************************/
/*                            EXPORTED FUNCTIONS                            */
/****************************************************************************/

bool CC_Version_GetCurrentRxOpts(RECEIVE_OPTIONS_TYPE_EX *rx_opts)
{
  if (m_current_rx_opts == NULL) {
    return false;
  } else {
    memcpy(rx_opts, m_current_rx_opts, sizeof(RECEIVE_OPTIONS_TYPE_EX));
    return true;
  }
}

/****************************************************************************/
/*                             WEAK FUNCTIONS                               */
/****************************************************************************/

/**
 * Get firmware version of given firmware target index.
 *
 * This function is weakly defined so that an application can implement it in case of support for
 * more firmware targets.
 *
 * @param[in] firmwareTargetIndex Firmware target index of the custom firmware target.
 * @param[out] pVariantgroup returns pointer to application version group number n.
 */
ZW_WEAK void CC_Version_GetFirmwareVersion_handler(
  __attribute__((unused)) uint8_t firmwareTargetIndex,
  __attribute__((unused)) VG_VERSION_REPORT_V2_VG* pVariantgroup)
{
}

/****************************************************************************/
/*                            PRIVATE FUNCTIONS                             */
/****************************************************************************/

/**
 * Callback function for ZAF TSE to send User or Credential Reports to multiple
 * destinations
 */
static void send_migration_report_tse(
  zaf_tx_options_t * p_tx_options,
  __attribute__((unused)) const void * const p_data
  )
{
  zaf_transport_tx(
    &m_report_tse_buffer.ZW_Common.cmdClass,
    sizeof(ZW_VERSION_MIGRATION_REPORT_V4_FRAME),
    ZAF_TSE_TXCallback,
    p_tx_options
    );
}

/**
 * Transmit a Migration Report, either via Lifeline or in response to a Get operation.
 *
 * @param[in] rx_options Pointer to received frame options to use for response
 * @param[in] operation    Which particular migration operation is being reported on
 * @param[in] state        Pointer to operation state information
 * @param[in] notify_lifeline     Whether or not the report is intended to be transmitted to the Lifeline
 */
static void CC_Version_Migration_ReportTX(
  RECEIVE_OPTIONS_TYPE_EX *rx_options,
  const cc_version_migration_operation_t operation,
  const cc_version_migration_operation_state_t * const state,
  const bool notify_lifeline
)
{
  zaf_tx_options_t tx_options;
  zaf_transport_rx_to_tx_options(rx_options, &tx_options);
  memcpy(&m_report_rx_opts, rx_options, sizeof(RECEIVE_OPTIONS_TYPE_EX));
  ZW_VERSION_MIGRATION_REPORT_V4_FRAME * outFrame =
    &m_report_tse_buffer.ZW_VersionMigrationReportV4Frame;
  /* Pack report into outgoing frame */
  pack_migration_report(
    operation,
    state->status,
    state->remaining_time,
    outFrame
  );
  /* Transmit report to sender */
  zaf_transport_tx((uint8_t*)outFrame, sizeof(ZW_VERSION_MIGRATION_REPORT_V4_FRAME), NULL, &tx_options);
  /*
   * This will send to everything BUT the sender, or everything in lifeline if the report is
   * unsolicited.
   */
  if (notify_lifeline) {
    ZAF_TSE_Trigger((void *)&send_migration_report_tse, &m_report_rx_opts, false);
  }
}

static void
CC_Version_add_bootloader(
  VG_VERSION_REPORT_V2_VG *pVariantgroup)
{
  zpal_bootloader_info_t bootloader_info = { 0 };

  zpal_bootloader_get_info(&bootloader_info);
  pVariantgroup->firmwareVersion =
    ((uint8_t)((bootloader_info.version & ZPAL_BOOTLOADER_VERSION_MAJOR_MASK) >> ZPAL_BOOTLOADER_VERSION_MAJOR_SHIFT));
  pVariantgroup->firmwareSubVersion =
    ((uint8_t)((bootloader_info.version & ZPAL_BOOTLOADER_VERSION_MINOR_MASK) >> ZPAL_BOOTLOADER_VERSION_MINOR_SHIFT));
}

static received_frame_status_t CC_Version_handler(
  RECEIVE_OPTIONS_TYPE_EX *rxOpt,
  ZW_APPLICATION_TX_BUFFER *pCmd,
  __attribute__((unused)) uint8_t cmdLength,
  ZW_APPLICATION_TX_BUFFER * pFrameOut,
  uint8_t * pLengthOut)
{
  m_current_rx_opts = rxOpt;
  received_frame_status_t result = RECEIVED_FRAME_STATUS_NO_SUPPORT;
  SApplicationHandles * pAppHandles;

  if (true == Check_not_legal_response_job(rxOpt)) {
    /*Do not support endpoint bit-addressing */
    result = RECEIVED_FRAME_STATUS_FAIL;
  } else { /* Process command if valid */
    switch (pCmd->ZW_VersionGetFrame.cmd) {
      case VERSION_GET_V2:
      {
        uint8_t firmwareTargetIndex;/*firmware target number 1..N */
        uint8_t numberOfFirmwareTargets;
        pFrameOut->ZW_VersionReport1byteV2Frame.cmdClass = COMMAND_CLASS_VERSION_V2;
        pFrameOut->ZW_VersionReport1byteV2Frame.cmd = VERSION_REPORT_V2;

        pAppHandles = ZAF_getAppHandle();
        pFrameOut->ZW_VersionReport1byteV2Frame.zWaveLibraryType = pAppHandles->pProtocolInfo->eLibraryType;
        pFrameOut->ZW_VersionReport1byteV2Frame.zWaveProtocolVersion = pAppHandles->pProtocolInfo->ProtocolVersion.Major;
        pFrameOut->ZW_VersionReport1byteV2Frame.zWaveProtocolSubVersion = pAppHandles->pProtocolInfo->ProtocolVersion.Minor;
        pFrameOut->ZW_VersionReport1byteV2Frame.firmware0Version = zpal_get_app_version_major();
        pFrameOut->ZW_VersionReport1byteV2Frame.firmware0SubVersion = zpal_get_app_version_minor();
        pFrameOut->ZW_VersionReport1byteV2Frame.hardwareVersion = zaf_config_get_hardware_version();
        numberOfFirmwareTargets = zaf_config_get_firmware_target_count();
        if (zaf_config_get_bootloader_upgradable()) {
          numberOfFirmwareTargets++;
        }
        pFrameOut->ZW_VersionReport1byteV2Frame.numberOfFirmwareTargets = numberOfFirmwareTargets - 1;/*-1 : Firmware version 0*/

        for (firmwareTargetIndex = 1; firmwareTargetIndex < numberOfFirmwareTargets; firmwareTargetIndex++) {
          uint8_t * pFrame = (uint8_t *)&(pFrameOut->ZW_VersionReport1byteV2Frame.variantgroup1);
          if (zaf_config_get_bootloader_upgradable() && zaf_config_get_bootloader_target_id() == firmwareTargetIndex) {
            CC_Version_add_bootloader((VG_VERSION_REPORT_V2_VG *)(pFrame + 2 * (firmwareTargetIndex - 1)));
          } else {
            CC_Version_GetFirmwareVersion_handler(firmwareTargetIndex, (VG_VERSION_REPORT_V2_VG *)(pFrame + 2 * (firmwareTargetIndex - 1)));
          }
        }

        *pLengthOut = sizeof(pFrameOut->ZW_VersionReport1byteV2Frame) + (numberOfFirmwareTargets - 1) * sizeof(VG_VERSION_REPORT_V2_VG) - sizeof(VG_VERSION_REPORT_V2_VG); /*-1 is Firmware version 0*/
        result = RECEIVED_FRAME_STATUS_SUCCESS;
        break;
      }
      case VERSION_COMMAND_CLASS_GET_V2:
        pFrameOut->ZW_VersionCommandClassReportFrame.cmdClass = COMMAND_CLASS_VERSION_V2;
        pFrameOut->ZW_VersionCommandClassReportFrame.cmd = VERSION_COMMAND_CLASS_REPORT_V2;
        pFrameOut->ZW_VersionCommandClassReportFrame.requestedCommandClass = pCmd->ZW_VersionCommandClassGetFrame.requestedCommandClass;

        pAppHandles = ZAF_getAppHandle();
        uint8_t version = 0xFF;
        uint8_t cc = pCmd->ZW_VersionCommandClassGetFrame.requestedCommandClass;

        zaf_cc_list_t *cc_list = GetCommandClassList(false, SECURITY_KEY_NONE, 0);

        /*
        * Transport Service, Security S0 and S2 versions must be returned only if they are
        * listed in the NIF. Since these command classes must always be in the non-secure list, we
        * can use that list to check it.
        */
        if (memchr(cc_list->cc_list,
                  cc,
                  cc_list->list_size)) {
          // CC is in the list
          switch (pCmd->ZW_VersionCommandClassGetFrame.requestedCommandClass) {
            case COMMAND_CLASS_TRANSPORT_SERVICE:
              version = pAppHandles->pProtocolInfo->CommandClassVersions.TransportServiceVersion;
              break;
            case COMMAND_CLASS_SECURITY:
              version = pAppHandles->pProtocolInfo->CommandClassVersions.SecurityVersion;
              break;
            case COMMAND_CLASS_SECURITY_2:
              version = pAppHandles->pProtocolInfo->CommandClassVersions.Security2Version;
              break;
            default:
              // Do nothing.
              break;
          }
        }

        pFrameOut->ZW_VersionCommandClassReportFrame.commandClassVersion = version;

        if (0xFF == pFrameOut->ZW_VersionCommandClassReportFrame.commandClassVersion) {
          /*
          * When every CC uses the REGISTER_CC() macro, the compiler creates a section in the code.
          * Also two variables are automatically created and these represent the beginning and the end
          * of the section. The variables can be used to loop through the section.
          */
          CC_handler_map_latest_t const * iter = &cc_handlers_start;
          for ( ; iter < &cc_handlers_stop; ++iter) {
            if (pCmd->ZW_VersionCommandClassGetFrame.requestedCommandClass == iter->CC) {
              DPRINTF("\r\nCC: %#x - Version: %d\r\n", iter->CC, iter->version);
              pFrameOut->ZW_VersionCommandClassReportFrame.commandClassVersion = iter->version;
            }
          }
          if (0xFF == pFrameOut->ZW_VersionCommandClassReportFrame.commandClassVersion) {
            pFrameOut->ZW_VersionCommandClassReportFrame.commandClassVersion = 0; // Value if not found
          }
        }

        *pLengthOut = sizeof(pFrameOut->ZW_VersionCommandClassReportFrame);
        result = RECEIVED_FRAME_STATUS_SUCCESS;
        break;
      case VERSION_CAPABILITIES_GET_V3:
        pFrameOut->ZW_VersionCapabilitiesReportV3Frame.cmdClass    = COMMAND_CLASS_VERSION_V3;
        pFrameOut->ZW_VersionCapabilitiesReportV3Frame.cmd         = VERSION_CAPABILITIES_REPORT_V3;

        /*
        * The following pointer constellation might seem unnecessary, but it's done to make the
        * code more readable and to enable/disable each flags easier.
        *
        * Sigma Designs' Z-Wave applications support all commands, but the support can be toggled
        * corresponding to the Version Command Class specification
        */
        {
          uint8_t * pProperties = &(pFrameOut->ZW_VersionCapabilitiesReportV3Frame.properties1);
          *pProperties = 0x00;

          *pProperties |= VERSION_CAPABILITIES_REPORT_PROPERTIES1_VERSION_BIT_MASK_V3;
          *pProperties |= VERSION_CAPABILITIES_REPORT_PROPERTIES1_COMMAND_CLASS_BIT_MASK_V3;
          *pProperties |= VERSION_CAPABILITIES_REPORT_PROPERTIES1_Z_WAVE_SOFTWARE_BIT_MASK_V3;
          *pProperties |= (cc_version_is_migration_supported() ?
                            VERSION_CAPABILITIES_REPORT_PROPERTIES1_MIGRATION_SUPPORT_BIT_MASK_V4 : 0x00);
        }

        *pLengthOut = sizeof(pFrameOut->ZW_VersionCapabilitiesReportV3Frame);

        result = RECEIVED_FRAME_STATUS_SUCCESS;
        break;
      case VERSION_ZWAVE_SOFTWARE_GET_V3:
      {
        uint16_t zaf_build_no;
        zaf_build_no = zaf_config_get_build_no();
        uint16_t protocol_build_no;
        protocol_build_no = ZW_GetProtocolBuildNumber();

        pFrameOut->ZW_VersionZwaveSoftwareReportV3Frame.cmdClass    = COMMAND_CLASS_VERSION_V3;
        pFrameOut->ZW_VersionZwaveSoftwareReportV3Frame.cmd         = VERSION_ZWAVE_SOFTWARE_REPORT_V3;

        pFrameOut->ZW_VersionZwaveSoftwareReportV3Frame.sdkVersion1 = SDK_VERSION_MAJOR;
        pFrameOut->ZW_VersionZwaveSoftwareReportV3Frame.sdkVersion2 = SDK_VERSION_MINOR;
        pFrameOut->ZW_VersionZwaveSoftwareReportV3Frame.sdkVersion3 = SDK_VERSION_PATCH;

        pFrameOut->ZW_VersionZwaveSoftwareReportV3Frame.applicationFrameworkApiVersion1 = ZAF_VERSION_MAJOR;
        pFrameOut->ZW_VersionZwaveSoftwareReportV3Frame.applicationFrameworkApiVersion2 = ZAF_VERSION_MINOR;
        pFrameOut->ZW_VersionZwaveSoftwareReportV3Frame.applicationFrameworkApiVersion3 = ZAF_VERSION_PATCH;

        pFrameOut->ZW_VersionZwaveSoftwareReportV3Frame.applicationFrameworkBuildNumber1 = (uint8_t)(ZAF_BUILD_NO >> 8);
        pFrameOut->ZW_VersionZwaveSoftwareReportV3Frame.applicationFrameworkBuildNumber2 = (uint8_t)ZAF_BUILD_NO;

        pFrameOut->ZW_VersionZwaveSoftwareReportV3Frame.hostInterfaceVersion1 = 0;
        pFrameOut->ZW_VersionZwaveSoftwareReportV3Frame.hostInterfaceVersion2 = 0;
        pFrameOut->ZW_VersionZwaveSoftwareReportV3Frame.hostInterfaceVersion3 = 0;
        pFrameOut->ZW_VersionZwaveSoftwareReportV3Frame.hostInterfaceBuildNumber1 = (uint8_t)(0 >> 8);
        pFrameOut->ZW_VersionZwaveSoftwareReportV3Frame.hostInterfaceBuildNumber2 = (uint8_t)0;

        pAppHandles = ZAF_getAppHandle();
        pFrameOut->ZW_VersionZwaveSoftwareReportV3Frame.zWaveProtocolVersion1 = pAppHandles->pProtocolInfo->ProtocolVersion.Major;
        pFrameOut->ZW_VersionZwaveSoftwareReportV3Frame.zWaveProtocolVersion2 = pAppHandles->pProtocolInfo->ProtocolVersion.Minor;
        pFrameOut->ZW_VersionZwaveSoftwareReportV3Frame.zWaveProtocolVersion3 = pAppHandles->pProtocolInfo->ProtocolVersion.Revision;
        pFrameOut->ZW_VersionZwaveSoftwareReportV3Frame.zWaveProtocolBuildNumber1 = (uint8_t)(protocol_build_no >> 8);
        pFrameOut->ZW_VersionZwaveSoftwareReportV3Frame.zWaveProtocolBuildNumber2 = (uint8_t)protocol_build_no;

        pFrameOut->ZW_VersionZwaveSoftwareReportV3Frame.applicationVersion1 = zpal_get_app_version_major();
        pFrameOut->ZW_VersionZwaveSoftwareReportV3Frame.applicationVersion2 = zpal_get_app_version_minor();
        pFrameOut->ZW_VersionZwaveSoftwareReportV3Frame.applicationVersion3 = zpal_get_app_version_patch();
        pFrameOut->ZW_VersionZwaveSoftwareReportV3Frame.applicationBuildNumber1 = (uint8_t)(zaf_build_no >> 8);
        pFrameOut->ZW_VersionZwaveSoftwareReportV3Frame.applicationBuildNumber2 = (uint8_t)zaf_build_no;

        *pLengthOut = sizeof(pFrameOut->ZW_VersionZwaveSoftwareReportV3Frame);

        result = RECEIVED_FRAME_STATUS_SUCCESS;
        break;
      }
      case VERSION_MIGRATION_CAPABILITIES_GET_V4:
      {
        // Check and make sure that migration is supported before answering
        // CC:0086.04.19.11.001
        if (!cc_version_is_migration_supported()) {
          result = RECEIVED_FRAME_STATUS_NO_SUPPORT;
          break;
        }
        ZW_VERSION_MIGRATION_CAPABILITIES_REPORT_1BYTE_V4_FRAME *outFrame =
          &pFrameOut->ZW_VersionMigrationCapabilitiesReport1byteV4Frame;
        /* Populate report frame based on capabilites configured in the device */
        outFrame->cmdClass = COMMAND_CLASS_VERSION_V4;
        outFrame->cmd = VERSION_MIGRATION_CAPABILITIES_REPORT_V4;
        uint8_t *num_ops = &outFrame->numberOfSupportedMigrationOperations;
        uint8_t *op_ids = &outFrame->migrationOperationId1;
        *num_ops = 0; // Reset to zero just in case
        /*
        * Iterate through the available migration operations,
        * and verify how many and which ones are supported.
        *
        * Note that they start at 1.
        * Req. numbers:
        * CC:0086.04.1A.11.001
        * CC:0086.04.1A.13.002
        */
        for (uint8_t i = CC_VERSION_MIGRATION_OPERATION_USER_CODE_TO_U3C; i < CC_VERSION_MIGRATION_OPERATION_EOT; i++) {
          if (cc_version_is_migration_operation_supported((cc_version_migration_operation_t)i)) {
            op_ids[*num_ops] = i;
            (*num_ops) += 1;
          }
        }
        *pLengthOut = (sizeof(ZW_VERSION_MIGRATION_CAPABILITIES_REPORT_1BYTE_V4_FRAME) - 1 + *num_ops);
        result = RECEIVED_FRAME_STATUS_SUCCESS;
        break;
      }
      case VERSION_MIGRATION_SET_V4:
      {
        ZW_VERSION_MIGRATION_SET_V4_FRAME *inFrame = &pCmd->ZW_VersionMigrationSetV4Frame;
        // Check and make sure that migration (and operation) are supported before answering
        // CC:0086.04.19.11.001
        // CC:0086.04.1B.11.001
        if (!cc_version_is_migration_supported()) {
          result = RECEIVED_FRAME_STATUS_NO_SUPPORT;
          break;
        } else if (!cc_version_is_migration_operation_supported(inFrame->migrationOperationId) ||
                   !CC_Version_Migration_Start(inFrame->migrationOperationId)) {
          result = RECEIVED_FRAME_STATUS_FAIL;
          break;
        }
        result = RECEIVED_FRAME_STATUS_SUCCESS;
        break;
      }
      case VERSION_MIGRATION_GET_V4:
      {
        ZW_VERSION_MIGRATION_GET_V4_FRAME *inFrame = &pCmd->ZW_VersionMigrationGetV4Frame;
        // Check and make sure that migration (and operation) are supported before answering
        // CC:0086.04.19.11.001
        // CC:0086.04.1C.11.001
        if (!cc_version_is_migration_supported()) {
          result = RECEIVED_FRAME_STATUS_NO_SUPPORT;
          break;
        } else if (!cc_version_is_migration_operation_supported(inFrame->migrationOperationId)) {
          result = RECEIVED_FRAME_STATUS_FAIL;
          break;
        }
        cc_version_migration_operation_state_t state = { 0 };
        if (!CC_Version_Migration_GetStatus(inFrame->migrationOperationId, &state)) {
          result = RECEIVED_FRAME_STATUS_FAIL;
          break;
        }
        /* Populate and send report */
        ZW_VERSION_MIGRATION_REPORT_V4_FRAME *outFrame =
          &pFrameOut->ZW_VersionMigrationReportV4Frame;

        pack_migration_report(
          inFrame->migrationOperationId,
          state.status,
          state.remaining_time,
          outFrame);

        *pLengthOut=sizeof(ZW_VERSION_MIGRATION_REPORT_V4_FRAME);

        result = RECEIVED_FRAME_STATUS_SUCCESS;
        break;
      }
      /* Unsupported case */
      default:
        // Do nothing.
        break;
    }
  }
  /* Reset current RX options */
  m_current_rx_opts = NULL;
  return result;
}

static uint8_t lifeline_reporting(ccc_pair_t * p_ccc_pair)
{
  p_ccc_pair->cmdClass = COMMAND_CLASS_VERSION_V4;
  p_ccc_pair->cmd      = VERSION_MIGRATION_REPORT_V4;
  return 1;
}

REGISTER_CC_V4(COMMAND_CLASS_VERSION, VERSION_VERSION_V4, CC_Version_handler, NULL, NULL, lifeline_reporting, 0x00, NULL, NULL);

static void
version_migration_event_handler(const uint8_t event, const void * p_data)
{
  switch (event) {
    case MIGRATION_EVENT_ON_STATUS_CHANGE: {
      if (p_data != NULL) {
        cc_version_migration_event_data_t *event_data =
          (cc_version_migration_event_data_t*)p_data;
        // Extract event data and send report to the controller that triggered the event
        CC_Version_Migration_ReportTX(
          event_data->rx_opts,
          event_data->operation,
          &event_data->state,
          true);
        break;
      }
    }
    default:
      break;
  }
}

ZAF_EVENT_DISTRIBUTOR_REGISTER_CC_EVENT_HANDLER(COMMAND_CLASS_VERSION_V4, version_migration_event_handler);
