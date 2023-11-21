cmake_minimum_required(VERSION 3.24)

project(minigzip C)

find_package(ZLIB REQUIRED)
include_directories(${ZLIB_INCLUDE_DIRS})

set(SOURCES
    minigzip.c)

add_library(minigzip ${SOURCES})
target_include_directories(minigzip PUBLIC ${ZLIB_INCLUDE_DIRS})
target_link_libraries(minigzip PUBLIC ZLIB::ZLIB)

include(GNUInstallDirs)
install(TARGETS minigzip
    EXPORT minigzipTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
