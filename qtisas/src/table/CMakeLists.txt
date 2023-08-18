#
# Project: QtiSAS
# License: GNU GPL Version 3 (see LICENSE)
# Copyright (C) by the authors:
#   2023 Konstantin Kholostov <k.kholostov@fz-juelich.de>
# Description: cmake subfolder for Table
#

set(SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/ExportDialog.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ExtractDataDialog.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Header.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Myselection.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Mytable.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/SetColValuesDialog.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/SortDialog.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Table.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/TableDialog.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/TableStatistics.cpp)

set(HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/ExportDialog.h
    ${CMAKE_CURRENT_SOURCE_DIR}/ExtractDataDialog.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Header.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Myselection.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Mytable.h
    ${CMAKE_CURRENT_SOURCE_DIR}/SetColValuesDialog.h
    ${CMAKE_CURRENT_SOURCE_DIR}/SortDialog.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Table.h
    ${CMAKE_CURRENT_SOURCE_DIR}/TableDialog.h
    ${CMAKE_CURRENT_SOURCE_DIR}/TableStatistics.h)

set(table_SOURCES ${SOURCES} PARENT_SCOPE)
set(table_HEADERS ${HEADERS} PARENT_SCOPE)
