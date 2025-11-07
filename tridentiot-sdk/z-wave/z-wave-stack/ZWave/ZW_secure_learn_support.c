// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file ZW_secure_learn_support.c
 * @copyright 2022 Silicon Laboratories Inc.
 */
#include "ZW_lib_defines.h"

#include <sc_types.h>
#include "Secure_learnRequired.h"
#include "Secure_learn.h"
#include "ZW_ctimer.h"
#include "ZW_secure_learn_support.h"
#include "ZW_Security_Scheme0.h"
#include <ZW_Security_Scheme2.h>
#include <ZW_security_api.h>
#include <s2_classcmd.h>
#include "ZW_CCList.h"
#ifdef ZW_CONTROLLER
#include <ZW_controller_api.h>
#elif defined ZW_SLAVE
#include <ZW_slave.h>
#endif
#include <ZW_protocol.h>

#include "ZW_keystore.h"
#include <ZW_main.h>
#include <Assert.h>

//#define DEBUGPRINT
#include <DebugPrint.h>


#define PRINT_STATE() DPRINTF("+%X", ctx.stateConfVector[0]);

#define KEY_CLASS_S0                  0x80
#define KEY_CLASS_S2_UNAUTHENTICATED  0x01
#define KEY_CLASS_S2_AUTHENTICATED    0x02
#define KEY_CLASS_S2_ACCESS           0x04

#define KEY_CLASS_ALL                 0xFF

#define SECURITY_SCHEME_0_BIT 0x1

#define SECURITY_SCHEME_NONE  0
#define SECURITY_SCHEME_S0    1

//Number of implemented schemes
#define N_SCHEMES 1

static Secure_learn ctx = { 0 };
static struct ctimer timer = { 0 };
static uint8_t m_SecureKeysRequested = 0;

static E_SECURE_LEARN_ERRNO secureLearnErrno;

const ZW_NETWORK_KEY_VERIFY_FRAME key_verify ={
  .cmdClass = COMMAND_CLASS_SECURITY,
  .cmd = NETWORK_KEY_VERIFY
};


/*Forward */
static void send_and_raise(node_id_t node, uint8_t *pBufData, uint8_t dataLength, TxOptions_t txOptions, bool secure);

/**
 * These frames are never used at the same time.
 */
static union
{
  ZW_SECURITY_SCHEME_INHERIT_FRAME scheme_inherit_frame;
  ZW_SECURITY_SCHEME_REPORT_FRAME scheme_report_frame;
  ZW_SECURITY_SCHEME_GET_FRAME scheme_get_frame;
  uint8_t key_set_frame[16 + 2];
  uint8_t cmd[64];
} tx = { 0 };

#ifdef ZW_CONTROLLER
static LEARN_INFO_T sLearnInfo = { 0 };

extern sec_add_complete_t secureAddAppCbFunc;
#endif
static sec_learn_complete_t secureLearnCompleteCallback;

void runCycle(void)
{
  uint8_t oldState = ctx.stateConfVector[0];  
  secure_learn_runCycle(&ctx);
  if (oldState != ctx.stateConfVector[0])
  {
/*
   Security 0 inclusion statemachine changed state, so we need to run runCycle again. We do this by
   triggering an event.
*/    
    SecurityRunCycleNotify();
    DPRINTF("+%02X%02X", oldState, ctx.stateConfVector[0]);
  }


}
/************************************* Common functions *******************************************/

/**
 * Returns the current secureLearnErrno - useful in Smart Start to determine if
 * s2 inclusion was successful
 */
E_SECURE_LEARN_ERRNO
secure_learn_get_errno(void)
{
  return secureLearnErrno;
}


/**
 * Set the current secureLearnErrno - useful in Smart Start to determine if
 * s2 inclusion was successful
 */
void
secure_learn_set_errno(E_SECURE_LEARN_ERRNO error_number)
{
  secureLearnErrno = error_number;
}


/**
 * Set the current secureLearnErrno E_SECURE_LEARN_ERRNO_COMPLETE (no error)
 */
