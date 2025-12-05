
set(SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/parser-ascii.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/parser-hdf5.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/parser-image.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/parser-yaml.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/parser-xml.cpp)

set(HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/parser-ascii.h
    ${CMAKE_CURRENT_SOURCE_DIR}/parser-hdf5.h
    ${CMAKE_CURRENT_SOURCE_DIR}/parser-image.h
    ${CMAKE_CURRENT_SOURCE_DIR}/parser-yaml.h
    ${CMAKE_CURRENT_SOURCE_DIR}/parser-xml.h)

set(INCLUDEDIRS
    ${CMAKE_CURRENT_SOURCE_DIR}/)

set(parsers_SOURCES ${SOURCES} PARENT_SCOPE)
set(parsers_HEADERS ${HEADERS} PARENT_SCOPE)
set(parsers_INCLUDEDIRS ${INCLUDEDIRS} PARENT_SCOPE)
