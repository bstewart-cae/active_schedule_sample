// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file ZW_Security_Scheme0.c
 * @copyright 2022 Silicon Laboratories Inc.
 */
#include <stdint.h>
#include "ZW_lib_defines.h"
#include "Assert.h"
#include <ZW_libsec.h>
#include <ZW_ctimer.h>
#include <ZW_Security_Scheme0.h>
#include <ZW_protocol.h>
#include <s2_keystore.h>
#include <zpal_entropy.h>
#include <ZW_transport.h>
#include <ZW_keystore.h>
#include <zpal_power_manager.h>
//#define DEBUGPRINT
#include <DebugPrint.h>
#ifdef ZWAVE_PSA_SECURE_VAULT
#include "psa/ZW_psa.h"
#endif

#define NONCE_TABLE_SIZE 5*3
#define NUM_TX_SESSIONS 2
#define NONCE_TIMEOUT 10
#define MAX_NONCES 10
#define MAX_RXSESSIONS 2

typedef struct nonce {
  uint8_t src;
  uint8_t dst;
  uint8_t timeout;
  uint8_t reply_nonce; //indicate if this nonce from a enc message sent by me
  uint8_t nonce[8];
  struct ctimer ctimer;
} nonce_t;

typedef enum {
  RX_INIT,
  RX_ENC1,
  RX_ENC2,
  RX_SESSION_DONE
} rx_session_state_t;

typedef struct {
  uint8_t snode;
  uint8_t dnode;
  rx_session_state_t state;
  uint8_t seq_nr;
  uint8_t msg[S0_MAX_ENCRYPTED_MSG_SIZE];
  uint8_t msg_len;
  uint32_t timeout;
} rx_session_t;

extern  void AES128_ECB_encrypt(uint8_t* input, const uint8_t* key, uint8_t *output);

#define NONCE_OPT 0
#define NONCE_TIMEOUT_MSEC ( NONCE_TIMEOUT * CLOCK_SECOND ) /* Validity time of a received nonce in milliseconds*/
#define TIMEOUT_ON 1
#define TIMEOUT_OFF 0

#define NONCE_REQUEST_TIMEOUT (10 * CLOCK_SECOND) /** Maximum awake waiting time for receiving a nonce report after requesting it, in milliseconds. */
#define NONCE_REPORT_TIMEOUT  500                 /** Maximum awake waiting time for receiving a secure encapsulation after sending nonce report, in milliseconds */

/**/
#define node_copy(pDst, pSrc) *(pDst) = *(pSrc)


// commandClassIdentifier, commandIdentifier, commandByte is payload in
// ZW_SECURITY_MESSAGE_ENCAPSULATION_1BYTE_FRAME
#define S0_PAYLOAD 3

// ZW_SECURITY_MESSAGE_ENCAPSULATION_1BYTE_FRAME = 23 bytes
#define S0_ENCAP_HEADER_LEN (23 - S0_PAYLOAD)

typedef enum {
  NONCE_GET,
  NONCE_GET_SENT,
  ENC_MSG,
  ENC_MSG_SENT,
  ENC_MSG2,
  ENC_MSG2_SENT,
  TX_DONE,
  TX_FAIL} tx_state_t;

typedef struct sec_tx_session {
  TRANSMIT_OPTIONS_TYPE txOpt;
  const uint8_t* pData;            //Pointer the data to be encrypted
  uint8_t data_len;         //Length of the data to be encrypted
  tx_state_t state;
  struct ctimer timer;      //Session timer
  uint8_t crypted_msg[46];
  uint8_t seq;              //Sequence number used in multisegment messages
  STransmitCallback callback;  //Callback function, called when the session has been terminated
  /* This struct holds the extended tx status info until it can be delivered to the application.
   * It will be delivered when S0 protocol has finished. */
  TX_STATUS_TYPE sExtendedTxStatusS0;
} sec_tx_session_t;

typedef struct _AUTHDATA_ {
  uint8_t sh; /* Security Header for authentication */
  uint8_t senderNodeID; /* Sender ID for authentication */
  uint8_t receiverNodeID; /* Receiver ID for authentication */
  uint8_t payloadLength; /* Length of authenticated payload */
} auth_data_t;


static sec_tx_session_t the_tx_session;

/** Power lock for staying awake after requesting a Nonce Report */
static zpal_pm_handle_t s0_tx_power_lock;
/** Power lock for staying awake after sending a Nonce Report, waiting to RX an encrypted message */
static zpal_pm_handle_t s0_rx_power_lock;

// Storage of the frame to be send, so that S0 does not require the package to remain
// available in the higher earlier levels of protocol and app.
static uint8_t aFrame[TX_BUFFER_SIZE];

