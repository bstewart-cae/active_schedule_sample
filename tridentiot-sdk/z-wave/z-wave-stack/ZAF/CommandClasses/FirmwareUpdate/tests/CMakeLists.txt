#Please note that files have deliberately been named Udate and not Update
#Files named Update can trigger Windows to require administrator rights
################################################################################
# Add test for Firmware Update command class.
################################################################################

set(test_CC_FirmwareUdate_src
  test_CC_FirmwareUdate.c
  ../src/CC_FirmwareUpdate.c
  ../src/ota_util.c
  ${ZAF_CCDIR}/Version/mocks/CC_Version_mock_extern.c
  ${ZAF_UTILDIR}/mocks/mock_ZW_TransportEndpoint.c
  ${ZAF_UTILDIR}/mocks/ZW_TransportSecProtocol_mock.c
  ${ZAF_UTILDIR}/mocks/ZAF_nvm_mock.c
  ${ZAF_UTILDIR}/mocks/ZAF_nvm_app_mock.c
  ${ZAF_UTILDIR}/mocks/ZAF_transport_mock.c
  ${ZAF_UTILDIR}/ZAF_CC_Invoker.c
  )

add_unity_test(NAME test_CC_FirmwareUdate
               FILES ${test_CC_FirmwareUdate_src}
               LIBRARIES mock
                         SwTimerMock
                         AppTimerMock
                         CRC
                         ZAF_CommonInterfaceMock
                         test_common
                         Utils
                         zpal_mock
                         zaf_config_mock
                         zaf_common_helper_cmock
                         zaf_transport_layer_cmock
                         CC_SupervisionMock
)

target_compile_definitions(test_CC_FirmwareUdate PRIVATE # The misspelling of update is deliberate.
  test_CommandClassFirmwareUpate_defines
  ZW_SLAVE
  ZAF_CONFIG_NUMBER_OF_END_POINTS=0
  MAX_ASSOCIATION_GROUPS=1
  MAX_ASSOCIATION_IN_GROUP=1
  APP_MANUFACTURER_ID=0x1234
  APP_FIRMWARE_ID=0x5678
  CC_FIRMWARE_UPDATE_CONFIG_OTA_MULTI_FRAME=1
)
target_include_directories(test_CC_FirmwareUdate
  PUBLIC
    "../src" # For the private header
    "../inc"
    "../config"
    ${ZAF_CCDIR}/Association
    ${ZAF_CCDIR}/Common
    ${ZAF_CCDIR}/Supervision/inc
    ${ZAF_CCDIR}/Version
    ${ZAF_CCDIR}/ManufacturerSpecific
    ${ZAF_UTILDIR}
    ${ZAF_UNITTESTEXTERNALS}
    ${ZWAVE_MOCKS_DIR}
    ${ZW_ROOT}
)
