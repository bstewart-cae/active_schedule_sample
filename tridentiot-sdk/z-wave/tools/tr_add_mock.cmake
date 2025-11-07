# SPDX-FileCopyrightText: 2025 Trident IoT, LLC <https://www.tridentiot.com>
# SPDX-License-Identifier: LicenseRef-TridentMSLA

find_package(Ruby)
# The RUBY_EXECUTABLE variable does not exist when the ADD_MOCK function is
# called So instead store the value in cache and use the cached value
set(CMOCK_RUBY_EXECUTABLE
    ${RUBY_EXECUTABLE}
    CACHE INTERNAL "")

set(CMOCK_DIR "${CMAKE_SOURCE_DIR}/z-wave-stack/ThirdParty/cmock")
set(CMOCK_CONFIG_PATH "${CMAKE_SOURCE_DIR}/z-wave-stack/DevTools/zwave_cmock_config.yml")

#
# ADD_MOCK()
#
function(add_mock TARGET HEADERS)
  #set(OPTIONS "")
  #set(SINGLE_VALUE_ARGS "TARGET")
  #set(MULTI_VALUE_ARGS "HEADERS")
  #cmake_parse_arguments(ADD_MOCK "${OPTIONS}" "${SINGLE_VALUE_ARGS}" "${MULTI_VALUE_ARGS}" ${ARGN} )

  set(ADD_MOCK_HEADERS ${HEADERS})

  #message(STATUS "ADD_MOCK_HEADERS: ${ADD_MOCK_HEADERS}")

  foreach(HEADER ${ADD_MOCK_HEADERS})
    #message(STATUS "HEADER: ${HEADER}")
    get_filename_component(BASE ${HEADER} NAME_WE)
    get_filename_component(DIR ${HEADER} DIRECTORY)

    if(IS_ABSOLUTE ${HEADER})
      list(APPEND MOCK_HEADER_FILES "${HEADER}")
      list(APPEND MOCK_HEADER_DIRS "${DIR}")
    else()
      list(APPEND MOCK_HEADER_FILES "${CMAKE_CURRENT_SOURCE_DIR}/${HEADER}")
      list(APPEND MOCK_HEADER_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/${DIR}")
    endif()

    list(APPEND MOCK_SOURCE_FILES
         "${CMAKE_CURRENT_BINARY_DIR}/mocks/${BASE}_mock.c")
  endforeach()

  #message(STATUS "MOCK_HEADER_FILES: ${MOCK_HEADER_FILES}")
  #message(STATUS "MOCK_SOURCE_FILES: ${MOCK_SOURCE_FILES}")

  add_custom_command(
    OUTPUT ${MOCK_SOURCE_FILES}
    DEPENDS ${MOCK_HEADER_FILES} ${CMOCK_CONFIG_PATH}
    COMMAND ${CMOCK_RUBY_EXECUTABLE} ${CMOCK_DIR}/lib/cmock.rb -o${CMOCK_CONFIG_PATH} ${MOCK_HEADER_FILES})

  add_library(${TARGET} STATIC ${MOCK_SOURCE_FILES})

  target_link_libraries(${TARGET} PUBLIC cmock)

  target_include_directories(${TARGET}
    PUBLIC
      ${CMAKE_CURRENT_BINARY_DIR}/mocks
      ${MOCK_HEADER_DIRS}
  )

  if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(${TARGET} PRIVATE "-fPIC")
  endif()
endfunction()
