cmake_minimum_required(VERSION 3.10)

if (${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.12")
    cmake_policy(SET CMP0074 NEW)
endif ()

project(tamuanova C)

find_package(GSL REQUIRED)
include_directories(${PROJECT_SOURCE_DIR}/include)

set(SOURCES
    anova_1.c
    anova_2.c)

set(HEADERS
    tamu_anova.h)

add_definitions(-DTAMUANOVA_EXPORTS=1)

add_library(tamuanova_static STATIC ${SOURCES})
if (MSVC AND BUILD_SHARED_LIBS)
    set_target_properties(tamuanova_static PROPERTIES OUTPUT_NAME "tamuanova_static")
else ()
    set_target_properties(tamuanova_static PROPERTIES OUTPUT_NAME "tamuanova")
endif ()
target_link_libraries(tamuanova_static PUBLIC GSL::gsl GSL::gslcblas)

if (BUILD_SHARED_LIBS)
    add_library(tamuanova_shared SHARED $<TARGET_OBJECTS:tamuanova_static>)
    set_target_properties(tamuanova_shared PROPERTIES OUTPUT_NAME "tamuanova")
    target_link_libraries(tamuanova_shared PUBLIC GSL::gsl GSL::gslcblas)
endif ()

include(GNUInstallDirs)
install(TARGETS tamuanova_static
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
if (BUILD_SHARED_LIBS)
    install(TARGETS tamuanova_shared
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
endif ()
install(FILES ${HEADERS}
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/tamuanova)
