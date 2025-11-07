# SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
# SPDX-FileCopyrightText: 2025 Trident IoT, LLC <https://www.tridentiot.com>
#
# SPDX-License-Identifier: BSD-3-Clause

if(${ZW_CONFIG_IMPORT_LIBRARIES} STREQUAL "true")
  message(DEBUG "Import QueueNotifying library from ${ZW_CONFIG_LIBRARY_DESTINATION}/libZWaveSlave.a")
  add_library(QueueNotifying STATIC IMPORTED GLOBAL)
  set_target_properties(QueueNotifying PROPERTIES IMPORTED_LOCATION ${ZW_CONFIG_LIBRARY_DESTINATION}/libZWaveSlave.a)
  target_include_directories(QueueNotifying INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
else()
  message(DEBUG "Create QueueNotifying library.")
add_library(QueueNotifying OBJECT QueueNotifying.c)

target_include_directories(QueueNotifying
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
  PRIVATE
    $<TARGET_PROPERTY:zpal_${PLATFORM_VARIANT},INTERFACE_INCLUDE_DIRECTORIES>
)

target_link_libraries(QueueNotifying
  PRIVATE
    Assert
    DebugPrint
)
endif()

add_test_subdirectory(mocks)
