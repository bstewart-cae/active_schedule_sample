zw_add_object_cc(NAME CC_PowerLevel
  SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/CC_PowerLevel.c
  DEPENDS
    SwTimer
    CC_Supervision
)

target_compile_definitions(CC_PowerLevel
  PRIVATE
    ZW_SLAVE
)

add_test_subdirectory(tests)