/************************ AES Helper functions ********************************************/
static uint8_t aes_key[16];
static uint8_t aes_iv[16];

void aes_encrypt(uint8_t *in, uint8_t* out) {
#ifdef ZWAVE_PSA_AES
  uint32_t key_id = ZWAVE_ECB_TEMP_ENC_KEY_ID;
  /* Import key into secure vault */
  zw_wrap_aes_key_secure_vault(&key_id, aes_key, ZW_PSA_ALG_ECB_NO_PAD);
  zw_psa_aes_ecb_encrypt(key_id, in, out);
  /* Remove key from vault */
  zw_psa_destroy_key(key_id);
#else
  AES128_ECB_encrypt(in, aes_key, out);
#endif
}

void aes_set_key(uint8_t* key,uint8_t* iv) {
  memcpy(aes_key,key,16);
  memcpy(aes_iv,iv,16);
}

void aes_ofb(uint8_t* pData,uint8_t len) {
  uint8_t i;
  for (i = 0; i < len; i++)
  {
    if ((i & 0xF) == 0x0)
    {
      aes_encrypt(aes_iv, aes_iv);
    }
    pData[i] ^= aes_iv[(i & 0xF)];
  }
}

/*
 * Calculate the authtag for the message,
 */
void aes_cbc_mac(uint8_t* pData,uint8_t len,uint8_t* mac) {
  uint8_t i;

  DPRINT("/n");

  aes_encrypt(aes_iv,mac);
  for (i = 0; i < len; i++)
  {
    mac[i & 0xF] ^= pData[i];
    if ((i & 0xF) == 0xF)
    {
      aes_encrypt(mac, mac);
    }
  }

  /*if len is not divisible by 16 do the final pass, this is the padding described in the spec */
  if(len & 0xF)
  {
    aes_encrypt(mac, mac);
  }
}


void aes_random8(uint8_t*d ) {
  zpal_get_random_data(d, 8);
}

/******************************** Nonce Management **********************************************/

static nonce_t nonce_table[NONCE_TABLE_SIZE];  //Nonces received or sent

void ZCB_nonce_timer_timeout(void* pData);

/**
 * Register a new nonce from sent from src to dst
 */
static uint8_t register_nonce(uint8_t src, uint8_t dst,uint8_t reply_nonce, uint8_t nonce[8]) {
  uint8_t i;

  DPRINT("/k");
  if(reply_nonce) {
    /*Only one reply nonce is allowed*/
    for (i=0; i< NONCE_TABLE_SIZE; i++)
    {
      if( nonce_table[i].reply_nonce &&
          nonce_table[i].timeout > 0 &&
          nonce_table[i].src == src &&
          nonce_table[i].dst == dst)
      {
        DPRINT("WARNING: Reply nonce overwritten\n\r");
        memcpy(nonce_table[i].nonce,nonce,8);
        nonce_table[i].timeout = TIMEOUT_ON;
        DPRINTF("## New table entry %d\r\n",i);
        ctimer_set(&nonce_table[i].ctimer, NONCE_TIMEOUT_MSEC, ZCB_nonce_timer_timeout, (void *)&nonce_table[i]);
        return 1;
      }
    }
  }

  for (i=0; i< NONCE_TABLE_SIZE; i++)
  {
    if(nonce_table[i].timeout == 0)
    {
      nonce_table[i].src = src;
      nonce_table[i].dst = dst;
      nonce_table[i].reply_nonce = reply_nonce;
      memcpy(nonce_table[i].nonce,nonce,8);
      nonce_table[i].timeout = TIMEOUT_ON;
      DPRINTF("## New table entry %d\r\n",i);
      ctimer_set(&nonce_table[i].ctimer, NONCE_TIMEOUT_MSEC, ZCB_nonce_timer_timeout, (void *)&nonce_table[i]);
      return 1;
    }
  }

  DPRINT("ERROR: Nonce table is full\n\r");
  return 0;
}

/**
 * Receive nonce sent from src to dst, if th ri. If a nonce
 * is found the remove all entries from that src->dst combination
 * from the table.
 *
 * If If any_nonce is set then ri is ignored
 */
static uint8_t get_nonce(uint8_t src, uint8_t dst,uint8_t ri, uint8_t nonce[8],uint8_t any_nonce) {
  uint8_t i;

  for (i=0; i< NONCE_TABLE_SIZE; i++)
  {
    if(nonce_table[i].timeout>0 && nonce_table[i].src == src && nonce_table[i].dst == dst)
    {
      if(any_nonce ||  nonce_table[i].nonce[0] == ri)
      {
        memcpy(nonce,nonce_table[i].nonce,8);
        return 1;
      }
    }
  }
  return 0;
}


