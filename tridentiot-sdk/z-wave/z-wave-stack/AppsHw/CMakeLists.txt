# SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
#
# SPDX-License-Identifier: BSD-3-Clause

add_library(AppsHw INTERFACE)
target_include_directories(AppsHw
  INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/inc
)

add_test_subdirectory(mocks)
