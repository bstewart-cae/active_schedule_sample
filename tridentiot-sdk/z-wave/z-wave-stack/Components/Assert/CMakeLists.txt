# SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
# SPDX-FileCopyrightText: 2025 Trident IoT, LLC <https://www.tridentiot.com>
#
# SPDX-License-Identifier: BSD-3-Clause

if(NOT CMAKE_BUILD_TYPE STREQUAL Test)
  if(${ZW_CONFIG_IMPORT_LIBRARIES} STREQUAL "true")
    message(DEBUG "Import Assert library from ${ZW_CONFIG_LIBRARY_DESTINATION}/libZWaveSlave.a")
    add_library(Assert STATIC IMPORTED GLOBAL)
    set_target_properties(Assert PROPERTIES IMPORTED_LOCATION ${ZW_CONFIG_LIBRARY_DESTINATION}/libZWaveSlave.a)
    target_include_directories(Assert INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
  else()
    message(DEBUG "Create Assert library.")
    add_library(Assert OBJECT Assert_zw.c)
    target_include_directories(Assert
      PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${ZW_ROOT}/ZWave/API
    )
    target_link_libraries(Assert
      PRIVATE
        DebugPrint
        zpal
    )
  endif()
endif()

add_test_subdirectory(Test)
add_test_subdirectory(mocks)