void
secure_learn_set_errno_reset(void)
{
  secureLearnErrno = E_SECURE_LEARN_ERRNO_COMPLETE;
}

/*
 * Save security states to eeprom
 */
void
security_save_state(void)
{
  /* The netkey is already persisted when receiving Key Set frame,
   * no need to do it here again. */
  //sec0_persist_netkey(sec0_network_key);
}

void security_register_power_locks(void)
{
  sec0_register_power_locks();
  sec2_register_power_locks();
}

void
security_init(uint8_t SecureKeysRequested)
{
  DPRINT("$i");
  m_SecureKeysRequested = SecureKeysRequested;
  ZW_KeystoreInit();

  secure_learn_init(&ctx);
  secure_learn_enter(&ctx);

  /* Initialize the transport layer*/
  /* TODO: How to tell state machine if we support multiple schemes? */
  if (sec0_init())
  {
    secure_learnIface_set_net_scheme(&ctx, SECURITY_SCHEME_S0);
  }
  else
  {
    secure_learnIface_set_net_scheme(&ctx, SECURITY_SCHEME_NONE);
  }
  sec2_init();
#ifdef ZW_SLAVE
  secure_learnIface_set_isController(&ctx, false);
#else
  secure_learnIface_set_isController(&ctx, true);
#endif

  secure_learn_set_errno_reset();

  s2_inclusion_init(SECURITY_2_SCHEME_1_SUPPORT,
                    KEX_REPORT_CURVE_25519,
                    SecureKeysRequested);
}


void
security_learn_exit(void)
{
  if (!secure_learn_isActive(&ctx, secure_learn_main_region_Idle))
  {
    secure_learn_init(&ctx);
    secure_learn_enter(&ctx);
  }
}


/**
 * Reset security. Must be called when leaving
 * TODO maybe this should be a part of the state machine
 */
void security_reset(void)
{
  DPRINT("$r");
  keystore_network_key_clear(KEY_CLASS_ALL);
  sec2_reset_nonces_tables();
  security_init(m_SecureKeysRequested);
}


static void ZCB_send_data_callback( __attribute__((unused)) ZW_Void_Function_t Context, uint8_t status,
                                    __attribute__((unused)) TX_STATUS_TYPE* statusType)
{
  DPRINTF("$a%02X", status);
  if (status == TRANSMIT_COMPLETE_OK)
  {
    DPRINTF("TX done ok\n$Q%02X", ctx.iface.isController);

    secure_learnIface_raise_tx_done(&ctx);
  } else
  {
    DPRINT("TX done fail\n");
    secure_learnIface_raise_tx_fail(&ctx);
  }
  /* FIXME: Why is several 2+ runCycles needed here? */
  runCycle(); /* NewKey -> end*/
  runCycle(); /* end -> ?? */
  runCycle(); /* ?? -> ?? */  
}


/**
 * timeout function used by the statemachine timer
 */
void ZCB_timeout(void* user)
{
  DPRINT("$t");
  secure_learn_raiseTimeEvent(&ctx, user);
  runCycle();
}

void secure_learn_setTimer(const sc_eventid evid, const sc_integer time_ms, __attribute__((unused)) const sc_boolean periodic)
{
  ctimer_set(&timer, time_ms, ZCB_timeout, evid);
}

void secure_learn_unsetTimer(__attribute__((unused)) const sc_eventid evid)
{
  ctimer_stop(&timer);
}


void
secure_learnIface_send_commands_supported(const sc_integer node, const sc_integer txOptions)
{
  DPRINT("$s");
  tx.cmd[0] = COMMAND_CLASS_SECURITY;
  tx.cmd[1] = SECURITY_COMMANDS_SUPPORTED_REPORT;
  tx.cmd[2] = 0;
  const SCommandClassList_t * pCCList = CCListGet(SECURITY_KEY_S0);
  memcpy(&tx.cmd[3], pCCList->pCommandClasses, pCCList->iListLength);

  DPRINTF("%02X%02X%02X", tx.cmd[3], tx.cmd[4], tx.cmd[5]);
  send_and_raise(node, (uint8_t *) tx.cmd, 3 + pCCList->iListLength, txOptions, true);
}

