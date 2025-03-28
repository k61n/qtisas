cmake_minimum_required(VERSION 3.10)

if (${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.12")
    cmake_policy(SET CMP0074 NEW)
endif ()

project(minigzip C)

find_package(ZLIB REQUIRED)
include_directories(${ZLIB_INCLUDE_DIRS})

set(SOURCES
    minigzip.c)

add_definitions(-DMINIGZIP_EXPORTS=1)

add_library(minigzip_static STATIC ${SOURCES})
if (MSVC AND BUILD_SHARED_LIBS)
    set_target_properties(minigzip_static PROPERTIES OUTPUT_NAME "minigzip_static")
else ()
    set_target_properties(minigzip_static PROPERTIES OUTPUT_NAME "minigzip")
endif ()
target_link_libraries(minigzip_static PUBLIC ZLIB::ZLIB)

if (BUILD_SHARED_LIBS)
    add_library(minigzip_shared SHARED $<TARGET_OBJECTS:minigzip_static>)
    set_target_properties(minigzip_shared PROPERTIES OUTPUT_NAME "minigzip")
    target_link_libraries(minigzip_shared PUBLIC ZLIB::ZLIB)
endif ()

include(GNUInstallDirs)
install(TARGETS minigzip_static
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
if (BUILD_SHARED_LIBS)
    install(TARGETS minigzip_shared
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
endif ()
