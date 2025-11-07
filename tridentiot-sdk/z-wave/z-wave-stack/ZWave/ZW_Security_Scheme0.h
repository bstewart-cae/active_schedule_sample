// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file ZW_Security_Scheme0.h
 * @copyright 2022 Silicon Laboratories Inc.
 */
#ifndef SECURITY_SCHEME0_H_
#define SECURITY_SCHEME0_H_

#include <stdint.h>
#include <stdbool.h>
#include <ZW_libsec.h>
#include <ZW_transport_transmit_cb.h>

#define S0_MAX_ENCRYPTED_MSG_SIZE 128

/* Transport Security 0 command class version */
#define TRANSPORT_SECURITY_SUPPORTED_VERSION SECURITY_VERSION

/**
 * Register a nonce by source and destination
 */
void sec0_register_nonce(uint8_t src, uint8_t dst, uint8_t* nonce);

uint8_t sec0_send_data(node_t snode, node_t dnode,const void* pData, uint8_t len, TxOptions_t txoptions,
  const STransmitCallback* pCallback);


/**
 * Retrieve netkey from keystore (NVM) and
 * return true if netkey valid
 *        false if netkey ZERO
 */
bool sec0_unpersist_netkey();

void sec0_persist_netkey( uint8_t *netkey);
void sec0_clear_netkey();

uint8_t sec0_decrypt_message(uint8_t snode, uint8_t dnode, uint8_t* enc_data, uint8_t enc_data_length,uint8_t* dec_message);
void sec0_send_nonce(uint8_t snode, uint8_t dnode, TxOptions_t txoptions);

/**
 * One-time registration of power locks
 */
void sec0_register_power_locks(void);

/**
 * Initialize Security Scheme 0.
 */
/* Returns true if network key is set (i.e. nonzero) */
bool sec0_init();

void sec0_abort_all_tx_sessions();

typedef enum
{
  SEC0_STATE_IDLE,
  SEC0_STATE_NONCE_ACTIVE,
  SEC0_STATE_RXSESSION_ACTIVE,
  SEC0_STATE_TXSESSION_ACTIVE
} sec0_state_t;

/* Report if Security 0 layer are idle or active (any rxsession, txsession or nonce active) */
/* Returns: SEC0_STATE_NONCE_ACTIVE     - if Security 0 layer has an active NONCE */
/*          SEC0_STATE_RXSESSION_ACTIVE - if Security 0 layer has an active rx session */
/*          SEC0_STATE_TXSESSION_ACTIVE - if Security 0 layer has an active tx session */
/*          SEC0_STATE_IDLE             - if Security 0 layer is IDLE */
sec0_state_t sec0_state();

bool sec0_busy();

void sec0_send_key_verify(uint8_t sourceNode);

#endif /* SECURITY_SCHEME0_H_ */
