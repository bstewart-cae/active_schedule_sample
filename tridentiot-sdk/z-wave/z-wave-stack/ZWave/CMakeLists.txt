# SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
# SPDX-FileCopyrightText: 2025 Trident IoT, LLC <https://www.tridentiot.com>
#
# SPDX-License-Identifier: BSD-3-Clause

add_test_subdirectory(Test)
add_test_subdirectory(Protocol/Test)

# #################################################################################################
# Z-Wave API library
# #################################################################################################
add_library(ZWaveAPI INTERFACE)
target_include_directories(ZWaveAPI
  INTERFACE
    ${ZWAVE_API_DIR}
    $<TARGET_PROPERTY:Assert,INTERFACE_INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:NodeMask,INTERFACE_INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:QueueNotifying,INTERFACE_INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:zpal_${PLATFORM_VARIANT},INTERFACE_INCLUDE_DIRECTORIES>
)

if ((NOT CMAKE_BUILD_TYPE MATCHES Release) AND (NOT CMAKE_BUILD_TYPE MATCHES Debug))
  #
  # NOTICE
  #
  # Stop processing this file when not building for Release or Debug
  # but add the proprietary tests before return
  if(IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/proprietary)
    add_test_subdirectory(proprietary/tests)
  endif()
  return()
endif()

include (${ZW_ROOT}/DevTools/CMakeFunctions/debug_strip.cmake)

add_subdirectory(Protocol)

# UC expect to find ZWave libraries in ZWave/lib for release build
# and ZWave/lib_debug for debug build.
IF(CMAKE_BUILD_TYPE MATCHES Release)
  file(MAKE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/lib)
  set(ZWAVE_LIB_OUTPUT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/lib)
ELSE(CMAKE_BUILD_TYPE MATCHES Release)
  file(MAKE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/lib_debug)
  set(ZWAVE_LIB_OUTPUT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/lib_debug)
ENDIF(CMAKE_BUILD_TYPE MATCHES Release)

if(${ZW_CONFIG_IMPORT_LIBRARIES} STREQUAL "false")

