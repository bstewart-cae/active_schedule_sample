zw_add_object_cc(NAME CC_UserCode
  INCLUDES
    ${CMAKE_CURRENT_SOURCE_DIR}/inc
    ${CMAKE_CURRENT_SOURCE_DIR}/config
  SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/CC_UserCode.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/cc_user_code_database.c
  DEPENDS
    CC_Association
    ZAF_TSE_weak
    ZAF_EventDistributor_soc
)

zw_add_object_cc(NAME CC_UserCode_nvm
  INCLUDES
    ${CMAKE_CURRENT_SOURCE_DIR}/inc/
    ${CMAKE_CURRENT_SOURCE_DIR}/src/
    ${CMAKE_CURRENT_SOURCE_DIR}/config
  SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/cc_user_code_nvm.c
  DEPENDS
    CC_Association
)

add_test_subdirectory(mocks)
add_test_subdirectory(tests)
