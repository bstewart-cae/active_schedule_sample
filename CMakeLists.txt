
cmake_minimum_required(VERSION 3.23.5)

project(active_schedule_sample
        VERSION 1.0.0
        LANGUAGES C ASM)

# Set app build configuration that should overwrite default configuration
set(ZWSDK_CONFIG_REGION REGION_US_LR)

# Include default app build configuration
include(config.cmake)

# Add app/ as an include directory because hardware/ depends on it.
include_directories(app)

set(APP_NAME ${CMAKE_PROJECT_NAME})
set(ACTIVE_SCHEDULE_SAMPLE_TARGET_NAME_TEMPLATE "{{name}}_{{hardware}}.elf")

add_subdirectory(tridentiot-sdk/z-wave)
add_subdirectory(hardware)
add_subdirectory(app)
