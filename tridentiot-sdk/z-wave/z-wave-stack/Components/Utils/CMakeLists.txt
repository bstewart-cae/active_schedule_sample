# SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
#
# SPDX-License-Identifier: BSD-3-Clause

add_library(Utils INTERFACE)
target_include_directories(Utils
  INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}
)
