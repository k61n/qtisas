
set(SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/combobox-in-table.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/fit-function-explorer.cpp)

set(HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/combobox-in-table.h
    ${CMAKE_CURRENT_SOURCE_DIR}/msleep.h
    ${CMAKE_CURRENT_SOURCE_DIR}/fit-function-explorer.h)

set(INCLUDEDIRS
    ${CMAKE_CURRENT_SOURCE_DIR}/)

set(common_SOURCES ${SOURCES} PARENT_SCOPE)
set(common_HEADERS ${HEADERS} PARENT_SCOPE)
set(common_INCLUDEDIRS ${INCLUDEDIRS} PARENT_SCOPE)
