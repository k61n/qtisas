cmake_minimum_required(VERSION 3.10)

project(alglib)

set(CMAKE_CXX_STANDARD 17)

file(GLOB_RECURSE SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")
file(GLOB_RECURSE HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h")

if (MSVC)
    add_compile_options($<$<CXX_COMPILER_ID:MSVC>:/MP>)
endif ()

add_library(alglib_static STATIC ${SOURCES} ${HEADERS})
#if (MSVC AND BUILD_SHARED_LIBS)
#    set_target_properties(alglib_static PROPERTIES OUTPUT_NAME "alglib_static")
#else ()
    set_target_properties(alglib_static PROPERTIES OUTPUT_NAME "alglib")
#endif ()
target_include_directories(alglib_static PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

#if (BUILD_SHARED_LIBS)
#    add_library(alglib_shared SHARED $<TARGET_OBJECTS:alglib_static>)
#    set_target_properties(alglib_shared PROPERTIES OUTPUT_NAME "alglib")
#    target_include_directories(alglib_shared PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
#endif ()

include(GNUInstallDirs)
install(TARGETS alglib_static
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
#if (BUILD_SHARED_LIBS)
#    install(TARGETS alglib_shared
#        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
#endif ()
install(FILES ${HEADERS}
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/alglib)
