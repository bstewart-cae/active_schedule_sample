find_package(Ruby)
# The RUBY_EXECUTABLE variable does not exist when the ADD_MOCK function is
# called So instead store the value in cache and use the cached value
set(CMOCK_RUBY_EXECUTABLE
    ${RUBY_EXECUTABLE}
    CACHE INTERNAL "")

# Documentation add_mock(<TARGET_NAME> header1 [header2] ... [headerN]  )
#
# Make a mock library using cmake given a list of header files.
function(add_mock TARGET)
  set(MOCK_HEADERS ${ARGV})
  # Pop the first argument of the list
  list(REMOVE_AT MOCK_HEADERS 0)
  add_mock_custom_config(
    TARGET
      ${TARGET} 
    CONFIG_FILE
      ${ZW_ROOT}/DevTools/zwave_cmock_config.yml
    HEADERS
      ${MOCK_HEADERS}
  )
endfunction()

function(add_cc_config_mock TARGET)
  set(MOCK_HEADERS ${ARGV})
  # Pop the first argument of the list
  list(REMOVE_AT MOCK_HEADERS 0)
  add_mock_custom_config(
    TARGET
      ${TARGET} 
    CONFIG_FILE
      ${ZW_ROOT}/DevTools/cc_config_cmock_config.yml
    HEADERS
      ${MOCK_HEADERS}
    SUFFIX
      "_config_mock"
  )
endfunction()

# Documentation add_mock(<TARGET_NAME> <CONFIGFILE> header1 [header2] ... [headerN])
#
# Make a mock library using cmake given a list of header files.
function(add_mock_custom_config)

  set(OPTIONS "")
  set(SINGLE_VALUE_ARGS "TARGET" "CONFIG_FILE" "SUFFIX")
  set(MULTI_VALUE_ARGS "HEADERS")
  cmake_parse_arguments(ADD_MOCK_CUSTOM_CONFIG "${OPTIONS}" "${SINGLE_VALUE_ARGS}" "${MULTI_VALUE_ARGS}" ${ARGN} )

  if(NOT DEFINED ADD_MOCK_CUSTOM_CONFIG_SUFFIX)
    set(SUFFIX "_mock")
  else()
    set(SUFFIX ${ADD_MOCK_CUSTOM_CONFIG_SUFFIX})
  endif()
  set(CMOCK_DIR "${ZW_ROOT}/ThirdParty/cmock")

  foreach(HEADER ${ADD_MOCK_CUSTOM_CONFIG_HEADERS})
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
         "${CMAKE_CURRENT_BINARY_DIR}/mocks/${BASE}${SUFFIX}.c")
  endforeach()

  list(REMOVE_DUPLICATES MOCK_HEADER_DIRS)

  add_custom_command(
    OUTPUT ${MOCK_SOURCE_FILES}
    DEPENDS ${MOCK_HEADER_FILES} ${ADD_MOCK_CUSTOM_CONFIG_CONFIG_FILE}
    COMMAND ${CMOCK_RUBY_EXECUTABLE} ${CMOCK_DIR}/lib/cmock.rb
            -o${ADD_MOCK_CUSTOM_CONFIG_CONFIG_FILE} ${MOCK_HEADER_FILES})

  add_library(${ADD_MOCK_CUSTOM_CONFIG_TARGET} STATIC ${MOCK_SOURCE_FILES})
  target_include_directories(${ADD_MOCK_CUSTOM_CONFIG_TARGET} PUBLIC ${CMAKE_CURRENT_BINARY_DIR}/mocks
                                              ${MOCK_HEADER_DIRS})
  target_link_libraries(${ADD_MOCK_CUSTOM_CONFIG_TARGET} PUBLIC cmock)
  if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(${ADD_MOCK_CUSTOM_CONFIG_TARGET} PRIVATE "-fPIC")
  endif()
endfunction()

# Generates a mocking library based of the provided target.
# It will generate mock implemenations for every header in the public include directory of the provided target.
# uses unity/cmock to generate mocks.
function(target_add_mock)
  # first argument is the target to make unit-test exe of.
  list(POP_FRONT ARGV TARGET)
  get_target_property(incl_dirs ${TARGET} INTERFACE_INCLUDE_DIRECTORIES)

  set(headers "")
  foreach(dir ${incl_dirs})
    file(GLOB files ${dir}/*.h)
    list(APPEND headers "${files}")
  endforeach()

  # add additional headers
  list(APPEND headers "${ARGV}")

  list(LENGTH headers len)
  if (NOT ${len} EQUAL 0)
    add_mock("${TARGET}_mock" ${headers})
    get_target_property(libs ${TARGET} LINK_LIBRARIES)
    foreach(lib ${libs})
      # filter libs-list so actual cmake targets are left.
      # $<TARGET_EXISTS is not robust enough, filter out files with extension.
      # we know already they are not actual cmake targets.
      string(FIND "${lib}" "." found_index REVERSE)
      if (${found_index} EQUAL -1)
        target_include_directories(
          "${TARGET}_mock"
          PUBLIC "$<$<TARGET_EXISTS:${lib}>:$<TARGET_PROPERTY:${lib},INTERFACE_INCLUDE_DIRECTORIES>>")
        endif()
    endforeach()
    target_compile_options("${TARGET}_mock" PRIVATE "-fPIC")
  else()
    message(WARNING "Nothing to mock for ${TARGET}. no headers found")
    message(WARNING "looked in \"${incl_dirs}\"")
  endif()

endfunction()