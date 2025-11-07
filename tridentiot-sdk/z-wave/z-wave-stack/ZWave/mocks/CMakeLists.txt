# SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
#
# SPDX-License-Identifier: BSD-3-Clause

set(mock_src 
  ${CMAKE_CURRENT_SOURCE_DIR}/ZW_keystore_mock.c
  ${CMAKE_CURRENT_SOURCE_DIR}/ZW_transport_mock.c
  ${CMAKE_CURRENT_SOURCE_DIR}/ZW_timer_mock.c
  ${CMAKE_CURRENT_SOURCE_DIR}/ZW_mem_mock.c
  ${CMAKE_CURRENT_SOURCE_DIR}/ZW_slave_mock.c
  ${CMAKE_CURRENT_SOURCE_DIR}/ZW_crc_mock.c
  ${CMAKE_CURRENT_SOURCE_DIR}/ZW_main_mock.c
  ${CMAKE_CURRENT_SOURCE_DIR}/ZW_main_region_mock.c
  ${CMAKE_CURRENT_SOURCE_DIR}/ZW_version_mock.c
  ${CMAKE_CURRENT_SOURCE_DIR}/ZW_phy_050x_mock.c
  ${CMAKE_CURRENT_SOURCE_DIR}/ZW_phy_uart0_050x_mock.c
  ${CMAKE_CURRENT_SOURCE_DIR}/ZW_Security_Scheme2_mock.c
 )
add_library(ZWaveProtocolMocks ${mock_src})
set_target_properties(ZWaveProtocolMocks PROPERTIES COMPILE_DEFINITIONS "ZW_SLAVE_ENHANCED_232;ZW_MEMFUNCTION_FAKE")
target_include_directories(ZWaveProtocolMocks
  PUBLIC
    "${ZW_ROOT}/ZWave"
    "${ZW_ROOT}/ZWave/Protocol"
)