static void nonce_clear(uint8_t src, uint8_t dst)
{
  uint8_t i;
  /*Remove entries from table from that source dest combination */
  for (i=0; i< NONCE_TABLE_SIZE; i++)
  {
    if(nonce_table[i].timeout && (nonce_table[i].src == src) && (nonce_table[i].dst == dst))
    {
      DPRINT("WARNING: Clearing nonce entry\n");
      nonce_table[i].timeout = TIMEOUT_OFF;
      ctimer_stop(&nonce_table[i].ctimer);
    }
  }
}


void ZCB_nonce_timer_timeout(void* pData)
{
  nonce_t *ptr = (nonce_t *) pData;

  DPRINT("## Timeout expired\r\n");
  ptr->timeout = TIMEOUT_OFF;
}


/********************************Security TX Code ***************************************************************/
static void tx_session_state_set(tx_state_t state); // reentrant;
static uint8_t enckey[16];
static uint8_t authkey[16];

/**
 * Retrieve netkey from keystore (NVM) and
 * return true if netkey valid
 *        false if netkey ZERO
 */
bool
sec0_unpersist_netkey(void)
{
#if defined(ZW_SLAVE_ROUTING) || defined(ZW_CONTROLLER)
/* slave_routing_ZW050x doesnt support NVM yet */
  uint8_t p[16];
  bool retVal = true;

  DPRINT("/s");
  if (false == keystore_network_key_read(KEY_CLASS_S0, aes_key))
  {
    memset(aes_key, 0, 16);
    retVal = false;
  }
#ifdef DEBUGPRINT
  {
    uint8_t b;
    for (b=0; b<16; b++)
    {
      if (aes_key[b])
      {
        DPRINT("/!");
        break;
      }
    }
  }
#endif
  memset(p,0x55,16);
  aes_encrypt(p,authkey);
  memset(p,0xAA,16);
  aes_encrypt(p,enckey);
#ifdef DEBUGPRINT
  {
    for (uint32_t b = 0; b < 16; b++)
    {
      DPRINTF("%02X", authkey[b]);
    }

    DPRINT("\r\n");
  }
#endif
  return retVal;
#else
  return false;
#endif /* ZW_SLAVE_ROUTING || ZW_CONTROLLER */
}


void sec0_persist_netkey( __attribute__((unused)) uint8_t *netkey)
{
#if defined(ZW_SLAVE_ROUTING) || defined(ZW_CONTROLLER)
/* slave_routing_ZW050x doesnt support NVM yet */
  keystore_network_key_write(KEY_CLASS_S0, netkey);
#endif
}


void ZCB_tx_timeout(__attribute__((unused)) void* user) {
  DPRINT("ERROR: Security0 transmit timeout\n\r");
  tx_session_state_set(TX_FAIL);
}

/**
 * Encrypt a message and write the encrypted data into the_tx_session.crypted_msg
 */
static uint8_t encrypt_msg(uint8_t pass2) {
  uint8_t iv[16] = { 0 }; /* Initialization vector for enc, dec,& auth */
  uint8_t mac[16] = { 0 };
  uint8_t tmpnonce[8]; /* temporary work nonce */
  uint8_t len; // Length of the encrypted part
  uint8_t more_to_send;
  uint8_t* enc_data;
  auth_data_t* auth;
  uint8_t maxlen = 46;

  len = the_tx_session.data_len;
  more_to_send = 0;

  /*Check if we should break this message in two */
  if(len + 20 > maxlen) {
    len = maxlen-20;
    more_to_send = 1;
  }

  ASSERT((size_t)(len + 20) <= sizeof(the_tx_session.crypted_msg));
  /* Make the IV */

  do {
    aes_random8(iv);
  } while( get_nonce(the_tx_session.txOpt.bSrcNode,the_tx_session.txOpt.destNode,iv[0],tmpnonce,false) );

  /*Choose a nonce from sender */
  if(!get_nonce(the_tx_session.txOpt.destNode,the_tx_session.txOpt.bSrcNode,0,iv+8,true))
  {
    DPRINTF("ERROR: Nonce for node %d --> %d is not found\n\r", the_tx_session.txOpt.destNode, the_tx_session.txOpt.bSrcNode);
    return 0;
  }
  else
  {
    nonce_clear(the_tx_session.txOpt.destNode,the_tx_session.txOpt.bSrcNode);
  }

  /*Register my nonce */
  register_nonce(the_tx_session.txOpt.bSrcNode,the_tx_session.txOpt.destNode,true,iv);

  /* Setup pointers */
  enc_data = the_tx_session.crypted_msg + 10;
  auth = (auth_data_t* )(the_tx_session.crypted_msg + 6);

  /* Copy data into a second buffer Insert security flags */

  if(pass2)
  {
    *enc_data = SECURITY_MESSAGE_ENCAPSULATION_PROPERTIES1_SEQUENCED_BIT_MASK | SECURITY_MESSAGE_ENCAPSULATION_PROPERTIES1_SECOND_FRAME_BIT_MASK | (the_tx_session.seq & 0xF);
  }
  else if(more_to_send)
  {
    *enc_data = SECURITY_MESSAGE_ENCAPSULATION_PROPERTIES1_SEQUENCED_BIT_MASK  | (the_tx_session.seq & 0xF);
  }
  else
  {
    *enc_data = 0;
  }

  memcpy(enc_data+1, the_tx_session.pData, len);

  /*Encrypt */
  aes_set_key(enckey,iv);
  aes_ofb(enc_data,len+1);

  /*Fill in the auth structure*/
  auth->sh = more_to_send ? SECURITY_MESSAGE_ENCAPSULATION_NONCE_GET : SECURITY_MESSAGE_ENCAPSULATION;
  auth->senderNodeID = the_tx_session.txOpt.bSrcNode;
  auth->receiverNodeID = the_tx_session.txOpt.destNode;
  auth->payloadLength = len + 1;

  /* Authtag */
  aes_set_key(authkey,iv);
  aes_cbc_mac((uint8_t*)auth,4 + len+1,mac);

  the_tx_session.crypted_msg[0] = COMMAND_CLASS_SECURITY;
  the_tx_session.crypted_msg[1] = auth->sh;
  memcpy(the_tx_session.crypted_msg+2, iv,8);

  the_tx_session.crypted_msg[2 +8 + len + 1] = iv[8];
  memcpy(the_tx_session.crypted_msg+2 + 8 + len + 2,mac,8);

  the_tx_session.pData += len;
  the_tx_session.data_len -=len;
  return len + 20;
}


