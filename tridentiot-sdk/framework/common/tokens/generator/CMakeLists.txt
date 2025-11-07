# SPDX-License-Identifier: LicenseRef-TridentMSLA
# SPDX-FileCopyrightText: 2025 Trident IoT, LLC <https://www.tridentiot.com>
cmake_minimum_required(VERSION 3.23.5)

# Project name and type
project(token_def_gen
        VERSION 0.1
        LANGUAGES C)

add_library(token_def
            SHARED
            token_def.c)

target_include_directories(token_def
                           PUBLIC
                           ${CMAKE_CURRENT_SOURCE_DIR}/..
                           ${PLATFORM_PATH}/tokens)

# Add the -fPIC flag for position-independent code
set_target_properties(token_def PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_compile_options(token_def PRIVATE -Wno-pointer-to-int-cast)

if(DEFINED TR_PLATFORM)
    target_compile_definitions(token_def
                               PUBLIC
                               TR_PLATFORM_${TR_PLATFORM})    
endif()

if(DEFINED APP_INCLUDE_FILE)
    target_compile_definitions(token_def
                               PUBLIC
                               APP_INCLUDE_FILE="${APP_INCLUDE_FILE}")
endif()

if(DEFINED APP_INCLUDE_PATH)
    target_include_directories(token_def
                               PUBLIC
                               ${APP_INCLUDE_PATH})
endif()
