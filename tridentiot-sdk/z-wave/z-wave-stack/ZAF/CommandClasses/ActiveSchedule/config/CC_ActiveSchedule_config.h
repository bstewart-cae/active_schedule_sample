/*
 * SPDX-License-Identifier: BSD-3-Clause
 * SPDX-FileCopyrightText: 2025 Card Access Engineering, LLC.
 */
/**
 * @file CC_ActiveSchedule_config.h
 */

 #ifndef _CC_ACTIVE_SCHEDULE_CONFIG_H_
 #define _CC_ACTIVE_SCHEDULE_CONFIG_H_

 /**
  * \defgroup configuration Configuration
  * Configuration
  *
  * \addtogroup configuration
  * @{
  */
 /**
  * \defgroup command_class_active_schedule_configuration Command Class Active Schedule Configuration
  * Command Class Active Schedule Configuration
  *
  * \addtogroup command_class_active_schedule_configuration
  * @{
  */

 /**
  * Maximum number of CCs using Active Schedule that are supported by the node
  *
  */
 #if !defined(CC_ACTIVE_SCHEDULE_MAX_NUM_SUPPORTED_CCS)
 #define CC_ACTIVE_SCHEDULE_MAX_NUM_SUPPORTED_CCS  1
 #endif /* CC_ACTIVE_SCHEDULE_MAX_NUM_SUPPORTED_CCS */

 #endif // _CC_ACTIVE_SCHEDULE_CONFIG_H_
