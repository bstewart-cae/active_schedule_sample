// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file ZW_protocol_commands.h
 * @copyright 2022 Silicon Laboratories Inc.
 */
#ifndef ZWAVE_ZW_PROTOCOL_COMMANDS_H_
#define ZWAVE_ZW_PROTOCOL_COMMANDS_H_

/* Z-Wave protocol commands */
#define ZWAVE_CMD_NOP                               0x00
#define ZWAVE_CMD_NODE_INFO                         0x01
#define ZWAVE_CMD_REQUEST_NODE_INFO                 0x02
#define ZWAVE_CMD_ASSIGN_IDS                        0x03
#define ZWAVE_CMD_FIND_NODES_IN_RANGE               0x04
#define ZWAVE_CMD_GET_NODES_IN_RANGE                0x05
#define ZWAVE_CMD_RANGE_INFO                        0x06
#define ZWAVE_CMD_CMD_COMPLETE                      0x07
#define ZWAVE_CMD_TRANSFER_PRESENTATION             0x08
#define ZWAVE_CMD_TRANSFER_NODE_INFO                0x09
#define ZWAVE_CMD_TRANSFER_RANGE_INFO               0x0a
#define ZWAVE_CMD_TRANSFER_END                      0x0b
#define ZWAVE_CMD_ASSIGN_RETURN_ROUTE               0x0c
#define ZWAVE_CMD_NEW_NODE_REGISTERED               0x0d
#define ZWAVE_CMD_NEW_RANGE_REGISTERED              0x0e
#define ZWAVE_CMD_TRANSFER_NEW_PRIMARY_COMPLETE     0x0f

#define ZWAVE_CMD_AUTOMATIC_CONTROLLER_UPDATE_START 0x10
#define ZWAVE_CMD_SUC_NODE_ID                       0x11
#define ZWAVE_CMD_SET_SUC                           0x12
#define ZWAVE_CMD_SET_SUC_ACK                       0x13

#define ZWAVE_CMD_ASSIGN_SUC_RETURN_ROUTE           0x14
#define ZWAVE_CMD_STATIC_ROUTE_REQUEST              0x15

#ifdef ZW_SELF_HEAL
#define ZWAVE_CMD_LOST                              0x16
#define ZWAVE_CMD_ACCEPT_LOST                       0x17
#endif /*ZW_SELF_HEAL*/

#define ZWAVE_CMD_NOP_POWER                         0x18

#define ZWAVE_CMD_RESERVE_NODE_IDS                  0x19
#define ZWAVE_CMD_RESERVED_IDS                      0x1a
#ifdef ZW_SENSOR
#define ZWAVE_CMD_SENSOR_FIND_NODES_IN_RANGE        0x1b
#define ZWAVE_CMD_SENSOR_GET_NODES_IN_RANGE         0x1c
#define ZWAVE_CMD_SENSOR_RANGE_INFO                 0x1d
#endif

#define ZWAVE_CMD_GET_SUC_NODE_ID                   0x1e  // Obsolete

#define ZWAVE_CMD_NODES_EXIST                       0x1f
#define ZWAVE_CMD_NODES_EXIST_REPLY                 0x20
#define ZWAVE_CMD_GET_NODES_EXIST                   0x21

#define ZWAVE_CMD_SET_NWI_MODE                      0x22

#define ZWAVE_CMD_EXCLUDE_REQUEST                   0x23

#define ZWAVE_CMD_ASSIGN_RETURN_ROUTE_PRIORITY      0x24
#define ZWAVE_CMD_ASSIGN_SUC_RETURN_ROUTE_PRIORITY  0x25

/* Smart Start - INIF */
#define ZWAVE_CMD_INCLUDED_NODE_INFO                0x26

/* Smart Start Prime inclusion process - Node Information  */
#define ZWAVE_CMD_SMARTSTART_PRIME_NODE_INFO        0x27
/* Smart Start Include process - Node Information  */
#define ZWAVE_CMD_SMARTSTART_INCLUDE_NODE_INFO      0x28

/* Zensor protocol commands */
#define ZWAVE_ZENSOR_CMD_BIND_REQ                   0x01
#define ZWAVE_ZENSOR_CMD_BIND_ACCEPT                0x02
#define ZWAVE_ZENSOR_CMD_BIND_COMPLETE              0x03

/* Z-Wave Long Range protocol commands */
#define ZWAVE_LR_CMD_NOP                            0x00
#define ZWAVE_LR_CMD_NODE_INFO                      0x01
#define ZWAVE_LR_CMD_REQUEST_NODE_INFO              0x02
#define ZWAVE_LR_CMD_ASSIGN_IDS                     0x03
#define ZWAVE_LR_CMD_EXCLUDE_REQUEST                0x23
#define ZWAVE_LR_CMD_INCLUDED_NODE_INFO             0x26
#define ZWAVE_LR_CMD_SMARTSTART_PRIME_NODE_INFO     0x27
#define ZWAVE_LR_CMD_SMARTSTART_INCLUDE_NODE_INFO   0x28
#define ZWAVE_LR_CMD_EXCLUDE_REQUEST_CONFIRM        0x29

/* Non-secure port of inclusion done */
#define ZWAVE_LR_CMD_NON_SECURE_INCLUSION_COMPLETE 0x2A

#endif /* ZWAVE_ZW_PROTOCOL_COMMANDS_H_ */