void /*RET Nothing                  */
SecurityCommandHandler(
  uint8_t  *pCmd_,         /* IN  Frame payload pointer */
  uint8_t  cmdLength,    /* IN  Payload length        */
  RECEIVE_OPTIONS_TYPE *rxopt)  /* rxopt struct to use (may be partially filled out if
                                   parsing encapsulated command */
{
  ZW_APPLICATION_TX_BUFFER *pCmd;
  uint8_t txOption;
  uint8_t sourceNode;

  sourceNode = rxopt->sourceNode;
  pCmd = (ZW_APPLICATION_TX_BUFFER*)pCmd_;
  txOption = ((rxopt->rxStatus & RECEIVE_STATUS_LOW_POWER) ? TRANSMIT_OPTION_LOW_POWER : 0) | TRANSMIT_OPTION_ACK
      | TRANSMIT_OPTION_EXPLORE;

  DPRINTF("$h%02X", pCmd->ZW_Common.cmd);
  /* TODO: Security S0 commands should only be parsed if NOT UnSecurely included -> unincluded and securely included = OK */

  switch (pCmd->ZW_Common.cmd)
  {
    /*Learn mode */
    case NETWORK_KEY_SET:
      /* TO#6569 fix - S0 Network key could be overwritten with Network Key set command class */
      if ((rxopt->securityKey == SECURITY_KEY_S0) &&
          secure_learn_isActive(&ctx, secure_learn_main_region_LearnMode_r1_Scheme_report) &&
      /* TO#6612 fix - Make sure the received Network Key Set frame contains a full 16 byte Network Key */
      /* TO#7038 fix - Network key is accepted if key payload is 16 bytes or more -> */
      /*               use first 16 bytes and ignore rest */
          ((sizeof(ZW_NETWORK_KEY_SET_1BYTE_FRAME) + 15) <= cmdLength))
      {
        sec0_persist_netkey(&pCmd->ZW_NetworkKeySet1byteFrame.networkKeyByte1);
        if (sec0_init())
        {
          secure_learnIface_set_net_scheme(&ctx, SECURITY_SCHEME_S0);
        }
        secure_learnIface_set_txOptions(&ctx, txOption);
        secure_learnIface_raise_key_set(&ctx, sourceNode);
      }
      break;

    case SECURITY_SCHEME_GET:
      /* S0 inclusion initiated */
      /* Stop S2 inclusion machine */
      sec2_inclusion_abort();
#ifdef ZW_SLAVE
      ForceAssignComplete();
#endif
      DPRINT("$g");
      /* TO#7023 fix ignore SECURITY_SCHEME_GET if no Scheme value defined */
      if (cmdLength >= sizeof(ZW_SECURITY_SCHEME_GET_FRAME))
      {
        /* Frame length acceptable */
        DPRINT("$0");
        /* TO6613 fix - We just received a S0 command so we assume S0 are supported */
        /* regardless of the transfered ZW_SecuritySchemeGetFrame.supportedSecuritySchemes */
        secure_learnIface_set_scheme(&ctx, SECURITY_SCHEME_0_BIT);
        DPRINTF("%02X%02X", txOption, sourceNode);
        secure_learnIface_set_txOptions(&ctx, txOption);
        secure_learnIface_raise_scheme_get(&ctx, sourceNode);
      }
      else
      {
        /* Faulty SECURITY_SCHEME_GET received - abort S0 */
        security_learn_exit();
      }
      DPRINTF("-%02X", ctx.stateConfVector[0]);
      break;

#ifdef ZW_CONTROLLER
    case SECURITY_SCHEME_INHERIT:
      if(rxopt->securityKey == SECURITY_KEY_S0)
      {
        secure_learnIface_set_txOptions(&ctx, txOption);
        secure_learnIface_raise_scheme_inherit(&ctx, sourceNode);
      }
      break;

      /* Add node */
    case SECURITY_SCHEME_REPORT:
      if(rxopt->securityKey == SECURITY_KEY_S0 || secure_learn_isActive(&ctx, secure_learn_main_region_InclusionMode_r1_SchemeRequest))
      {
        if (pCmd->ZW_SecuritySchemeGetFrame.supportedSecuritySchemes)
        {
          secure_learnIface_set_scheme(&ctx, pCmd->ZW_SecuritySchemeGetFrame.supportedSecuritySchemes);
        } else
        {
          secure_learnIface_set_scheme(&ctx, SECURITY_SCHEME_0_BIT);
        }
        secure_learnIface_raise_scheme_report(&ctx, sourceNode);
      }
      break;

    case NETWORK_KEY_VERIFY:
      if(rxopt->securityKey == SECURITY_KEY_S0)
      {
        secure_learnIface_raise_key_verify(&ctx, sourceNode);
      }
      break;
#endif

      /* General */
    case SECURITY_COMMANDS_SUPPORTED_GET:
      DPRINTF("-%02X", ctx.stateConfVector[0]);
      if(rxopt->securityKey == SECURITY_KEY_S0)
      {
        DPRINT("$1");
        secure_learnIface_set_txOptions(&ctx, txOption);
        secure_learnIface_set_node(&ctx,sourceNode);
        /* FIXME: AES what it the value parameter of this event used for */
        secure_learnIface_raise_commandsSupportedRequest(&ctx,(sc_string)NULL);
      }
      break;

    case SECURITY_NONCE_GET:
      sec0_send_nonce(sourceNode, g_nodeID, txOption);
      break;

    case SECURITY_NONCE_REPORT:
      if (cmdLength < 10) break;
      /* FIXME: should source and nodeID be swapped? */
      sec0_register_nonce(sourceNode, g_nodeID, &pCmd->ZW_SecurityNonceReportFrame.nonceByte1);
      break;

    case SECURITY_MESSAGE_ENCAPSULATION_NONCE_GET:
    case SECURITY_MESSAGE_ENCAPSULATION:
      {
       /*TODO This is not ok for Large Z/IP frames */
        uint8_t rxBuffer[S0_MAX_ENCRYPTED_MSG_SIZE];
        uint8_t len;

        len = sec0_decrypt_message(sourceNode, g_nodeID,(uint8_t*)pCmd,cmdLength,rxBuffer);
        if (len)
        {
          rxopt->securityKey = SECURITY_KEY_S0;

          CommandHandler_arg_t args = {
            .cmd = rxBuffer,
            .cmdLength = len,
            .rxOpt = rxopt
          };
          CommandHandler(&args);
        }
        if(pCmd->ZW_Common.cmd == SECURITY_MESSAGE_ENCAPSULATION_NONCE_GET)
        {
          sec0_send_nonce(sourceNode, g_nodeID, txOption);
        }
        return;
        break;
      }
    default:
      break;
  }
  runCycle();
  DPRINT("$H");
}

