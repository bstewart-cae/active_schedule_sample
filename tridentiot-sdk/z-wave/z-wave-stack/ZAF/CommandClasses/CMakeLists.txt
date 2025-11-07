# SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
# SPDX-FileCopyrightText: 2025 Trident IoT, LLC <https://www.tridentiot.com>
# SPDX-License-Identifier: BSD-3-Clause

#
# Sets config properties for a given target.
#
# Notice: This function must be invoked where the target is created as it
# relies on CMAKE_CURRENT_SOURCE_DIR.
#
# @param[in] TARGET           Target to set the properties for.
# @param[in] CONFIG_KEY       Key used in the configuration input file (.yaml).
# @param[in] CONFIG_TEMPLATES Names of templates in templates/ directory.
#
function(zw_add_config_properties)
  set(OPTIONS "")
  set(SINGLE_VALUE_ARGS TARGET CONFIG_KEY)
  set(MULTI_VALUE_ARGS CONFIG_TEMPLATES)
  cmake_parse_arguments("" "${OPTIONS}" "${SINGLE_VALUE_ARGS}" "${MULTI_VALUE_ARGS}" ${ARGN} )

  if(NOT DEFINED _CONFIG_KEY)
    message(FATAL_ERROR "CONFIG_KEY is not set for zw_add_template_properties()!")
  endif()

  if(NOT DEFINED _CONFIG_TEMPLATES)
    message(FATAL_ERROR "CONFIG_TEMPLATES is not set for zw_add_template_properties()!")
  endif()

  # Save the template path for later use by an application target, if it exists.
  set(TEMPLATE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/templates/")
  if(EXISTS ${TEMPLATE_DIR})
    set_target_properties(${_TARGET} PROPERTIES CONFIG_TEMPLATE_DIR "${TEMPLATE_DIR}")
  endif()

  # Save the configuration key used in the .yaml file.
  if(${_CONFIG_KEY} STREQUAL "")
    message(FATAL_ERROR "Empty CONFIG_KEY given!")
  else()
    set_target_properties(${_TARGET} PROPERTIES CONFIG_KEY "${_CONFIG_KEY}")
  endif()

  # Save expected generated filenames in a property if the corresponding template exists.
  foreach(TEMPLATE ${_CONFIG_TEMPLATES})
    set(TEMPLATE_PATH ${TEMPLATE_DIR}/${TEMPLATE})
    if(EXISTS ${TEMPLATE_PATH})
      set_property(TARGET ${_TARGET} APPEND PROPERTY CONFIG_TEMPLATES ${TEMPLATE_PATH})
      string(REPLACE ".jinja" "" EXPECTED_GENERATED_FILE ${TEMPLATE})
      set_property(TARGET ${_TARGET} APPEND PROPERTY CONFIG_EXPECTED_GENERATED_FILES ${EXPECTED_GENERATED_FILE})
    else()
      message(FATAL_ERROR "Template not found: ${TEMPLATE_PATH}")
    endif()
  endforeach()
endfunction()

##
# Creates a new INTERFACE library for Command Class.
# Source files will be compiled with target that this
# library is linked to (e.g. end application).
#
# @param[in] NAME             Target name.
# @param[in] INCLUDES         List of include paths.
# @param[in] SOURCES          List of source files.
# @param[in] DEPENDS          List of Command Class dependencies.
# @param[in] CONFIG_KEY       Key used in the configuration input file (.yaml).
# @param[in] CONFIG_TEMPLATES Names of templates in templates/ directory.
#
function(zw_add_interface_cc)
  set(OPTIONS "")
  set(SINGLE_VALUE_ARGS NAME CONFIG_KEY)
  set(MULTI_VALUE_ARGS INCLUDES SOURCES DEPENDS CONFIG_TEMPLATES)
  cmake_parse_arguments(ADD_CC "${OPTIONS}" "${SINGLE_VALUE_ARGS}" "${MULTI_VALUE_ARGS}" ${ARGN} )

  message(DEBUG "Adding interface library for ${ADD_CC_NAME}")

  add_library(${ADD_CC_NAME} INTERFACE)
  target_include_directories(${ADD_CC_NAME}
    INTERFACE
      ${ADD_CC_INCLUDES}
  )
  target_sources(${ADD_CC_NAME}
    INTERFACE
      ${ADD_CC_SOURCES}
  )
  foreach(LIB ${ADD_CC_DEPENDS})
    target_include_directories(${ADD_CC_NAME}
      INTERFACE
        $<TARGET_PROPERTY:${LIB},INTERFACE_INCLUDE_DIRECTORIES>
    )
  endforeach()

  if(DEFINED ADD_CC_CONFIG_KEY AND DEFINED ADD_CC_CONFIG_TEMPLATES)
    zw_add_config_properties(
      TARGET ${ADD_CC_NAME}
      CONFIG_KEY ${ADD_CC_CONFIG_KEY}
      CONFIG_TEMPLATES ${ADD_CC_CONFIG_TEMPLATES}
    )
  endif()
endfunction(zw_add_interface_cc)

##
# Creates a new OBJECT library for Command Class.
# Source files will be compiled to object files.
#
# @param[in] NAME             Target name.
# @param[in] INCLUDES         List of include paths.
# @param[in] SOURCES          List of source files.
# @param[in] DEPENDS          List of Command Class dependencies.
# @param[in] CONFIG_KEY       Key used in the configuration input file (.yaml).
# @param[in] CONFIG_TEMPLATES Names of templates in the templates/ directory.
#
function(zw_add_object_cc)
  set(OPTIONS "")
  set(SINGLE_VALUE_ARGS NAME CONFIG_KEY)
  set(MULTI_VALUE_ARGS INCLUDES SOURCES DEPENDS CONFIG_TEMPLATES)
  cmake_parse_arguments(ADD_CC "${OPTIONS}" "${SINGLE_VALUE_ARGS}" "${MULTI_VALUE_ARGS}" ${ARGN} )

  message(DEBUG "Adding object library for ${ADD_CC_NAME}")

  add_library(${ADD_CC_NAME} OBJECT EXCLUDE_FROM_ALL)
  target_include_directories(${ADD_CC_NAME}
    PUBLIC
      ${ADD_CC_INCLUDES}
      ${ZAF_CONFIGDIR}/inc
      ${ZAF_UTILDIR}
      ${ZAF_UTILDIR}/_commonIF
  )
  target_sources(${ADD_CC_NAME}
    PRIVATE
      ${ADD_CC_SOURCES}
  )
  target_link_libraries(${ADD_CC_NAME}
    PUBLIC
      DebugPrint
      Utils
      CC_Common
      ZWaveAPI
    PRIVATE
      zaf_transport_layer_interface
  )
  foreach(LIB ${ADD_CC_DEPENDS})
    target_include_directories(${ADD_CC_NAME}
      PUBLIC
        $<TARGET_PROPERTY:${LIB},INTERFACE_INCLUDE_DIRECTORIES>
    )
  endforeach()

  if(DEFINED ADD_CC_CONFIG_KEY AND DEFINED ADD_CC_CONFIG_TEMPLATES)
    zw_add_config_properties(
      TARGET ${ADD_CC_NAME}
      CONFIG_KEY ${ADD_CC_CONFIG_KEY}
      CONFIG_TEMPLATES ${ADD_CC_CONFIG_TEMPLATES}
    )
  endif()
endfunction(zw_add_object_cc)

# Loops through the folders in "." and automatically adds a subdirectory.
# Folders starting with "_" are related to test and is added using add_test_subdirectory().
file(GLOB dummy_src RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "${CMAKE_CURRENT_SOURCE_DIR}/*")
foreach(subdir ${dummy_src})
  string(REGEX MATCH "^_" output_dummy ${subdir})

  if (IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${subdir})
    if (NOT subdir MATCHES "CMakeFiles")
      if (NOT ${output_dummy} STREQUAL "")
        add_test_subdirectory(${subdir})
      else()
        add_subdirectory(${subdir})
      endif()
    endif()
  endif()
endforeach()
