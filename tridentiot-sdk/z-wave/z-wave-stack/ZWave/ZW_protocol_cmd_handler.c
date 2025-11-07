// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

#include "ZW_protocol_cmd_handler.h"

/**
 * This is the first of the registered handlers.
 */
extern const zw_protocol_cmd_handler_map_t __start_zw_protocol_cmd_handlers;
#define protocol_cmd_handlers_start __start_zw_protocol_cmd_handlers
/**
 * This marks the end of the handlers. The element
 * after the last element. This means that this element
 * is not valid.
 */
extern const zw_protocol_cmd_handler_map_t __stop_zw_protocol_cmd_handlers;
#define protocol_cmd_handlers_stop __stop_zw_protocol_cmd_handlers

/**
 * This is the first of the registered long range handlers.
 */
extern const zw_protocol_cmd_handler_map_t __start_zw_protocol_cmd_handlers_lr;
#define protocol_cmd_handlers_lr_start __start_zw_protocol_cmd_handlers_lr
/**
 * This marks the end of the long range handlers. The element
 * after the last element. This means that this element
 * is not valid.
 */
extern const zw_protocol_cmd_handler_map_t __stop_zw_protocol_cmd_handlers_lr;
#define protocol_cmd_handlers_lr_stop __stop_zw_protocol_cmd_handlers_lr


bool zw_protocol_invoke_cmd_handler(uint8_t *pCmd, uint8_t cmdLength, RECEIVE_OPTIONS_TYPE *rxopt)
{
  zw_protocol_cmd_handler_map_t const * iter = &protocol_cmd_handlers_start;
  for ( ; iter < &protocol_cmd_handlers_stop; ++iter)
  {
    if (iter->cmd == pCmd[1])
    {
      iter->pHandler(pCmd, cmdLength, rxopt);
      return true;
    }
  }

  return false;
}

bool zw_protocol_invoke_cmd_handler_lr(uint8_t *pCmd, uint8_t cmdLength, RECEIVE_OPTIONS_TYPE *rxopt)
{
  zw_protocol_cmd_handler_map_t const * iter = &protocol_cmd_handlers_lr_start;
  for ( ; iter < &protocol_cmd_handlers_lr_stop; ++iter)
  {
    if (iter->cmd == pCmd[1])
    {
      iter->pHandler(pCmd, cmdLength, rxopt);
      return true;
    }
  }

  return false;
}
