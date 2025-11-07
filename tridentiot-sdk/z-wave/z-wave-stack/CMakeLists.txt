# SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
# SPDX-FileCopyrightText: 2025 Trident IoT, LLC <https://www.tridentiot.com>
# SPDX-License-Identifier: BSD-3-Clause
cmake_minimum_required(VERSION 3.23.5)

# Convenient for getting more CMake output when running "make".
#set(CMAKE_MESSAGE_LOG_LEVEL DEBUG)

# Python3 is required by this build system.
find_package(Python3 REQUIRED)
if(NOT DEFINED Python3_FOUND)
  message(FATAL_ERROR "Python not found")
endif()

## Setup common directory pointers
SET(ZW_ROOT                ${CMAKE_CURRENT_SOURCE_DIR} CACHE INTERNAL zw_root)
SET(COMPONENTS_DIR         ${CMAKE_CURRENT_SOURCE_DIR}/Components)
SET(CONFIG_DIR             ${CMAKE_CURRENT_SOURCE_DIR}/config CACHE INTERNAL config_dir)
SET(ZWAVE_API_DIR          ${CMAKE_CURRENT_SOURCE_DIR}/ZWave/API)
SET(ZWAVE_CONFIG_DIR       ${CMAKE_CURRENT_SOURCE_DIR}/ZWave/ZWaveConfig)
SET(ZAF_TOOL               ${CMAKE_CURRENT_SOURCE_DIR}/ZAF/tool CACHE INTERNAL zaf_tool)
SET(ZAF_CONFIGDIR          ${CMAKE_CURRENT_SOURCE_DIR}/ZAF/configuration CACHE INTERNAL zaf_config)
SET(ZAF_UTILDIR            ${CMAKE_CURRENT_SOURCE_DIR}/ZAF/ApplicationUtilities CACHE INTERNAL zaf_utildir)
SET(ZAF_CCDIR              ${CMAKE_CURRENT_SOURCE_DIR}/ZAF/CommandClasses CACHE INTERNAL zaf_ccdir)
SET(ZAF_UNITTESTEXTERNALS  ${CMAKE_CURRENT_SOURCE_DIR}/ZAF/UnitTestExternals)
SET(ZAF_PROTOCOL_CONFIG    ${CMAKE_CURRENT_SOURCE_DIR}/ZAF/ProtocolConfig CACHE INTERNAL zaf_protocol_config)
SET(ZPAL_API_DIR           ${CMAKE_CURRENT_SOURCE_DIR}/PAL/inc)
SET(ZWAVE_PSA_INTEG_DIR    ${CMAKE_CURRENT_SOURCE_DIR}/ZWave/psa)
SET(FREERTOS_CONFIG_DIR    ${CONFIG_DIR}/FreeRTOS) # Path to Z-Wave FreeRTOS config headers
SET(SUBTREE_LIBS2          ${CMAKE_CURRENT_SOURCE_DIR}/SubTree/libs2)
SET(DEVTOOLS_DIR           ${CMAKE_CURRENT_SOURCE_DIR}/DevTools CACHE INTERNAL DEVTOOLS_DIR)
# When moving folders around, please remove the path that is no longer valid! (apart from creating new paths)

# Sets the install destination of the Z-Wave stack libraries (Controller/End Device).
if(NOT DEFINED ZW_CONFIG_LIBRARY_DESTINATION)
  set(ZW_CONFIG_LIBRARY_DESTINATION ${ZW_ROOT}/lib)
endif()

# Default behavior is to compile and link source code instead of importing a static library.
if(NOT DEFINED ZW_CONFIG_IMPORT_LIBRARIES)
  set(ZW_CONFIG_IMPORT_LIBRARIES "false")
endif()

include(DevTools/CMakeFunctions/list_subdirs.cmake)
list_subdirs(PLATFORMS ${ZW_ROOT}/platform)