# #################################################################################################
# Z-Wave common library
#
# Purpose:
#   Defines a library with all the Z-Wave sources that don't depend on platform nor controller/slave
#   specific code. This library exist to keep a decent structure and speed up compilation.
#
# Files in this library MUST be required by both slave and controller, but the files MUST NOT
# depend on any slave/controller specific defines nor any platform files.
# #################################################################################################
add_library(zw_common OBJECT
  ZW_event.c
  ZW_node.c
  linked_list.c
  ZW_nvm.c
  ZW_NVMCaretaker.c
  ZW_home_id_hash.c
  ZW_protocol_cmd_handler.c
)
target_include_directories(zw_common
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(zw_common
  PUBLIC
    ZWaveAPI
    SyncEvent
  PRIVATE
    DebugPrint
    Utils
)

# This list contains sources common between slave enhaced and controller Makefiles
SET(ZWAVE_COMMON_SOURCES
    ZW_dynamic_tx_power.c
    ZW_CCList.c
    ZW_ctimer.c
    ZW_explore.c
    ZW_explore_excl_req.c
    ZW_explore_incl_req.c
    ZW_home_id_generator.c
    ZW_main.c
    ZW_main_region.c
    ZW_network_management.c
    ZW_noise_detect.c
    ZW_protocol_interface.c
    ZW_frames_filters.c

    Protocol/ZW_DataLinkLayer.c
    Protocol/ZW_Frame.c
    Protocol/ZW_receivefilter_transport.c
    Protocol/ZW_receivefilter_learnmode.c
    Protocol/ZW_Channels.c

    ZW_transport.c
    ZW_ismyframe.c
    ZW_build_tx_header.c
    ZW_transport_multi.c
    ZW_transport_transmit_cb.c
    ZW_tx_queue.c
    ZW_txq_protocol.c

    ZW_timer.c
    ZW_UserTask.c

    ZW_lr_virtual_node_id.c
    ZW_DataLinkLayer_utils.c

    ZW_dynamic_tx_power_algorithm.c
)

if(IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/proprietary)
  add_subdirectory(proprietary)
endif()

add_library(zwave_common_base INTERFACE)
target_include_directories(zwave_common_base
  INTERFACE
    # Directories
    ${CMAKE_CURRENT_SOURCE_DIR}

    # Includes from platform dependent libraries
    $<TARGET_PROPERTY:zpal_${PLATFORM_VARIANT},INTERFACE_INCLUDE_DIRECTORIES>

    # Includes from independent libraries
    $<TARGET_PROPERTY:Assert,INTERFACE_INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:DebugPrint,INTERFACE_INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:EventDistributor,INTERFACE_INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:NodeMask,INTERFACE_INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:QueueNotifying,INTERFACE_INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:SwTimer,INTERFACE_INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:SyncEvent,INTERFACE_INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:Utils,INTERFACE_INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:MfgTokens,INTERFACE_INCLUDE_DIRECTORIES>
)
target_sources(zwave_common_base
  INTERFACE
    ${ZWAVE_COMMON_SOURCES}
    # Independent objects
    $<TARGET_OBJECTS:Assert>
    $<TARGET_OBJECTS:DebugPrint>
    $<TARGET_OBJECTS:EventDistributor>
    $<TARGET_OBJECTS:SyncEvent>
    $<TARGET_OBJECTS:NodeMask>
    $<TARGET_OBJECTS:MfgTokens>
    $<TARGET_OBJECTS:QueueNotifying>
    $<TARGET_OBJECTS:SwTimer>
    $<TARGET_OBJECTS:zw_common>
)

# This list contains sources only used by controller Makefiles
SET(ZWAVE_CONTROLLER_SOURCES
    Controller/ZW_controller.c
    Controller/ZW_controller_network_info_storage.c
    Controller/ZW_inclusion_controller.c
    Controller/ZW_replication.c
    Controller/ZW_routing.c
    Controller/ZW_routing_cache.c
    Controller/ZW_routing_init.c
    Controller/ZW_initiate_shutdown.c
)

# This list contains sources only used by slave enhanced Makefiles
SET(ZWAVE_SLAVE_SOURCES
    EndDevice/ZW_slave.c
    EndDevice/ZW_routing_all.c
    EndDevice/ZW_slave_network_info_storage.c
)

# This list contains additional sources used by slave enhanced Makefiles when compiling with security
# and transport service.
SET(ZWAVE_SLAVE_SECURITY_SOURCES
    ZW_Security_Scheme0.c
    ZW_secure_learn_support.c
    src-gen/Secure_learn.c
    ZW_Security_Scheme2.c
    ZW_s2_inclusion_glue.c
    ${SUBTREE_LIBS2}/transport_service/transport_service2.c
    ${SUBTREE_LIBS2}/transport_service/transport2_fsm.c
    ZW_transport_service_wrapper.c
)



# ############################################################
# Defines a library for the Z-Wave CONTROLLER keystore.
# ############################################################
add_library(zw_keystore_controller OBJECT
  ZW_keystore.c
  key_generation.c
)

target_include_directories(zw_keystore_controller
  PRIVATE
    ${SUBTREE_LIBS2}/include
    ${ZW_ROOT}/ZWave
    ${ZW_ROOT}/ZWave/Controller
    ${ZWAVE_CONFIG_DIR}
    $<TARGET_PROPERTY:DebugPrint,INTERFACE_INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:SyncEvent,INTERFACE_INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:Utils,INTERFACE_INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:MfgTokens,INTERFACE_INCLUDE_DIRECTORIES>
    ${MBEDTLS_DIR}/include/
)

target_compile_definitions(zw_keystore_controller
  PRIVATE
    "DllExport=extern" # Required by libs2
    ZW_controller_lib
)

target_link_libraries(zw_keystore_controller
  PUBLIC
    ZWaveAPI
)

# ############################################################
# Defines a library for the Z-Wave SLAVE keystore.
# ############################################################

add_library(zw_keystore_slave OBJECT
  ZW_keystore.c
  ZW_qrcode.c
  key_generation.c
)

target_include_directories(zw_keystore_slave
  PRIVATE
    ${SUBTREE_LIBS2}/include
    ${ZW_ROOT}/ZWave
    ${ZW_ROOT}/ZWave/EndDevice
    ${ZWAVE_CONFIG_DIR}
    ${ZWAVE_PSA_INTEG_DIR}
    ${COMPONENTS_DIR}/Assert
    $<TARGET_PROPERTY:SyncEvent,INTERFACE_INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:DebugPrint,INTERFACE_INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:Utils,INTERFACE_INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:MfgTokens,INTERFACE_INCLUDE_DIRECTORIES>
    ${MBEDTLS_DIR}/include/
)

target_compile_definitions(zw_keystore_slave
  PRIVATE
    "DllExport=extern" # Required by libs2
    ZW_slave_lib
)
if(PLATFORM_VARIANT STREQUAL "800s")
  target_compile_definitions(zw_keystore_slave
    PRIVATE
      -DZWAVE_PSA_SECURE_VAULT=1
      -DZWAVE_PSA_AES=1)
endif()

target_link_libraries(zw_keystore_slave
  PUBLIC
    ZWaveAPI
)

# ############################################################
# Defines a libs2 library for the Z-Wave controller.
# ############################################################
if (NOT PLATFORM_VARIANT STREQUAL "800s")
  SET(S2_SW_CRYPTO
    ${SUBTREE_LIBS2}/crypto/aes/aes.c)
endif()
add_library(libs2_controller OBJECT
  ${SUBTREE_LIBS2}/crypto/ctr_drbg/ctr_drbg.c
  ${SUBTREE_LIBS2}/crypto/curve25519/generic/base.c
  ${SUBTREE_LIBS2}/crypto/curve25519/generic/smult.c
  ${S2_SW_CRYPTO}
)

target_include_directories(libs2_controller
  PUBLIC
    ${SUBTREE_LIBS2}/include
)

target_compile_definitions(libs2_controller
  PRIVATE
    "DllExport=extern" # Required by libs2
    ZW_CONTROLLER
  )

if(PLATFORM_VARIANT STREQUAL "800s")
  target_compile_definitions(libs2_controller
  PUBLIC
    -DZWAVE_PSA_SECURE_VAULT=1
    -DZWAVE_PSA_AES=1)
endif()

# ############################################################
# Defines a libs2 library for the Z-Wave slave.
# ############################################################
set_source_files_properties(${SUBTREE_LIBS2}/crypto/kderiv/kderiv.c PROPERTIES COMPILE_FLAGS -Wno-unused-parameter)
add_library(libs2_slave_crypto OBJECT
  ${SUBTREE_LIBS2}/crypto/ctr_drbg/ctr_drbg.c
  ${SUBTREE_LIBS2}/crypto/nextnonce/nextnonce.c
  ${SUBTREE_LIBS2}/crypto/kderiv/kderiv.c
  ${SUBTREE_LIBS2}/crypto/curve25519/generic/bigint.c
)

target_include_directories(libs2_slave_crypto
  PUBLIC
    ${SUBTREE_LIBS2}/include
    ${COMPONENTS_DIR}/Assert
  PRIVATE
    ${SUBTREE_LIBS2}/crypto/curve25519/generic
)

target_compile_definitions(libs2_slave_crypto
  PRIVATE
    "DllExport=extern" # Required by libs2
    CCM_USE_PREDEFINED_VALUES
)

if(PLATFORM_VARIANT STREQUAL "800s")
  target_compile_definitions(libs2_slave_crypto
  PUBLIC
    -DZWAVE_PSA_SECURE_VAULT=1
    -DZWAVE_PSA_AES=1)
else()
  target_sources(libs2_slave_crypto
    PRIVATE
      ${SUBTREE_LIBS2}/crypto/aes/aes.c
      ${SUBTREE_LIBS2}/crypto/aes-cmac/aes_cmac.c
      ${SUBTREE_LIBS2}/crypto/curve25519/generic/base.c
      ${SUBTREE_LIBS2}/crypto/curve25519/generic/smult.c
      ${SUBTREE_LIBS2}/crypto/ccm/ccm.c
  )
endif()

# ############################################################
# Defines a libs2 library for the Z-Wave slave.
# ############################################################
add_library(libs2_slave_protocol OBJECT
  ${SUBTREE_LIBS2}/protocol/S2.c
  ${SUBTREE_LIBS2}/inclusion/s2_inclusion.c
)

target_include_directories(libs2_slave_protocol
  PUBLIC
    ${SUBTREE_LIBS2}/include
  PRIVATE
    ${ZW_ROOT}/ZWave/
    ${ZW_ROOT}/ZWave/EndDevice
    ${ZWAVE_PSA_INTEG_DIR}
    ${MBEDTLS_DIR}/include/
    ${SUBTREE_LIBS2}/crypto/curve25519/generic
    ${COMPONENTS_DIR}/SyncEvent/
)
target_link_libraries(libs2_slave_protocol
  PUBLIC
    Assert
    zpal
    ZWaveAPI  # Required by S2.c Can we avoid depending on this here?
)

if(PLATFORM_VARIANT STREQUAL "800s")
  target_compile_definitions(libs2_slave_protocol
    PUBLIC
      -DZWAVE_PSA_SECURE_VAULT=1
      -DZWAVE_PSA_AES=1)
endif()

# ############################################################
# Build all supported variants of the controller bridge/static
# ############################################################
add_library(ZWaveController STATIC
  # Add source files here
  ${ZWAVE_COMMON_SOURCES}
  ${ZWAVE_CONTROLLER_SOURCES}
  $<TARGET_OBJECTS:libs2_controller> # Required by random number generator
  $<TARGET_OBJECTS:zw_keystore_controller>
)

if(PLATFORM_VARIANT STREQUAL "700s")
  target_compile_definitions(ZWaveController
    PRIVATE
      ZWAVE_MIGRATE_FILESYSTEM
  )
endif()

if(PLATFORM_VARIANT STREQUAL "800s")
  target_sources(ZWaveController
    PRIVATE
      $<TARGET_OBJECTS:zw_psa_lib>
  )
endif()

target_include_directories(ZWaveController
  PUBLIC
    ${ZW_ROOT}/ZWave/Controller
  PRIVATE
    ./src-gen
    ./Protocol
    ${ZWAVE_CONFIG_DIR}
    $<TARGET_PROPERTY:libs2_controller,INTERFACE_INCLUDE_DIRECTORIES>
)

target_compile_definitions(ZWaveController
  PRIVATE
    ZW_controller_lib
  INTERFACE
    ZW_CONTROLLER
    ZW_CONTROLLER_STATIC
    ZW_CONTROLLER_BRIDGE
)

target_link_libraries(ZWaveController
  PRIVATE
    zwave_common_base
)

IF(CMAKE_BUILD_TYPE MATCHES Release)
  lib_debug_strip(ZWaveController)
ENDIF(CMAKE_BUILD_TYPE MATCHES Release)

ADD_CUSTOM_COMMAND(TARGET ZWaveController
  POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy libZWaveController.a ${ZWAVE_LIB_OUTPUT_DIR}/libZWaveController_${PLATFORM_VARIANT}.a
  COMMENT "Copying libZWaveController.a to ${ZWAVE_LIB_OUTPUT_DIR}/libZWaveController_${PLATFORM_VARIANT}.a"
)

# ############################################################
# Build all supported variants of the controller test
# ############################################################
add_library(ZWaveControllerTest_${PLATFORM_VARIANT} STATIC
  # Add source files here
  ${ZWAVE_COMMON_SOURCES}
  ${ZWAVE_CONTROLLER_SOURCES}
  ${ZW_COMMON_OBJECTS}
  $<TARGET_OBJECTS:libs2_controller> # Required by random number generator
  $<TARGET_OBJECTS:zw_keystore_controller>
)

if (PLATFORM_VARIANT STREQUAL "700s")
  target_compile_definitions(ZWaveControllerTest_${PLATFORM_VARIANT}
    PRIVATE
      ZWAVE_MIGRATE_FILESYSTEM
  )
endif()

target_include_directories(ZWaveControllerTest_${PLATFORM_VARIANT}
  PUBLIC
    ${ZW_COMMON_PUBLIC_INCLUDES}
  PRIVATE
    ./src-gen
    ./Protocol
    ${ZW_ROOT}/ZWave/ZWaveConfig
    ${ZW_ROOT}/ZWave/Controller
    $<TARGET_PROPERTY:libs2_controller,INTERFACE_INCLUDE_DIRECTORIES>
)

target_compile_definitions(ZWaveControllerTest_${PLATFORM_VARIANT}
  PRIVATE
    ${BUILD_DEFINITIONS}
    ZW_controller_lib
    ZW_CONTROLLER_TEST_LIB
  INTERFACE
    ZW_CONTROLLER
    ZW_CONTROLLER_STATIC
    ZW_CONTROLLER_BRIDGE
    ZW_CONTROLLER_TEST_LIB
)

target_link_libraries(ZWaveControllerTest_${PLATFORM_VARIANT}
  PRIVATE
    zwave_common_base
)

IF(CMAKE_BUILD_TYPE MATCHES Release)
  lib_debug_strip(ZWaveControllerTest_${PLATFORM_VARIANT})
ENDIF(CMAKE_BUILD_TYPE MATCHES Release)

ADD_CUSTOM_COMMAND(TARGET ZWaveControllerTest_${PLATFORM_VARIANT}
  POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy libZWaveControllerTest_${PLATFORM_VARIANT}.a ${ZWAVE_LIB_OUTPUT_DIR}
  COMMENT "Copying libZWaveControllerTest_${PLATFORM_VARIANT}.a to ${ZWAVE_LIB_OUTPUT_DIR}"
)

# #########################################
# Build all supported variants of the slave
# #########################################
SET(ZWAVE_SLAVE_COMMON_LIBS
  ${ZWAVE_PHY_SECURITY_SOURCES}
  ${ZWAVE_COMMON_SOURCES}
  ${ZWAVE_SLAVE_SOURCES}
  ${ZWAVE_SLAVE_SECURITY_SOURCES}
  $<TARGET_OBJECTS:CRC>
  $<TARGET_OBJECTS:zw_keystore_slave>
  $<TARGET_OBJECTS:libs2_slave_crypto>
  $<TARGET_OBJECTS:libs2_slave_protocol>
)

  if(PLATFORM_VARIANT STREQUAL "800s")
    SET(ZWAVE_SLAVE_LIBS
      ${ZWAVE_SLAVE_COMMON_LIBS}
      $<TARGET_OBJECTS:zw_psa_lib>
    )
  else()
    SET(ZWAVE_SLAVE_LIBS
      ${ZWAVE_SLAVE_COMMON_LIBS}
    )
  endif()

add_library(ZWaveSlave STATIC
  # Add source files here
  ${ZWAVE_SLAVE_LIBS}
)

if(PLATFORM_VARIANT STREQUAL "700s")
  target_compile_definitions(ZWaveSlave
    PRIVATE
      ZWAVE_MIGRATE_FILESYSTEM
  )
endif()

if(PLATFORM_VARIANT STREQUAL "800s")
  target_compile_definitions(ZWaveSlave
    PUBLIC
      -DZWAVE_PSA_SECURE_VAULT=1
      -DZWAVE_PSA_AES=1)
  target_include_directories(ZWaveSlave
    PRIVATE
    ${ZWAVE_PSA_INTEG_DIR} # Needed by ZW_main.c
  )
endif()

target_include_directories(ZWaveSlave
  PUBLIC
    $<TARGET_PROPERTY:CRC,INTERFACE_INCLUDE_DIRECTORIES>
    ${ZW_ROOT}/ZWave/EndDevice/
  PRIVATE
    ./src-gen
    ./Protocol
    ${ZWAVE_CONFIG_DIR}
    $<TARGET_PROPERTY:libs2_slave_crypto,INTERFACE_INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:libs2_slave_protocol,INTERFACE_INCLUDE_DIRECTORIES>
)

if(PLATFORM_VARIANT STREQUAL "800s")
  target_include_directories(ZWaveSlave
    PRIVATE
      $<TARGET_PROPERTY:zw_psa_lib,INTERFACE_INCLUDE_DIRECTORIES>
  )
endif()

target_compile_definitions(ZWaveSlave
  PRIVATE
    ZW_slave_lib
  INTERFACE
    ZW_SLAVE
    ZW_SLAVE_ROUTING
    ZW_SECURITY_PROTOCOL
    SINGLE_CONTEXT
)

target_link_libraries(ZWaveSlave
  PRIVATE
    zwave_common_base
)

IF(CMAKE_BUILD_TYPE MATCHES Release)
  lib_debug_strip(ZWaveSlave)
ENDIF(CMAKE_BUILD_TYPE MATCHES Release)

ADD_CUSTOM_COMMAND(TARGET ZWaveSlave
  POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy libZWaveSlave.a ${ZWAVE_LIB_OUTPUT_DIR}/libZWaveSlave_${PLATFORM_VARIANT}.a
  COMMENT "Copying libZWaveSlave.a to ${ZWAVE_LIB_OUTPUT_DIR}/libZWaveSlave_${PLATFORM_VARIANT}.a"
)
endif()

# ##############################################################################
# Source vs. library logic
# ##############################################################################
if(${ZW_CONFIG_IMPORT_LIBRARIES} STREQUAL "false")
  # TODO: If an application developer uses install(), it will override the
  # libraries when running "make install" and building with
  # ZW_CONFIG_IMPORT_LIBRARIES set to true.
  install(
    TARGETS ZWaveSlave ZWaveController
    CONFIGURATIONS Release
    DESTINATION ${ZW_CONFIG_LIBRARY_DESTINATION}
  )
else()
  # Import static libraries
  message(DEBUG "Import ZWaveSlave from ${ZW_CONFIG_LIBRARY_DESTINATION}/libZWaveSlave.a")
  add_library(ZWaveSlave STATIC IMPORTED GLOBAL)
  set_target_properties(ZWaveSlave PROPERTIES IMPORTED_LOCATION ${ZW_CONFIG_LIBRARY_DESTINATION}/libZWaveSlave.a)
  target_include_directories(ZWaveSlave
    INTERFACE
      ${ZW_SDK_ROOT}/z-wave-stack/PAL/inc/
      ${ZW_SDK_ROOT}/platform/TridentIoT/config
  )
  target_compile_definitions(ZWaveSlave
    INTERFACE
      $<$<CONFIG:Release>:NO_DEBUGPRINT>
      $<$<CONFIG:Release>:NDEBUG>
      ZW_SLAVE
      ZW_SLAVE_ROUTING
      ZW_SECURITY_PROTOCOL
  )

  message(DEBUG "Import ZWaveController from ${ZW_CONFIG_LIBRARY_DESTINATION}/libZWaveController.a")
  add_library(ZWaveController STATIC IMPORTED GLOBAL)
  set_target_properties(ZWaveController PROPERTIES IMPORTED_LOCATION ${ZW_CONFIG_LIBRARY_DESTINATION}/libZWaveController.a)
  target_include_directories(ZWaveController
    INTERFACE
      ${ZW_SDK_ROOT}/z-wave-stack/PAL/inc/
      ${ZW_SDK_ROOT}/platform/TridentIoT/config
  )
  target_compile_definitions(ZWaveController
    INTERFACE
      $<$<CONFIG:Release>:NO_DEBUGPRINT>
      $<$<CONFIG:Release>:NDEBUG>
      ZW_CONTROLLER
      ZW_CONTROLLER_STATIC
      ZW_CONTROLLER_BRIDGE
  )
endif()
