/// ***************************************************************************
/// SPDX-License-Identifier: LicenseRef-TridentMSLA
/// SPDX-FileCopyrightText: 2024 Trident IoT, LLC <https://www.tridentiot.com>
/// ***************************************************************************

/**
 * @file tr_board_perseus.h
 *
 * @brief Defines relevant pins on the Perseus board.
 *
 */
#ifndef TR_BOARD_PERSEUS_H
#define TR_BOARD_PERSEUS_H

#include <stdint.h>

static const uint32_t TR_BOARD_LED_RED   = 9;
static const uint32_t TR_BOARD_LED_GREEN = 14;
static const uint32_t TR_BOARD_LED_BLUE  = 8;

static const uint32_t TR_BOARD_BTN1 = 20;
static const uint32_t TR_BOARD_BTN2 = 15;

static const uint32_t TR_BOARD_UART0_USB_TX = 17;
static const uint32_t TR_BOARD_UART0_USB_RX = 16;

static const uint32_t TR_BOARD_UART2_TAG_TX = 6;
static const uint32_t TR_BOARD_UART2_TAG_RX = 7;

static const uint32_t TR_BOARD_LED_LEARN_MODE = TR_BOARD_LED_GREEN;
static const uint32_t TR_BOARD_BTN_LEARN_MODE = TR_BOARD_BTN1;

#endif // TR_BOARD_PERSEUS_H
