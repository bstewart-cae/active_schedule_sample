# SPDX-FileCopyrightText: 2025 Trident IoT, LLC <https://www.tridentiot.com>
# SPDX-License-Identifier: LicenseRef-TridentMSLA

set(CMAKE_SYSTEM_NAME Generic) # https://cmake.org/cmake/help/latest/variable/CMAKE_SYSTEM_NAME.html
set(CMAKE_SYSTEM_PROCESSOR Armv8-M)

set(PLATFORM "T32CZ20")

# The platform variant is supposed to be something like RT581, RT584z, etc.
# but let's stick to same as PLATFORM for now.
set(PLATFORM_VARIANT ${PLATFORM})

set(TOOLCHAIN_PREFIX arm-none-eabi-)
find_program(BINUTILS_PATH ${TOOLCHAIN_PREFIX}gcc NO_CACHE)

if (NOT BINUTILS_PATH)
    message(FATAL_ERROR "ARM GCC toolchain not found")
endif ()

get_filename_component(ARM_TOOLCHAIN_DIR ${BINUTILS_PATH} DIRECTORY)
# Without that flag CMake is not able to pass test compilation check
if (${CMAKE_VERSION} VERSION_EQUAL "3.6.0" OR ${CMAKE_VERSION} VERSION_GREATER "3.6")
    set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
else ()

endif ()

if (DEFINED LINKER_SCRIPT_PATH AND EXISTS ${LINKER_SCRIPT_PATH})
  set(LINKER_SCRIPT_PATH_LOCAL ${LINKER_SCRIPT_PATH})
else()
  set(LINKER_SCRIPT_PATH_LOCAL "${TRISDK_PATH}/Library/RT58x/Device/GCC/app_gcc_cm3_mcu.ld")
endif()

message(STATUS "LINKER SCRIPT: ${LINKER_SCRIPT_PATH_LOCAL}")

math(EXPR NVM_STORAGE_SIZE "1024 * 80" OUTPUT_FORMAT HEXADECIMAL)

set (LINKERSCRIPT_SYMBOLS "-Xlinker --defsym -Xlinker NVM_STORAGE_SIZE=${NVM_STORAGE_SIZE} ")
set (LINKERSCRIPT_SYMBOLS "${LINKERSCRIPT_SYMBOLS} -Xlinker --defsym -Xlinker __STACK_SIZE=0x00000800")
set (LINKERSCRIPT_SYMBOLS "${LINKERSCRIPT_SYMBOLS} -Xlinker --defsym -Xlinker __HEAP_SIZE=0x00002000")
set (LINKERSCRIPT_SYMBOLS "${LINKERSCRIPT_SYMBOLS} -Xlinker --defsym -Xlinker __RET_SRAM_SIZE=0x00000400")

set(CMAKE_EXE_LINKER_FLAGS_INIT "-lm -T ${LINKER_SCRIPT_PATH_LOCAL} -Xlinker --no-warn-rwx-segments -Xlinker --gc-sections ${LINKERSCRIPT_SYMBOLS}")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS_INIT}" CACHE STRING "" FORCE)

set(CMAKE_C_COMPILER ${TOOLCHAIN_PREFIX}gcc)
set(CMAKE_ASM_COMPILER ${CMAKE_C_COMPILER})
set(CMAKE_CXX_COMPILER ${TOOLCHAIN_PREFIX}g++)
set(CMAKE_AR ${TOOLCHAIN_PREFIX}gcc-ar)
set(CMAKE_RANLIB ${TOOLCHAIN_PREFIX}gcc-ranlib)

execute_process(COMMAND ${CMAKE_C_COMPILER} -print-sysroot
    OUTPUT_VARIABLE ARM_GCC_SYSROOT OUTPUT_STRIP_TRAILING_WHITESPACE)

# Default C compiler flags
set(CMAKE_C_FLAGS_DEBUG_INIT "-mcmse --specs=nosys.specs --specs=nano.specs  -g3 -Og -gdwarf-2 -mcpu=cortex-m33 -mthumb -std=c99 -ffunction-sections -fdata-sections -mfpu=fpv5-sp-d16 -mfloat-abi=softfp")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG_INIT}" CACHE STRING "" FORCE)
set(CMAKE_C_FLAGS_RELEASE_INIT "-mcmse --specs=nosys.specs --specs=nano.specs  -Os -mcpu=cortex-m33 -mthumb -std=c99 -ffunction-sections -fdata-sections -mfpu=fpv5-sp-d16 -mfloat-abi=softfp ")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE_INIT}" CACHE STRING "" FORCE)

# Unused build configurations. Flags are not verified.
set(CMAKE_C_FLAGS_MINSIZEREL_INIT "-Os")
set(CMAKE_C_FLAGS_MINSIZEREL "${CMAKE_C_FLAGS_MINSIZEREL_INIT}" CACHE STRING "" FORCE)
set(CMAKE_C_FLAGS_RELWITHDEBINFO_INIT "-O2 -g")
set(CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS_RELWITHDEBINFO_INIT}" CACHE STRING "" FORCE)

# Default C++ compiler flags
# Unused.
set(CMAKE_CXX_FLAGS_DEBUG_INIT "-g3 -Og")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG_INIT}" CACHE STRING "" FORCE)
set(CMAKE_CXX_FLAGS_RELEASE_INIT "-O3")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE_INIT}" CACHE STRING "" FORCE)
set(CMAKE_CXX_FLAGS_MINSIZEREL_INIT "-Os")
set(CMAKE_CXX_FLAGS_MINSIZEREL "${CMAKE_CXX_FLAGS_MINSIZEREL_INIT}" CACHE STRING "" FORCE)
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO_INIT "-O2 -g")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO_INIT}" CACHE STRING "" FORCE)

set(CMAKE_OBJCOPY ${ARM_TOOLCHAIN_DIR}/${TOOLCHAIN_PREFIX}objcopy CACHE INTERNAL "objcopy tool")
set(CMAKE_SIZE_UTIL ${ARM_TOOLCHAIN_DIR}/${TOOLCHAIN_PREFIX}size CACHE INTERNAL "size tool")

set(CMAKE_SYSROOT ${ARM_GCC_SYSROOT})
set(CMAKE_FIND_ROOT_PATH ${BINUTILS_PATH})
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

# TODO: Replace with non-Silabs-specific define.
add_compile_definitions(EFR32ZG)
add_compile_definitions(CHIP_VERSION=RT58X_MPB)
add_compile_definitions($<$<CONFIG:Debug>:ZWSDK_DEBUG_BUILD>)
add_compile_definitions($<$<CONFIG:Release>:ZWSDK_RELEASE_BUILD>)
