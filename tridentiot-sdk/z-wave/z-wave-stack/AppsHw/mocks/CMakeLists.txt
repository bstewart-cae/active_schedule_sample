# SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
#
# SPDX-License-Identifier: BSD-3-Clause

add_library(AppsHwMock OBJECT)
target_sources(AppsHwMock
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/app_hw_mock.c
    ${CMAKE_CURRENT_SOURCE_DIR}/board_indicator_mock.c
    ${CMAKE_CURRENT_SOURCE_DIR}/board_init_mock.c
)
target_link_libraries(AppsHwMock
  PUBLIC
    mock
    AppsHw
)

add_mock(AppsHw_cmock ${CMAKE_CURRENT_SOURCE_DIR}/../inc/board_indicator.h ${CMAKE_CURRENT_SOURCE_DIR}/../inc/board_init.h)
