# SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
#
# SPDX-License-Identifier: BSD-3-Clause

zw_add_object_cc(NAME CC_Version
  INCLUDES
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/config
    ${CMAKE_CURRENT_SOURCE_DIR}/inc
    ${CONFIG_DIR}
  SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/CC_Version.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/cc_version_migration.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/cc_version_migration_config_weak.c
  DEPENDS
    CC_Association
    ZAF_TSE
    ZAF_EventDistributor_soc
)

add_test_subdirectory(tests)
add_test_subdirectory(mocks)
