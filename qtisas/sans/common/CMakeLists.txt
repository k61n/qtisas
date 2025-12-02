
set(SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/combobox-in-table.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/fit-function-explorer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/highlighter.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/repository-synchronization.cpp)

set(HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/combobox-in-table.h
    ${CMAKE_CURRENT_SOURCE_DIR}/naturalsort.h
    ${CMAKE_CURRENT_SOURCE_DIR}/fit-function-explorer.h
    ${CMAKE_CURRENT_SOURCE_DIR}/highlighter.h
    ${CMAKE_CURRENT_SOURCE_DIR}/repository-synchronization.h)

set(INCLUDEDIRS
    ${CMAKE_CURRENT_SOURCE_DIR}/)

set(common_SOURCES ${SOURCES} PARENT_SCOPE)
set(common_HEADERS ${HEADERS} PARENT_SCOPE)
set(common_INCLUDEDIRS ${INCLUDEDIRS} PARENT_SCOPE)
