// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

#ifndef ZW_PROTOCOL_CMD_HANDLER_H_
#define ZW_PROTOCOL_CMD_HANDLER_H_

#include <ZW_transport_api.h>

typedef void (*zw_protocol_cmd_handler_t)(uint8_t *pCmd, uint8_t cmdLength, RECEIVE_OPTIONS_TYPE *rxopt);

typedef struct
{
  uint8_t cmd;
  zw_protocol_cmd_handler_t pHandler;
}
zw_protocol_cmd_handler_map_t;

#define ZW_PROTOCOL_CMD_HANDLER_SECTION    "zw_protocol_cmd_handlers"
#define ZW_PROTOCOL_CMD_HANDLER_LR_SECTION "zw_protocol_cmd_handlers_lr"

#define ZW_PROTOCOL_ADD_CMD(cmd) \
  static void zw_protocol_cmd_handler_fcn_##cmd(uint8_t *pCmd, uint8_t cmdLength, RECEIVE_OPTIONS_TYPE *rxopt); /* Prototype */ \
  static const zw_protocol_cmd_handler_map_t zw_protocol_cmd_handler_##cmd __attribute__((__used__, __section__( ZW_PROTOCOL_CMD_HANDLER_SECTION ))) = {cmd,zw_protocol_cmd_handler_fcn_##cmd}; \
  static void zw_protocol_cmd_handler_fcn_##cmd(__attribute__((unused)) uint8_t *pCmd, __attribute__((unused)) uint8_t cmdLength, __attribute__((unused)) RECEIVE_OPTIONS_TYPE *rxopt)

#define ZW_PROTOCOL_ADD_CMD_LR(cmd) \
  static void zw_protocol_cmd_handler_lr_fcn_##cmd(uint8_t *pCmd, uint8_t cmdLength, RECEIVE_OPTIONS_TYPE *rxopt); /* Prototype */ \
  static const zw_protocol_cmd_handler_map_t zw_protocol_cmd_handler_lr_##cmd __attribute__((__used__, __section__( ZW_PROTOCOL_CMD_HANDLER_LR_SECTION ))) = {cmd,zw_protocol_cmd_handler_lr_fcn_##cmd}; \
  static void zw_protocol_cmd_handler_lr_fcn_##cmd(__attribute__((unused)) uint8_t *pCmd, __attribute__((unused)) uint8_t cmdLength, __attribute__((unused)) RECEIVE_OPTIONS_TYPE *rxopt)

/**
 * Invoke protocol command handler.
 *
 * @param[in] pCmd      Payload from the received frame.
 * @param[in] cmdLength Number of command bytes including the command.
 * @param[in] rxopt     Receive options.
 * @return true if handler for given command was invoked, false if no handler was found
 */
bool zw_protocol_invoke_cmd_handler(uint8_t *pCmd, uint8_t cmdLength, RECEIVE_OPTIONS_TYPE *rxopt);

/**
 * Invoke long range protocol command handler.
 *
 * @param[in] pCmd      Payload from the received frame.
 * @param[in] cmdLength Number of command bytes including the command.
 * @param[in] rxopt     Receive options.
 * @return true if handler for given command was invoked, false if no handler was found
 */
bool zw_protocol_invoke_cmd_handler_lr(uint8_t *pCmd, uint8_t cmdLength, RECEIVE_OPTIONS_TYPE *rxopt);

#endif /* ZW_PROTOCOL_CMD_HANDLER_H_ */