cmake_minimum_required(VERSION 3.24)
project(qtexengine)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt4 COMPONENTS QtCore QtGui REQUIRED)

set(SOURCES
    src/QTeXPaintDevice.cpp
    src/QTeXPaintEngine.cpp)

set(HEADERS
    src/QTeXEngine.h)

add_library(qtexengine ${SOURCES} ${HEADERS})
target_include_directories(qtexengine PUBLIC ${QtCore_INCLUDE_DIRS} ${QtGui_INCLUDE_DIRS})
target_link_libraries(qtexengine PUBLIC Qt4::QtCore Qt4::QtGui)

include(GNUInstallDirs)
install(TARGETS qtexengine
        EXPORT qtexengineTargets
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(FILES ${HEADERS}
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/qtexengine)
