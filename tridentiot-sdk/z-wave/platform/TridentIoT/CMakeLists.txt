# SPDX-FileCopyrightText: 2025 Trident IoT, LLC <https://www.tridentiot.com>
# SPDX-License-Identifier: LicenseRef-TridentMSLA

set(FREERTOS_KERNEL_DIR ${ZW_SDK_ROOT}/z-wave-stack/ThirdParty/FreeRTOS-Kernel/)

function(create_zpal_libraries)
######################################################################
# FreeRTOS lib
######################################################################
set(FREERTOS_SOURCES
  ${FREERTOS_KERNEL_DIR}/event_groups.c
  ${FREERTOS_KERNEL_DIR}/list.c
  ${FREERTOS_KERNEL_DIR}/queue.c
  ${FREERTOS_KERNEL_DIR}/tasks.c
  ${FREERTOS_KERNEL_DIR}/timers.c
  ${FREERTOS_KERNEL_DIR}/portable/MemMang/heap_4.c
)

if(${PLATFORM} STREQUAL "T32CZ20")
  list(APPEND FREERTOS_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/PAL/src/freertos/freertos_port_t32cz20.c
    ${FREERTOS_KERNEL_DIR}/portable/GCC/ARM_CM33_NTZ/non_secure/portasm.c # Only for CM33
  )
elseif(${PLATFORM} STREQUAL "ARM")
  list(APPEND FREERTOS_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/PAL/src/freertos/freertos_port.c
  )
else()
  message(FATAL_ERROR "Unknown platform")
endif()

add_library(FreeRtos_${PLATFORM_VARIANT} OBJECT ${FREERTOS_SOURCES})

target_include_directories(FreeRtos_${PLATFORM_VARIANT}
  PUBLIC
    ${FREERTOS_KERNEL_DIR}/include
    ${ZW_SDK_ROOT}/z-wave-stack/config/FreeRTOS/
    ${TRISDK_PERIPHERAL_PATH}/Include/
    ${TRISDK_PATH}/Library/CMSIS/Include/
    ${TRISDK_PHY_PATH}/include
  PRIVATE
    ${ZW_SDK_ROOT}/platform/TridentIoT/PAL/inc/

    # ZPAL Power Manager
    ${CMAKE_CURRENT_SOURCE_DIR}/PAL/src

    ${ZW_SDK_ROOT}/z-wave-stack/Components/DebugPrint
)

if(${PLATFORM} STREQUAL "T32CZ20")
  target_include_directories(FreeRtos_${PLATFORM_VARIANT}
    PRIVATE
          ${ZW_SDK_ROOT}/platform/TridentIoT/config
    PUBLIC
      # Platform specific
      ${FREERTOS_KERNEL_DIR}/portable/GCC/ARM_CM33_NTZ/non_secure
      ${TRISDK_PATH}/Library/RT584/Include
  )
elseif(${PLATFORM} STREQUAL "ARM")
  target_include_directories(FreeRtos_${PLATFORM_VARIANT}
    PUBLIC
      # Platform specific
      ${FREERTOS_KERNEL_DIR}/portable/GCC/ARM_CM3/
      ${TRISDK_PATH}/Library/RT58x/Include
  )
else()
  message(FATAL_ERROR "Unknown platform")
endif()

target_compile_options(FreeRtos_${PLATFORM_VARIANT}
  PRIVATE
    "-Wno-unused-parameter"
)

if(${PLATFORM} STREQUAL "ARM")
######################################################################
# Create imported object library with RF driver
######################################################################
  set(zpal_rf_lib ${TRISDK_PATH}/Middleware/Prebuild/lib_rf_mcu.a)
  add_custom_command(
    OUTPUT comm_subsystem_drv.o rf_common_init.o rf_mcu.o rf_mcu_ahb.o rf_mcu_spi.o
    COMMAND ${CMAKE_AR} -x ${zpal_rf_lib}
    COMMENT "Extracting objects from ${zpal_rf_lib}"
  )

  add_library(rm_rf OBJECT IMPORTED)
    set_property(TARGET rm_rf PROPERTY IMPORTED_OBJECTS comm_subsystem_drv.o rf_common_init.o rf_mcu.o rf_mcu_ahb.o rf_mcu_spi.o)
