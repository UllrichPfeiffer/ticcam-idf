# This file was automatically generated for projects
# without default 'CMakeLists.txt' file.
cmake_minimum_required(VERSION 3.5)

set(CORE_SRCS 
main.cpp
AD5686.cpp
PID_v1.cpp
TicWaveLogo6_98x30.c
MCP466_DigitalPot.cpp
tw_class.cpp
global_var.cpp
colormap.cpp
ADS1100.cpp
)
#app_httpd.cpp

set(includedirs
.
../components/lvgl/src/font/
#../components/conversions/include/
)

set(requires M5Core2 lvgl )

idf_component_register(INCLUDE_DIRS ${includedirs} SRCS ${CORE_SRCS} )
