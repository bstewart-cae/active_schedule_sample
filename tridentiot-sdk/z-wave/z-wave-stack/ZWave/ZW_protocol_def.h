// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file ZW_protocol_def.h
 * @copyright 2022 Silicon Laboratories Inc.
 */
#ifndef ZWAVE_ZW_PROTOCOL_DEF_H_
#define ZWAVE_ZW_PROTOCOL_DEF_H_

/* Number of bytes in a homeID */
#define HOMEID_LENGTH      4

/* Max hops in route */
#define MAX_REPEATERS      4

/* predefined node ID's */
#define NODE_UNINIT         0x00    /* uninitialized slave node */
#define NODE_SLAVE_MIN      0x01    /* first slave node */
#define NODE_CONTROLLER     0x01    /* network controller node */
#define NODE_CONTROLLER_OLD 0xEF    /* pre 3.40 devkit NODE_CONTROLLER */
#define NODE_BROADCAST      0xFF    /* broadcast */
#define BEAM_BROADCAST      0xFF    /* broadcast address for wakeup beams */

#define NODE_BROADCAST_LR      0xFFF    /* broadcast LR*/
#define BEAM_BROADCAST_LR      0xFFF    /* broadcast address for LR wakeup beams */
#define NODE_RESERVED_BEGIN_LR 0xFA1    /* reserved nodeID LR - start of range */
#define NODE_RESERVED_END_LR   0xFFE    /* reserved nodeID LR - end of range */

#endif /* ZWAVE_ZW_PROTOCOL_DEF_H_ */
