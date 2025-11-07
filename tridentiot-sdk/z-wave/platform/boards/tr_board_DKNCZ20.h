/// ***************************************************************************
/// SPDX-License-Identifier: LicenseRef-TridentMSLA
/// SPDX-FileCopyrightText: 2024 Trident IoT, LLC <https://www.tridentiot.com>
/// ***************************************************************************

/**
 * @file tr_board_DKNCZ20.h
 *
 * @brief Defines relevant pins on the DKNCZ20 board.
 *
 */
#ifndef TR_BOARD_DKNCZ20_H
#define TR_BOARD_DKNCZ20_H

#include <stdint.h>

static const uint32_t TR_BOARD_LED_RED   = 1;
static const uint32_t TR_BOARD_LED_GREEN = 30;
static const uint32_t TR_BOARD_LED_BLUE  = 31;

static const uint32_t TR_BOARD_BTN1 = 4;
static const uint32_t TR_BOARD_BTN2 = 5;

static const uint32_t TR_BOARD_UART0_TX = 17;
static const uint32_t TR_BOARD_UART0_RX = 16;

static const uint32_t TR_BOARD_LED_LEARN_MODE = TR_BOARD_LED_GREEN;
static const uint32_t TR_BOARD_BTN_LEARN_MODE = TR_BOARD_BTN1;

#endif // TR_BOARD_DKNCZ20_H
