# For more information about build system see
# https://docs.espressif.com/projects/esp-idf/en/latest/api-guides/build-system.html
# The following five lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.5)
#set(APPEND COMPONENT_REQUIRES "conversions")
set(APPEND COMPONENT_REQUIRES "M5Core2")
set(APPEND COMPONENT_REQUIRES "arduinoFFT")
set(APPEND COMPONENT_REQUIRES "lvgl")
set(APPEND COMPONENT_REQUIRES "Crypto")
set(APPEND COMPONENT_REQUIRES "tw-camera")
set(APPEND COMPONENT_REQUIRES "SdFat-2.2.2")
set(APPEND COMPONENT_REQUIRES "cJSON")
message("Components:" "${COMPONENT_REQUIRES}")

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(TW_ID02_rev3c)
idf_build_set_property(COMPILE_OPTIONS "-Wno-error=address" APPEND)
idf_build_set_property(COMPILE_OPTIONS "-Wno-error=maybe-uninitialized" APPEND)
idf_build_set_property(COMPILE_OPTIONS "-Wno-unused-variable" APPEND)
idf_build_set_property(COMPILE_OPTIONS "-Wno-missing-field-initializers" APPEND)
idf_build_set_property(COMPILE_OPTIONS "-DLV_CONF_INCLUDE_SIMPLE=1" APPEND)
#idf_build_set_property(COMPILE_OPTIONS "-DLV_CONF_PATH= ${CMAKE_SOURCE_DIR}/main/lv_conf.h" APPEND)
idf_build_set_property(COMPILE_OPTIONS "-I../main" APPEND)
idf_build_set_property(COMPILE_OPTIONS "-DBOARD_HAS_PSRAM" APPEND)