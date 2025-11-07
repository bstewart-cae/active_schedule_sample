# SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
#
# SPDX-License-Identifier: BSD-3-Clause

add_library(zpal INTERFACE)
target_include_directories(zpal
  INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/inc
)

add_subdirectory(src)

add_test_subdirectory(mocks)
add_test_subdirectory(test)