static void ZCB_noce_get_callback(__attribute__((unused)) ZW_Void_Function_t Context, uint8_t status,
                                  __attribute__((unused)) TX_STATUS_TYPE* extendedTxStatus)
{
  DPRINTF("/g%02X", the_tx_session.state);
  if (the_tx_session.state == NONCE_GET)
  {
    tx_session_state_set( status == TRANSMIT_COMPLETE_OK ? NONCE_GET_SENT : TX_FAIL);
  }
}


static void ZCB_msg1_callback(__attribute__((unused)) ZW_Void_Function_t Context, uint8_t status, TX_STATUS_TYPE* extendedTxStatus)
{
  if (the_tx_session.state == ENC_MSG)
  {
    the_tx_session.sExtendedTxStatusS0 = *extendedTxStatus;
    tx_session_state_set( status == TRANSMIT_COMPLETE_OK ? ENC_MSG_SENT : TX_FAIL);
  }
}


static void ZCB_msg2_callback(__attribute__((unused)) ZW_Void_Function_t Context, uint8_t status, TX_STATUS_TYPE* extendedTxStatus)
{
  if (the_tx_session.state == ENC_MSG2)
  {
    the_tx_session.sExtendedTxStatusS0 = *extendedTxStatus;
    tx_session_state_set( status == TRANSMIT_COMPLETE_OK ? ENC_MSG2_SENT : TX_FAIL);
  }
}


