/// ***************************************************************************
///
/// @file platform_utils.h
///
/// SPDX-License-Identifier: LicenseRef-TridentMSLA
/// SPDX-FileCopyrightText: 2025 Trident IoT, LLC <https://www.tridentiot.com>
/// ***************************************************************************

#ifndef PLATFORM_UTILS_H_
#define PLATFORM_UTILS_H_

#include <stdint.h>
#include <zpal_radio.h>
#include <exit_codes.h>

void pu_init(void);

void pu_exit(exit_code_t exit_code);
void pu_exit_from_isr(exit_code_t exit_code);

const char* pu_get_pty(void);

void pu_stack_init(void);
void pu_wait_stack_init(void);

void pu_apps_hw_init(void (*cmd_handler)(char, void*), void* arg);
void pu_wait_apps_hw_init(void);
void pu_handle_apps_hw_cmd(char cmd);

void pu_get_dsk(uint8_t dsk[16]);
node_id_t pu_get_node_id(void);
void pu_get_qrcode(uint8_t *qrcode);

#endif /* PLATFORM_UTILS_H_ */
