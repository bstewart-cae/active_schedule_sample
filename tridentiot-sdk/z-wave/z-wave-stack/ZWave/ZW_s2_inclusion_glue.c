// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file ZW_s2_inclusion_glue.c
 * @copyright 2022 Silicon Laboratories Inc.
 *
 * @brief Z-Wave specific glue code for the libs2/s2_inclusion module.
 */
#include "ZW_lib_defines.h"

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/
#include <ZW_typedefs.h>
#include <ZW_protocol.h>
#include <ZW_MAC.h>
#include <ZW_tx_queue.h>
#include <ZW_transport_api.h>
#include <stdint.h>
#include <ZW_libsec.h>
#include "s2_inclusion.h"
#include <ZW_ctimer.h>
#include "ZW_protocol_interface.h"
#include "ZW_CCList.h"
#ifdef ZW_SLAVE
#include <ZW_slave.h>
#else
#include <ZW_controller_api.h>
#endif
#include <kderiv.h>
#include <S2.h>
#include <curve25519.h>
#include <ZW_secure_learn_support.h>
#include <ZW_Security_Scheme0.h>
#include <s2_keystore.h>
#include <ZW_s2_inclusion_glue.h>
#include <zpal_radio.h>

#include "TickTime.h"

#include <DebugPrint.h>

// Extra checks to keep TRANSMIT_COMPLETE_CODES aligned between ZW_transport_api.h and S2.h
_Static_assert(TRANSMIT_COMPLETE_OK == S2_TRANSMIT_COMPLETE_OK);
_Static_assert(TRANSMIT_COMPLETE_NO_ACK == S2_TRANSMIT_COMPLETE_NO_ACK);
_Static_assert(TRANSMIT_COMPLETE_FAIL == S2_TRANSMIT_COMPLETE_FAIL);
_Static_assert(TRANSMIT_COMPLETE_VERIFIED == S2_TRANSMIT_COMPLETE_VERIFIED);


/****************************************************************************/
/*                      PRIVATE TYPES and DEFINITIONS                       */
/****************************************************************************/

/**
 *  definitions for Security S2 inclusion Authentication
 */
typedef enum _E_SECURTIY_S2_AUTHENTICATION_
{
  SECURITY_AUTHENTICATION_SSA = 0x00,
  SECURITY_AUTHENTICATION_CSA = 0x01
} e_security_s2_authentication_t;

sec_learn_complete_t s2_inclusion_secureLearnAppCbFunc;

/****************************************************************************/
/*                              IMPORTED DATA                               */
/****************************************************************************/
extern struct S2* s2_ctx;  //TODO: Ok to access this directly?

/****************************************************************************/
/*                              PRIVATE DATA                                */
/****************************************************************************/
static struct ctimer s2_itimer = { 0 };

#ifndef ZW_CONTROLLER
static uint8_t challenge_response[SECURITY_KEY_S2_PUBLIC_CSA_DSK_LENGTH];
static s_application_security_event_data_t securityEventData = { 0 };
#endif

static struct sTxQueueEmptyEvent sTxQueueEmptyEvent_elem = { 0 };

/****************************************************************************/
/*                              PRIVATE FUNCTIONS                           */
/****************************************************************************/


/**
* Callback function used to inform S2 inclusion machine that TxQueue is empty
* and
*/
void ZCB_s2_inclusion_txqueue_empty(void)
{
#ifndef ZW_CONTROLLER
  /* Post S2 inclusion Event - ready for lengthy calculation... */
  s2_inclusion_challenge_response(s2_ctx, true, (const uint8_t*)challenge_response, SECURITY_KEY_S2_PUBLIC_CSA_DSK_LENGTH);
#endif
}

void s2_inclusion_set_txqueue_empty_event(void)
{
  TxQueueEmptyEvent_Add(&sTxQueueEmptyEvent_elem, ZCB_s2_inclusion_txqueue_empty);
}

/****************************************************************************/
/*                              EXPORTED FUNCTIONS                          */
/****************************************************************************/

/**
* Implements the s2_inclusion timer. The interval is converted from 10 ms units
* to underlying the underlying ctimer 1 ms unit.
* \param[in] ctxt Unused - global s2_ctx used instead.
* \param[in] interval Timer interval in units of 10 ms.
* \return true if succesful
* \return false if failed.
*/
uint8_t s2_inclusion_set_timeout(__attribute__((unused)) struct S2* ctxt, uint32_t interval) /* interval i 10 ms units */
{
  uint32_t interval_in_ms;

  interval_in_ms = interval * 10;
  ctimer_set(&s2_itimer,interval_in_ms,(void(*)(void *))s2_inclusion_notify_timeout, s2_ctx);

  return true;
}

/**
 * Stop the inclusion timer
 */
void s2_inclusion_stop_timeout(void)
{
  ctimer_stop(&s2_itimer);
}


void s2_inclusion_glue_set_securelearnCbFunc(sec_learn_complete_t secureLearnCbFunc)
{
  s2_inclusion_secureLearnAppCbFunc = secureLearnCbFunc;
}