if(NOT CMAKE_BUILD_TYPE STREQUAL Test)
  if(NOT DEFINED PLATFORM)
    string(REPLACE ";" "\n\t" PLATFORMS_STR "${PLATFORMS}")
    message(FATAL_ERROR "PLATFORM not defined. Available built-in platforms:\n\t${PLATFORMS_STR}")
  endif()
  message("PLATFORM: ${PLATFORM}")

  project(z-wave-stack C CXX ASM)
else()
  project(z-wave-stack-unit-test C CXX ASM)
  set(PLATFORM_VARIANT Test)
endif()


##
# @defgroup toplevel_cmake Toplevel CMakeLists.txt

##
# @addtogroup toplevel_cmake
#
# @brief This module contains documentation for the toplevel CMakeLists.txt file
#
# @details The toplevel @b CMakeLists.txt are responsible for
# - Build system setup of all variations related to @b CMAKE_BUILD_TYPE, see @ref build_types ,
#   hence, no other @b CMakeLists.txt should test the value @b CMAKE_BUILD_TYPE, except functions /
#   macros described in @ref custom_func .
#
# @{

IF( NOT CMAKE_BUILD_TYPE )
  SET( CMAKE_BUILD_TYPE Debug CACHE STRING "Build Type" FORCE)
ENDIF( NOT CMAKE_BUILD_TYPE )

add_subdirectory(DevTools/CMakeFunctions)
add_subdirectory(Doc)

# Set C standard
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_EXTENSIONS ON)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_EXTENSIONS ON)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set compile options
add_compile_options(
  -Wall
  -Werror
  -Werror=vla
  -Wparentheses
)

# Set compile definitions
add_compile_definitions(
  $<$<CONFIG:Release>:NO_DEBUGPRINT>
  $<$<CONFIG:Release>:NDEBUG>
)

IF( CMAKE_BUILD_TYPE STREQUAL Test )
  SET(ZWAVE_MOCKS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/ZWave/mocks)
  SET(PAL_MOCKS_DIR   ${CMAKE_CURRENT_SOURCE_DIR}/PAL/mocks)
  SET(MEMORYCHECK_COMMAND_OPTIONS "${MEMORYCHECK_COMMAND_OPTIONS} --verbose --errors-for-leak-kinds=all --show-leak-kinds=all --leak-check=full --error-exitcode=1 --track-origins=yes")

  include(CTest)
  enable_testing()

  add_compile_definitions(UNIT_TEST)

  add_compile_options(
    -m32
    -funwind-tables
    -g3
    -O0
    $<$<COMPILE_LANGUAGE:CXX>:-fexceptions>
    --coverage
  )

  add_link_options(
    -m32
    -funwind-tables
    -Wl,-Map=output.map
    --coverage
  )

  # compile the unity version bundled along with cmock sources.
  add_library(unity2
    ThirdParty/cmock/vendor/unity/src/unity.c
    SubTree/TestFramework/unity/generic/unity_print.c)
  target_include_directories(unity2
    PUBLIC
      ThirdParty/cmock/vendor/unity/src # Unity in CMock folder MUST be the first include folder so that unity.h is taken from here.
      SubTree/TestFramework/unity
  )
  target_compile_options(unity2 PRIVATE "-fPIC")

  if(NOT TARGET cmock)
    # Build the cmock library and link the above compiled unity with the cmock library
    add_library(cmock STATIC ThirdParty/cmock/src/cmock.c)
    target_include_directories(cmock PUBLIC ThirdParty/cmock/src)
    target_link_libraries(cmock PUBLIC unity2)
    target_compile_options(cmock PRIVATE "-fPIC")
  endif()
  # and the add_mock functions to make it available to the individual modules
  include(DevTools/CMakeFunctions/add_mock.cmake)

  option(BUILD_SHARED_LIBS "" OFF)
  option(BUILD_TESTS "" OFF)
  add_subdirectory(ThirdParty/kaitai-cpp/)
  #add_subdirectory(kaitai-structs/)
ELSE()
  SET(DISABLE_MOCK 1)
  add_compile_definitions(SINGLE_CONTEXT) ## Use single context for libs2 on zw700.
  add_compile_options(-Werror=unused-parameter)