static void tx_session_state_set(tx_state_t state)
// reentrant
{
  uint8_t len;
#if NONCE_OPT
  uint8_t dummy[8];
#endif

  static const uint8_t nonce_get[] =  { COMMAND_CLASS_SECURITY, SECURITY_NONCE_GET };
  DPRINTF("/t%02X", state);
  the_tx_session.state = state;

  DPRINTF("State %d\n\r",state);
  switch(the_tx_session.state) {
  case NONCE_GET:
#if NONCE_OPT
    if(get_nonce(the_tx_session.txOpt.destNode,the_tx_session.txOpt.bSrcNode,0,dummy))
    {
      tx_session_state_set(ENC_MSG);
      return;
    }
#endif
    ctimer_set(&the_tx_session.timer,NONCE_REQUEST_TIMEOUT,ZCB_tx_timeout,0);
    const STransmitCallback TxCallbackNoceGet = { .pCallback = ZCB_noce_get_callback, .Context = 0 };
    if(!ZW_SendDataEx(nonce_get,sizeof(nonce_get),
        &the_tx_session.txOpt, &TxCallbackNoceGet) )
    {
      tx_session_state_set(TX_FAIL);
    }
    zpal_pm_stay_awake(s0_tx_power_lock, NONCE_REQUEST_TIMEOUT);
    break;

  case NONCE_GET_SENT:
	// TODO: Shorter timeout...
    break;

  case ENC_MSG:
    ctimer_stop(&the_tx_session.timer);
    len = encrypt_msg(0);
    nonce_clear(the_tx_session.txOpt.bSrcNode, the_tx_session.txOpt.destNode);
    const STransmitCallback TxCallbackMsg1 = { .pCallback = ZCB_msg1_callback, .Context = 0 };
    if(len==0 || !ZW_SendDataEx(the_tx_session.crypted_msg, len, &the_tx_session.txOpt, &TxCallbackMsg1) )
    {
      DPRINT("/X");
      tx_session_state_set(TX_FAIL);
    }
    else
    {
      DPRINTF("m%02X", the_tx_session.data_len);
      if(the_tx_session.data_len!=0)
      {
        /*Wait for the second nonce*/
        ctimer_set(&the_tx_session.timer,NONCE_REQUEST_TIMEOUT,ZCB_tx_timeout,0);
        zpal_pm_stay_awake(s0_tx_power_lock, NONCE_REQUEST_TIMEOUT);  /* TODO: Shorter timeout based on round-trip time for first message */
      }
    }
    break;

  case ENC_MSG_SENT:
    /*If there is no more data to send*/
    if(the_tx_session.data_len == 0)
    {
      tx_session_state_set(TX_DONE);
    }
    break;

  case ENC_MSG2:
    ctimer_stop(&the_tx_session.timer);
    len = encrypt_msg(1);
    nonce_clear(the_tx_session.txOpt.bSrcNode, the_tx_session.txOpt.destNode);
    const STransmitCallback TxCallbackMsg2 = { .pCallback = ZCB_msg2_callback, .Context = 0 };
    if(len == 0 || !ZW_SendDataEx(the_tx_session.crypted_msg,len, &the_tx_session.txOpt, &TxCallbackMsg2) )
    {
      tx_session_state_set(TX_FAIL);
    }
    break;

  case ENC_MSG2_SENT:
    tx_session_state_set(TX_DONE);
    break;

  case TX_DONE:
    the_tx_session.txOpt.destNode = 0;
    ctimer_stop(&the_tx_session.timer);
    zpal_pm_cancel(s0_tx_power_lock);
    ZW_TransmitCallbackInvoke(&the_tx_session.callback, TRANSMIT_COMPLETE_OK, &the_tx_session.sExtendedTxStatusS0);
    break;

  case TX_FAIL:
    the_tx_session.txOpt.destNode = 0;
    ctimer_stop(&the_tx_session.timer);
    zpal_pm_cancel(s0_tx_power_lock);
    ZW_TransmitCallbackInvoke(&the_tx_session.callback, TRANSMIT_COMPLETE_FAIL, &the_tx_session.sExtendedTxStatusS0);
    break;
  }
}

/**
 * Get the next sequence number no node may recieve the same sequence number in two concurrent transmissions
 */
static uint8_t get_seq(void) {
  static uint8_t s=0;
  s++; //FIXME make this routine do the right thing.
  return s;
}

uint8_t sec0_send_data(node_t snode, node_t dnode,const void* pData, uint8_t len, TxOptions_t txoptions,
  const STransmitCallback* pCallback) {
  DPRINT("/n");
  if (the_tx_session.txOpt.destNode != 0)
  {
    DPRINTF("ERROR: Already have one tx session from node %d to %d\n\r", snode, dnode);
    return false;
  }

  //Safeguard against buffer overflow
  if (len > sizeof(aFrame))
  {
    return false;
  }

  memcpy(aFrame, pData, len);

  the_tx_session.txOpt.bSrcNode = snode;
  the_tx_session.txOpt.destNode = dnode;
  the_tx_session.pData = (uint8_t*)aFrame;
  the_tx_session.data_len = len;
  the_tx_session.txOpt.txOptions = txoptions;
  the_tx_session.callback = *pCallback;
  the_tx_session.seq = get_seq();

  DPRINTF("New sessions for src %d dst %d\n\r", snode, dnode);
  tx_session_state_set(NONCE_GET);
  return true;
}

/**
 * Register a nonce by source and destination
 */
void sec0_register_nonce(uint8_t src, uint8_t dst, uint8_t* nonce) {
  DPRINTF("sec0_register_nonce: dst: %d, src: %d\n\r", dst, src);
  if(the_tx_session.txOpt.destNode == src && the_tx_session.txOpt.bSrcNode == dst)
  {
    register_nonce(src,dst,false,nonce);
    if( the_tx_session.state == NONCE_GET_SENT || the_tx_session.state == NONCE_GET )
    {
      //memcpy(the_tx_session.nonce,nonce,8);
      tx_session_state_set(ENC_MSG);
    } else if (the_tx_session.state == ENC_MSG_SENT  || (the_tx_session.state == ENC_MSG && the_tx_session.data_len>0))
    {
      //memcpy(the_tx_session.nonce,nonce,8);
      tx_session_state_set(ENC_MSG2);
    }
  }
  else
  {
    DPRINTF("ERROR: Nonce report but not for me src %d dst %d\n\r", src, dst);
  }
}

