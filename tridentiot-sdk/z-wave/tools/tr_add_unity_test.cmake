# SPDX-FileCopyrightText: 2025 Trident IoT, LLC <https://www.tridentiot.com>
# SPDX-License-Identifier: LicenseRef-TridentMSLA

#
# ADD_UNITY_TEST
#
message(STATUS "Before: Using add_unity_test() from ${CMAKE_CURRENT_SOURCE_DIR}")
if (NOT COMMAND TR_ADD_UNITY_TEST)
message(STATUS "After: Using add_unity_test() from ${CMAKE_CURRENT_SOURCE_DIR}")
function(TR_ADD_UNITY_TEST)
  set(OPTIONS USE_CPP DISABLED)
  set(SINGLE_VALUE_ARGS "NAME" "TEST_BASE")
  set(MULTI_VALUE_ARGS "FILES" "LIBRARIES" "INCLUDES")
  cmake_parse_arguments(ADD_UNITY_TEST "${OPTIONS}" "${SINGLE_VALUE_ARGS}" "${MULTI_VALUE_ARGS}" ${ARGN} )

  foreach(LIB ${ADD_UNITY_TEST_LIBRARIES})
    if (NOT TARGET ${LIB})
      message(FATAL_ERROR "Library ${LIB} not found :(")
    endif()
  endforeach()

  set(RUNNER_EXTENSION c)
  if (ADD_UNITY_TEST_USE_CPP)
    set(RUNNER_EXTENSION cpp)
  endif (ADD_UNITY_TEST_USE_CPP)

  set(RUNNER_BASE "${ADD_UNITY_TEST_NAME}.${RUNNER_EXTENSION}")
  if (NOT ${ADD_UNITY_TEST_TEST_BASE} STREQUAL "")
    set(RUNNER_BASE "${ADD_UNITY_TEST_TEST_BASE}")
  endif (NOT ${ADD_UNITY_TEST_TEST_BASE} STREQUAL "")

  if ("${ADD_UNITY_TEST_NAME}" STREQUAL "")
    list(GET ADD_UNITY_TEST_UNPARSED_ARGUMENTS 0 ADD_UNITY_TEST_NAME)
    list(REMOVE_AT ADD_UNITY_TEST_UNPARSED_ARGUMENTS 0})
    set(RUNNER_BASE "${ADD_UNITY_TEST_NAME}.${RUNNER_EXTENSION}")
    set(ADD_UNITY_TEST_FILES "${ADD_UNITY_TEST_UNPARSED_ARGUMENTS}")
  endif ("${ADD_UNITY_TEST_NAME}" STREQUAL "")

  add_custom_command(OUTPUT ${ADD_UNITY_TEST_NAME}_runner.${RUNNER_EXTENSION}
       COMMAND ${Python3_EXECUTABLE} ${TR_TEST_TOOLS_DIR}/generate_unit_test_runner.py -i ${RUNNER_BASE} -t ${TR_TEST_TOOLS_DIR}/unit_test_runner.c.jinja -o ${CMAKE_CURRENT_BINARY_DIR}
       DEPENDS ${RUNNER_BASE} ${TR_TEST_TOOLS_DIR}/generate_unit_test_runner.py ${TR_TEST_TOOLS_DIR}/unit_test_runner.c.jinja
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  )
  add_executable(${ADD_UNITY_TEST_NAME} ${ADD_UNITY_TEST_NAME}_runner.${RUNNER_EXTENSION} ${RUNNER_BASE} ${ADD_UNITY_TEST_FILES})
  target_link_libraries( ${ADD_UNITY_TEST_NAME} ${ADD_UNITY_TEST_LIBRARIES} unity)
  target_include_directories(${ADD_UNITY_TEST_NAME} PRIVATE . ${ADD_UNITY_TEST_INCLUDES})
  target_compile_options(${ADD_UNITY_TEST_NAME}
    PRIVATE
      -Wpedantic
  )

  add_test(${ADD_UNITY_TEST_NAME} ${ADD_UNITY_TEST_NAME} )

  if(${ADD_UNITY_TEST_DISABLED})
    set_tests_properties(${TEST_NAME} PROPERTIES DISABLED True)
  endif()
endfunction(TR_ADD_UNITY_TEST)
endif (NOT COMMAND TR_ADD_UNITY_TEST)