/**
 * Store security state, ie. save scheme and keys.
 */
void secure_learnIfaceL_save_state(void)
{
  //security_save_state();
}

/**
 * S0 inclusion timeout - S0 state machine timeout on Scheme Get - indicate S0 inclusion statemachine timeout
 */
void secure_learnIface_state_start_timeout(void)
{
  secure_learn_set_errno(E_SECURE_LEARN_ERRNO_S0_TIMEOUT);
}

/**
 * New keys should be generated.
 */
void secure_learnIfaceL_new_keys(void)
{
  DPRINT("$n");
#ifdef ZW_SLAVE
  /* This state should never happen in slaves*/
  ASSERT(0);
#else
//#error Controller library security not implemented yet
#endif
}

void secure_learnIface_complete(const sc_integer scheme_mask)
{
  DPRINT("$c");
  /*Update eeprom with negotiated scheme*/
  if (scheme_mask)
  {
    //LOG_PRINTF("Secure add/inclusion succeeded, with schemes 0x%x\n", scheme_mask);
  } else
  {
    DPRINT("ERROR: Secure add/inclusion failed\n");
  }

  if (secureLearnCompleteCallback)
  {
    DPRINT("C");
    /* For compatibility, always return ELEARNSTATUS_ASSIGN_COMPLETE. Security status obtained
     * through other means. */
#ifdef ZW_SLAVE
    if ((E_SECURE_LEARN_ERRNO_S0_TIMEOUT != secure_learn_get_errno()) ||
        (0 == (m_SecureKeysRequested & SECURITY_KEY_S2_MASK))) // false means the S2 inclusion state machine isn't active
    {
      // Not a timeout. Or a timeout in a scenario where the S2 inclusion
      // state machine is not engaged. Inclusion is complete.
      secureLearnCompleteCallback(ELEARNSTATUS_ASSIGN_COMPLETE, g_nodeID);
      secureLearnCompleteCallback = NULL;
    }
    else
    {
      /* S0 timed out - wait for S2 inclusion statemachine to end inclusion */
      secure_learn_set_errno_reset();
    }
#else
    sLearnInfo.bStatus = LEARN_MODE_DONE;
    sLearnInfo.bSource = nodeID;
    sLearnInfo.pCmd = NULL;
    sLearnInfo.bLen = 0;
    secureLearnCompleteCallback(&sLearnInfo);
    secureLearnCompleteCallback = NULL;
#endif
  }
}