ENDIF( CMAKE_BUILD_TYPE STREQUAL Test )

IF( NOT DEFINED ZW_VERSION_MAJOR )
set(ZW_VERSION_MAJOR  25)
set(ZW_VERSION_MINOR  1)
set(ZW_VERSION_PATCH  0)
ENDIF()
IF( NOT DEFINED ZAF_VERSION_MAJOR )
set(ZAF_VERSION_MAJOR 25)
set(ZAF_VERSION_MINOR 1)
set(ZAF_VERSION_PATCH 0)
ENDIF()
IF( NOT DEFINED SDK_VERSION_MAJOR )
set(SDK_VERSION_MAJOR 25)
set(SDK_VERSION_MINOR 1)
set(SDK_VERSION_PATCH 0)
ENDIF()
IF( NOT DEFINED ZW_PROTOCOL_MANUFACTURER_ID )
# Z-Wave Alliance Manufacturer ID
set(ZW_PROTOCOL_MANUFACTURER_ID 0x031D)
ENDIF()

message ("ZW_VERSION: ${ZW_VERSION_MAJOR}.${ZW_VERSION_MINOR}.${ZW_VERSION_PATCH}")
message ("ZAF_VERSION: ${ZAF_VERSION_MAJOR}.${ZAF_VERSION_MINOR}.${ZAF_VERSION_PATCH}")
message ("SDK_VERSION: ${SDK_VERSION_MAJOR}.${SDK_VERSION_MINOR}.${SDK_VERSION_PATCH}")
message ("ZW_PROTOCOL_MANUFACTURER_ID: ${ZW_PROTOCOL_MANUFACTURER_ID}")

# make it available in source code
add_compile_definitions(
  ZW_VERSION_MAJOR=${ZW_VERSION_MAJOR}
  ZW_VERSION_MINOR=${ZW_VERSION_MINOR}
  ZW_VERSION_PATCH=${ZW_VERSION_PATCH}
  ZAF_VERSION_MAJOR=${ZAF_VERSION_MAJOR}
  ZAF_VERSION_MINOR=${ZAF_VERSION_MINOR}
  ZAF_VERSION_PATCH=${ZAF_VERSION_PATCH}
  SDK_VERSION_MAJOR=${SDK_VERSION_MAJOR}
  SDK_VERSION_MINOR=${SDK_VERSION_MINOR}
  SDK_VERSION_PATCH=${SDK_VERSION_PATCH}
  ZW_PROTOCOL_MANUFACTURER_ID=${ZW_PROTOCOL_MANUFACTURER_ID}
)

SET(SDK_VERSION ${SDK_VERSION_MAJOR}_${SDK_VERSION_MINOR}_${SDK_VERSION_PATCH})

IF($ENV{BUILD_NUMBER})
  SET(ZW_BUILD_NO $ENV{BUILD_NUMBER})
ELSE()
# the value 0xABCD is used in a unit test case
  SET(ZW_BUILD_NO 0xABCD)
ENDIF()

message("ZW_BUILD_NO: ${ZW_BUILD_NO}")

SET(ZAF_BUILD_NO ${ZW_BUILD_NO})
add_compile_definitions(
  ZW_BUILD_NO=${ZW_BUILD_NO}
  ZAF_BUILD_NO=${ZAF_BUILD_NO}
)

find_program(GIT "git")
IF((DEFINED ENV{GIT_COMMIT}) AND ($ENV{GIT_COMMIT} MATCHES "^[a-fA-F0-9]+$"))
  # GIT_COMMIT exists and it is valid commit ID number.
  SET(GIT_HASH $ENV{GIT_COMMIT})
ELSEIF(GIT)
  execute_process(
    COMMAND ${GIT} log -1 --format=%H
    RESULT_VARIABLE GIT_LOG_RESULT
    OUTPUT_VARIABLE GIT_LOG_OUTPUT
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )
  IF((NOT GIT_LOG_RESULT) AND (GIT_LOG_OUTPUT MATCHES "^[a-fA-F0-9]+$"))
    SET(GIT_HASH ${GIT_LOG_OUTPUT})
  ENDIF()
