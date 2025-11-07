// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file ZW_Security_Scheme2.h
 * @copyright 2022 Silicon Laboratories Inc.
 */
#ifndef _ZW_SECURITY_SCHEME2_H_
#define _ZW_SECURITY_SCHEME2_H_
#include <ZW_transport_transmit_cb.h>
#include <s2_protocol.h>

extern struct S2* s2_ctx;

typedef  struct MPAN mpan_file[MPAN_TABLE_SIZE] ;
typedef  struct SPAN span_file[SPAN_TABLE_SIZE] ;

#ifdef ZW_SECURITY_PROTOCOL

/* Transport Security 2 command class version */
#define TRANSPORT_SECURITY_2_SUPPORTED_VERSION SECURITY_2_VERSION

uint8_t
sec2_send_data(
  void* vdata,
  uint16_t len,
  TRANSMIT_OPTIONS_TYPE *txOptionsEx,
  const STransmitCallback* pCallback);

uint8_t
sec2_send_data_multi(
  void* vdata,
  uint16_t len,
  TRANSMIT_MULTI_OPTIONS_TYPE *txOptionsMultiEx,
  const STransmitCallback* pCallback);

void /*RET Nothing                  */
Security2CommandHandler(
  uint8_t *pCmd,        /* IN  Frame payload pointer */
  uint8_t cmdLength,    /* IN  Payload length        */
  RECEIVE_OPTIONS_TYPE *rxopt);

/**
 * One-time registration of power locks
 */
void sec2_register_power_locks(void);

/**
 * Initialize Security 2 and return true (else false) if security 2 key exist (none zero).
 */
bool
sec2_init(void);

/**
 * Stop S2 inclusion machine - if applicable
 */
void
sec2_inclusion_abort(void);

uint8_t s2_busy(void);

/**
 * Let Sec2 module save any information to the file system 
 * Currently we only save MPAN and SPAN nnoces tables
 */

void sec2_PowerDownHandler(void);

/**
 * Reset MPAN and SPAN nonces tables (Set tp ZERO).
 */

void sec2_reset_nonces_tables(void);
#endif

#endif /* _ZW_SECURITY_SCHEME2_H_ */