endif()


######################################################################
# FlashDB lib
######################################################################
if(${TARGET_FS} STREQUAL "FLASH_DB")
set(FLASH_DB_DIR ${ZW_SDK_ROOT}/ThirdParty/flash_db/)

set(FLASH_DB_SOURCES
  ${FLASH_DB_DIR}/src/fdb_kvdb.c
  ${FLASH_DB_DIR}/src/fdb_utils.c
  ${FLASH_DB_DIR}/src/fdb.c
  ${FLASH_DB_DIR}/port/fal/src/fal_flash.c
  ${FLASH_DB_DIR}/port/fal/src/fal_partition.c
  ${FLASH_DB_DIR}/port/fal/src/fal.c

)
set_source_files_properties(${FLASH_DB_SOURCES} PROPERTIES COMPILE_FLAGS  "-Ofast -mtune=cortex-m33 -funroll-loops")

add_library(flash_db_${PLATFORM_VARIANT} OBJECT ${FLASH_DB_SOURCES})

target_include_directories(flash_db_${PLATFORM_VARIANT}
  PUBLIC
    ${ZW_SDK_ROOT}/platform/TridentIoT/PAL/src/flash_db/inc
    ${FLASH_DB_DIR}/port/fal/inc/
    ${TRISDK_PERIPHERAL_PATH}/Include/
    ${TRISDK_PATH}/Library/CMSIS/Include/
    ${TRISDK_PHY_PATH}/include
  PRIVATE
    ${ZW_SDK_ROOT}/platform/TridentIoT/PAL/inc/
    ${ZW_SDK_ROOT}/platform/TridentIoT/PAL/src/
    ${ZW_SDK_ROOT}/z-wave-stack/Components/DebugPrint
)

target_compile_definitions(flash_db_${PLATFORM_VARIANT}
  PRIVATE
    -DNVM_STORAGE_SIZE=${NVM_STORAGE_SIZE}
    -DTR_PLATFORM_${PLATFORM_VARIANT}
)

target_compile_options(flash_db_${PLATFORM_VARIANT}
  PRIVATE
    "-Wno-unused-parameter"
)

set (FS_LIB $<TARGET_OBJECTS:flash_db_${PLATFORM_VARIANT}>)
set (FS_INC $<TARGET_PROPERTY:flash_db_${PLATFORM_VARIANT},INTERFACE_INCLUDE_DIRECTORIES>)
endif()
######################################################################
# TRISDK library
######################################################################
set(TRISDK_SOURCES
    ${TRISDK_PERIPHERAL_PATH}/flashctl.c
    ${TRISDK_PERIPHERAL_PATH}/sysctrl.c
    ${TRISDK_PERIPHERAL_PATH}/sysfun.c
    ${TRISDK_PERIPHERAL_PATH}/lpm.c
    ${TRISDK_PERIPHERAL_PATH}/sadc.c
    ${TRISDK_PERIPHERAL_PATH}/timer.c
    ${TRISDK_PERIPHERAL_PATH}/pwm.c
    ${TRISDK_PERIPHERAL_PATH}/mp_sector.c

    ${TRISDK_PATH}/Middleware/RFB/rfb.c
    ${TRISDK_PATH}/Middleware/RFB/rfb_comm_common.c
    ${TRISDK_PATH}/Middleware/RFB/rfb_comm_15p4Mac.c
    ${TRISDK_PATH}/Middleware/RFB/rfb_comm_subg.c
    ${TRISDK_PATH}/Middleware/RFB/rfb_port.c

    ${TRISDK_PATH}/tokens/tr_platform_tokens.c
    ${TRIDENT_SDK_ROOT}/framework/common/tokens/tr_mfg_tokens.c
    ${TRIDENT_SDK_ROOT}/framework/utility/tlv_parser/tr_tlv_parser.c
  )

