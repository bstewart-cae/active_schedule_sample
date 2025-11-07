# SPDX-License-Identifier: LicenseRef-TridentMSLA
# SPDX-FileCopyrightText: 2025 Trident IoT, LLC <https://www.tridentiot.com>

##
# @brief Builds the bootloader project for the cooresponding PLATFORM
#
# This function builds the bootloader project found in the bootloader directory
# for the defined PLATFORM
#
# Required Inputs:
# @param[in] BTL_SOURCE_DIR           path/to/bootloader source files
# @param[in] BTL_BINARY_DIR           path/to/bootloader binary directory - where build output goes
# @param[in] BTL_TOOLCHAIN_FILE_PATH  path/to/bootloader-toolchain.cmake
# @param[in] BTL_LINKER_SCRIPT_PATH   path/to/bootloader-linker-script.ld
# @param[in] PLATFORM                 High level platform name (e.g. T32CM11).
# @param[in] CMAKE_BUILD_TYPE         Debug or Release
# @param[in] TRIDENT_SDK_ROOT         Path to Trident IoT SDK root directory.
#
# Conditionally Required Inputs:
# @param[in] FLASH_SIZE               Flash size part number character - required for T32CM11
#
function(tr_build_bootloader)
    # Check if required variables are defined
    if(NOT DEFINED BTL_SOURCE_DIR)
        message(WARNING "BTL_SOURCE_DIR is not defined. Skipping bootloader build.")
        return()
    endif()

    if(NOT DEFINED BTL_BINARY_DIR)
        message(WARNING "BTL_BINARY_DIR is not defined. Skipping bootloader build.")
        return()
    endif()

    # Check if source directory exists
    if(NOT EXISTS "${BTL_SOURCE_DIR}")
        message(WARNING "Bootloader project not found at: ${BTL_SOURCE_DIR}")
        return()
    endif()

    message(STATUS "Bootloader project found at: ${BTL_SOURCE_DIR}")

    # Check if required toolchain and linker script variables are defined
    if(NOT DEFINED BTL_TOOLCHAIN_FILE_PATH)
        message(WARNING "BTL_TOOLCHAIN_FILE_PATH is not defined. Cannot build bootloader.")
        return()
    endif()

    if(NOT DEFINED BTL_LINKER_SCRIPT_PATH)
        message(WARNING "BTL_LINKER_SCRIPT_PATH is not defined. Cannot build bootloader.")
        return()
    endif()

    # Check if additional required variables exist
    if(NOT DEFINED PLATFORM)
        message(WARNING "PLATFORM is not defined. Cannot build bootloader.")
        return()
    endif()

    if(NOT DEFINED FLASH_SIZE AND NOT ${PLATFORM} STREQUAL "T32CZ20")
        message(WARNING "FLASH_SIZE is not defined. Cannot build bootloader.")
        return()
    endif()

    if(NOT DEFINED CMAKE_BUILD_TYPE)
        message(WARNING "CMAKE_BUILD_TYPE is not defined. Cannot build bootloader.")
        return()
    endif()

    if(NOT DEFINED TRIDENT_SDK_ROOT)
        message(WARNING "TRIDENT_SDK_ROOT is not defined. Cannot build bootloader.")
        return()
    endif()

    # All variables are defined and valid, proceed with building
    include(ExternalProject)

    set(BTL_CMAKE_ARGS
        -DCMAKE_TOOLCHAIN_FILE=${BTL_TOOLCHAIN_FILE_PATH}
        -DLINKER_SCRIPT_PATH=${BTL_LINKER_SCRIPT_PATH}
        -DPLATFORM=${PLATFORM}
        -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
        -DTRIDENT_SDK_ROOT=${TRIDENT_SDK_ROOT}
    )

    if(${PLATFORM} STREQUAL "T32CM11")
        list(APPEND BTL_CMAKE_ARGS -DFLASH_SIZE=${FLASH_SIZE})
    endif()

    ExternalProject_Add(
        tr_bootloader
        SOURCE_DIR ${BTL_SOURCE_DIR}
        BINARY_DIR ${BTL_BINARY_DIR}
        CMAKE_ARGS ${BTL_CMAKE_ARGS}
        # skip installation step
        INSTALL_COMMAND ""
    )
endfunction()
