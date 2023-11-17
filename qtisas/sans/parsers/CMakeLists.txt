#
# Project: QtiSAS
# License: GNU GPL Version 3 (see LICENSE)
# Copyright (C) by the authors:
#   2023 Vitaliy Pipich <v.pipich@gmail.com>
# Description: cmake subfolder for Parsers
#

set(SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/parser-xml.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/parser-yaml.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/parser-hdf5.cpp)

set(HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/parser-xml.h
    ${CMAKE_CURRENT_SOURCE_DIR}/parser-yaml.h
    ${CMAKE_CURRENT_SOURCE_DIR}/parser-hdf5.h)

set(parsers_SOURCES ${SOURCES} PARENT_SCOPE)
set(parsers_HEADERS ${HEADERS} PARENT_SCOPE)