if(${PLATFORM} STREQUAL "T32CZ20")
  list(APPEND TRISDK_SOURCES
    ${TRISDK_PATH}/Library/RT584/Device/startup_cm33.c
    ${TRISDK_PATH}/Library/RT584/Peripheral/system_cm33.c

    ${TRISDK_PERIPHERAL_PATH}/rtc.c
    ${TRISDK_PERIPHERAL_PATH}/otp.c
    ${TRISDK_PERIPHERAL_PATH}/trng.c
    ${TRISDK_PERIPHERAL_PATH}/mp_sector.c

    ${TRISDK_PATH}/Library/RT584/PHY/rt569Smp/rt569s_fw.c
    ${TRISDK_PATH}/Library/RT584/PHY/comm_subsystem_drv.c
    ${TRISDK_PATH}/Library/RT584/PHY/rf_common_init.c
    ${TRISDK_PATH}/Library/RT584/PHY/rf_mcu.c
    ${TRISDK_PATH}/Library/RT584/PHY/rf_mcu_ahb.c
    ${TRIDENT_SDK_ROOT}/framework/hal/T32CZ20/tr_hal_common.c
    ${TRIDENT_SDK_ROOT}/framework/hal/T32CZ20/tr_hal_gpio.c
    ${TRIDENT_SDK_ROOT}/framework/hal/T32CZ20/tr_hal_wdog.c
    ${TRIDENT_SDK_ROOT}/framework/hal/T32CZ20/tr_hal_uart.c
    ${TRIDENT_SDK_ROOT}/framework/hal/T32CZ20/tr_hal_power.c
  )
elseif(${PLATFORM} STREQUAL "ARM")
  list(APPEND TRISDK_SOURCES
    ${TRISDK_PATH}/Library/RT58x/Device/GCC/gcc_startup_cm3_mcu.S
    ${TRISDK_PATH}/Library/RT58x/Device/system_cm3_mcu.c

    ${TRISDK_PHY_PATH}/rt569mp/rt569mp_fw.c
    ${TRISDK_PHY_PATH}/rt569mp/rt569mp_init.c
  )
else()
  message(FATAL_ERROR "Unknown platform")
endif()

add_library(trisdk OBJECT ${TRISDK_SOURCES})

if(${PLATFORM} STREQUAL "T32CZ20")
  target_compile_definitions(trisdk
    PRIVATE
      -DRT584_SHUTTLE_IC=0
      -DRFB_15p4_MAC_ENABLED=1
      -DRFB_SUBG_ENABLED=1
      -DRCO16K_ENABLE=1
      -DTR_PLATFORM_${PLATFORM_VARIANT}
  )
elseif(${PLATFORM} STREQUAL "ARM")
  target_compile_definitions(trisdk
    PRIVATE
      -DRFB_15p4_MAC_ENABLED=1
      -DRFB_SUBG_ENABLED=1
  )
endif()

set(TRISDK_INCLUDES_PRIVATE
  ${TRISDK_PATH}/Library/CMSIS/Include/
  ${TRISDK_PERIPHERAL_PATH}/Include/
  ${TRISDK_PHY_PATH}/include/
  ${TRISDK_PATH}/Middleware/RUCI/include/
)

target_include_directories(trisdk
  PUBLIC
    ${TRISDK_PERIPHERAL_PATH}/Include
    ${TRISDK_PATH}/Middleware/RFB/include/
    ${CMAKE_CURRENT_SOURCE_DIR}/config/ # Required for rt584_security_enable_config.h
    $<TARGET_PROPERTY:FreeRtos_${PLATFORM_VARIANT},INTERFACE_INCLUDE_DIRECTORIES>
    ${TRISDK_PATH}/Middleware/Portable/Utility/include/
    ${TRISDK_PATH}/Middleware/Portable/System/Include/
    ${TRISDK_PATH}/tokens/
    ${TRIDENT_SDK_ROOT}/framework/common/tokens/
    ${TRIDENT_SDK_ROOT}/framework/utility/tlv_parser/
    ${TRIDENT_SDK_ROOT}/framework/hal/include/
    ${TRIDENT_SDK_ROOT}/framework/hal/T32CZ20/
  PRIVATE
    ${TRISDK_INCLUDES_PRIVATE}
    ${CMAKE_CURRENT_SOURCE_DIR}/PAL/inc/ # Required for project_config.h
    ${CMAKE_CURRENT_SOURCE_DIR}/PAL/src/ # Required for tr_platform_tokens.c
)

