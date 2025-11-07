# SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
#
# SPDX-License-Identifier: BSD-3-Clause

set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR x86)

find_program(CMAKE_SIZE  NAMES size)

add_compile_options(
  -m32
  -funwind-tables
  -fshort-enums
  -g3
  -fdata-sections
  -ffunction-sections
  $<$<CONFIG:Release>:-fmerge-all-constants>
  $<$<COMPILE_LANGUAGE:CXX>:-fexceptions>
)

add_link_options(
  -m32
  -Wl,--gc-sections
  -Wl,--no-export-dynamic
  -Wl,--compress-debug-sections=zlib
)
