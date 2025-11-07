# SPDX-License-Identifier: LicenseRef-TridentMSLA
# SPDX-FileCopyrightText: 2025 Trident IoT, LLC <https://www.tridentiot.com>
#
# This function reads the passed in tridentiot-sdk.toml file (which lives at the root 
# of the tridentiot-sdk), reads the YEAR.MONTH.PATCH version, and populates cmake variables 
# that can be used from the parent scope.
#
# Usage example:
# tr_get_sdk_version("${TRIDENT_SDK_ROOT}/tridentiot-sdk.toml")
# 
# After calling the function, you can use:
# ${TSDK_VERSION_YEAR}
# ${TSDK_VERSION_MONTH}
# ${TSDK_VERSION_PATCH}

function(tr_get_sdk_version TOML_FILE)
    # Check if file exists
    if(NOT EXISTS "${TOML_FILE}")
        message(FATAL_ERROR "TOML file not found: ${TOML_FILE}")
    endif()
    
    # Read the file content
    file(READ "${TOML_FILE}" TOML_CONTENT)
    
    # Parse version fields using regex
    # Pattern matches: key = value (integers) or key = "value" (quoted strings)
    string(REGEX MATCH "year[ \t]*=[ \t]*([0-9]+)" YEAR_MATCH "${TOML_CONTENT}")
    string(REGEX MATCH "month[ \t]*=[ \t]*([0-9]+)" MONTH_MATCH "${TOML_CONTENT}")
    string(REGEX MATCH "patch[ \t]*=[ \t]*([0-9]+)" PATCH_MATCH "${TOML_CONTENT}")
    
    # Extract the captured groups
    if(YEAR_MATCH)
        string(REGEX REPLACE "year[ \t]*=[ \t]*([0-9]+)" "\\1" VERSION_YEAR "${YEAR_MATCH}")
        set(TSDK_VERSION_YEAR "${VERSION_YEAR}" PARENT_SCOPE)
    else()
        message(FATAL_ERROR "Could not find 'year' field in TOML file")
    endif()
    
    if(MONTH_MATCH)
        string(REGEX REPLACE "month[ \t]*=[ \t]*([0-9]+)" "\\1" VERSION_MONTH "${MONTH_MATCH}")
        set(TSDK_VERSION_MONTH "${VERSION_MONTH}" PARENT_SCOPE)
    else()
        message(FATAL_ERROR "Could not find 'month' field in TOML file")
    endif()
    
    if(PATCH_MATCH)
        string(REGEX REPLACE "patch[ \t]*=[ \t]*([0-9]+)" "\\1" VERSION_PATCH "${PATCH_MATCH}")
        set(TSDK_VERSION_PATCH "${VERSION_PATCH}" PARENT_SCOPE)
    else()
        message(FATAL_ERROR "Could not find 'patch' field in TOML file")
    endif()
    
    # Optional: Print the parsed values for debugging
    message(STATUS "Parsed TSDK version: ${TSDK_VERSION_YEAR}.${TSDK_VERSION_MONTH}.${TSDK_VERSION_PATCH}")
endfunction()
