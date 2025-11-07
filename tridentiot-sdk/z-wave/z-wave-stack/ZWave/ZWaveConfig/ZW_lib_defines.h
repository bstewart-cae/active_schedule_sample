// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file ZW_lib_defines.h
 * @copyright 2022 Silicon Laboratories Inc.
 *
 * @brief Include file defining the library defines for all
 * Z-Wave library variants.
 */
#ifndef _ZW_LIB_DEFINES_H_
#define _ZW_LIB_DEFINES_H_

/*
 * The following defines are required for the Slave library only.
 */
#if defined(ZW_slave_lib)
#define ZW_SLAVE_ROUTING
#define ZW_SLAVE_ENHANCED_232
#define ZW_BEAM_RX_WAKEUP
#ifndef ZW_SLAVE
#define ZW_SLAVE                // This is going to be a slave device.
#endif
#define USE_RESPONSEROUTE
#define ZW_RETURN_ROUTE_UPDATE
#define ZW_RETURN_ROUTE_PRIORITY
#define ZW_SECURITY_PROTOCOL    /* Security in protocol enabled. This is explicitly for slave devices. */
#define USE_TRANSPORT_SERVICE   /* Transport layer can resort to Transport Services if needed and specified */
#ifndef SINGLE_CONTEXT
#define SINGLE_CONTEXT          /* S2 security uses a single context to save code and memory */
#endif // SINGLE_CONTEXT
#define ZW_REPEATER
#define ZW_SELF_HEAL
#endif

/*
 * The following defines are required for the Controller library only.
 */
#if defined(ZW_controller_lib)
#define ZW_CONTROLLER           // This is going to be a controller device.
#define ZW_ROUTED_DISCOVERY
#define ZW_ROUTE_DIVERSITY
#define REPLACE_FAILED
#define NO_PREFERRED_CALC
#define ZW_MOST_USED_NEEDS_REPLACE
#define ZW_CONTROLLER_BRIDGE    // Always powered and has virtual nodes.
#define ZW_CONTROLLER_STATIC    // Always powered.
#define MULTIPLE_LWR
#define ZW_REPEATER             // Always listening devices.
#define ZW_SELF_HEAL
#endif

#endif /* _ZW_LIB_DEFINES_H_ */
