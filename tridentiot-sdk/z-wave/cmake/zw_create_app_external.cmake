# SPDX-FileCopyrightText: 2025 Trident IoT, LLC <https://www.tridentiot.com>
# SPDX-License-Identifier: LicenseRef-TridentMSLA

##
# @brief Generates hardware libraries
#
# Discovers source files found in directories matching the following pattern:
#
# "src_hw_<identifier>_<platform variant>":
# Must contain target specific source files and header files. A target is
# created for each folder matching this pattern and the containing source files
# and header files will be used for the matching target.
# The <identifier> offers a way to identify the target and the compiled binary.
# The <platform variant> must match a supported platform variant, e.g. T32CZ20.
#
# Notice: The CMake team recommends not to use GLOB to collect a list of source
# files because CMake doesn't automatically discover newly created files:
# https://cmake.org/cmake/help/latest/command/file.html#glob
#
# This function is deliberately using GLOB for the purpose of convenience.
# Hence, one would need to run CMake manually to discover new source files.
# Example run in the root directory:
# $ cmake --preset T32CZ20.Debug
#
# @param[in] NAME         Name of the application to generate hardware libraries for. The name MUST
#                         match the name passed to zw_create_app().
# @param[in] SOURCES      List of additional source files to build for each generated hardware library.
# @param[in] INCLUDES     List of include directories to apply to each generated hardware library.
# @param[in] LIBS         List of libraries to link to each generated hardware library.
# @param[in] DEFINITIONS  List of definitions to set for each generated hardware library.
#
function(tr_zw_generate_hardware_libraries)
  message(DEBUG "Invoke tr_zw_generate_hardware_libraries()")

  #set(options OPTIONAL FAST) # No options required, but keep it for now.
  set(oneValueArgs NAME)
  set(multiValueArgs SOURCES INCLUDES LIBS DEFINITIONS)
  cmake_parse_arguments(HW "" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  if(NOT (CMAKE_BUILD_TYPE MATCHES Release OR CMAKE_BUILD_TYPE MATCHES Debug))
    return()
  endif()

  set(APP_NAME "${HW_NAME}")

  string(TOUPPER ${APP_NAME} APP_NAME_UPPERCASE)
  string(CONCAT NEW_VARIABLE_NAME "ZW_" ${APP_NAME_UPPERCASE} "_HARDWARE_LIBRARIES")

  # Find hardware directories and create one or more libraries based on the result.
  message(DEBUG "Finding hardware source dirs matching ${CMAKE_CURRENT_SOURCE_DIR}/src_hw_*_*")
  file(GLOB hw_folders RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} src_hw_*_*)
  list(LENGTH hw_folders hw_folders_count)
  message(DEBUG "Found ${hw_folders_count} dirs")
  if (${hw_folders_count} GREATER 0)
    # Found at least one hardware specific source code directory.
    foreach(hw_folder_raw ${hw_folders})
      if (NOT IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${hw_folder_raw})
        continue()
      endif()

      string(REPLACE "src_hw_" "" hw_folder ${hw_folder_raw})
      string(REPLACE "_" ";" dir_parts ${hw_folder})
      message(DEBUG "Dir parts: ${dir_parts}")
      list(GET dir_parts -1 found_platform)

      if(NOT ${found_platform} STREQUAL ${PLATFORM_VARIANT})
        message(DEBUG "Ignoring ${found_platform} because it doesn't match current platform: ${PLATFORM_VARIANT}")
        continue()
      endif()

      file(GLOB_RECURSE src_hw_files ${CMAKE_CURRENT_SOURCE_DIR}/${hw_folder_raw}/*.c)
      message(DEBUG "Found sources in ${CMAKE_CURRENT_SOURCE_DIR}/${hw_folder_raw}/: ${src_hw_files}")

      set(target_name "${APP_NAME}_${hw_folder}")

      # Create the target corresponding to the hardware directory.
      message(DEBUG "Creating hardware library.")
      add_library(${target_name} OBJECT
        ${HW_SOURCES}
        ${src_hw_files}
      )

      list(APPEND LIST_OF_LIBRARIES ${target_name})

      target_include_directories(${target_name}
        PUBLIC
          ${CMAKE_CURRENT_SOURCE_DIR}/${hw_folder_raw}/
          $<TARGET_PROPERTY:zpal_${PLATFORM_VARIANT},INTERFACE_INCLUDE_DIRECTORIES>
          $<TARGET_PROPERTY:AppsHw_platform,INTERFACE_INCLUDE_DIRECTORIES>
          ${ZW_SDK_ROOT}/platform/boards
      )
      foreach(INCLUDE ${HW_INCLUDES})
        target_include_directories(${target_name} PUBLIC ${INCLUDE})
      endforeach(INCLUDE ${HW_INCLUDES})

      # Add the application directory as include directory per default, but only
      # if the SDK is the root project. Otherwise, rely on CMakeLists.txt.jinja
      # to set the right include directories.
      if(${ZWSDK_IS_ROOT_PROJECT} STREQUAL "true")
        if (${HW_NAME} STREQUAL "serial_api")
          set(PREFIX "zwave_ncp")
        else()
          set(PREFIX "zwave_soc")
        endif()
        set(DIR_NAME "${PREFIX}_${HW_NAME}")
        set(ABSOLUTE_DIR_NAME ${ZW_SDK_ROOT}/z-wave-stack/Apps/${DIR_NAME}/)
        message(DEBUG "Include the application directory: ${ABSOLUTE_DIR_NAME}")
        if(EXISTS ${ABSOLUTE_DIR_NAME})
          target_include_directories(${target_name} PUBLIC ${ABSOLUTE_DIR_NAME})
        endif()
      endif()

      target_compile_definitions(${target_name}
        PUBLIC
          ${HW_DEFINITIONS}
          -DTR_PLATFORM_${PLATFORM_VARIANT}
        PRIVATE
          GIT_HASH_ID="${GIT_HASH}"
      )

      target_compile_options(${target_name}
        PRIVATE
          -Wall
          -Wpedantic
          -Wconversion
          -Wextra
          -Werror
      )

      foreach(LIB ${HW_LIBS})
        target_include_directories(${target_name}
          PRIVATE
            $<TARGET_PROPERTY:${LIB},INTERFACE_INCLUDE_DIRECTORIES>
        )
        target_compile_definitions(${target_name}
          PRIVATE
            $<TARGET_PROPERTY:${LIB},INTERFACE_COMPILE_DEFINITIONS>
        )
        target_sources(${target_name} PUBLIC $<TARGET_OBJECTS:${LIB}>)
      endforeach()
    endforeach()
  else()
    # Do nothing.
  endif()

  set(${NEW_VARIABLE_NAME} "${LIST_OF_LIBRARIES}" CACHE INTERNAL ${NEW_VARIABLE_NAME})
endfunction()

##
# @brief Creates Z-Wave application targets
#
# This function creates an application target for each combination of region and hardware library.
#
# For instance, if regions REGION_US_LR and REGION_EU_LR are passed to REGIONS and two hardware
# libraries were generated by tr_zw_generate_hardware_libraries(), it will result in four (4)
# application targets (2 x 2 = 4).
#
# @param[in] NAME           Application name (e.g. my_temperature_sensor). This name
#                           MUST match the name passed to tr_zw_generate_hardware_libraries().
# @param[in] SOURCES        List of source files.
# @param[in] SOURCES_REGION List of source files to be compiled for each region.
# @param[in] INCLUDES       List of include directories to apply to all application targets.
# @param[in] LIBS           List of libraries to link to all application targets.
# @param[in] DEFINITIONS    List of definitions to set for all application targets.
# @param[in] REGIONS        List of regions.
# @param[in] CONFIG_FILE    Path to a YAML config file.
#                           Using this parameter is preferred over letting CMake find the YAML file.
#                           The automatic YAML file search will be removed eventually.
#
function(zw_create_app)
  set(oneValueArgs NAME CONFIG_FILE)
  set(multiValueArgs SOURCES SOURCES_REGION INCLUDES LIBS DEFINITIONS REGIONS)
  cmake_parse_arguments(CREATE_APP "" "${oneValueArgs}" "${multiValueArgs}" ${ARGN} )

  string(REGEX REPLACE "^zwave_(soc|ncp)_" "" APP_NAME_STRIPPED ${CREATE_APP_NAME})
  string(REGEX MATCH "(controller|end_device)$" library_type "${APP_NAME_STRIPPED}")
  string(REGEX REPLACE "_(api_controller|api_end_device)$" "_api" APP_NAME_STRIPPED ${APP_NAME_STRIPPED})
  string(REGEX MATCH "serial_api" serialapi_app ${APP_NAME_STRIPPED})

  if (${APP_NAME_STRIPPED} STREQUAL "serial_api")
    message(STATUS "Creating ${APP_NAME_STRIPPED} as ${library_type}...")
  else()
    message(STATUS "Creating ${APP_NAME_STRIPPED}...")
  endif()

  string(TOUPPER ${APP_NAME_STRIPPED} APP_NAME_STRIPPED_UPPERCASE)

  string(CONCAT NEW_VARIABLE_NAME "ZW_" ${APP_NAME_STRIPPED_UPPERCASE} "_HARDWARE_LIBRARIES")

  if(DEFINED ${NEW_VARIABLE_NAME})
    message(DEBUG "${NEW_VARIABLE_NAME} is set and contains: ${${NEW_VARIABLE_NAME}}")
  else()
    message(DEBUG "NEW_VARIABLE_NAME is not defined.")
  endif()

  list(LENGTH ${NEW_VARIABLE_NAME} hw_folders_count)
  message(DEBUG "hw_folders_count: ${hw_folders_count}")
  if(${hw_folders_count} EQUAL 0)
    message(STATUS "No hardware support found for ${APP_NAME_STRIPPED} ${library_type}. Skip.")
    return()
  endif()

  # Add mandatory sources
  list(APPEND CREATE_APP_SOURCES_REGION ${ZAF_PROTOCOL_CONFIG}/src/zaf_protocol_config.c)

  #
  # Get custom version if set
  #
  set(VERSION_VARIABLE_NAME "${APP_NAME_STRIPPED_UPPERCASE}_VERSION")

  if(DEFINED ${VERSION_VARIABLE_NAME})
    # Use custom version if it exists.
    set(VERSION "${${VERSION_VARIABLE_NAME}}")
  else()
    # Otherwise use the CMake project version.
    set(VERSION "${CMAKE_PROJECT_VERSION}")
  endif()
  if(DEFINED GIT_HASH AND NOT ${GIT_HASH} STREQUAL "")
    string(SUBSTRING ${GIT_HASH} 0 8 GIT_HASH_SHORT)
  else()
    set(GIT_HASH "12345678")
  endif()

  #
  # Get custom target name pattern if set
  #
  # TODO: Should the modifiable name be the output name instead of the target name?
  set(TARGET_NAME_TEMPLATE_VARIABLE_NAME "${APP_NAME_STRIPPED_UPPERCASE}_TARGET_NAME_TEMPLATE")

  if(${APP_NAME_STRIPPED} STREQUAL "serial_api" OR ("${serialapi_app}" STREQUAL "serial_api"))
    set(TARGET_NAME_TEMPLATE "zw_{{name}}_{{library_type}}_{{version}}_{{commit}}_{{hardware}}_{{region}}_{{build_type}}.elf")
  elseif(DEFINED ${TARGET_NAME_TEMPLATE_VARIABLE_NAME})
    set(TARGET_NAME_TEMPLATE "${${TARGET_NAME_TEMPLATE_VARIABLE_NAME}}")
  else()
    set(TARGET_NAME_TEMPLATE "zw_{{name}}_{{version}}_{{commit}}_{{hardware}}_{{region}}_{{build_type}}.elf")
  endif()

  if(${CMAKE_BUILD_TYPE} STREQUAL "Release")
    # Remove git commit hash.
    string(REPLACE _{{commit}} "" TARGET_NAME_TEMPLATE ${TARGET_NAME_TEMPLATE})
    # Remove build type
    string(REPLACE _{{build_type}} "" TARGET_NAME_TEMPLATE ${TARGET_NAME_TEMPLATE})
  endif()

  set(TARGET_NAME "${TARGET_NAME_TEMPLATE}")

  string(REPLACE "{{name}}"         "${APP_NAME_STRIPPED}" TARGET_NAME ${TARGET_NAME})
  string(REPLACE "{{library_type}}" "${library_type}" TARGET_NAME ${TARGET_NAME})
  string(REPLACE "{{version}}"      "${VERSION}" TARGET_NAME ${TARGET_NAME})
  string(REPLACE "{{commit}}"       "${GIT_HASH_SHORT}" TARGET_NAME ${TARGET_NAME})

  string(TOLOWER ${CMAKE_BUILD_TYPE} CMAKE_BUILD_TYPE_LOWER)
  string(REPLACE "{{build_type}}"  "${CMAKE_BUILD_TYPE_LOWER}" TARGET_NAME ${TARGET_NAME})

  # Creating this library for different purposes:
  # 1. List source files that requires compilation only once.
  # 2. Generate config files.
  add_library(${CREATE_APP_NAME}_lib OBJECT ${ZW_SDK_ROOT}/platform/apps/src/firmware_properties.c)
  target_include_directories(${CREATE_APP_NAME}_lib
    PRIVATE
      ${ZAF_CONFIGDIR}/config/
      ${ZAF_CONFIGDIR}/inc/
  )
  target_compile_definitions(${CREATE_APP_NAME}_lib
    PUBLIC
      ${CREATE_APP_DEFINITIONS}
      APP_VERSION=${CMAKE_PROJECT_VERSION_MAJOR}
      APP_REVISION=${CMAKE_PROJECT_VERSION_MINOR}
      APP_PATCH=${CMAKE_PROJECT_VERSION_PATCH}
  )

  if(NOT DEFINED CREATE_APP_CONFIG_FILE)
    message(STATUS "No config file passed to ${CMAKE_CURRENT_FUNCTION} for ${CREATE_APP_NAME}. Using file(GLOB).")
    # Find *.yaml files in the current directory (application directory).
    FILE(GLOB CC_CONFIG_FILES "*.yaml")
    list(LENGTH CC_CONFIG_FILES list_length)
    if(list_length GREATER 1)
      message(FATAL_ERROR "More than one *.yaml file found. Aborting.")
    elseif(list_length GREATER 0)
      message(DEBUG "Application config file: ${CC_CONFIG_FILES}")
    endif()
  else()
    if(NOT EXISTS "${CREATE_APP_CONFIG_FILE}")
      message(FATAL_ERROR "Config file was not found: ${CREATE_APP_CONFIG_FILE}")
    endif()
    set(CC_CONFIG_FILES "${CREATE_APP_CONFIG_FILE}")
    message(STATUS "Using config file from ${CC_CONFIG_FILES} for ${CREATE_APP_NAME}")
  endif()

  foreach(LIB ${CREATE_APP_LIBS})
    target_include_directories(${CREATE_APP_NAME}_lib
      PRIVATE
        $<TARGET_PROPERTY:${LIB},INTERFACE_INCLUDE_DIRECTORIES>
    )
    target_compile_definitions(${CREATE_APP_NAME}_lib
      PRIVATE
        $<TARGET_PROPERTY:${LIB},INTERFACE_COMPILE_DEFINITIONS>
    )

    if(${LIB} STREQUAL "ZAF")
      get_target_property(ZAF_INTERFACE_LINKED_LIBS ${LIB} INTERFACE_LINK_LIBRARIES)
    endif()
  endforeach()

  # Configuration libraries
  set(CONFIG_LIBS ${CREATE_APP_LIBS})
  list(APPEND CONFIG_LIBS ${ZAF_INTERFACE_LINKED_LIBS})
  set(OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/autogen")

  zw_generate_config_files(
    TARGET ${CREATE_APP_NAME}_lib
    CONFIG_INPUT_FILE ${CC_CONFIG_FILES}
    LIBRARIES ${CONFIG_LIBS}
    OUTPUT_DIR ${OUTPUT_DIR}
  )

  #
  # Loop through hardware libraries and regions to generate all target combinations.
  #
  foreach(HW_LIBRARY ${${NEW_VARIABLE_NAME}})
    string(REPLACE "${APP_NAME_STRIPPED}_" "" HW_LIBRARY_NAME ${HW_LIBRARY})
    string(TOLOWER ${HW_LIBRARY_NAME} HW_LIBRARY_NAME_LOWER)

    string(REPLACE "{{hardware}}"  "${HW_LIBRARY_NAME_LOWER}" TARGET_NAME_HARDWARE ${TARGET_NAME})

    foreach(REGION ${CREATE_APP_REGIONS})
      string(REPLACE "REGION_" "" REGION_STRIPPED ${REGION})
      string(TOLOWER ${REGION_STRIPPED} REGION_STRIPPED_LOWER)

      string(REPLACE "{{region}}"  "${REGION_STRIPPED_LOWER}" TARGET_NAME_HARDWARE_REGION ${TARGET_NAME_HARDWARE})

      message(STATUS "Creating target: ${TARGET_NAME_HARDWARE_REGION}")
      add_executable(${TARGET_NAME_HARDWARE_REGION} ${CREATE_APP_SOURCES} ${CREATE_APP_SOURCES_REGION} )
      target_link_libraries(${TARGET_NAME_HARDWARE_REGION}
        PRIVATE
          ${HW_LIBRARY}
          ${CREATE_APP_LIBS}
          zpal_${PLATFORM_VARIANT}
          ${CREATE_APP_NAME}_lib
        PUBLIC
          # link the z-wave protocol patch library.
          # This patch library repalce the s2 span/mpan storage function with our own
          zwave_patch_${PLATFORM_VARIANT}
      )

      target_include_directories(${TARGET_NAME_HARDWARE_REGION}
        PRIVATE
          ${CONFIG_DIR}/
          ${CONFIG_DIR}/zw_rf/
          $<TARGET_PROPERTY:zpal_${PLATFORM_VARIANT},INTERFACE_INCLUDE_DIRECTORIES>
          ${CREATE_APP_INCLUDES}
      )
      target_compile_definitions(${TARGET_NAME_HARDWARE_REGION}
        PRIVATE
          $<TARGET_PROPERTY:zpal_${PLATFORM_VARIANT},INTERFACE_COMPILE_DEFINITIONS>
          ZW_REGION=${REGION}
      )

      # Setting the keys for Serial API is a hack because the PC Controller
      # currently does not. This must be removed once the PC Controller
      # supports setting the keys.
      if(${APP_NAME_STRIPPED} STREQUAL "serial_api")
        if ((${library_type} STREQUAL "end_device"))
          target_compile_definitions(${TARGET_NAME_HARDWARE_REGION}
            PRIVATE
              ZAF_CONFIG_REQUEST_KEY_S0=1
              ZAF_CONFIG_REQUEST_KEY_S2_UNAUTHENTICATED=1
              ZAF_CONFIG_REQUEST_KEY_S2_AUTHENTICATED=1
          )
        endif()
      endif()

      if(NOT ${APP_NAME_STRIPPED} STREQUAL "serial_api")
        target_link_libraries(${TARGET_NAME_HARDWARE_REGION}
          PRIVATE
            # ZAF libraries created as interfaces that uses defines that can be
            # changed in the application level
            zaf_transport_layer
        )
      endif()
      string(REPLACE ".elf"  "" IMAGE_NAME_MINUS_EXT ${TARGET_NAME_HARDWARE_REGION})

      if(PLATFORM_LINKER_SCRIPT)
        set_target_properties(${TARGET_NAME_HARDWARE_REGION} PROPERTIES
          LINK_DEPENDS ${PLATFORM_LINKER_SCRIPT}
          LINK_FLAGS "-T ${PLATFORM_LINKER_SCRIPT} -Xlinker -Map=${IMAGE_NAME_MINUS_EXT}.map"
        )
      else()
        set_target_properties(${TARGET_NAME_HARDWARE_REGION} PROPERTIES
          LINK_FLAGS "-Xlinker -Map=${IMAGE_NAME_MINUS_EXT}.map"
        )
      endif()

      zw_app_callback(${TARGET_NAME_HARDWARE_REGION})

    endforeach(REGION ${CREATE_APP_REGIONS})
  endforeach(HW_LIBRARY )

  message(STATUS "Done creating target(s) for ${APP_NAME_STRIPPED}.")
endfunction()

##
# @brief Creates native application targets
#
# This function creates an application target for each hardware library.
#
# @param[in] NAME           Application name (e.g. my_temperature_sensor). This name
#                           MUST match the name passed to tr_zw_generate_hardware_libraries().
# @param[in] SOURCES        List of source files.
# @param[in] INCLUDES       List of include directories to apply to all application targets.
# @param[in] LIBRARIES      List of libraries to link to all application targets.
# @param[in] DEFINITIONS    List of definitions to set for all application targets.
#
function(zwsdk_create_native_app)
  set(oneValueArgs NAME)
  set(multiValueArgs SOURCES INCLUDES LIBRARIES DEFINITIONS)
  cmake_parse_arguments(CREATE_APP "" "${oneValueArgs}" "${multiValueArgs}" ${ARGN} )

  set(APP_NAME_STRIPPED "${CREATE_APP_NAME}")

  message(STATUS "Creating ${APP_NAME_STRIPPED}...")

  string(TOUPPER ${APP_NAME_STRIPPED} APP_NAME_STRIPPED_UPPERCASE)

  string(CONCAT NEW_VARIABLE_NAME "ZW_" ${APP_NAME_STRIPPED_UPPERCASE} "_HARDWARE_LIBRARIES")

  if(DEFINED ${NEW_VARIABLE_NAME})
    message(DEBUG "${NEW_VARIABLE_NAME} is set and contains: ${${NEW_VARIABLE_NAME}}")
  else()
    message(DEBUG "NEW_VARIABLE_NAME is not defined.")
  endif()

  list(LENGTH ${NEW_VARIABLE_NAME} hw_folders_count)
  message(DEBUG "hw_folders_count: ${hw_folders_count}")
  if(${hw_folders_count} EQUAL 0)
    message(STATUS "No hardware support found for ${APP_NAME_STRIPPED} ${library_type}. Skip.")
    return()
  endif()

  # Add mandatory sources
  list(APPEND CREATE_APP_SOURCES_COMMON ${ZAF_PROTOCOL_CONFIG}/src/zaf_protocol_config.c)

  #
  # Get custom version if set
  #
  set(VERSION_VARIABLE_NAME "${APP_NAME_STRIPPED_UPPERCASE}_VERSION")

  if(DEFINED ${VERSION_VARIABLE_NAME})
    # Use custom version if it exists.
    set(VERSION "${${VERSION_VARIABLE_NAME}}")
  else()
    # Otherwise use the CMake project version.
    set(VERSION "${CMAKE_PROJECT_VERSION}")
  endif()
  if(DEFINED GIT_HASH AND NOT ${GIT_HASH} STREQUAL "")
    string(SUBSTRING ${GIT_HASH} 0 8 GIT_HASH_SHORT)
  else()
    set(GIT_HASH "12345678")
  endif()

  #
  # Get custom target name pattern if set
  #
  # TODO: Should the modifiable name be the output name instead of the target name?
  set(TARGET_NAME_TEMPLATE_VARIABLE_NAME "${APP_NAME_STRIPPED_UPPERCASE}_TARGET_NAME_TEMPLATE")

  if(DEFINED ${TARGET_NAME_TEMPLATE_VARIABLE_NAME})
    set(TARGET_NAME_TEMPLATE "${${TARGET_NAME_TEMPLATE_VARIABLE_NAME}}")
  else()
    set(TARGET_NAME_TEMPLATE "zw_{{name}}_{{version}}_{{commit}}_{{hardware}}_{{build_type}}.elf")
  endif()

  if(${CMAKE_BUILD_TYPE} STREQUAL "Release")
    # Remove git commit hash.
    string(REPLACE _{{commit}} "" TARGET_NAME_TEMPLATE ${TARGET_NAME_TEMPLATE})
    # Remove build type
    string(REPLACE _{{build_type}} "" TARGET_NAME_TEMPLATE ${TARGET_NAME_TEMPLATE})
  endif()

  set(TARGET_NAME "${TARGET_NAME_TEMPLATE}")

  string(REPLACE "{{name}}"         "${APP_NAME_STRIPPED}" TARGET_NAME ${TARGET_NAME})
  string(REPLACE "{{library_type}}" "${library_type}" TARGET_NAME ${TARGET_NAME})
  string(REPLACE "{{version}}"      "${VERSION}" TARGET_NAME ${TARGET_NAME})
  string(REPLACE "{{commit}}"       "${GIT_HASH_SHORT}" TARGET_NAME ${TARGET_NAME})

  string(TOLOWER ${CMAKE_BUILD_TYPE} CMAKE_BUILD_TYPE_LOWER)
  string(REPLACE "{{build_type}}"  "${CMAKE_BUILD_TYPE_LOWER}" TARGET_NAME ${TARGET_NAME})

  # Creating this library for different purposes:
  # 1. List source files that requires compilation only once.
  # 2. Generate config files.
  add_library(${CREATE_APP_NAME}_lib OBJECT ${ZW_SDK_ROOT}/platform/apps/src/firmware_properties.c)
  target_include_directories(${CREATE_APP_NAME}_lib
    PRIVATE
      ${ZAF_CONFIGDIR}/config/
      ${ZAF_UTILDIR}/version/config/
      ${ZAF_CONFIGDIR}/inc/
      ${ZW_SDK_ROOT}/platform/TridentIoT/inc/
      ${ZW_SDK_ROOT}/platform/TridentIoT/PAL/src/
      ${TRISDK_PATH}/Library/CMSIS/Include/
      ${ZW_SDK_ROOT}/z-wave-stack/ZAF/configuration/inc/
      ${ZW_SDK_ROOT}/z-wave-stack/ZWave/API/
      $<TARGET_PROPERTY:zpal_${PLATFORM_VARIANT},INTERFACE_INCLUDE_DIRECTORIES>
  )
  target_compile_definitions(${CREATE_APP_NAME}_lib
    PUBLIC
      ${CREATE_APP_DEFINITIONS}
      APP_VERSION=${CMAKE_PROJECT_VERSION_MAJOR}
      APP_REVISION=${CMAKE_PROJECT_VERSION_MINOR}
      APP_PATCH=${CMAKE_PROJECT_VERSION_PATCH}
      TR_PLATFORM_${PLATFORM_VARIANT}
  )

  # Find *.yaml files in the current directory (application directory).
  FILE(GLOB CC_CONFIG_FILES "*.yaml")
  list(LENGTH CC_CONFIG_FILES list_length)
  if(list_length GREATER 1)
    message(FATAL_ERROR "More than one *.yaml file found. Aborting.")
  elseif(list_length GREATER 0)
    message(DEBUG "Application config file: ${CC_CONFIG_FILES}")
  endif()

  foreach(LIB ${CREATE_APP_LIBRARIES})
    target_include_directories(${CREATE_APP_NAME}_lib
      PRIVATE
        $<TARGET_PROPERTY:${LIB},INTERFACE_INCLUDE_DIRECTORIES>
    )
    target_compile_definitions(${CREATE_APP_NAME}_lib
      PRIVATE
        $<TARGET_PROPERTY:${LIB},INTERFACE_COMPILE_DEFINITIONS>
    )

    if(${LIB} STREQUAL "ZAF")
      get_target_property(ZAF_INTERFACE_LINKED_LIBS ${LIB} INTERFACE_LINK_LIBRARIES)
    endif()
  endforeach()

  # Configuration libraries
  set(CONFIG_LIBS ${CREATE_APP_LIBRARIES})
  list(APPEND CONFIG_LIBS ${ZAF_INTERFACE_LINKED_LIBS})
  set(OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/autogen")

  zw_generate_config_files(
    TARGET ${CREATE_APP_NAME}_lib
    CONFIG_INPUT_FILE ${CC_CONFIG_FILES}
    LIBRARIES ${CONFIG_LIBS}
    OUTPUT_DIR ${OUTPUT_DIR}
  )

  #
  # Loop through hardware libraries to generate all target combinations.
  #
  foreach(HW_LIBRARY ${${NEW_VARIABLE_NAME}})
    string(REPLACE "${APP_NAME_STRIPPED}_" "" HW_LIBRARY_NAME ${HW_LIBRARY})
    string(TOLOWER ${HW_LIBRARY_NAME} HW_LIBRARY_NAME_LOWER)

    string(REPLACE "{{hardware}}"  "${HW_LIBRARY_NAME_LOWER}" TARGET_NAME_HARDWARE ${TARGET_NAME})

    message(STATUS "Creating target: ${TARGET_NAME_HARDWARE}")

    add_executable(${TARGET_NAME_HARDWARE} ${CREATE_APP_SOURCES} ${CREATE_APP_SOURCES_COMMON} )

    target_link_libraries(${TARGET_NAME_HARDWARE}
      PRIVATE
        ${HW_LIBRARY}
        ${CREATE_APP_LIBRARIES}
        zpal_${PLATFORM_VARIANT}
        ${CREATE_APP_NAME}_lib
    )
    target_include_directories(${TARGET_NAME_HARDWARE}
      PRIVATE
        ${CONFIG_DIR}/
        ${CONFIG_DIR}/zw_rf/
        $<TARGET_PROPERTY:zpal_${PLATFORM_VARIANT},INTERFACE_INCLUDE_DIRECTORIES>
        ${CREATE_APP_INCLUDES}
    )
    target_compile_definitions(${TARGET_NAME_HARDWARE}
      PRIVATE
        $<TARGET_PROPERTY:zpal_${PLATFORM_VARIANT},INTERFACE_COMPILE_DEFINITIONS>
    )
    target_link_libraries(${TARGET_NAME_HARDWARE}
      PRIVATE
        # ZAF libraries created as interfaces that uses defines that can be
        # changed in the application level
        zaf_transport_layer
    )

    string(REPLACE ".elf"  "" IMAGE_NAME_MINUS_EXT ${TARGET_NAME_HARDWARE})

    if(PLATFORM_LINKER_SCRIPT)
      set_target_properties(${TARGET_NAME_HARDWARE} PROPERTIES
        LINK_DEPENDS ${PLATFORM_LINKER_SCRIPT}
        LINK_FLAGS "-T ${PLATFORM_LINKER_SCRIPT} -Xlinker -Map=${IMAGE_NAME_MINUS_EXT}.map"
      )
    else()
      set_target_properties(${TARGET_NAME_HARDWARE} PROPERTIES
        LINK_FLAGS "-Xlinker -Map=${IMAGE_NAME_MINUS_EXT}.map"
      )
    endif()

    zw_app_callback(${TARGET_NAME_HARDWARE})

  endforeach(HW_LIBRARY )

  message(STATUS "Done creating target(s) for ${APP_NAME_STRIPPED}.")
endfunction()