void ZCB_s2_evt_handler(zwave_event_t *evt) //TODO: Move to ZW_Security_Scheme2.c
{
  DPRINTF("In s2_evt_handler: %02X", evt->event_type);
  switch(evt->event_type)
  {
    case S2_NODE_INCLUSION_PUBLIC_KEY_CHALLENGE_EVENT:
      /* Perform ASYNC user confirmation of peer public key */
      /* The public key will be accepted when TX queue is empty. */
#ifdef ZW_CONTROLLER
      // Currently libs2 is not supported on a controller.
      // So nothing to do.
#else
      if (evt->evt.s2_event.s2_data.challenge_req.dsk_length > 0)
      {
        securityEventData.event = E_APPLICATION_SECURITY_EVENT_S2_INCLUSION_REQUEST_DSK_CSA;
        securityEventData.eventDataLength = 0;  /* No Event Data for this Security Event */
        ProtocolInterfacePassToAppSecurityEvent(&securityEventData);
      }
      else
      {
        // A Z-Wave slave must keep the DSK locally and accept the joining after ACK or routed ACK
        // has been transmitted, i.e. after TX queue empty event.
        challenge_response[0] = evt->evt.s2_event.s2_data.challenge_req.public_key[0];
        challenge_response[1] = evt->evt.s2_event.s2_data.challenge_req.public_key[1];
        challenge_response[2] = evt->evt.s2_event.s2_data.challenge_req.public_key[2];
        challenge_response[3] = evt->evt.s2_event.s2_data.challenge_req.public_key[3];
        s2_inclusion_set_txqueue_empty_event();
      }
#endif
      /* We will busy loop ECDH calculations shortly */
      DPRINT("S2_NODE_INCLUSION_PUBLIC_KEY_CHALLENGE_EVENT");
      break;

    case S2_NODE_JOINING_COMPLETE_EVENT:
      DPRINT("S2_NODE_JOINING_COMPLETE_EVENT");
      if (s2_inclusion_secureLearnAppCbFunc)
      {
#ifdef ZW_CONTROLLER
        s2_inclusion_secureLearnAppCbFunc(LEARN_MODE_DONE, nodeID);
#else
        //LR nodes must self destruct if no keys are granted.
        if (zpal_radio_is_long_range_locked() &&
           (0 == evt->evt.s2_event.s2_data.inclusion_complete.exchanged_keys) )
        {
          secure_learn_set_errno(E_SECURE_LEARN_ERRNO_FAILED);
          security_reset();
        }

        s2_inclusion_secureLearnAppCbFunc(ELEARNSTATUS_ASSIGN_COMPLETE, g_nodeID);
#endif
        s2_inclusion_secureLearnAppCbFunc = NULL;
      }
      /* Node security 2 included */
      sec0_unpersist_netkey();
      break;

    case S2_JOINING_COMPLETE_NEVER_STARTED_EVENT:
      {
        DPRINT("S2_JOINING_COMPLETE_NEVER_STARTED_EVENT");
        if (s2_inclusion_secureLearnAppCbFunc)
        {
          secure_learn_set_errno(E_SECURE_LEARN_ERRNO_COMPLETE_NEVER_STARTED);
#ifdef ZW_CONTROLLER
          s2_inclusion_secureLearnAppCbFunc(LEARN_MODE_DONE, nodeID);
#else
          s2_inclusion_secureLearnAppCbFunc(ELEARNSTATUS_ASSIGN_COMPLETE, g_nodeID);
#endif
          s2_inclusion_secureLearnAppCbFunc = NULL;
        }
        /* Node NOT security included */
        security_learn_exit();
      }
      break;

    case S2_NODE_INCLUSION_FAILED_EVENT:
      {
        DPRINT("S2_NODE_INCLUSION_FAILED_EVENT");
        if (s2_inclusion_secureLearnAppCbFunc)
        {
          secure_learn_set_errno(E_SECURE_LEARN_ERRNO_FAILED);
#ifdef ZW_CONTROLLER
          s2_inclusion_secureLearnAppCbFunc(LEARN_MODE_DONE, nodeID);
#else
          s2_inclusion_secureLearnAppCbFunc(ELEARNSTATUS_ASSIGN_COMPLETE, g_nodeID);
#endif
          s2_inclusion_secureLearnAppCbFunc = NULL;
        }
        /* Node NOT security included */
        /* Clear any security keys we might have negotiated and cached up until this point */
        security_reset();
      }
      break;

    case S2_NODE_INCLUSION_INITIATED_EVENT:
      {
        DPRINT("S2_NODE_INCLUSION_INITIATED_EVENT");
        /* S2 inclusion initiated */
        /* Stop S0 inclusion machine */
        security_learn_exit();
#ifdef ZW_SLAVE
        ForceAssignComplete();
#endif
      }
      break;

#ifdef ZW_CONTROLLER
    case S2_NODE_INCLUSION_COMPLETE_EVENT:
      static LEARN_INFO_T sLearnInfo;
      sLearnInfo.bSource = evt->evt.s2_event.peer.r_node;
      sLearnInfo.bStatus = ADD_NODE_STATUS_DONE;
      sLearnInfo.bLen = 0;
      zcbp_secureAddAppCallbackFunc(&sLearnInfo);
      break;
#endif

    default:
      DPRINTF("Unknown event: %02X", (evt->event_type & 0xFF));
      break;

  }
}


void s2_inclusion_glue_join_start(struct S2 *p_context)
{
  s2_connection_t peer ={
    .zw_tx_options = TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE | TRANSMIT_OPTION_EXPLORE,
    .class_id = 0xff,
    .l_node = g_nodeID
  };

  s2_inclusion_joining_start(p_context, &peer, SECURITY_AUTHENTICATION_SSA);
}


void S2_get_commands_supported(__attribute__((unused)) node_t lnode, uint8_t class_id, const uint8_t ** ppCmdClasses, uint8_t* pLength)
{
  const SCommandClassList_t* pCCList = CCListGet(class_id + 1);

  *pLength = pCCList->iListLength;
  *ppCmdClasses = pCCList->pCommandClasses;
}

void ZW_s2_inclusion_init(uint8_t keys)
{
  security_init(keys);
}