if(${PLATFORM} STREQUAL "T32CZ20")
  target_include_directories(trisdk
    PUBLIC
      ${TRISDK_PATH}/Library/RT584/Include
    PRIVATE
      ${TRISDK_PHY_PATH}/rt569Smp/include/
  )
elseif(${PLATFORM} STREQUAL "ARM")
  target_include_directories(trisdk
    PUBLIC
      ${TRISDK_PATH}/Library/RT58x/Include
    PRIVATE
      ${TRISDK_PHY_PATH}/rt569mp/include/
  )
endif()

######################################################################
# ZPAL library
######################################################################
set(ZPAL_SOURCES_PATH ${CMAKE_CURRENT_SOURCE_DIR}/PAL/src/)

if(${TARGET_FS} STREQUAL "FLASH_DB")
set (FS_SRC  ${ZPAL_SOURCES_PATH}/flashdb_low_lvl.c ${ZPAL_SOURCES_PATH}/zpal_nvm_flashdb.c)
endif()

set_source_files_properties(${FS_SRC} PROPERTIES COMPILE_FLAGS  "-Ofast -mtune=cortex-m33 -funroll-loops")

set(ZPAL_SOURCES
  ${ZPAL_SOURCES_PATH}/ZW_region_rf_settings.c
  ${ZPAL_SOURCES_PATH}/main.c
  ${FS_SRC}
  ${ZPAL_SOURCES_PATH}/zpal_bootloader.c
  ${ZPAL_SOURCES_PATH}/zpal_entropy.c
  ${ZPAL_SOURCES_PATH}/zpal_init.c
  ${ZPAL_SOURCES_PATH}/zpal_misc.c
  ${ZPAL_SOURCES_PATH}/zpal_power_manager.c
  ${ZPAL_SOURCES_PATH}/zpal_retention_register.c
  ${ZPAL_SOURCES_PATH}/zpal_storage_utils.c
  ${ZPAL_SOURCES_PATH}/zpal_uart.c
  ${ZPAL_SOURCES_PATH}/zpal_watchdog.c
  ${ZPAL_SOURCES_PATH}/xmodem_state_event.c
)

  add_library(zpal_${PLATFORM_VARIANT} STATIC
    ${ZPAL_SOURCES}
    $<TARGET_OBJECTS:FreeRtos_${PLATFORM_VARIANT}>
    ${FS_LIB}
    $<TARGET_OBJECTS:trisdk>
    ${ZW_SDK_ROOT}/z-wave-stack/PAL/src/zpal_radio_utils.c
    ${ZW_SDK_ROOT}/z-wave-stack/ThirdParty/mbedtls/library/sha1.c
    ${ZW_SDK_ROOT}/z-wave-stack/ThirdParty/mbedtls/library/platform_util.c
    ${ZW_SDK_ROOT}/z-wave-stack/Components/CRC/CRC.c
  )

  if(${PLATFORM} STREQUAL "T32CZ20")
    set(SOURCES_REQUIRING_COMPILE_FLAGS
      ${ZPAL_SOURCES_PATH}/rtc_util.c
      ${ZPAL_SOURCES_PATH}/zpal_radio.c
      ${ZPAL_SOURCES_PATH}/T32CZ20/zwave_radio.c
    )
    target_sources(zpal_${PLATFORM_VARIANT} PRIVATE ${SOURCES_REQUIRING_COMPILE_FLAGS})
    set_source_files_properties(${SOURCES_REQUIRING_COMPILE_FLAGS} PROPERTIES COMPILE_FLAGS  "-Ofast -mtune=cortex-m33 -funroll-loops")
    target_compile_definitions(zpal_${PLATFORM_VARIANT}
      PRIVATE
        -DNVM_STORAGE_SIZE=${NVM_STORAGE_SIZE}
        -DMBEDTLS_CONFIG_FILE="${ZW_SDK_ROOT}/platform/TridentIoT/config/mbedtls/mbedtls_config.h"
        -DTR_PLATFORM_${PLATFORM_VARIANT}
        -DRFB_SUBG_ENABLED=1
        -DRT584_SHUTTLE_IC=0
        -DRCO16K_ENABLE=1
    )
    target_include_directories(zpal_${PLATFORM_VARIANT}
      PRIVATE
        ${ZW_SDK_ROOT}/z-wave-stack/Components/CRC/
        ${CMAKE_CURRENT_SOURCE_DIR}/inc/
        ${TRISDK_PATH}/Middleware/FOTA/Include/
        ${TRISDK_PATH}/Middleware/RUCI/include/
      PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/PAL/inc/
        ${ZW_SDK_ROOT}/z-wave-stack/PAL/inc/
        ${ZW_SDK_ROOT}/platform/TridentIoT/PAL/src/
        ${ZW_SDK_ROOT}/platform/TridentIoT/PAL/src/T32CZ20/include/
        $<TARGET_PROPERTY:FreeRtos_${PLATFORM_VARIANT},INTERFACE_INCLUDE_DIRECTORIES>
        ${FS_INC}
        $<TARGET_PROPERTY:trisdk,INTERFACE_INCLUDE_DIRECTORIES>
        ${ZW_SDK_ROOT}/z-wave-stack/ZWave/API/
        ${ZW_SDK_ROOT}/z-wave-stack/ZWave/
        ${ZW_SDK_ROOT}/z-wave-stack/ZAF/ApplicationUtilities/version/config
        ${ZW_SDK_ROOT}/z-wave-stack/ZAF/configuration/config
        ${ZW_SDK_ROOT}/z-wave-stack/ZAF/configuration/inc
        ${ZW_SDK_ROOT}/z-wave-stack/Components/Assert/
        ${ZW_SDK_ROOT}/z-wave-stack/Components/Utils/
        ${ZW_SDK_ROOT}/z-wave-stack/Components/MfgTokens/
        ${ZW_SDK_ROOT}/z-wave-stack/Components/SwTimer
        ${ZW_SDK_ROOT}/z-wave-stack/ThirdParty/mbedtls/include/
    )
  elseif(${PLATFORM} STREQUAL "ARM")
    target_sources(zpal_${PLATFORM_VARIANT} PRIVATE
      $<TARGET_OBJECTS:rm_rf>
      ${ZPAL_SOURCES_PATH}/zpal_radio.c
      ${ZPAL_SOURCES_PATH}/radio_RT581/zwave_radio.c
      ${ZPAL_SOURCES_PATH}/radio_RT581/zwave_radio_rt581.c
      ${ZPAL_SOURCES_PATH}/radio_RT581/zwave_frame_gen.c
    )

    target_compile_definitions(zpal_${PLATFORM_VARIANT}
      PRIVATE
        -DNVM_STORAGE_SIZE=${NVM_STORAGE_SIZE}
        -DMBEDTLS_CONFIG_FILE="${CMAKE_SOURCE_DIR}/platform/TridentIoT/config/mbedtls/mbedtls_config.h"
        -DTR_PLATFORM_${PLATFORM_VARIANT}
        -DRFB_15p4_MAC_ENABLED=1
        -DRFB_SUBG_ENABLED=1
    )

    target_include_directories(zpal_${PLATFORM_VARIANT}
    PRIVATE
      ${ZW_SDK_ROOT}/z-wave-stack/Components/CRC/
      ${CMAKE_CURRENT_SOURCE_DIR}/inc/
    PUBLIC
      ${CMAKE_CURRENT_SOURCE_DIR}/PAL/inc/
      ${ZW_SDK_ROOT}/z-wave-stack/PAL/inc/
      ${ZW_SDK_ROOT}/platform/TridentIoT/PAL/src/
      ${ZW_SDK_ROOT}/platform/TridentIoT/PAL/src/radio_RT581/include/
      $<TARGET_PROPERTY:FreeRtos_${PLATFORM_VARIANT},INTERFACE_INCLUDE_DIRECTORIES>
      ${FS_INC}
      $<TARGET_PROPERTY:trisdk,INTERFACE_INCLUDE_DIRECTORIES>
      ${ZW_SDK_ROOT}/z-wave-stack/ZWave/API/
      ${ZW_SDK_ROOT}/z-wave-stack/ZWave/
      ${ZW_SDK_ROOT}/z-wave-stack/ZAF/ApplicationUtilities/version/config
      ${ZW_SDK_ROOT}/z-wave-stack/ZAF/configuration/config
      ${ZW_SDK_ROOT}/z-wave-stack/ZAF/configuration/inc
      ${ZW_SDK_ROOT}/z-wave-stack/Components/Assert/
      ${ZW_SDK_ROOT}/z-wave-stack/Components/Utils/
      ${ZW_SDK_ROOT}/z-wave-stack/Components/MfgTokens/
      ${ZW_SDK_ROOT}/z-wave-stack/Components/SwTimer
      ${ZW_SDK_ROOT}/z-wave-stack/ThirdParty/mbedtls/include/
    )
  endif()

  target_link_libraries(zpal_${PLATFORM_VARIANT}
    PRIVATE
      DebugPrint
      tr_ring_buffer
  )

  target_compile_options(zpal_${PLATFORM_VARIANT}
    PRIVATE
      "-Wall"
      "-Werror"
      "-Wpedantic"
      "-Wextra"
  )
