# SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
#
# SPDX-License-Identifier: BSD-3-Clause

add_library(zpal_common OBJECT
  ${CMAKE_CURRENT_SOURCE_DIR}/zpal_radio_utils.c
)

target_link_libraries(zpal_common
  PUBLIC
    zpal
    Assert
)