/**
 * SendData wrapper that raises a tx_done or tx_fial on completion.
 * Send using highest available security scheme
 */
static void send_and_raise(node_id_t node, uint8_t *pBufData, uint8_t dataLength, TxOptions_t txOptions, bool secure)
{
  /* FIXME: rxopt should be carried as a state var and ZW_RxToTxOptions() used */
  static TRANSMIT_OPTIONS_TYPE txopt;
  memset((uint8_t*)&txopt, 0, sizeof(txopt));
  txopt.destNode = node;
  txopt.txOptions = txOptions;
  txopt.securityKey = (secure) ? SECURITY_KEY_S0 : SECURITY_KEY_NONE;
  DPRINT("$r");
  const STransmitCallback TxCallback = { .pCallback = ZCB_send_data_callback, .Context = 0 };
  if (!ZW_SendDataEx(pBufData, dataLength, &txopt, &TxCallback))
  {
    secure_learnIface_raise_tx_fail(&ctx);
  }
}

uint8_t get_net_scheme(void)
{
  return secure_learnIface_get_net_scheme(&ctx);
}

/****************************** Learn mode related functions *************************************/

void secure_learnIface_update_callback(void)
{
}

/**
 * Enter learn mode after receiving ELEARNSTATUS_ASSIGN_COMPLETE
 *
 * sercurity_learn_begin starts a 10 seconds Secure inclusion timeout
 */
void security_learn_begin(sec_learn_complete_t __cb)
{
  DPRINT("$L");
  /* Only raise_learnRequest if Security 0 inclusion state is in EarlyStart - Not disabled and ASSIGN_NODEID_DONE received */
  if (bool_true == secure_learn_isActive(&ctx, secure_learn_main_region_LearnMode_r1_EarlyStart))
  {
    /* Assume inclusion succeeds */
    secure_learn_set_errno(E_SECURE_LEARN_ERRNO_COMPLETE);
    secureLearnCompleteCallback = __cb;
    secure_learnIface_raise_learnRequest(&ctx);
    runCycle();
  }
}

/**
 * Enter learn mode early, i.e. after receiving ASSIGN_NODEID_DONE.
 * This is needed because the ELEARNSTATUS_ASSIGN_COMPLETE callback may
 * come after the controller has proceeded with the secure inclusion.
 * Some would say the callback is late. Unfortunately, the frame flow
 * for inclusion does not allow a guaranteed timely callback.
 *
 * security_learn_begin_early() is a workaround to catch this case.
 */
