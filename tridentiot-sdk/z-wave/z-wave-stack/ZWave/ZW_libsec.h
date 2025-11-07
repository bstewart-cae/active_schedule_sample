// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file ZW_libsec.h
 * @copyright 2022 Silicon Laboratories Inc.
 *
 * @brief Temporary home of libsec-related defines.
 */
#ifndef ZW_LIBSEC_H_
#define ZW_LIBSEC_H_
#include <ZW_basis_api.h>
#include <ZW_security_api.h>
#include <ZW_protocol.h>
#include <stdint.h>
#include <S2.h>
#include <S2_external.h>

#ifdef ZW_SECURITY_PROTOCOL

/**
 * Structure describing how a package was received / should be transmitted
 */
typedef struct ts_param
{
  /**
   * Source node
   */
  uint16_t snode;
  /**
   * Destination node
   */
  uint16_t dnode;

  /**
   * Source endpoint
   */
  uint8_t sendpoint;

  /**
   * Destination endpoint
   */
  uint8_t dendpoint;

  /**
   * Transmit flags
   * see txOptions in \ref ZW_SendData
   */
  uint8_t tx_flags;

  /**
   * Receive flags
   * see rxOptions in \ref ApplicationCommandHandler
   */
  uint8_t rx_flags;

  /**
   * Security scheme used for this package
   */
  security_key_t scheme; // Security scheme
} ts_param_t;

/**
 * Copy transport parameter structure
 * @param dst
 * @param src
 */
void ts_param_copy(ts_param_t* dst, const ts_param_t* src);

/**
 * Convert transport parameters to a reply
 * @param dst
 * @param src
 */
void ts_param_make_reply(ts_param_t* dst, const ts_param_t* src);

/**
 * \return true if source and destinations are identical
 */
uint8_t ts_param_cmp(ts_param_t* a1, const ts_param_t* a2);

typedef void (*void_Byte_Callback_t)(uint8_t txStatus);


#endif /* ZW_SECURITY_PROTOCOL */

#endif /* ZW_LIBSEC_H_ */
