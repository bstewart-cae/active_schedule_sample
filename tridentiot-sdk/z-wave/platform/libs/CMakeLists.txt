# SPDX-FileCopyrightText: 2025 Trident IoT, LLC <https://www.tridentiot.com>
# SPDX-License-Identifier: LicenseRef-TridentMSLA

######################################################################
# Z-wave patch lib
######################################################################
set(ZW_PATCH_SOURCES
   ${ZW_SDK_ROOT}/platform/libs/src/s2_span_mpan_storage.c
)

add_library(zwave_patch_${PLATFORM_VARIANT} OBJECT ${ZW_PATCH_SOURCES})

target_include_directories(zwave_patch_${PLATFORM_VARIANT}
  PRIVATE
    ${ZW_SDK_ROOT}/z-wave-stack/SubTree/libs2/include
    ${ZW_SDK_ROOT}/platform/TridentIoT/inc
    ${ZW_SDK_ROOT}/z-wave-stack/Components/DebugPrint
    ${ZW_SDK_ROOT}//z-wave-stack/ZWave/API
    ${ZW_SDK_ROOT}//z-wave-stack/PAL/inc
)

target_compile_options(zwave_patch_${PLATFORM_VARIANT}
  PRIVATE
    "-Wno-unused-parameter"
)

target_compile_definitions(zwave_patch_${PLATFORM_VARIANT}
  PRIVATE
    -DTR_PLATFORM_${PLATFORM_VARIANT}
)

target_link_libraries(zwave_patch_${PLATFORM_VARIANT}
  PRIVATE
    DebugPrint
    Assert
)