void security_learn_begin_early(sec_learn_complete_t __cb)
{
  /* TODO: This should just raise an event, which triggers an
   * operation to update the callback. Then we can skip the
   * isActive check. */
  DPRINT("$E");
  if (bool_true == secure_learn_isActive(&ctx, secure_learn_main_region_Idle))
  {
    /* Assume inclusion succeeds */
    secure_learn_set_errno(E_SECURE_LEARN_ERRNO_COMPLETE);
    secureLearnCompleteCallback = __cb;
    secure_learnIface_raise_assignIdDone(&ctx);
    runCycle();
  }
}

void secure_learnIfaceL_send_scheme_report(const sc_integer node, const sc_integer txOptions, const sc_integer secure)
{

  uint8_t ctrlScheme; // The including controllers scheme
  uint8_t my_schemes;

  DPRINT("$e");
  ctrlScheme = secure_learnIface_get_scheme(&ctx);
  my_schemes = secure_learnIface_get_supported_schemes(&ctx) & ctrlScheme; //Common schemes
#ifdef ZW_CONTROLLER
  /* Check if controller supports schemes that I don't */
  if (ctrlScheme ^ my_schemes)
  {
    /* I'm not allowed to include nodes */

    /*FIXME ... what to do? */
  }
#endif
  tx.scheme_report_frame.cmdClass = COMMAND_CLASS_SECURITY;
  tx.scheme_report_frame.cmd = SECURITY_SCHEME_REPORT;
  if (my_schemes & SECURITY_SCHEME_0_BIT)
  {
    my_schemes &= ~SECURITY_SCHEME_0_BIT;
  }
  tx.scheme_report_frame.supportedSecuritySchemes = my_schemes;

  DPRINTF("%d", (uint8_t)secure);
  send_and_raise(node, (uint8_t *) &tx.scheme_report_frame, sizeof(ZW_SECURITY_SCHEME_REPORT_FRAME), txOptions, (bool)secure);
}


void secure_learnIfaceL_set_inclusion_key(void)
{
  keystore_network_key_clear(KEY_CLASS_S0);
  sec0_unpersist_netkey();
  s2_ctx->is_keys_restored = false;
}


#ifndef ZW_CONTROLLER
static void ZCB_send_key_verify_callback( __attribute__((unused)) ZW_Void_Function_t Context, uint8_t status,
                                          __attribute__((unused)) TX_STATUS_TYPE *psTxResult)
{
  DPRINTF("$V%02X", status);
  if (status == TRANSMIT_COMPLETE_OK)
  {
    DPRINT("TX done ok\n$Q");
    secure_learnIface_raise_tx_done(&ctx);
  } else
  {
    DPRINT("TX done fail\n");
    secure_learnIface_raise_tx_fail(&ctx);
  }
  /* FIXME: Why is several 2+ runCycles needed here? */
  runCycle(); /* NewKey -> end*/
  runCycle(); /* end -> ?? */  
}
#endif  /* ifndef ZW_CONTROLLER */


void secure_learnIfaceL_send_key_verify(const sc_integer node, const sc_integer txOptions)
{
#ifdef ZW_CONTROLLER
  send_and_raise(node, (uint8_t *) &key_verify, sizeof(key_verify), txOptions, true);
#else
  /* FIXME: rxopt should be carried as a state var and ZW_RxToTxOptions() used */
  static TRANSMIT_OPTIONS_TYPE txopt;
  memset((uint8_t*)&txopt, 0, sizeof(txopt));
  txopt.destNode = node;
  txopt.txOptions = txOptions;
  txopt.securityKey = SECURITY_KEY_S0;
  
  DPRINT("$}");
  const STransmitCallback TxCallback = { .pCallback = ZCB_send_key_verify_callback, .Context = 0 };
  if (!ZW_SendDataEx((uint8_t *) &key_verify, sizeof(key_verify), &txopt, &TxCallback))
  {
    DPRINT("${");
    secure_learnIface_raise_tx_fail(&ctx);
  }
#endif
}