endfunction(create_zpal_libraries)

set(ZPAL_LIBRARY_DESTINATION ${ZW_SDK_ROOT}/lib)
message(DEBUG "ZPAL_LIBRARY_DESTINATION: ${ZPAL_LIBRARY_DESTINATION}")

if(${ZWSDK_IS_ROOT_PROJECT} STREQUAL "true")
  create_zpal_libraries()

  install(
    TARGETS zpal_${PLATFORM_VARIANT}
    CONFIGURATIONS Release
    DESTINATION ${ZPAL_LIBRARY_DESTINATION}
  )
else()
  if(ZWSDK_CONFIG_USE_SOURCES)
    create_zpal_libraries()
  else()
    # Import static library
    message(DEBUG "Import ZPAL library from ${ZPAL_LIBRARY_DESTINATION}/libzpal_${PLATFORM_VARIANT}.a")
    add_library(zpal_${PLATFORM_VARIANT} STATIC IMPORTED GLOBAL)
    set_target_properties(zpal_${PLATFORM_VARIANT} PROPERTIES IMPORTED_LOCATION ${ZPAL_LIBRARY_DESTINATION}/libzpal_${PLATFORM_VARIANT}.a)
    target_include_directories(zpal_${PLATFORM_VARIANT}
      INTERFACE
        ${ZW_SDK_ROOT}/z-wave-stack/PAL/inc/
        ${ZW_SDK_ROOT}/platform/TridentIoT/config
        ${FREERTOS_KERNEL_DIR}/include
        ${ZW_SDK_ROOT}/z-wave-stack/config/FreeRTOS/
        ${ZW_SDK_ROOT}/platform/TridentIoT/PAL/inc/
        ${FREERTOS_KERNEL_DIR}/portable/GCC/ARM_CM33_NTZ/non_secure

        # This include dir should not be set here, but rather with the application.
        ${ZW_SDK_ROOT}/z-wave-stack/ZAF/ApplicationUtilities/version/config

        # This include dir should not be here because it's a source folder.
        ${ZW_SDK_ROOT}/platform/TridentIoT/PAL/src/

        ${TRISDK_PERIPHERAL_PATH}/Include/
        ${TRISDK_PATH}/Library/RT584/Include
        ${TRISDK_PHY_PATH}/include
        ${TRIDENT_SDK_ROOT}/framework/hal/include/
        ${TRIDENT_SDK_ROOT}/framework/hal/T32CZ20/
    )
  endif()
endif()
