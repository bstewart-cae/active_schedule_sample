# SPDX-License-Identifier: LicenseRef-TridentMSLA
# SPDX-FileCopyrightText: 2025 Trident IoT, LLC <https://www.tridentiot.com>
if(NOT DEFINED PLATFORM)
    message(FATAL_ERROR "PLATFORM must be defined for the HAL to build!")
endif()

if(NOT DEFINED TRIDENT_PLATFORM_PATH)
    set(TRIDENT_PLATFORM_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../../platform/${PLATFORM})
endif()

# collect all Trident HAL .c files
file(GLOB HAL_SRC ${CMAKE_CURRENT_SOURCE_DIR}/${PLATFORM}/*.c)

# set chip specific include path(s)
if(${PLATFORM} STREQUAL "T32CM11")
    set(HAL_INCLUDE
        ${TRIDENT_PLATFORM_PATH}/Library/RT58x/Include
        ${TRIDENT_PLATFORM_PATH}/Library/RT58x/PHY/include
        ${TRIDENT_PLATFORM_PATH}/Library/RT58x/Peripheral/Include
        ${TRIDENT_PLATFORM_PATH}/Library/CMSIS/Include
        ${TRIDENT_PLATFORM_PATH}/config/common
    )
elseif(${PLATFORM} STREQUAL "T32CZ20")
    if(NOT DEFINED ZW_SDK_ROOT)
        set(ZW_SDK_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../../z-wave)
    endif()

    set(HAL_INCLUDE
        ${TRIDENT_PLATFORM_PATH}/Library/RT584/PHY/include
        ${TRIDENT_PLATFORM_PATH}/Library/RT584/Peripheral/Include
        ${TRIDENT_PLATFORM_PATH}/Library/CMSIS/Include
        ${ZW_SDK_ROOT}/platform/TridentIoT/config
    )
else()
    message(FATAL_ERROR "HAL library build failure, unknown platform: ${PLATFORM}")
endif()

add_library(tr_hal OBJECT ${HAL_SRC})

target_include_directories(tr_hal
    PUBLIC
    ${HAL_INCLUDE}
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/${PLATFORM}
)

target_compile_options(tr_hal
  PRIVATE
    -Wpedantic
    -Wconversion
    -Wextra
    -Werror
    -Wall
)
