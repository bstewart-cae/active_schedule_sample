/// ***************************************************************************
///
/// @file CC_RemoteCLI.c
///
/// SPDX-License-Identifier: LicenseRef-TridentMSLA
/// SPDX-FileCopyrightText: 2023 Trident IoT, LLC <https://www.tridentiot.com>
/// ***************************************************************************

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/

#include <string.h>
#include "Assert.h"
#include "ZAF_TSE.h"
#include "CC_RemoteCLI.h"
#include "cc_remote_cli_config.h"
#include "zaf_config_api.h"
#include "ZAF_Common_interface.h"
#include "AppTimer.h"
#include "tr_ring_buffer.h"

//#define DEBUGPRINT // NOSONAR
#include "DebugPrint.h"

/****************************************************************************/
/*                      PRIVATE TYPES and DEFINITIONS                       */
/****************************************************************************/
/*************** Propritary command classes ****************/
#define COMMAND_CLASS_REMOTE_CLI                        0xC0

/* Remote CLI command class commands */
#define REMOTE_CLI_VERSION                              0x01
#define REMOTE_CLI_GET                                  0x02
#define REMOTE_CLI_REPORT                               0x03

/************************************************************/
/* Remote CLi Get frame                                     */
/************************************************************/
typedef struct _ZW_RemoteCLIGetFrame_
{
    uint8_t   cmdClass;                     /* The command class */
    uint8_t   cmd;                          /* The command */
    uint8_t   string_length_msb;            /* Length of command string */
    uint8_t   string_length_lsb;            /* Length of command string */
    uint8_t   command_string;               /* first byte of command string */
} ZW_RemoteCLIGetFrame_t;

/************************************************************/
/* Remote CLi Report frame                                  */
/************************************************************/
typedef struct ZW_RemoteCLIReportFrame
{
    uint8_t   cmdClass;                     /* The command class */
    uint8_t   cmd;                          /* The command */
    uint8_t   string_length_msb;            /* Length of command string */
    uint8_t   string_length_lsb;            /* Length of command string */
    uint8_t   output_string;                /* first byte of output string */
} ZW_RemoteCLIReportFrame_t;

/************************************************************/

/****************************************************************************/
/*                              PRIVATE DATA                                */
/****************************************************************************/
static SSwTimer buffer_timer;
static cc_cli_command_handler_t command_handler = NULL;
static zaf_tx_options_t tx_options;
static tr_ring_buffer_t ring_buffer;
static uint8_t data_buffer[CC_REMOTE_CLI_CONFIG_TRANSMIT_BUFFER_LENGTH];

static void CC_remote_cli_buffer_timeout(__attribute__((unused)) SSwTimer *pTimer);

/****************************************************************************/
/*                              EXPORTED FUNCTIONS                          */
/****************************************************************************/

void CC_remote_cli_set_command_handler(cc_cli_command_handler_t cli_command_handler)
{
  command_handler = cli_command_handler;
}

void CC_remote_cli_buffer_data(uint16_t data_length, uint8_t * data)
{
  uint8_t written = 0;

  DPRINTF("CC Remote CLI - Buffer %i ", data_length);

  // Add data to ring buffer
  while (data_length>0)
  {
    written += tr_ring_buffer_write(&ring_buffer, *data);
    data_length--;
    data++;
  }

  DPRINTF("- %i ", written);

  // if data was added, restart transmit timer
  if (written)
  {
    DPRINT("- timer start\n");
    // Start/restart timer to transmit buffer
    if (TimerIsActive(&buffer_timer))
      TimerRestart(&buffer_timer);
    else
      TimerStart(&buffer_timer, CC_REMOTE_CLI_CONFIG_TRANSMIT_BUFFER_TIMEOUT);
  }
  else
  {
    // No data added to buffer
    DPRINT("- full\n");
  }
}

/****************************************************************************/
/*                            PRIVATE FUNCTIONS                             */
/****************************************************************************/

static void CC_RemoteCLI_init_buffer()
{
  ring_buffer.p_buffer    = data_buffer;
  ring_buffer.buffer_size = CC_REMOTE_CLI_CONFIG_TRANSMIT_BUFFER_LENGTH;
  tr_ring_buffer_init(&ring_buffer);
}

