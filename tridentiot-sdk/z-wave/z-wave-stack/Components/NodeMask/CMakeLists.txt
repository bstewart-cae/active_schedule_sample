# SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
# SPDX-FileCopyrightText: 2025 Trident IoT, LLC <https://www.tridentiot.com>
#
# SPDX-License-Identifier: BSD-3-Clause
if(${ZW_CONFIG_IMPORT_LIBRARIES} STREQUAL "true")
  message(DEBUG "Import NodeMask library from ${ZW_CONFIG_LIBRARY_DESTINATION}/libZWaveSlave.a")
  add_library(NodeMask STATIC IMPORTED GLOBAL)
  set_target_properties(NodeMask PROPERTIES IMPORTED_LOCATION ${ZW_CONFIG_LIBRARY_DESTINATION}/libZWaveSlave.a)
  target_include_directories(NodeMask INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
else()
  message(DEBUG "Create NodeMask library.")
  add_library(NodeMask OBJECT NodeMask.c)
  target_include_directories(NodeMask
    PUBLIC
      "${CMAKE_CURRENT_SOURCE_DIR}"
      "${ZWAVE_API_DIR}"
  )
  target_link_libraries(NodeMask
    PUBLIC
      zpal
  )
endif()
add_test_subdirectory(mocks)