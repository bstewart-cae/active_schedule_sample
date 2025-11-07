
################################################################################
# Add test for Multilevel Sensor  command class.
################################################################################

set(test_CC_MultilevelSensor_src
    test_CC_MultilevelSensor_SensorHandler.c
    ../src/CC_MultilevelSensor_SensorHandler.c
    ../src/CC_MultilevelSensor_SensorHandlerTypes.c
)
add_unity_test(NAME test_CC_MultilevelSensor_SensorHandler
               FILES ${test_CC_MultilevelSensor_src}
               LIBRARIES mock
                         test_common
                         Utils
              )

target_include_directories(test_CC_MultilevelSensor_SensorHandler  PUBLIC
  ../config
  ../inc
  ../src
  ${ZAF_CCDIR}/Association
  ${ZAF_CCDIR}/Supervision/inc
  ${ZAF_CCDIR}/Common
  ${ZAF_UNITTESTEXTERNALS}
  ${ZAF_UTILDIR}/TrueStatusEngine
)

set(test_CC_MultilevelSensor_SensorHandlerTypes_src
    test_CC_MultilevelSensor_SensorHandlerTypes.c
    ../src/CC_MultilevelSensor_SensorHandler.c
    ../src/CC_MultilevelSensor_SensorHandlerTypes.c
)
add_unity_test(NAME test_CC_MultilevelSensor_SensorHandlerTypes
               FILES ${test_CC_MultilevelSensor_SensorHandlerTypes_src}
               LIBRARIES mock
                         test_common
                         Utils
              )

    target_include_directories(test_CC_MultilevelSensor_SensorHandlerTypes  PUBLIC
    ../config
    ../inc
    ../src
    ${ZAF_CCDIR}/Association
    ${ZAF_CCDIR}/Supervision/inc
    ${ZAF_CCDIR}/Common
    ${ZAF_UNITTESTEXTERNALS}
    ${ZAF_UTILDIR}/TrueStatusEngine
)

set(test_CC_MultilevelSensor_Support_src
    test_CC_MultilevelSensor_Support.c
    ${ZAF_CCDIR}/Common/mocks/CC_Common_mock.c
    ${ZAF_UTILDIR}/mocks/mock_ZW_TransportEndpoint.c
    ${ZAF_UTILDIR}/mocks/mock_ZW_TransportMulticast.c
    ../src/CC_MultilevelSensor_Support.c
    ../src/CC_MultilevelSensor_SensorHandler.c
    ../src/CC_MultilevelSensor_SensorHandlerTypes.c
    ${ZAF_UTILDIR}/ZAF_CC_Invoker.c
)

add_unity_test(NAME test_CC_MultilevelSensor_Support
                    FILES ${test_CC_MultilevelSensor_Support_src}
                    LIBRARIES   mock
                                test_common
                                QueueNotifyingMock
                                CC_CommonMock
                                NodeMask
                                Utils
                                ZAF_TSEMock
                                SwTimerMock
                                AppTimerMock
                                DebugPrintMock
                                AssertTest
                                CC_SupervisionMock
                )
target_include_directories(test_CC_MultilevelSensor_Support
    PRIVATE
    ../inc
    ../src
    PUBLIC
    ../config
    ${ZAF_CCDIR}/Association
    ${ZAF_CCDIR}/Common
    ${ZAF_CCDIR}/Supervision/inc
    ${ZAF_UTILDIR}/TrueStatusEngine
    ${ZAF_UNITTESTEXTERNALS}
    ${ZAF_UTILDIR}
)

# Because of the IMU sensor, the number of registered sensors should be increased from 3 to 6 (+3 axis)
add_compile_definitions(MULTILEVEL_SENSOR_REGISTERED_SENSOR_NUMBER_LIMIT=6)
