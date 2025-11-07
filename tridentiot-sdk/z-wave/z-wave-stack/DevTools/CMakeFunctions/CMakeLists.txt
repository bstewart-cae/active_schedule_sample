# SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
# SPDX-FileCopyrightText: 2025 Trident IoT, LLC <https://www.tridentiot.com>
#
# SPDX-License-Identifier: BSD-3-Clause

# This section contains common functions for CMakeLists.txt files.

##
# \skipline function\(add_executable\)
# \skipline function\(_add_executable\)
# \skipline function\(safeguard_add_executable\)

##
# @defgroup custom_func Custom CMake Functions

##
# @addtogroup custom_func
#
#
# @{
#
# @details Custom CMake Function description for build system
#

##
# @b Syntax
#
# &emsp; @c add_test_subdirectory( \b IN_SUBDIRECTORY )
#
# Function for adding a folder to the build process when build type is: @b Test
#
# Using this function ensures that the sub-folder defined by
# \b IN_SUBDIRECTORY will only be included if the CMake build type @b Test is used, see
# @ref build_types.
#
# This function shall be used to included unit test sub-directories.
#
# @param[in] IN_SUBDIRECTORY  name of \b Test sub directory
#
FUNCTION(add_test_subdirectory IN_SUBDIRECTORY)
  IF( CMAKE_BUILD_TYPE STREQUAL Test )
	  add_subdirectory(${IN_SUBDIRECTORY})
  ENDIF( CMAKE_BUILD_TYPE STREQUAL Test )
ENDFUNCTION(add_test_subdirectory IN_SUBDIRECTORY)

##
# @b Syntax
#
# &emsp; @c  to_hex(\b IN_NAME, \b OUT_NAME )
#
# Function for converting an .elf file to hex format.
#
# @param[in] IN_NAME  name of elf file to be converted
# @param[in] OUT_NAME name of hex file which is being written
#
function(to_hex IN_NAME OUT_NAME)
  add_custom_command (TARGET ${IN_NAME} POST_BUILD COMMAND ${CMAKE_OBJCOPY} ARGS -O ihex $<TARGET_FILE:${IN_NAME}> ${OUT_NAME} COMMENT "Converting to Intel hex format.")
endfunction(to_hex IN_NAME OUT_NAME)
