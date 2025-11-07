zw_add_object_cc(NAME CC_Supervision
  INCLUDES
    ${CMAKE_CURRENT_SOURCE_DIR}/inc
    ${CMAKE_CURRENT_SOURCE_DIR}/config
  SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/CC_Supervision.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/cc_supervision_config_api.c
  DEPENDS
    CC_MultiChannelControl
    ZAF_EventDistributor_soc
)

add_test_subdirectory (mocks)
add_test_subdirectory (tests)
