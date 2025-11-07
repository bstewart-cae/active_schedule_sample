/// ***************************************************************************
///
/// @file tr_platform.h
///
/// @brief  Serves as a way to abstact from architecture specific includes.
///
/// SPDX-License-Identifier: LicenseRef-TridentMSLA
/// SPDX-FileCopyrightText: 2024 Trident IoT, LLC <https://www.tridentiot.com>
/// ***************************************************************************

#if defined(TR_PLATFORM_T32CZ20)
  #include "cm33.h"
#else
  #error "NO TR_PLATFORM DEFINED!"
#endif

#ifndef ZWSDK_WEAK
#define ZWSDK_WEAK             __attribute__((weak))
#endif
