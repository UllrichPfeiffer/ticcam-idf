cmake_minimum_required(VERSION 3.5)

set(CORE_SRCS 
Crypto.cpp
SHA256.cpp
Hash.cpp
)

set(includedirs
.
)

set(requires arduino)

idf_component_register(INCLUDE_DIRS ${includedirs} SRCS ${CORE_SRCS} REQUIRES ${requires})
