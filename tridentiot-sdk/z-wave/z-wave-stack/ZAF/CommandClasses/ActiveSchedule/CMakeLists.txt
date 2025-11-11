#####################################################################
# SPDX-License-Identifier: BSD-3-Clause
# SPDX-FileCopyrightText: 2025 Card Access Engineering, LLC
#####################################################################

zw_add_object_cc(NAME CC_ActiveSchedule
  INCLUDES
    ${CMAKE_CURRENT_SOURCE_DIR}/config
    ${CMAKE_CURRENT_SOURCE_DIR}/inc
    ${CMAKE_CURRENT_SOURCE_DIR}/src
  SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/CC_ActiveSchedule.c
  DEPENDS
    ZAF_EventDistributor_soc
    ZAF_TSE_weak
    CRC
)

## Add test subdirectories
add_test_subdirectory(mocks)
add_test_subdirectory(tests)