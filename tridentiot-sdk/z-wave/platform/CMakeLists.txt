# SPDX-FileCopyrightText: 2025 Trident IoT, LLC <https://www.tridentiot.com>
# SPDX-License-Identifier: LicenseRef-TridentMSLA

if( CMAKE_BUILD_TYPE STREQUAL Test )
  add_subdirectory(test)
else()
  add_subdirectory(apps)
  add_subdirectory(TridentIoT)
  add_subdirectory(libs)
endif()
