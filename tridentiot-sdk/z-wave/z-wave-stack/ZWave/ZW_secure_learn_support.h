// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file ZW_secure_learn_support.h
 * @copyright 2022 Silicon Laboratories Inc.
 */

 /**
* The Z-Wave protocol has an interface layer between S2 and the protocol.
*
*
* @startuml
* title Z-Wave protocol S2 interface layer
* Secure_learn_support <|-- Security_Scheme2
* Secure_learn_support <|-- LibS2
* Security_Scheme2 <|-- s2_inclusion_glue
* Security_Scheme2 <|-- LibS2
* s2_inclusion_glue <|-- LibS2
* @enduml
*/

#ifndef ZW_SECURE_LEARN_SUPPORT_H_
#define ZW_SECURE_LEARN_SUPPORT_H_
#ifdef ZW_SECURITY_PROTOCOL
#include <ZW_typedefs.h>
#include <stdint.h>
#include <ZW_libsec.h>

#ifdef ZW_CONTROLLER
typedef VOID_CALLBACKFUNC(sec_learn_complete_t)(LEARN_INFO_T* psLearnInfo);
typedef VOID_CALLBACKFUNC(sec_add_complete_t)(LEARN_INFO_T* psLearnInfo);
#else
typedef VOID_CALLBACKFUNC(sec_learn_complete_t)(uint8_t status, node_id_t bNodeId);
#endif

typedef enum _E_SECURE_LEARN_ERRNO_
{
  E_SECURE_LEARN_ERRNO_COMPLETE,
  E_SECURE_LEARN_ERRNO_S0_TIMEOUT,
  E_SECURE_LEARN_ERRNO_FAILED,
  E_SECURE_LEARN_ERRNO_COMPLETE_NEVER_STARTED
} E_SECURE_LEARN_ERRNO;

void secure_learn_set_errno_reset();
void secure_learn_set_errno(E_SECURE_LEARN_ERRNO error_number);
E_SECURE_LEARN_ERRNO secure_learn_get_errno(void);

/**
 * One-time registration of power locks 
 */
void security_register_power_locks(void);

/**
 * One-time initialization of the state machine
 */
void security_init(uint8_t SecureKeysRequested);
/**
 * Resetting security states.
 */
void security_reset();
/**
 * Begin the learn mode
 * on completion the callback function is called with scheme mask negotiated for
 * the this node. An empty scheme mask means that the secure negotiation has failed.
 */
void security_learn_begin(sec_learn_complete_t __cb);

void security_learn_begin_early(sec_learn_complete_t __cb);

/**
 * Exit the S0 learn mode - if applicable
 */
void security_learn_exit();

/**
 * Begin the add node mode
 * on completion the callback function is called with scheme mask negotiated for
 * the new node. An empty scheme mask means that the secure negotiation has failed.
 *
 * \param node Node to negotiate with
 * \param controller true if the node is a controller
 * \param __cb callback function.
 */
void security_add_begin(uint8_t node, TxOptions_t txOptions,bool controller,sec_learn_complete_t __cb);
/**
 * Input handler for the state machine.
 * \param[IN] Payload from the received frame
 * \param[IN] Number of command bytes including the command
 * \param[IN] Receive options. May be partially filled out if parsing encapsulated frame.
 */
void
SecurityCommandHandler(
uint8_t  *pCmd_,
uint8_t  cmdLength,
RECEIVE_OPTIONS_TYPE *rxopt);



/**
 * Get the current net_scheme
 * \return bit mask of supported security schemes by this node in this network. If this function returns 0
 * no schemes are supported, ie. node is not securely included.
 */
uint8_t get_net_scheme();

/**
 * State machine polling...
 */
void secure_poll();
#endif

/**
 * Get the secure keys requested by application
 * \return bit mask of secure keys requested
 */
uint8_t getSecureKeysRequested(void);

#endif /* ZW_SECURE_LEARN_SUPPORT_H_ */
