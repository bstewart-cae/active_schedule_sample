# SPDX-FileCopyrightText: 2025 Trident IoT, LLC <https://www.tridentiot.com>
# SPDX-License-Identifier: LicenseRef-TridentMSLA

add_library(tr_ring_buffer OBJECT ${TRIDENT_SDK_ROOT}/framework/utility/ring_buffer/tr_ring_buffer.c)
target_include_directories(tr_ring_buffer PUBLIC ${TRIDENT_SDK_ROOT}/framework/utility/ring_buffer/)
target_compile_options(tr_ring_buffer
  PRIVATE
    -Wpedantic
    -Wconversion
    -Wextra
    -Werror
    -Wall
)

add_library(tr_cli_common OBJECT
             ${TRIDENT_SDK_ROOT}/framework/utility/cli/tr_cli.c
             ${TRIDENT_SDK_ROOT}/framework/utility/cli/tr_cli_command_parser.c
           )
target_include_directories(tr_cli_common
  PUBLIC ${TRIDENT_SDK_ROOT}/framework/utility/cli/
         ${TRIDENT_SDK_ROOT}/framework/utility/printf/)
target_link_libraries(tr_cli_common PRIVATE Assert zpal_${PLATFORM_VARIANT})
target_compile_options(tr_cli_common
  PRIVATE
    -Wpedantic
    -Wconversion
    -Wextra
    -Werror
    -Wall
)
target_compile_definitions(tr_cli_common INTERFACE TR_CLI_ENABLED)
