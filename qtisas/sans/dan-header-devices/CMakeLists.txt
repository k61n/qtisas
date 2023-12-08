#
# Project: QtiSAS
# License: GNU GPL Version 3 (see LICENSE)
# Copyright (C) by the author(s):
#   2023 Vitaliy Pipich <v.pipich@gmail.com>
# Description: cmake subfolder for Dan-File-Manager
#

set(SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/monitors.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/selector.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/sample.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/collimation.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/detector.cpp)

set(HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/monitors.h
    ${CMAKE_CURRENT_SOURCE_DIR}/selector.h
    ${CMAKE_CURRENT_SOURCE_DIR}/sample.h
    ${CMAKE_CURRENT_SOURCE_DIR}/collimation.h
    ${CMAKE_CURRENT_SOURCE_DIR}/detector.h)

set(dan-header-devices_SOURCES ${SOURCES} PARENT_SCOPE)
set(dan-header-devices_HEADERS ${HEADERS} PARENT_SCOPE)
