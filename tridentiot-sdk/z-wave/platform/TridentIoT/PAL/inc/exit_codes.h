/// ****************************************************************************
/// @file exit_codes.h
///
/// SPDX-License-Identifier: LicenseRef-TridentMSLA
/// SPDX-FileCopyrightText: 2025 Trident IoT, LLC <https://www.tridentiot.com>
/// ***************************************************************************

#ifndef EXIT_CODES_H_
#define EXIT_CODES_H_

typedef enum
{
  EXIT_CODE_QUIT,
  EXIT_CODE_RESTART,
  EXIT_CODE_SLEEP,
  EXIT_CODE_WATCHDOG,
} exit_code_t;

#endif /* EXIT_CODES_H_ */
