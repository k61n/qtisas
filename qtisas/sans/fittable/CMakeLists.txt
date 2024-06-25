
set(SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/fittable18.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/fittable-data.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/fittable-explorer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/fittable-fitting.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/fittable-fitting-fitting.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/fittable-init.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/fittable-simulate.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/fittable-simulate-simulate.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/fitting.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/glaguerre.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/glegendre.cpp)

set(HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/fittable18.h
    ${CMAKE_CURRENT_SOURCE_DIR}/fitting.h)

set(UI_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/fittable.ui)

set(fittable_SOURCES ${SOURCES} PARENT_SCOPE)
set(fittable_HEADERS ${HEADERS} PARENT_SCOPE)
set(fittable_UIS ${UI_SOURCES} PARENT_SCOPE)
