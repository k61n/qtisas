
set(SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/compile18.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/compile-code-menu.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/compile-explorer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/compile-info.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/compile-init.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/compile-parameters.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/compile-sasview.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/compile-settings.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/highlighter.cpp)

set(HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/compile18.h
    ${CMAKE_CURRENT_SOURCE_DIR}/highlighter.h)

set(UI_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/compile.ui)

set(INCLUDEDIRS
    ${CMAKE_CURRENT_SOURCE_DIR}/)

set(compile_SOURCES ${SOURCES} PARENT_SCOPE)
set(compile_HEADERS ${HEADERS} PARENT_SCOPE)
set(compile_UIS ${UI_SOURCES} PARENT_SCOPE)
set(compile_INCLUDEDIRS ${INCLUDEDIRS} PARENT_SCOPE)
