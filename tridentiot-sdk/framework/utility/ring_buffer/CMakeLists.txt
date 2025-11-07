# SPDX-License-Identifier: LicenseRef-TridentMSLA
# SPDX-FileCopyrightText: 2025 Trident IoT, LLC <https://www.tridentiot.com>
add_library(tr_ring_buffer OBJECT ${CMAKE_CURRENT_SOURCE_DIR}/tr_ring_buffer.c)
target_include_directories(tr_ring_buffer PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_options(tr_ring_buffer
  PRIVATE
    -Wpedantic
    -Wconversion
    -Wextra
    -Werror
    -Wall
)
