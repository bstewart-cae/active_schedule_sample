################################################################################
# Add test for Notification command class.
################################################################################

set(test_CC_Notification_src
  test_CC_Notification.cpp
  ../src/CC_Notification.c
  ${ZAF_UTILDIR}/mocks/mock_ZW_TransportMulticast.c
  ${ZAF_UTILDIR}/mocks/mock_ZW_TransportEndpoint.c
  ${ZAF_UTILDIR}/ZAF_CC_Invoker.c
  ${ZAF_CONFIGDIR}/src/zaf_config_api.c
  )
add_unity_test(NAME test_CC_Notification
               FILES ${test_CC_Notification_src}
               LIBRARIES mock
                         QueueNotifyingMock
                         Utils
                         test_common
                         NodeMask
                         ZAF_TSEMock
                         cc_notification_io_cmock
                         cc_notification_config_api_cmock
                         CC_CommonMock
                         AssertTest
               USE_CPP
               USE_UNITY_WITH_CMOCK
)
target_compile_definitions(test_CC_Notification PRIVATE
  MAX_ASSOCIATION_GROUPS=3
  MAX_ASSOCIATION_IN_GROUP=5
  CC_NOTIFICATION_MAX_NOTIFICATIONS=3
  ZAF_CONFIG_NUMBER_OF_END_POINTS=0
  ZAF_CONFIG_APP_ROLE_TYPE=ZWAVEPLUS_INFO_REPORT_ROLE_TYPE_END_NODE_ALWAYS_ON
)
target_include_directories(test_CC_Notification PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/../
  ${ZAF_CCDIR}/Common
  ${ZAF_UTILDIR}
  ${ZAF_UNITTESTEXTERNALS}
  ${ZAF_CONFIGDIR}/config
  ${ZAF_CONFIGDIR}/inc
  ${CONFIG_DIR}
  ../config
  ../mock
  ${ZAF_CCDIR}/Association
  ${ZAF_CCDIR}/Association/config
)


################################################################################
# Add test for notification
################################################################################

set(test_notification_src
  test_notification.cpp
  ../src/CC_Notification.c
  ${ZAF_CONFIGDIR}/src/zaf_config_api.c
  ${ZAF_UTILDIR}/ZAF_CC_Invoker.c
  )

add_unity_test(NAME test_notification
               FILES ${test_notification_src}
               LIBRARIES mock
                         cc_notification_io_cmock
                         cc_notification_config_api_cmock
                         AssertTest
                         ZAF_TSEMock
                         ZW_TransportEndpoint_cmock
                         CC_Common_cmock
               USE_CPP
               USE_UNITY_WITH_CMOCK
)

target_compile_definitions(test_notification PRIVATE
  CC_NOTIFICATION_MAX_NOTIFICATIONS=3
  ZAF_CONFIG_NUMBER_OF_END_POINTS=3
  CC_ASSOCIATION_MAX_GROUPS_PER_ENDPOINT=3
  CC_ASSOCIATION_MAX_NODES_IN_GROUP=5
  ZAF_CONFIG_APP_ROLE_TYPE=ZWAVEPLUS_INFO_REPORT_ROLE_TYPE_END_NODE_ALWAYS_ON
)

target_include_directories(test_notification PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/../
  ${ZAF_CCDIR}/Common
  ${ZAF_UTILDIR}
  ${ZWAVE_API_DIR}
  ${ZAF_CONFIGDIR}/inc
  ${ZAF_CONFIGDIR}/config
  ${CONFIG_DIR}
  ${COMPONENTS_DIR}/DebugPrint
  ${COMPONENTS_DIR}/Utils
  ../config
  ../mock
  ${ZAF_CCDIR}/Association
  ${ZAF_CCDIR}/Association/config
)
target_link_libraries(test_notification
  zpal
)
