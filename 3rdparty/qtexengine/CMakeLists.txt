cmake_minimum_required(VERSION 3.24)
project(qtexengine)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt4 COMPONENTS QtCore QtGui REQUIRED)

set(SOURCES
    src/QTeXPaintDevice.cpp
    src/QTeXPaintEngine.cpp
)

set(HEADERS
    src/QTeXEngine.h
)

add_library(qtexengine ${SOURCES} ${HEADERS})
set_target_properties(qtexengine PROPERTIES LINK_SEARCH_START_STATIC 1)
target_include_directories(qtexengine PUBLIC ${QtCore_INCLUDE_DIRS} ${QtGui_INCLUDE_DIRS})
target_link_libraries(qtexengine PUBLIC Qt4::QtCore Qt4::QtGui)
