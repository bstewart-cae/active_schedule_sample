# SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
#
# SPDX-License-Identifier: BSD-3-Clause

# This function strip out debug information from released libraries
function(lib_debug_strip LIB_IN_FILENAME)
  add_custom_command (TARGET ${LIB_IN_FILENAME} POST_BUILD
                      COMMAND ${CMAKE_STRIP} -g  --strip-unneeded -x -X ${CMAKE_CURRENT_BINARY_DIR}/lib${LIB_IN_FILENAME}.a
                      DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/lib${LIB_IN_FILENAME}.a
                      COMMENT "Stripping debug information"
  )
endfunction()