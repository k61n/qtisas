
set(SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/dan18.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/dan18-init.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/dan18-mask.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/dan18-merge1d.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/dan18-options.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/dan18-options-header.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/dan18-process.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/dan18-process-dandan.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/dan18-process-matrix.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/dan18-rawdata-addfiles.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/dan18-rawdata-extractor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/dan18-rawdata-realtime.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/dan18-rawdata-tof.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/dan18-rawdata-tools.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/dan18-sens.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/dan18-settings.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/deadTimeRoot.cpp)

set(HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/dan18.h
    ${CMAKE_CURRENT_SOURCE_DIR}/deadTimeRoot.h)

set(UI_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/dan.ui)

set(dan_SOURCES ${SOURCES} PARENT_SCOPE)
set(dan_HEADERS ${HEADERS} PARENT_SCOPE)
set(dan_UIS ${UI_SOURCES} PARENT_SCOPE)