/********************************** Add node related ******************************************/

#ifdef ZW_CONTROLLER
void security_add_begin(uint8_t node, TxOptions_t txOptions, bool controller, sec_add_complete_t __cb)
{
  if (secure_learn_isActive(&ctx, secure_learn_main_region_Idle))
  { // Annoying check but needed to protect the isController variable
    secureAddAppCbFunc = __cb;
    secure_learnIface_set_isController(&ctx, controller);
    secure_learnIface_set_txOptions(&ctx, txOptions);
    secure_learnIface_raise_inclusionRequest(&ctx, node);
    runCycle();
  }
}

void secure_learnIfaceI_send_scheme_get(const sc_integer node, const sc_integer txOptions)
{
  tx.scheme_get_frame.cmdClass = COMMAND_CLASS_SECURITY;
  tx.scheme_get_frame.cmd = SECURITY_SCHEME_GET;
  tx.scheme_get_frame.supportedSecuritySchemes = 0;
  send_and_raise(node, (uint8_t *) &tx.scheme_get_frame, sizeof(tx.scheme_get_frame), txOptions, true);
}

void secure_learnIfaceI_send_key(const sc_integer node, const sc_integer txOptions)
{
  uint8_t inclusionKey[16];
  tx.key_set_frame[0] = COMMAND_CLASS_SECURITY;
  tx.key_set_frame[1] = NETWORK_KEY_SET;

  /*TODO It might be better to do this in some other way, meybe a txOption .... */
  memset(inclusionKey, 0, 16);

  send_and_raise(node, (uint8_t *) &tx.key_set_frame, sizeof(tx.key_set_frame), txOptions, true);
}

void secure_learnIfaceI_send_scheme_inherit(const sc_integer node, const sc_integer txOptions)
{
  tx.scheme_inherit_frame.cmdClass = COMMAND_CLASS_SECURITY;
  tx.scheme_inherit_frame.cmd = SECURITY_SCHEME_INHERIT;
  tx.scheme_inherit_frame.supportedSecuritySchemes = 0;
  send_and_raise(node, (uint8_t *) &tx.scheme_inherit_frame, sizeof(tx.scheme_inherit_frame), txOptions, true);
}

void secure_learnIfaceI_restore_key(void)
{
  if (sec0_init())
  {
    secure_learnIface_set_net_scheme(&ctx, SECURITY_SCHEME_S0);
  }
}

#else /* ZW_CONTROLLER */
/* The state machine should never call these on a slave */
void security_add_begin(__attribute__((unused)) uint8_t node,
                        __attribute__((unused)) TxOptions_t txOptions,
                        __attribute__((unused)) bool controller,
                        __attribute__((unused)) sec_learn_complete_t __cb) {
  ASSERT(false);
}
void secure_learnIfaceI_send_scheme_get(__attribute__((unused)) const sc_integer node,
                                        __attribute__((unused)) const sc_integer txOptions)
{
  ASSERT(false);
}
void secure_learnIfaceI_send_key( __attribute__((unused)) const sc_integer node,
                                  __attribute__((unused)) const sc_integer txOptions)
{
  ASSERT(false);
}
void secure_learnIfaceI_send_scheme_inherit(__attribute__((unused)) const sc_integer node,
                                            __attribute__((unused)) const sc_integer txOptions)
{
  ASSERT(false);
}
void secure_learnIfaceI_restore_key(void)
{
  ASSERT(false);
}
#endif /* ZW_CONTROLLER */

/**
 * Register which schemes are supported by a node
 */
/* This interface function is called on both slaves and controllers,
 * despite belonging to IfaceI (which is mostly controller stuff) */
void secure_learnIfaceI_register_scheme(__attribute__((unused)) const sc_integer node,
                                        __attribute__((unused)) const sc_integer scheme)
{
}

uint8_t getSecureKeysRequested(void)
{
  return m_SecureKeysRequested;
}
