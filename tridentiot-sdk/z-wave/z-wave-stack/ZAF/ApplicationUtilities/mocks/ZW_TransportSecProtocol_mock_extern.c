// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file ZW_TransportSecProtocol_mock_extern.c
 * @copyright 2022 Silicon Laboratories Inc.
 */
#include <ZW_typedefs.h>
#include <ZW_classcmd.h>
#include <ZW_transport_api.h>

void
Transport_ApplicationCommandHandler(
  ZW_APPLICATION_TX_BUFFER *pCmd, /* IN Payload from the received frame, */
                                  /*    the command is the very first byte */
  uint8_t cmdLength,                 /* IN Number of command bytes including the command */
  RECEIVE_OPTIONS_TYPE *rxOpt)    /* IN rxOpt struct to use (may be partially filled */
                                  /*    out if parsing encapsulated command) */
{

}
