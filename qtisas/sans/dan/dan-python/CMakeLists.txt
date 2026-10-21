#
# Project: QtiSAS
# License: GNU GPL Version 3 (see LICENSE)
# Copyright (C) by the author(s):
#   2026 Vitaliy Pipich <v.pipich@gmail.com>
# Description: cmake subfolder for the DAN-SANS Python interface
#

set(SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/dan-python.cpp)

set(HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/dan-python.h)

set(dan-python_SOURCES ${SOURCES} PARENT_SCOPE)
set(dan-python_HEADERS ${HEADERS} PARENT_SCOPE)