/******************************** Security RX code ***********************************/


/*
 *
 * I could do a properly rx session if it was allowed to report the same nonce
 * multiple times if it was unused.
 *
 * I transmit NONCE_GET, but i miss the ack so it its transmitted multiple times
 * I thus receive multiple reports. Couldnt these report contain the same nonce?
 * I dont see the problem, I code
 *
 */
static uint8_t is_expired(uint32_t timeout) {
  uint32_t diff = timeout - xTaskGetTickCount();
  /* Check if there was an overflow or if diff is actually 0 */
  return ((diff & 0x80000000UL) != 0) || diff == 0;
}

uint8_t is_free(rx_session_t* e) {
  return ((e->state == RX_SESSION_DONE) || is_expired(e->timeout));
}

static rx_session_t rxsessions[MAX_RXSESSIONS];

/**
 * Get a new free RX session.
 */
rx_session_t* new_rx_session(uint8_t snode,uint8_t dnode) {
  uint8_t i;
  for(i=0; i < MAX_RXSESSIONS; i++)
  {
    if( is_free(&rxsessions[i]))
    {
      rxsessions[i].snode=snode;
      rxsessions[i].dnode=dnode;
      rxsessions[i].timeout = xTaskGetTickCount() + CLOCK_SECOND*10; //Timeout in 10s
      return &rxsessions[i];
    }
  }
  return 0;
}

void free_rx_session(rx_session_t* s) {
  //memset(the_tx_session.nonce,0,sizeof(the_tx_session.nonce));
  s->timeout = 0;
  s->state = RX_SESSION_DONE;
}

void sec0_register_power_locks(void)
{
  s0_tx_power_lock = zpal_pm_register(ZPAL_PM_TYPE_USE_RADIO);
  s0_rx_power_lock = zpal_pm_register(ZPAL_PM_TYPE_USE_RADIO);
}

bool sec0_init(void) {
  uint8_t i, sec0_isActivated;
  sec0_isActivated = sec0_unpersist_netkey();

  for(i=0; i < MAX_RXSESSIONS; i++)
  {
    free_rx_session(&rxsessions[i]);
  }
  for(i=0; i < NUM_TX_SESSIONS; i++)
  {
    ctimer_stop(&the_tx_session.timer);
    memset((uint8_t*)&the_tx_session,0,sizeof(sec_tx_session_t));
  }

  for(i =0; i < NONCE_TABLE_SIZE; i++)
  {
    nonce_table[i].timeout = TIMEOUT_OFF;
    ctimer_stop(&nonce_table[i].ctimer); // is it needed here?
  }

  zpal_pm_cancel(s0_tx_power_lock);
  zpal_pm_cancel(s0_rx_power_lock);

  DPRINTF("&%02X", sec0_isActivated);
  return sec0_isActivated;
}

void sec0_abort_all_tx_sessions(void) {
  if(the_tx_session.txOpt.destNode)
  {
    tx_session_state_set(TX_FAIL);
  }
}


/* Report if Security 0 layer are idle or active (any rxsession, txsession or nonce active) */
/* Returns: SEC0_STATE_NONCE_ACTIVE     - if Security 0 layer has an active NONCE */
/*          SEC0_STATE_RXSESSION_ACTIVE - if Security 0 layer has an active rx session */
/*          SEC0_STATE_TXSESSION_ACTIVE - if Security 0 layer has an active tx session */
/*          SEC0_STATE_IDLE             - if Security 0 layer is IDLE */
sec0_state_t
sec0_state(void)
{
  uint8_t i;
  rx_session_t* e;
  /* Are we Securtiy 0 included - do we have a network key - Correct for Slaves only??? */
  if (SECURITY_KEY_S0_BIT & keystore_get_cached_security_keys())
  {
    /* Any rxsessions active */
    e = &rxsessions[0];
    for(i = 0; i < MAX_RXSESSIONS; i++, e++)
    {
      if (!is_free(e)) {
        return SEC0_STATE_RXSESSION_ACTIVE;
      } else {
        /*reset the state and the time counter*/
        free_rx_session(e);
      }
    }
    /* Any txsessions active */
    if (the_tx_session.txOpt.destNode)
    {
      return SEC0_STATE_TXSESSION_ACTIVE;
    }
    /* Any NONCE active */
    for (i = 0; i < NONCE_TABLE_SIZE; i++)
    {
      if (nonce_table[i].timeout != 0)
      {
        return SEC0_STATE_NONCE_ACTIVE;
      }
    }
  }
  return SEC0_STATE_IDLE;
}


