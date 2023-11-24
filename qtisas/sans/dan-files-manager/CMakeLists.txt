#
# Project: QtiSAS
# License: GNU GPL Version 3 (see LICENSE)
# Copyright (C) by the author(s):
#   2023 Vitaliy Pipich <v.pipich@gmail.com>
# Description: cmake subfolder for Dan-File-Manager
#

set(SOURCES
${CMAKE_CURRENT_SOURCE_DIR}/dan-files-manager.cpp)

set(HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/dan-files-manager.h)

set(dan-files-manager_SOURCES ${SOURCES} PARENT_SCOPE)
set(dan-files-manager_HEADERS ${HEADERS} PARENT_SCOPE)
