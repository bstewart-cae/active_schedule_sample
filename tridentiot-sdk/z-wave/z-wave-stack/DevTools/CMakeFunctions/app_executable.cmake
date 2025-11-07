# SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
# SPDX-FileCopyrightText: 2025 Trident IoT, LLC <https://www.tridentiot.com>
#
# SPDX-License-Identifier: BSD-3-Clause

function(zw_generate_config_files)
  set(OPTIONS "")
  set(SINGLE_VALUE_ARGS TARGET CONFIG_INPUT_FILE OUTPUT_DIR)
  set(MULTI_VALUE_ARGS LIBRARIES)

  # Prefix input variables with GCF (Generate Config Files).
  cmake_parse_arguments("GCF" "${OPTIONS}" "${SINGLE_VALUE_ARGS}" "${MULTI_VALUE_ARGS}" ${ARGN} )

  # Must be populated with all the generated source files.
  set(generated_config_files)

  foreach(CONFIG_LIB ${GCF_LIBRARIES})
    get_target_property(CONFIG_TEMPLATE_DIR ${CONFIG_LIB} CONFIG_TEMPLATE_DIR)
    if(NOT ${CONFIG_TEMPLATE_DIR} STREQUAL "CONFIG_TEMPLATE_DIR-NOTFOUND")
      # Config template directory property is set. Hence, it's a CC library.
      message(DEBUG "Processing config for ${CONFIG_LIB}")

      get_target_property(CONFIG_KEY ${CONFIG_LIB} CONFIG_KEY)
      if(${CONFIG_KEY} STREQUAL "CONFIG_KEY-NOTFOUND")
        message(FATAL_ERROR "No config key set for ${CONFIG_LIB}!")
      endif()

      # Get the templates to pass to DEPENDS.
      get_target_property(CONFIG_TEMPLATES ${CONFIG_LIB} CONFIG_TEMPLATES)
      list(LENGTH CONFIG_TEMPLATES config_templates_length)
      # Check the length. If it's >1, the comparison will fail.
      if(${config_templates_length} EQUAL 1 AND CONFIG_TEMPLATES STREQUAL "CONFIG_TEMPLATES-NOTFOUND")
        message(FATAL_ERROR "No config templates set for ${CONFIG_LIB}!")
      endif()

      # Get the expected config source files to pass to OUTPUT.
      get_target_property(EXPECTED_CONFIG_SOURCE_FILES ${CONFIG_LIB} CONFIG_EXPECTED_GENERATED_FILES)
      list(LENGTH EXPECTED_CONFIG_SOURCE_FILES expected_config_source_files_length)
      # Check the length. If it's >1, the comparison will fail.
      if(${expected_config_source_files_length} EQUAL 1 AND EXPECTED_CONFIG_SOURCE_FILES STREQUAL "EXPECTED_CONFIG_SOURCE_FILES-NOTFOUND")
        message(FATAL_ERROR "No expected source files set for ${CONFIG_LIB}")
      endif()

      # Prepend the output directory.
      set(FILES_WITH_PATH "")
      foreach(FILE ${EXPECTED_CONFIG_SOURCE_FILES})
        set(FILE_WITH_PATH ${GCF_OUTPUT_DIR}/${FILE})
        list(APPEND FILES_WITH_PATH ${FILE_WITH_PATH})
        list(APPEND generated_config_files ${FILE_WITH_PATH})
      endforeach()

      # Create the custom command that generates the file(s) from the template(s).
      add_custom_command(
        OUTPUT ${FILES_WITH_PATH}
        COMMAND ${Python3_EXECUTABLE} ${DEVTOOLS_DIR}/cc_configurator/zw_generate_config_files.py --output-dir ${GCF_OUTPUT_DIR} --config-file ${GCF_CONFIG_INPUT_FILE} --template-dir ${CONFIG_TEMPLATE_DIR} --key ${CONFIG_KEY}
        DEPENDS ${DEVTOOLS_DIR}/cc_configurator/zw_generate_config_files.py ${GCF_CONFIG_INPUT_FILE} ${CONFIG_TEMPLATES}
        COMMENT "Generating ${EXPECTED_CONFIG_SOURCE_FILES} for ${CONFIG_LIB} (${GCF_CONFIG_INPUT_FILE})"
      )
    endif()
  endforeach()

  message(DEBUG "generated_config_files: ${generated_config_files}")

  # Save header files for later use in this function.
  set(generated_header_files ${generated_config_files})
  list(FILTER generated_header_files INCLUDE REGEX "\\.h$")
  message(DEBUG "Generated header files: ${generated_header_files}")

  # Use the generated source files, if any.
  list(FILTER generated_config_files INCLUDE REGEX "\\.c$")
  message(DEBUG "Generated source files: ${generated_config_files}")
  list(LENGTH generated_config_files generated_config_files_length)
  if(${generated_config_files_length} GREATER 0)
    target_sources(${GCF_TARGET} PRIVATE ${generated_config_files})
  endif()

  # The target must depend on generated (expected) header files to ensure they are generated.
  list(LENGTH generated_header_files generated_header_files_length)
  if(${generated_header_files_length} GREATER 0)
    add_custom_target(${GCF_TARGET}_generated_headers DEPENDS ${generated_header_files})
    add_dependencies(${GCF_TARGET} ${GCF_TARGET}_generated_headers)

    # If any header files was generated, make sure to add the output directory
    # as an include directory.
    target_include_directories(${GCF_TARGET} PUBLIC ${GCF_OUTPUT_DIR})
  endif()