/**
 * Get a specific nonce from the nonce table. The session must not be expired
 */
rx_session_t* get_rx_session_by_nodes(uint8_t snode, uint8_t dnode) {
  uint8_t i;
  rx_session_t* e;
  for(i=0; i < MAX_RXSESSIONS; i++)
  {
    e = &rxsessions[i];
    if( !is_free(e) &&
        e->dnode == dnode &&
        e->snode == snode)
    {
      return e;
    }
  }
  return 0;
}

/**
 * Send a nonce from given source to given destination. The nonce is registered internally
 */
void sec0_send_nonce(uint8_t snode, uint8_t dnode, TxOptions_t txoptions) {
  uint8_t nonce[8] = {0};
  uint8_t tmpnonce[8]; /* temporary work nonce */
  static ZW_SECURITY_NONCE_REPORT_FRAME nonce_res;
  static TRANSMIT_OPTIONS_TYPE txo;

  memset((uint8_t*)&txo, 0, sizeof(txo));
  memset((uint8_t*)&nonce_res, 0, sizeof(nonce_res));
  /*Swap source and destination*/
  txo.bSrcNode = dnode;
  txo.destNode = snode;
  txo.txOptions = txoptions;

  do {
    aes_random8(nonce);
  } while( get_nonce(dnode,snode,nonce[0],tmpnonce,false) );

  nonce_res.cmdClass = COMMAND_CLASS_SECURITY;
  nonce_res.cmd = SECURITY_NONCE_REPORT;
  memcpy(&nonce_res.nonceByte1,nonce,8);


  const STransmitCallback TxCallback = { .pCallback = 0, .Context = 0 };
  if( ZW_SendDataEx((uint8_t*)&nonce_res,sizeof(nonce_res), &txo, &TxCallback))
  {
    DPRINT("/x");
    register_nonce(dnode,snode,false,nonce);
    zpal_pm_stay_awake(s0_rx_power_lock, NONCE_REPORT_TIMEOUT); /* Stay awake to receive the encrypted message */
  }
}

/**
 * Decrypt an encrypted message, returning the length of the decrypted message. The decrypted message
 * is written into dec_message
 */
