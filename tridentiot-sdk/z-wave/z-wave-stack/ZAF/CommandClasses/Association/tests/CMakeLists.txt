################################################################################
# Add test for Multi Channel Association command class.
################################################################################
set(test_CC_MultiChanAssociation_src
  test_CC_MultiChanAssociation.cpp
  ../src/CC_MultiChanAssociation.c
  #../association_plus.c # The file is included in the test file. 
  ${ZAF_UTILDIR}/mocks/ZW_TransportSecProtocol_mock.c
  ${ZAF_UTILDIR}/mocks/mock_ZW_TransportEndpoint.c
  ${ZAF_UTILDIR}/mocks/mock_ZW_TransportMulticast.c
  ${ZAF_UTILDIR}/mocks/ZAF_nvm_mock.c
  ${ZAF_UTILDIR}/mocks/ZAF_nvm_app_mock.c
  ${ZAF_UTILDIR}/mocks/AppTimer_mock.c
  ${ZAF_UTILDIR}/ZAF_CC_Invoker.c
  )
add_unity_test(NAME test_CC_MultiChanAssociation
               FILES ${test_CC_MultiChanAssociation_src}
               LIBRARIES mock
                         Utils
                         SwTimerMock
                         ZAF_CommonInterfaceMock
                         ZAF_TSEMock
                         ZAF_CmdPublisher_Mock
                         zpal_mock
                         test_common
                         cc_association_group_info_cmock
                         cc_association_config_api_cmock
               USE_UNITY_WITH_CMOCK
               USE_CPP)
target_compile_definitions(test_CC_MultiChanAssociation PRIVATE
  test_CC_MultiChanAssociation_defines
  CC_ASSOCIATION_MAX_NODES_IN_GROUP=5
  ZAF_CONFIG_NUMBER_OF_END_POINTS=4
  CC_ASSOCIATION_MAX_GROUPS_PER_ENDPOINT=3
)
target_include_directories(test_CC_MultiChanAssociation
  PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/.."
    ${ZAF_UTILDIR}
    ${ZAF_CCDIR}/Common
    ${ZAF_UNITTESTEXTERNALS}
    ${ZAF_CONFIGDIR}/config
    ../config
)


################################################################################
# Add test for Association command class.
################################################################################
set(test_CC_Association_src
  test_CC_Association.cpp
  ../src/CC_Association.c
  ../src/CC_MultiChanAssociation.c
  #../association_plus.c # The file is included in the test file.
  ${ZAF_UTILDIR}/mocks/ZW_TransportSecProtocol_mock.c
  ${ZAF_UTILDIR}/mocks/mock_ZW_TransportEndpoint.c
  ${ZAF_UTILDIR}/mocks/mock_ZW_TransportMulticast.c
  ${ZAF_UTILDIR}/mocks/ZAF_nvm_mock.c
  ${ZAF_UTILDIR}/mocks/ZAF_nvm_app_mock.c
  ${ZAF_UTILDIR}/mocks/AppTimer_mock.c
  ${ZAF_UTILDIR}/ZAF_CC_Invoker.c
  )
add_unity_test(NAME test_CC_Association
               FILES ${test_CC_Association_src}
               LIBRARIES mock
                         Utils
                         SwTimerMock
                         test_common
                         ZAF_CommonInterfaceMock
                         ZAF_TSEMock
                         ZAF_CmdPublisher_Mock
                         zpal_mock
                         cc_association_group_info_cmock
                         cc_association_config_api_cmock
               USE_UNITY_WITH_CMOCK
               USE_CPP)
target_compile_definitions(test_CC_Association PRIVATE
  test_CC_Association_defines
  CC_ASSOCIATION_MAX_NODES_IN_GROUP=5
  ZAF_CONFIG_NUMBER_OF_END_POINTS=4
  CC_ASSOCIATION_MAX_GROUPS_PER_ENDPOINT=3
)
target_include_directories(test_CC_Association
  PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/.."
    ${ZAF_CCDIR}/Common
    ${ZAF_UTILDIR}
    ${ZAF_UNITTESTEXTERNALS}
    ${ZAF_CONFIGDIR}/config
    ../config
)


################################################################################
# Add test for Association Group Info command class.
################################################################################
set(test_CC_AssociationGroupInfo_src
  test_CC_AssociationGroupInfo.c
  ../src/CC_AssociationGroupInfo.c
  ${ZAF_UTILDIR}/mocks/mock_ZW_TransportEndpoint.c
  ${ZAF_UTILDIR}/ZAF_CC_Invoker.c
  ${ZAF_UTILDIR}/mocks/ZW_TransportSecProtocol_mock.c
)

add_unity_test(NAME test_CC_AssociationGroupInfo
               FILES ${test_CC_AssociationGroupInfo_src}
               LIBRARIES mock
                         SwTimerMock
                         Utils
                         ZAF_CommonInterfaceMock
                         AppTimerMock
                         test_common
                         NodeMask
                         cc_association_config_api_cmock
                         zaf_transport_layer_cmock
                USE_UNITY_WITH_CMOCK
                         )
