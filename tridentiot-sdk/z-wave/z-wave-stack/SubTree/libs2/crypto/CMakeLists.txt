# SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
# SPDX-FileCopyrightText: 2025 Trident IoT, LLC <https://www.tridentiot.com>
#
# SPDX-License-Identifier: BSD-3-Clause

cross_system_dir("curve25519" "" CURVE_INCLUDE_DIR)
include_directories(${CURVE_INCLUDE_DIR})

add_definitions(-DCCM_USE_PREDEFINED_VALUES)

set(CURVE_SRC curve25519/generic/smult.c curve25519/generic/base.c
              curve25519/generic/bigint.c)

set_source_files_properties(kderiv/kderiv.c PROPERTIES COMPILE_FLAGS
                                                       -Wno-unused-parameter)

if (PLATFORM_VARIANT STREQUAL "800s")
  set(CRYPTO_SRC
    ctr_drbg/ctr_drbg.c
    kderiv/kderiv.c
    curve25519/generic/bigint.c
    nextnonce/nextnonce.c)
else()
  set(CRYPTO_SRC
    ccm/ccm.c
    aes-cmac/aes_cmac.c
    ctr_drbg/ctr_drbg.c
    nextnonce/nextnonce.c
    kderiv/kderiv.c
    ${CURVE_SRC}
    ${SCRAMBLER_SRC})
endif()

if(${CMAKE_BUILD_TYPE} STREQUAL "Test")
#if(NOT "${CMAKE_PROJECT_NAME}" STREQUAL "SDK")
  add_library(aes aes/aes.c)
  add_library(s2crypto ${CRYPTO_SRC})
  target_compile_definitions(s2crypto PUBLIC "DllExport=extern")
  target_include_directories(s2crypto PUBLIC "${CURVE_INCLUDE_DIR}" ../include)
  if(WIN32)
    add_library(s2cryptoShared SHARED ${CRYPTO_SRC} aes/aes.c)
  endif()
endif()
