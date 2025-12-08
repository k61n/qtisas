#
# Project: QtiSAS
# License: GNU GPL Version 3 (see LICENSE)
# Copyright (C) by the authors:
#   2025 Vitaliy Pipich <v.pipich@gmail.com>
# Description: cmake subfolder for SANS instruments
#

set(SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/dan-sans-instruments.cpp)

set(HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/dan-sans-instruments.h)

set(dan-sans-instruments_SOURCES ${SOURCES} PARENT_SCOPE)
set(dan-sans-instruments_HEADERS ${HEADERS} PARENT_SCOPE)
