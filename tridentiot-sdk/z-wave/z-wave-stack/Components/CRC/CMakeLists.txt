# SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
# SPDX-FileCopyrightText: 2025 Trident IoT, LLC <https://www.tridentiot.com>
#
# SPDX-License-Identifier: BSD-3-Clause

if(${ZW_CONFIG_IMPORT_LIBRARIES} STREQUAL "true")
  message(DEBUG "Import CRC library from ${ZW_CONFIG_LIBRARY_DESTINATION}/libZWaveSlave.a")
  add_library(CRC STATIC IMPORTED GLOBAL)
  set_target_properties(CRC PROPERTIES IMPORTED_LOCATION ${ZW_CONFIG_LIBRARY_DESTINATION}/libZWaveSlave.a)
  target_include_directories(CRC INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
else()
  message(DEBUG "Create CRC library.")
  add_library(CRC OBJECT CRC.c)
  target_include_directories(CRC PUBLIC ".")
endif()

add_test_subdirectory(tests)
