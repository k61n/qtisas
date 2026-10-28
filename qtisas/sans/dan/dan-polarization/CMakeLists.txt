#
# Project: QtiSAS
# License: GNU GPL Version 3 (see LICENSE)
# Copyright (C) by the author(s):
#   2026 Vitaliy Pipich <v.pipich@gmail.com>
# Description: cmake subfolder for Dan-Polarization (the PN-mode corrections)
#

set(SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/dan-polarization-corrections.cpp)

set(HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/dan-polarization-corrections.h)

set(dan-polarization_SOURCES ${SOURCES} PARENT_SCOPE)
set(dan-polarization_HEADERS ${HEADERS} PARENT_SCOPE)