ENDIF()

IF(NOT DEFINED GIT_HASH)
  SET(GIT_HASH 0000000000000000000000000000000000000000)
ENDIF()

add_compile_definitions( GIT_HASH_ID="${GIT_HASH}" )
message("GIT_HASH: ${GIT_HASH}")

add_test_subdirectory(SubTree/TestFramework)
add_test_subdirectory(mocks)

if(NOT CMAKE_BUILD_TYPE STREQUAL Test)
  string(REPLACE "." ";" PLATFORM_LIST ${PLATFORM})
  list(GET PLATFORM_LIST 0 PLATFORM_NAME)
  if(${PLATFORM_NAME} IN_LIST PLATFORMS)
    # Use internal platform if the given platform is
    # matching one of the supported built-in platforms.
    add_subdirectory(platform/${PLATFORM})
  endif()

  # Add global compile options for current platform
  add_compile_options(${PLATFORM_COMPILE_OPTIONS})
  # Add global compile definitions for current platform
  add_compile_definitions(${PLATFORM_DEFINITIONS})
else()
  foreach(PLATFORM ${PLATFORMS})
    add_subdirectory(platform/${PLATFORM})
  endforeach()
endif()

# ZWave/API is used internally as ZWAVE_API_DIR, but ZWAVE_INCLUDE_DIR is also kept, because it's used in libs2.
SET(ZWAVE_INCLUDE_DIR ${ZWAVE_API_DIR} ) # Z-Wave include dir is needed for lib s2.
# Additional include path for libs2
if(NOT CMAKE_BUILD_TYPE STREQUAL Test)
  SET(OPTIONAL_SYSTEM_INCLUDE ${COMPONENTS_DIR}/Assert ${ZPAL_API_DIR})
else()
  SET(OPTIONAL_SYSTEM_INCLUDE ${ZPAL_API_DIR})
endif()
add_subdirectory(${SUBTREE_LIBS2})

add_subdirectory(AppsHw)
add_subdirectory(Components)
add_subdirectory(PAL)
add_subdirectory(ZAF)
add_subdirectory(ZWave)

###################################################################################################
# Setting up what's needed for apps
###################################################################################################
# Default frequency is given with REGION_DEFAULT. This can be changed using command line argument
option (CUSTOM_FREQ "CUSTOM_FREQ" 0)


IF(CUSTOM_FREQ)
  SET(FREQ_LIST ${CUSTOM_FREQ} CACHE INTERNAL FREQ_LIST)
ELSEIF (DEFINED ENV{ZWAVE_USER_FREQ})
  # If the environment variable ZWAVE_USER_FREQ is defined we *only*
  # build for that frequency
  SET(FREQ_LIST $ENV{ZWAVE_USER_FREQ} CACHE INTERNAL FREQ_LIST)
ELSE()
  # Default regions built if no region is specified
  SET(FREQ_LIST REGION_EU REGION_US REGION_US_LR CACHE INTERNAL FREQ_LIST)
ENDIF()

IF(PLATFORM STREQUAL "x86")
  SET(FREQ_LIST REGION_DEFAULT CACHE INTERNAL FREQ_LIST)
ENDIF()

# Frequency type is not relevant to test, but ZW_REGION must be defined
IF( CMAKE_BUILD_TYPE STREQUAL Test )
  add_compile_definitions(ZW_REGION=REGION_DEFAULT)
ENDIF()

if(NOT DEFINED ZW_CONFIG_MANUFACTURER_ID)
  set(ZW_CONFIG_MANUFACTURER_ID "0x031D") # Z-Wave Alliance Manufacturer ID
endif()

include(${CMAKE_CURRENT_SOURCE_DIR}/DevTools/CMakeFunctions/app_executable.cmake)

if(DEFINED ZW_SKIP_APPS AND ZW_SKIP_APPS EQUAL 1)
  # Do not include example applications.
else()
  add_subdirectory(Apps)
  IF(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/TestApps)
    add_subdirectory(TestApps)
  ENDIF()
endif()

##
# @}
#
