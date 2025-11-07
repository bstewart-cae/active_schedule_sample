# SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
# SPDX-FileCopyrightText: 2025 Trident IoT, LLC <https://www.tridentiot.com>
#
# SPDX-License-Identifier: BSD-3-Clause
if(${ZW_CONFIG_IMPORT_LIBRARIES} STREQUAL "true")
  message(DEBUG "Import SyncEvent library from ${ZW_CONFIG_LIBRARY_DESTINATION}/libZWaveSlave.a")
  add_library(SyncEvent STATIC IMPORTED GLOBAL)
  set_target_properties(SyncEvent PROPERTIES IMPORTED_LOCATION ${ZW_CONFIG_LIBRARY_DESTINATION}/libZWaveSlave.a)
  target_include_directories(SyncEvent INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
else()
  message(DEBUG "Create SyncEvent library.")
  add_library(SyncEvent OBJECT SyncEvent.c)
  target_include_directories(SyncEvent PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/")
endif()
add_test_subdirectory(Test)