target_compile_definitions(test_CC_AssociationGroupInfo PRIVATE
test_CC_AssociationGroupInfo_defines
  ZAF_CONFIG_NUMBER_OF_END_POINTS=4
  CC_ASSOCIATION_MAX_GROUPS_PER_ENDPOINT=3
  CC_ASSOCIATION_MAX_NODES_IN_GROUP=1
)
target_include_directories(test_CC_AssociationGroupInfo
  PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/../src"
    ${ZAF_CCDIR}/Common
    ${ZAF_UTILDIR}
    ${ZAF_UNITTESTEXTERNALS}
    ${ZAF_CONFIGDIR}/config
    ../config
)


################################################################################
# Add test for Association Plus
################################################################################
set(test_association_plus_src
  test_association_plus.c
  ../src/association_plus.c
  ${ZAF_UTILDIR}/mocks/ZAF_nvm_mock.c
  ${ZAF_UTILDIR}/mocks/ZAF_nvm_app_mock.c
  ${ZAF_UTILDIR}/mocks/ZW_TransportSecProtocol_mock.c
  ${ZAF_UTILDIR}/mocks/mock_ZW_TransportEndpoint.c
  ${ZAF_UTILDIR}/mocks/AppTimer_mock.c
  ${COMPONENTS_DIR}/SwTimer/mocks/SwTimer_mock.c
  ${ZAF_UTILDIR}/ZAF_CC_Invoker.c
)

add_unity_test(NAME test_association_plus
               FILES ${test_association_plus_src}
               LIBRARIES mock
                         ZAF_CommonInterfaceMock
                         zpal_mock
                         Utils
                         cc_association_group_info_cmock
                         cc_association_config_api_cmock
               USE_UNITY_WITH_CMOCK
)
target_compile_definitions(test_association_plus PRIVATE
  test_agi_defines
  ASSOCIATION_ALLOCATION_MAX=238  # This puts a limit on the sum of the next three parameters
  ZAF_CONFIG_NUMBER_OF_END_POINTS=4
  CC_ASSOCIATION_MAX_GROUPS_PER_ENDPOINT=3
  CC_ASSOCIATION_MAX_NODES_IN_GROUP=231 # 232 excluding node itself
)
target_include_directories(test_association_plus
  PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/.."
    ${ZAF_CCDIR}/Common
    ${ZAF_UTILDIR}
    ${ZAF_UTILDIR}/_commonIF
    ${ZAF_UNITTESTEXTERNALS}
    ${ZAF_CONFIGDIR}/config
    ../config
    ${COMPONENTS_DIR}/SwTimer
)

################################################################################
# CMock based unit test of Association Plus module
################################################################################

add_unity_test(
  NAME
    test_association_plus_cmock
  FILES
    test_association_plus_cmock.c
    ../src/association_plus.c
  LIBRARIES
    unity2
    Utils
    Assert_cmock
    FreeRTOS_cmock
    QueueNotifying_cmock
    NodeMask
    DebugPrint_cmock
    ZAF_Common_interface_cmock
    cc_association_group_info_cmock
    ZW_TransportSecProtocol_cmock
    ZAF_nvm_app_cmock
  USE_UNITY_WITH_CMOCK
)
target_compile_definitions(test_association_plus_cmock PRIVATE
  CC_ASSOCIATION_MAX_GROUPS_PER_ENDPOINT=2
)
target_include_directories(test_association_plus_cmock
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${ZAF_UTILDIR}
    ${ZWAVE_API_DIR}
    ${ZPAL_API_DIR}
    ../config
    ${ZAF_CONFIGDIR}/config
)

################################################################################
# Add test for Association Group mapping
################################################################################
set(test_AssociationGroupMapping_src
  test_AssociationGroupMapping_runner.c
  test_AssociationGroupMapping.c
  ../src/CC_AssociationGroupInfo.c
  ../src/association_plus.c
  ../src/CC_Association.c
  ../src/CC_MultiChanAssociation.c
  ${ZAF_UTILDIR}/ZAF_CC_Invoker.c
  ${ZAF_UTILDIR}/mocks/mock_ZW_TransportEndpoint.c
  ${ZAF_UTILDIR}/mocks/ZW_TransportSecProtocol_mock.c
  ${ZAF_UTILDIR}/mocks/mock_ZW_TransportMulticast.c
  ${ZAF_UTILDIR}/mocks/ZAF_nvm_mock.c
  ${ZAF_UTILDIR}/mocks/ZAF_nvm_app_mock.c
)

add_unity_test(NAME test_AssociationGroupMapping
               FILES ${test_AssociationGroupMapping_src}
               LIBRARIES mock
                         SwTimerMock
                         Utils
                         ZAF_CommonInterfaceMock
                         AppTimerMock
                         test_common
                         NodeMask
                         zpal_mock
                         cc_association_config_api_cmock
                         zaf_transport_layer_cmock
               USE_UNITY_WITH_CMOCK
)
target_compile_definitions(test_AssociationGroupMapping PRIVATE
test_AssociationGroupMapping_defines
  ZAF_CONFIG_NUMBER_OF_END_POINTS=4
  CC_ASSOCIATION_MAX_GROUPS_PER_ENDPOINT=4
  CC_ASSOCIATION_MAX_NODES_IN_GROUP=5
)
target_include_directories(test_AssociationGroupMapping
  PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/.."
    ${ZAF_CCDIR}/Association/src
    ${ZAF_UTILDIR}
    ${ZAF_UTILDIR_TARGET}
    ${ZAF_UNITTESTEXTERNALS}
    ${ZAF_CONFIGDIR}/config
    ../config
)
