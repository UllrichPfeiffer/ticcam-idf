cmake_minimum_required(VERSION 3.5)

set(CORE_SRCS 
src/M5Core2.cpp
src/AXP192.cpp
src/M5Display.cpp
src/M5Touch.cpp
src/RTC.cpp
src/Speaker.cpp
src/utility/CommUtil.cpp
src/utility/In_eSPI.cpp
src/utility/M5Button.cpp
src/utility/M5Timer.cpp
src/utility/MPU6886.cpp
src/utility/MahonyAHRS.cpp
src/utility/PointAndZone.cpp
src/utility/Sprite.cpp
src/utility/quaternionFilters.cpp
build/CMakeFiles/3.20.4/CompilerIdCXX/CMakeCXXCompilerId.cpp
)

set(includedirs
src
src/Fonts
src/Fonts/Custom
src/Fonts/GFXFF
src/utility
)

set(requires arduino)

idf_component_register(INCLUDE_DIRS ${includedirs} SRCS ${CORE_SRCS} REQUIRES ${requires})
