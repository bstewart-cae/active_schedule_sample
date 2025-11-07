add_definitions( -DUNIT_TEST )
set(TEST_TOOLS_DIR ${CMAKE_CURRENT_LIST_DIR} CACHE INTERNAL test_tools_dir)
find_package(PythonInterp)
include(CMakeFunctions.cmake)

# Prevent multiple creations of unity library
if (NOT TARGET unity)
  message(STATUS "Creating unity target in ${CMAKE_CURRENT_SOURCE_DIR}")
  add_subdirectory(unity)
else()
  message(STATUS "Skipping unity target in ${CMAKE_CURRENT_SOURCE_DIR}")
endif()

# Prevent multiple creations of mock library
if (NOT TARGET mock)
  message(STATUS "Creating mock target in ${CMAKE_CURRENT_SOURCE_DIR}")
  # Check whether the building of mock framework is disabled.
  # Building of the mock framework can be disabled for the c51 or asip simply by adding the following line to the specific project at higher level:
  # e.g.: set(DISABLE_MOCK 1)
  # Warning, Do not set DISABLE_MOCK in this file.
  if (NOT DEFINED DISABLE_MOCK)
    add_subdirectory(mock)
  endif (NOT DEFINED DISABLE_MOCK)
else()
  message(STATUS "Skipping mock target in ${CMAKE_CURRENT_SOURCE_DIR}")
endif()