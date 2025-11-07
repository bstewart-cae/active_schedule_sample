# SPDX-FileCopyrightText: 2025 Trident IoT, LLC <https://www.tridentiot.com>
# SPDX-License-Identifier: LicenseRef-TridentMSLA

zw_add_interface_cc(NAME CC_RemoteCLI
  INCLUDES
    ${CMAKE_CURRENT_SOURCE_DIR}/inc
    ${CMAKE_CURRENT_SOURCE_DIR}/config
    ${ZAF_CONFIGDIR}/inc
  SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/CC_RemoteCLI.c
  DEPENDS
    ZAF_Actuator
    ZAF_TSE_weak
    CC_Supervision
    tr_ring_buffer
)
