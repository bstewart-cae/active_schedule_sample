# SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
# SPDX-FileCopyrightText: 2025 Trident IoT, LLC <https://www.tridentiot.com>
#
# SPDX-License-Identifier: BSD-3-Clause
if(${ZW_CONFIG_IMPORT_LIBRARIES} STREQUAL "true")
  message(DEBUG "Import MfgTokens library from ${ZW_CONFIG_LIBRARY_DESTINATION}/libZWaveSlave.a")
  add_library(MfgTokens STATIC IMPORTED GLOBAL)
  set_target_properties(MfgTokens PROPERTIES IMPORTED_LOCATION ${ZW_CONFIG_LIBRARY_DESTINATION}/libZWaveSlave.a)
  target_include_directories(MfgTokens INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
else()
  message(DEBUG "Create MfgTokens library.")
  add_library(MfgTokens OBJECT MfgTokens.c)
  target_include_directories(MfgTokens
    PUBLIC
      ${CMAKE_CURRENT_SOURCE_DIR}
  )
  target_link_libraries(MfgTokens
    PRIVATE
      Assert
      DebugPrint
      zpal
  )
endif()

add_test_subdirectory(mocks)