uint8_t sec0_decrypt_message(uint8_t snode, uint8_t dnode, uint8_t* enc_data, uint8_t enc_data_length,uint8_t* dec_message) {
    uint8_t iv[16] = { 0 }; /* Initialization vector for enc, dec,& auth */
    uint8_t mac[16] = { 0 };
    rx_session_t *s;
    uint8_t *enc_payload;
    uint8_t ri;
    auth_data_t* auth;
    uint8_t flags;
    uint8_t i=0;
    // Allocate a buffer that's large enough for the largest frame size that's valid
    // Maximum valid size is S0_MAX_ENCRYPTED_MSG_SIZE + overhead + auth header size
    uint8_t auth_buff[S0_MAX_ENCRYPTED_MSG_SIZE + S0_ENCAP_HEADER_LEN + sizeof(auth_data_t)];

    if(enc_data_length < S0_ENCAP_HEADER_LEN)
    {
      DPRINT("ERROR: Encrypted message is too short\n\r");
      return 0;
    }


    if((enc_data_length - S0_ENCAP_HEADER_LEN) > S0_MAX_ENCRYPTED_MSG_SIZE) {
      DPRINT("ERROR: Encrypted message is too long\n\r");
      return 0;
    }

    ri =  enc_data[enc_data_length-9];

    /*Build the IV*/
    memcpy(iv,enc_data+2,8);

    /*Find the nonce in the nonce table */
    if(!get_nonce(dnode,snode,ri,&iv[8],false))
    {
      DPRINTF("ERROR: Nonce for %d -> %d not found", dnode, snode);
      return 0;
    }
    else
    {
      nonce_clear(dnode,snode);
    }

#if NONCE_OPT
    register_nonce(snode,dnode,enc_data+2);
#endif

    s = get_rx_session_by_nodes(snode,dnode);
    if(!s)
    {
      s = new_rx_session(snode,dnode);
      if(s)
      {
        s->state = RX_INIT;
//        s->state = RX_ENC1;
      }
      else
      {
        DPRINT("WARNING: no more RX sessions available\n\r");
        return 0;
      }
    }

    // When we get a session that's in progress, verify the size of the data we have and the data we're about
    // to add do not go over our total output buffer size. If it does, drop the frame and the session pool will free up the
    // invalid session in a little bit.
    if((s->state != RX_INIT) && ((s->msg_len + enc_data_length - S0_ENCAP_HEADER_LEN) > S0_MAX_ENCRYPTED_MSG_SIZE)) {
      DPRINT("ERROR: Combined data for encrypted message is too long\n\r");
      return 0;
    }

    enc_payload = enc_data+2+8;

    /*Temporary use dec_message for auth verification*/
    auth = (auth_data_t*)auth_buff;
    /*Fill in the auth structure*/
    auth->sh = enc_data[1];
    auth->senderNodeID = snode;
    auth->receiverNodeID = dnode;
    auth->payloadLength = enc_data_length - 19;
    memcpy( (uint8_t*)auth+4, enc_payload, auth->payloadLength);

    /* Authtag */
    aes_set_key(authkey,iv);

      DPRINT("%");
      for (i = 0; i < 16; i++)
      {
        DPRINTF("%02X", iv[i]);
      }
      DPRINT("-");

    aes_cbc_mac((uint8_t*)auth,4 + auth->payloadLength,mac);

      DPRINT("-");
      for (i = 0; i < 4 + auth->payloadLength; i++)
      {
        DPRINTF("%02X", ((uint8_t*)auth)[i]);
      }
      DPRINT("-");

    if(memcmp(mac, enc_data+ enc_data_length-8,8) !=0 )
    {
      DPRINT("ERROR: Unable to verify auth tag\n\r");
      for (i = 0; i < 16; i++)
      {
        DPRINTF("%02X", authkey[i]);
      }
      DPRINT("-");
      for (i = 0; i < 16; i++)
      {
        DPRINTF("%02X", mac[i]);
      }
      return 0;
    }
    DPRINT("Authentication verified\n\r");
    /*Decrypt */
    aes_set_key(enckey,iv);
    aes_ofb(enc_payload,auth->payloadLength);

    flags = *enc_payload;

    if(flags & SECURITY_MESSAGE_ENCAPSULATION_PROPERTIES1_SEQUENCED_BIT_MASK )
    {
      if((flags & SECURITY_MESSAGE_ENCAPSULATION_PROPERTIES1_SECOND_FRAME_BIT_MASK)==0)
      {
        //First frame
        s->seq_nr = flags & 0xF;
        DPRINTF("ERROR: State is %i seq %i expected %i\n\r", s->state, flags & 0xF, s->seq_nr);
        s->msg_len = enc_data_length - 20;
        s->state=RX_ENC1;
        memcpy(s->msg, enc_payload+1,s->msg_len);
        return 0;
      }
      else
      {
        //Second frame
        if((s->state != RX_ENC1) || (flags & 0xF) != s->seq_nr)
        {
          DPRINTF("ERROR: State is %i seq %i expecetd %i\n\r",s->state,flags & 0xF,s->seq_nr);
          goto state_error;
        }
        else
        {
          s->state  = RX_ENC2;
        }
        memcpy(dec_message,s->msg, s->msg_len);
        memcpy(dec_message + s->msg_len,enc_payload+1 ,enc_data_length - 20);

        free_rx_session(s);
        return (s->msg_len + enc_data_length - 20);
      }
    }
    else
    {
      /* Single frame message */
      memcpy(dec_message,enc_payload+1 ,enc_data_length - 20);
      free_rx_session(s);
      /* Allow us to go back to sleep, since we now received a secure message after sending out a nonce report */
      zpal_pm_cancel(s0_rx_power_lock);
      return (enc_data_length - 20);
    }
    return 0;
state_error:
  DPRINT("ERROR: Security RX session is not in the right state\n\r");
  return 0;
}

/******* Wakeup handling ***********/
#if 0
/*===========================   ZW_SecurePowerUpInit   =======================
**    InitSecurePowerUp
**
**    Side effects :
**
**--------------------------------------------------------------------------*/
/*
Declaration: void ZW_SecurePowerUpInit()
Called: On power-up or reset
Task: Reset internal nonce table, external nonce record, and nonce request record
(mark all as vacant) and register timer service
Arguments: None
Return value: None
Global vars: None
ltemp data: None
*/
void
ZW_SecurePowerUpInit(void)
{
 /* Fixme: Add wakeup handling*/
#if 0
  /* Reset nonce tables */
  InitSecureWakeUp();
  /* Register timer service (100 ms, forever) */
  if (!NonceTimerServiceHanler)
  {
    NonceTimerServiceHanler = TimerStart(ZCB_NonceTimerService, 10, TIMER_FOREVER);
  }
#endif
}
#endif

void sec0_clear_netkey(void)
{
#if defined(ZW_SLAVE_ROUTING) || defined(ZW_CONTROLLER)
  /* slave_routing_ZW050x doesnt support NVM yet */
  keystore_network_key_clear(KEY_CLASS_S0);
#endif
}

bool
sec0_busy(void)
{
  return (SEC0_STATE_IDLE != sec0_state());
}
