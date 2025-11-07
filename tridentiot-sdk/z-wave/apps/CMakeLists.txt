# SPDX-FileCopyrightText: 2025 Trident IoT, LLC <https://www.tridentiot.com>
# SPDX-License-Identifier: LicenseRef-TridentMSLA

if(DEFINED ZW_SKIP_APPS AND ZW_SKIP_APPS EQUAL 1)
    # Do not include example applications.
else()
  add_subdirectory(window_sensor)
  add_subdirectory(zniffer)
  add_subdirectory(radio_cli)
  if (EXISTS fs_test)
    add_subdirectory(fs_test)
  endif()
endif()
