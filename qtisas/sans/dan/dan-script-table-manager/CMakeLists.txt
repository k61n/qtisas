#
# Project: QtiSAS
# License: GNU GPL Version 3 (see LICENSE)
# Copyright (C) by the author(s):
#   2024 Vitaliy Pipich <v.pipich@gmail.com>
# Description: cmake subfolder for Dan-Script-Table-Manager
#

set(SOURCES
${CMAKE_CURRENT_SOURCE_DIR}/dan-script-table-manager.cpp)

set(HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/dan-script-table-manager.h)

set(dan-script-table-manager_SOURCES ${SOURCES} PARENT_SCOPE)
set(dan-script-table-manager_HEADERS ${HEADERS} PARENT_SCOPE)
