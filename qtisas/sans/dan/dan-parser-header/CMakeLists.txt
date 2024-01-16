#
# Project: QtiSAS
# License: GNU GPL Version 3 (see LICENSE)
# Copyright (C) by the authors:
#   2023 Vitaliy Pipich <v.pipich@gmail.com>
# Description: cmake subfolder for Header Parser
#

set(SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/dan-parser-header.cpp)

set(HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/dan-parser-header.h)

set(dan-parser-header_SOURCES ${SOURCES} PARENT_SCOPE)
set(dan-parser-header_HEADERS ${HEADERS} PARENT_SCOPE)
