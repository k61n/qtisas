#
# Project: QtiSAS
# License: GNU GPL Version 3 (see LICENSE)
# Copyright (C) by the author(s):
#   2024 Vitaliy Pipich <v.pipich@gmail.com>
# Description: cmake subfolder for Dan-Configuration-Widgets
#

set(SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/dan-conf-selector-const-table-header.cpp)

set(HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/dan-conf-selector-const-table-header.h)

set(dan-configuration-widgets_SOURCES ${SOURCES} PARENT_SCOPE)
set(dan-configuration-widgets_HEADERS ${HEADERS} PARENT_SCOPE)
