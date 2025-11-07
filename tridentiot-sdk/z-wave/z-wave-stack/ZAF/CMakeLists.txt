# SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
# SPDX-FileCopyrightText: 2025 Trident IoT, LLC <https://www.tridentiot.com>
#
# SPDX-License-Identifier: BSD-3-Clause

add_subdirectory(ApplicationUtilities)
add_subdirectory(CommandClasses)
add_subdirectory(configuration)
add_subdirectory(ProtocolConfig)
add_subdirectory(TransportLayer)

IF(NOT CMAKE_BUILD_TYPE MATCHES Test)
  #
  # The ZAF library must contain source files that DO NOT depend on
  # application specific defines defined in app.c source file.
  #
  add_library(ZAF OBJECT EXCLUDE_FROM_ALL)

  target_sources(ZAF
    PRIVATE
      ${ZAF_UTILDIR}/_commonIF/ZAF_Common_helper.c
      ${ZAF_UTILDIR}/CCListGenerator/zaf_cc_list_generator.c
      ${ZAF_UTILDIR}/nvm/zaf_nvm_soc.c
      ${ZAF_UTILDIR}/ZAF_CmdPublisher.c
      ${ZAF_UTILDIR}/ZAF_command_class_utils.c
      ${ZAF_UTILDIR}/ZAF_network_learn.c
      ${ZAF_UTILDIR}/ZAF_network_management.c
      ${ZAF_UTILDIR}/ZAF_transport.c
      ${ZAF_UTILDIR}/ZW_TransportSecProtocol.c
      ${ZAF_UTILDIR}/ZW_TransportEndpoint.c
      ${ZAF_UTILDIR}/ZAF_CC_Invoker.c
    INTERFACE
      ${ZAF_CONFIGDIR}/src/zaf_config_api.c
      ${ZAF_UTILDIR}/ZW_TransportMulticast.c
      $<TARGET_OBJECTS:ZAF_AppTimer>
      $<TARGET_OBJECTS:ZAF_CommonInterface>
      $<TARGET_OBJECTS:ZAF_nvm>
      $<TARGET_OBJECTS:ZAF_retention_register>
      $<TARGET_OBJECTS:ZAF_TSE_weak>
      $<TARGET_OBJECTS:CC_Common>
      $<TARGET_OBJECTS:CC_DeviceResetLocally>
      $<TARGET_OBJECTS:CC_Indicator>
      $<TARGET_OBJECTS:CC_ManufacturerSpecific>
      $<TARGET_OBJECTS:CC_PowerLevel>
      $<TARGET_OBJECTS:CC_Supervision>
      $<TARGET_OBJECTS:CC_Version>
      $<TARGET_PROPERTY:CC_FirmwareUpdate,INTERFACE_SOURCES>
      $<TARGET_PROPERTY:CC_MultiChannelControl,INTERFACE_SOURCES>
  )

  target_include_directories(ZAF
    INTERFACE
      # ZAF config can be overriden by application, so ZAF can't depend on it.
      ${ZAF_CONFIGDIR}/config
    PUBLIC
      ${ZWAVE_API_DIR} # mainly for ZW_classcmd.h
      ${ZAF_UTILDIR}
      ${ZAF_UTILDIR}/_commonIF
      ${ZAF_UTILDIR}/nvm
      ${ZAF_UTILDIR}/CCListGenerator
      ${ZAF_UTILDIR}/EventHandling
      $<TARGET_PROPERTY:CC_FirmwareUpdate,INTERFACE_INCLUDE_DIRECTORIES>
      $<TARGET_PROPERTY:CC_MultiChannelControl,INTERFACE_INCLUDE_DIRECTORIES>
  )

  target_link_libraries(ZAF
    PUBLIC
      zpal
      Assert
      EventDistributor
      QueueNotifying
      AppsHw
      CC_Common
      CC_DeviceResetLocally
      CC_Indicator
      CC_ManufacturerSpecific
      CC_PowerLevel
      CC_Supervision
      CC_Version
      ZAF_AppName
      zaf_protocol_config_interface
      zaf_transport_layer_interface
      ZAF_DEBUGPRINT
    INTERFACE
      CC_Association
      CC_ZWavePlusInfo
  )

  target_compile_options(ZAF PRIVATE -Wpedantic -Wconversion -Wextra)

################################################################################
# Create DebugPrint library
################################################################################

  add_library(ZAF_DEBUGPRINT OBJECT EXCLUDE_FROM_ALL
      ${ZAF_UTILDIR}/ZAF_PrintAppInfo.c
   )

  target_include_directories(ZAF_DEBUGPRINT
    PUBLIC
      ${ZAF_UTILDIR}
      ${CONFIG_DIR}
      ${ZWAVE_API_DIR}
      ${ZPAL_API_DIR}
      ${ZPAL_SILABS_INCLUDES}
  )

  target_link_libraries(ZAF_DEBUGPRINT
    PRIVATE
      DebugPrint
      ZAF_AppName
  )

  # target_compile_options(ZAF_DEBUGPRINT PRIVATE -Wpedantic -Wconversion -Wextra -Wno-gnu-zero-variadic-macro-arguments)
ENDIF(NOT CMAKE_BUILD_TYPE MATCHES Test)