endfunction()

##
# @b Syntax
#
# &emsp; @c  zw_create_app()
#
# Generates a number of Z-Wave application targets based on region list.
# The output will be an .elf file and a .map file for each combination.
#
# @param[in] NAME            Application name (e.g. zwave_soc_switch_on_off) (Required)
# @param[in] SOURCES         List of source files that will be compiled once. (Optional)
# @param[in] SOURCES_REGION  List of source files that will be compiled for every region. (Required)
# @param[in] INCLUDES        List of include paths. (Optional)
# @param[in] LIBS            List of libraries to be linked with the application target. (Optional)
# @param[in] DEFINITIONS     List of definitions. (Optional)
# @param[in] REGIONS         List of regions. (Required)
#
if(NOT COMMAND zw_create_app)
function(zw_create_app)
  set(oneValueArgs NAME)
  set(multiValueArgs SOURCES SOURCES_REGION INCLUDES LIBS DEFINITIONS REGIONS)
  cmake_parse_arguments(IN "" "${oneValueArgs}" "${multiValueArgs}" ${ARGN} )

  if(NOT DEFINED IN_NAME)
    message(FATAL_ERROR "Application name is not passed to ${CMAKE_CURRENT_FUNCTION}")
  endif()

  # Skip Apps unsupported by current platform
  IF(NOT IN_NAME IN_LIST PLATFORM_SUPPORTED_APPS)
    return()
  ENDIF()
  message(NOTICE "Creating ${IN_NAME}")

  if(NOT DEFINED IN_SOURCES)
    message(STATUS "No non-region sources passed to ${CMAKE_CURRENT_FUNCTION}")
  endif()

  if(NOT DEFINED IN_SOURCES_REGION)
    message(FATAL_ERROR "No region sources passed to ${CMAKE_CURRENT_FUNCTION}")
  endif()

  if(NOT DEFINED IN_INCLUDES)
    message(STATUS "No include directories passed to ${CMAKE_CURRENT_FUNCTION}")
  endif()

  if(NOT DEFINED IN_LIBS)
    message(STATUS "No libraries passed to ${CMAKE_CURRENT_FUNCTION}")
  endif()

  if(NOT DEFINED IN_DEFINITIONS)
    message(STATUS "No definitions passed to ${CMAKE_CURRENT_FUNCTION}")
  endif()

  if(NOT DEFINED IN_REGIONS)
    message(FATAL_ERROR "No regions passed to ${CMAKE_CURRENT_FUNCTION}")
  endif()

  # TODO BUILD: Make target output name include version, but remove version from target name.
  # set_target_properties(<target> PROPERTIES OUTPUT_NAME <something_else>)
  # TODO BUILD: Make custom targets for convenience, e.g. "make apps_zgm13_EU"
  # add_custom_target(apps_zgm13_eu DEPENDS <app targets>)

  set(BASE_NAME ZW_${IN_NAME}_${SDK_VERSION} )

  IF($ENV{BUILD_NUMBER})
    set(BASE_NAME ${BASE_NAME}_${ZW_BUILD_NO} )
  ENDIF()

  list(APPEND IN_SOURCES_REGION ${ZAF_PROTOCOL_CONFIG}/src/zaf_protocol_config.c)

  set(APP_LIB_TARGET_NAME lib_${IN_NAME})

  add_library(${APP_LIB_TARGET_NAME} OBJECT
    ${IN_SOURCES}
  )

  target_compile_options(${APP_LIB_TARGET_NAME}
    PRIVATE
      -Wpedantic
      -Wconversion
      -Wextra
  )

  target_compile_definitions(${APP_LIB_TARGET_NAME}
    PRIVATE
      ${IN_DEFINITIONS}
  )

  target_include_directories(${APP_LIB_TARGET_NAME}
    PUBLIC
      ${CONFIG_DIR}
    PRIVATE
      ${CMAKE_CURRENT_SOURCE_DIR}
      ${IN_INCLUDES}
  )

  target_include_directories(${APP_LIB_TARGET_NAME} SYSTEM
    PUBLIC
      $<TARGET_PROPERTY:zpal_${PLATFORM_VARIANT},INTERFACE_SYSTEM_INCLUDE_DIRECTORIES>
  )


  if (NOT IN_NAME STREQUAL "zwave_ncp_serial_api_controller" AND NOT IN_NAME STREQUAL "zwave_ncp_serial_api_end_device")
    target_link_libraries(${APP_LIB_TARGET_NAME}
      PRIVATE
        # ZAF libraries created as interfaces that uses defines that can be
        # changed in the application level
        zaf_transport_layer
    )
  endif()

  # Find *.yaml files in the current directory (application directory).
  FILE(GLOB CC_CONFIG_FILES "*.yaml")
  list(LENGTH CC_CONFIG_FILES list_length)
  if(list_length GREATER 1)
    message(FATAL_ERROR "More than one *.yaml file found. Aborting.")
  elseif(list_length GREATER 0)
    message(DEBUG "Application config file: ${CC_CONFIG_FILES}")
  endif()

  foreach(LIB ${IN_LIBS})
    target_include_directories(${APP_LIB_TARGET_NAME}
      PRIVATE
        $<TARGET_PROPERTY:${LIB},INTERFACE_INCLUDE_DIRECTORIES>
    )
    target_compile_definitions(${APP_LIB_TARGET_NAME}
      PRIVATE
        $<TARGET_PROPERTY:${LIB},INTERFACE_COMPILE_DEFINITIONS>
    )

    if(${LIB} STREQUAL "ZAF")
      get_target_property(ZAF_INTERFACE_LINKED_LIBS ${LIB} INTERFACE_LINK_LIBRARIES)
    endif()
  endforeach()

  # Configuration libraries
  set(CONFIG_LIBS ${IN_LIBS})
  list(APPEND CONFIG_LIBS ${ZAF_INTERFACE_LINKED_LIBS})
  set(OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/autogen")

  zw_generate_config_files(
    TARGET ${APP_LIB_TARGET_NAME}
    CONFIG_INPUT_FILE ${CC_CONFIG_FILES}
    LIBRARIES ${CONFIG_LIBS}
    OUTPUT_DIR ${OUTPUT_DIR}
  )

  foreach(CURRENT_FREQ ${IN_REGIONS})
    # Add frequency suffix for non-x86 builds
    if(PLATFORM STREQUAL "x86")
      set(IMAGE_NAME ${BASE_NAME}_${PLATFORM_VARIANT})
    else()
      set(IMAGE_NAME ${BASE_NAME}_${PLATFORM_VARIANT}_${CURRENT_FREQ})
    endif()

    # For debug builds add a suffix so that they are easy recognisable
    if(CMAKE_BUILD_TYPE MATCHES Debug)
      set(GBL_NAME ${GBL_NAME}_DEBUG)
      set(IMAGE_NAME ${IMAGE_NAME}_DEBUG)
    endif()

    if(IN_NAME MATCHES "_20dBm$")
      set(CONFIG_DIR_ZW_RF  ${CONFIG_DIR}/zw_rf/20dbm)
    else()
      set(CONFIG_DIR_ZW_RF  ${CONFIG_DIR}/zw_rf)
    endif()

    add_executable(${IMAGE_NAME}.elf "${IN_SOURCES_REGION}")
    install(TARGETS ${IMAGE_NAME}.elf RUNTIME DESTINATION bin)
    set(CPACK_PACKAGE_VENDOR "Z-Wave-Alliance")
    set(CPACK_PACKAGE_HOMEPAGE_URL "https://z-wavealliance.org/")
    set(CPACK_PACKAGE_VERSION_MAJOR "0")
    set(CPACK_PACKAGE_VERSION_MINOR "0")
    set(CPACK_PACKAGE_VERSION_PATCH "0")
    SET(CPACK_INCLUDE_TOPLEVEL_DIRECTORY OFF)
    install(FILES
      "${CMAKE_SOURCE_DIR}/LICENSES/BSD-3-Clause.txt"
      TYPE DOC
    )

    include(CPack)

    target_include_directories(${IMAGE_NAME}.elf
      PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}"
        "${CONFIG_DIR_ZW_RF}"
    )

    target_compile_options(${IMAGE_NAME}.elf
      PRIVATE
        -Wpedantic
        -Wconversion
        -Wextra
    )

    # Generate HEX file for non-x86 builds
    if(NOT PLATFORM STREQUAL "x86")
      to_hex(${IMAGE_NAME}.elf ${IMAGE_NAME}.hex)
    endif()

    target_compile_definitions(${IMAGE_NAME}.elf
      PRIVATE
        ${IN_DEFINITIONS}
        $<TARGET_PROPERTY:zpal_${PLATFORM_VARIANT},INTERFACE_COMPILE_DEFINITIONS>
        ZW_REGION=${CURRENT_FREQ}
    )

    target_include_directories(${IMAGE_NAME}.elf SYSTEM
      PRIVATE
        $<TARGET_PROPERTY:zpal_${PLATFORM_VARIANT},INTERFACE_SYSTEM_INCLUDE_DIRECTORIES>
    )

    target_include_directories(${IMAGE_NAME}.elf
      PRIVATE
        ${IN_INCLUDES}
        $<TARGET_PROPERTY:zpal_${PLATFORM_VARIANT},INTERFACE_INCLUDE_DIRECTORIES>
    )

    target_link_libraries(${IMAGE_NAME}.elf
      PRIVATE
        ${APP_LIB_TARGET_NAME}
        ${IN_LIBS}
        AppsHw_platform
        zpal_${PLATFORM_VARIANT}

    )

    if(PLATFORM_LINKER_SCRIPT)
      set_target_properties(${IMAGE_NAME}.elf PROPERTIES
        LINK_DEPENDS ${PLATFORM_LINKER_SCRIPT}
        LINK_FLAGS "-T ${PLATFORM_LINKER_SCRIPT} -Xlinker -Map=${IMAGE_NAME}.map"
      )
    else()
      set_target_properties(${IMAGE_NAME}.elf PROPERTIES
        LINK_FLAGS "-Xlinker -Map=${IMAGE_NAME}.map"
      )
    endif()

    message("${IMAGE_NAME}.elf was created")

    if (COMMAND zw_app_callback)
      # zw_app_callback MUST NOT be defined within the z-wave-stack CMake
      # project, but can be defined by a parent CMake project if desired.
      # This can be useful for generating SDK vendor specific files for each
      # application, for instance a firmware update image.
      zw_app_callback(${IMAGE_NAME}.elf)
    endif()
  endforeach(CURRENT_FREQ ${IN_REGIONS})
endfunction()
endif(NOT COMMAND zw_create_app)