static received_frame_status_t CC_RemoteCLI_handler(
    cc_handler_input_t * input,
    __attribute__((unused)) cc_handler_output_t * output)
{
  DPRINT("CC Remote CLI - Received cmd ");
  switch (input->frame->ZW_Common.cmd)
  {
    case REMOTE_CLI_GET:
    {
      ZW_RemoteCLIGetFrame_t *cli_get_buffer = (ZW_RemoteCLIGetFrame_t*)input->frame;
      DPRINTF("GET - ");
      uint16_t string_length = cli_get_buffer->string_length_lsb +
                               ((uint16_t)cli_get_buffer->string_length_msb << 8);
      //uint16_t string_length = input->frame->ZW_RemoteCLIGetFrame.string_length_lsb +
      //                         ((uint16_t)input->frame->ZW_RemoteCLIGetFrame.string_length_msb << 8);
      DPRINTF("%i ", string_length);
      zaf_transport_rx_to_tx_options(input->rx_options, &tx_options);

#ifdef DEBUGPRINT
    char *character = (char*)&cli_get_buffer->command_string;
    DPRINT("\"");
    for (uint8_t cnt=string_length; cnt>0; cnt--)
    {
      DPRINTF("%c", *character);
      character++;
    }
    DPRINT("\"\n");
#endif

      if (NULL != command_handler)
      {
        // Call CLI command parser
        command_handler(string_length, (char*)&cli_get_buffer->command_string);
      }
      break;
    }

    case REMOTE_CLI_REPORT:
    {
      DPRINT("REPORT\n ");
      return RECEIVED_FRAME_STATUS_NO_SUPPORT;
      break;
    }

    default:
      DPRINT("unknown\n ");
      return RECEIVED_FRAME_STATUS_NO_SUPPORT;
      break;
  }
  return RECEIVED_FRAME_STATUS_SUCCESS;
}

static void
CC_remote_cli_TXCallback(__attribute__((unused)) transmission_result_t * pTransmissionResult)
{
  DPRINTF("\nCC Remote CLI - Transmit Complete %i\n", pTransmissionResult->status);
  // Transmit again if there is more data in the ring buffer
  CC_remote_cli_buffer_timeout(&buffer_timer);
}

static void
CC_remote_cli_buffer_timeout(__attribute__((unused)) SSwTimer *pTimer)
{
  size_t available_bytes = tr_ring_buffer_get_available(&ring_buffer);

  DPRINTF("CC Remote CLI - Timeout - %i - ", available_bytes);

  // Send report frame with buffered data
  if (available_bytes)
  {
    //Check length of data to avoid Tx buffer overrun
    const uint16_t max_payload = ZAF_getAppHandle()->pNetworkInfo->MaxPayloadSize - sizeof(ZW_RemoteCLIReportFrame_t);
    if (available_bytes > max_payload)
    {
      available_bytes = max_payload;
    }

    ZW_APPLICATION_TX_BUFFER txBuf;
    ZW_RemoteCLIReportFrame_t *cli_tx_buffer = (ZW_RemoteCLIReportFrame_t*)&txBuf;
    //uint8_t bPadding[TX_DATA_MAX_DATA_SIZE];

    // Set up report frame with string from CLI
    cli_tx_buffer->cmdClass           = COMMAND_CLASS_REMOTE_CLI;
    cli_tx_buffer->cmd                = REMOTE_CLI_REPORT;
    cli_tx_buffer->string_length_msb  = (uint8_t)(available_bytes >> 8);
    cli_tx_buffer->string_length_lsb  = (uint8_t)(available_bytes & 0xFF);

    // copy data to transmit buffer
    tr_ring_buffer_read(&ring_buffer, &cli_tx_buffer->output_string, available_bytes);

#ifdef DEBUGPRINT
    char *character = (char*)&cli_tx_buffer->output_string;
    DPRINTF("%i \"", available_bytes);
    for (uint8_t cnt=available_bytes; cnt>0; cnt--)
    {
      DPRINTF("%c", *character);
      character++;
    }
    DPRINT("\" ");
#endif

    if (tx_options.dest_node_id != 0)
    {
      DPRINTF("Transmit %i\n", sizeof(ZW_RemoteCLIReportFrame_t) + available_bytes - 1);
      // Transmit frame
      (void) zaf_transport_tx((uint8_t *)&txBuf,
                              sizeof(ZW_RemoteCLIReportFrame_t) + available_bytes - 1,
                              CC_remote_cli_TXCallback,
                              &tx_options);
    }
    else
    {
      // Discard CLI data, no destination
      DPRINT("Discard \n");
    }
  }
}

/*******************************************************************************************************
 * Linker magic - Creates a section for an array of registered CCs and mapped CCs to the Remote CLI CC.
 ******************************************************************************************************/

static void init_and_reset(void)
{
  DPRINT("CC Remote CLI - init\n");
  memset(&tx_options, 0, sizeof(zaf_tx_options_t));
  AppTimerRegister(&buffer_timer, false, &CC_remote_cli_buffer_timeout);
  CC_RemoteCLI_init_buffer();
}

REGISTER_CC_V5(COMMAND_CLASS_REMOTE_CLI, REMOTE_CLI_VERSION, CC_RemoteCLI_handler, NULL, NULL, NULL, 0, init_and_reset, init_and_reset);
