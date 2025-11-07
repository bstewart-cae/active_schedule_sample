# SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
# SPDX-FileCopyrightText: 2025 Trident IoT, LLC <https://www.tridentiot.com>
#
# SPDX-License-Identifier: BSD-3-Clause
if(${ZW_CONFIG_IMPORT_LIBRARIES} STREQUAL "true")
  message(DEBUG "Import DebugPrint library from ${ZW_CONFIG_LIBRARY_DESTINATION}/libZWaveSlave.a")
  add_library(DebugPrint STATIC IMPORTED GLOBAL)
  set_target_properties(DebugPrint PROPERTIES IMPORTED_LOCATION ${ZW_CONFIG_LIBRARY_DESTINATION}/libZWaveSlave.a)
  target_include_directories(DebugPrint INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
else()
  message(DEBUG "Create DebugPrint library.")
  add_library(DebugPrint OBJECT DebugPrint.c)
  target_include_directories(DebugPrint
    PUBLIC
      ${CMAKE_CURRENT_SOURCE_DIR}
    PRIVATE
      $<TARGET_PROPERTY:Utils,INTERFACE_INCLUDE_DIRECTORIES>
  )
endif()

add_test_subdirectory(mocks)
