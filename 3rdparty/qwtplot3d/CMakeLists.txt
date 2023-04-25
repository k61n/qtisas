cmake_minimum_required(VERSION 3.24)
project(qwtplot3d)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt5 COMPONENTS Core OpenGL Widgets REQUIRED)
find_package(OpenGL REQUIRED)
find_package(PNG REQUIRED)

include_directories(
        ${PROJECT_SOURCE_DIR}/include
        ${PROJECT_SOURCE_DIR}/3rdparty/gl2ps
        ${OPENGL_INCLUDE_DIRS})

set(SOURCES
    src/qwt3d_autoscaler.cpp
    src/qwt3d_axis.cpp
    src/qwt3d_color.cpp
    src/qwt3d_colorlegend.cpp
    src/qwt3d_coordsys.cpp
    src/qwt3d_curve.cpp
    src/qwt3d_dataviews.cpp
    src/qwt3d_drawable.cpp
    src/qwt3d_enrichment_std.cpp
    src/qwt3d_extglwidget.cpp
    src/qwt3d_function.cpp
    src/qwt3d_gridmapping.cpp
    src/qwt3d_gridplot.cpp
    src/qwt3d_io_gl2ps.cpp
    src/qwt3d_io_reader.cpp
    src/qwt3d_io.cpp
    src/qwt3d_label.cpp
    src/qwt3d_lighting.cpp
    src/qwt3d_meshplot.cpp
    src/qwt3d_mousekeyboard.cpp
    src/qwt3d_movements.cpp
    src/qwt3d_parametricsurface.cpp
    src/qwt3d_plot.cpp
    src/qwt3d_scale.cpp
#    src/qwt3d_surfaceplot.cpp
    src/qwt3d_types.cpp
    3rdparty/gl2ps/gl2ps.c)

set(HEADERS
    include/qwt3d_autoptr.h
    include/qwt3d_autoscaler.h
    include/qwt3d_axis.h
    include/qwt3d_color.h
    include/qwt3d_colorlegend.h
    include/qwt3d_coordsys.h
    include/qwt3d_curve.h
    include/qwt3d_drawable.h
    include/qwt3d_enrichment_std.h
    include/qwt3d_enrichment.h
    include/qwt3d_extglwidget.h
    include/qwt3d_function.h
    include/qwt3d_global.h
    include/qwt3d_graphplot.h
    include/qwt3d_gridmapping.h
    include/qwt3d_helper.h
    include/qwt3d_io_gl2ps.h
    include/qwt3d_io_reader.h
    include/qwt3d_io.h
    include/qwt3d_label.h
    include/qwt3d_mapping.h
    include/qwt3d_multiplot.h
    include/qwt3d_openglhelper.h
    include/qwt3d_parametricsurface.h
    include/qwt3d_plot.h
    include/qwt3d_portability.h
    include/qwt3d_scale.h
#    include/qwt3d_surfaceplot.h
    include/qwt3d_types.h
    include/qwt3d_volumeplot.h
    3rdparty/gl2ps/gl2ps.h)

add_library(qwtplot3d ${SOURCES} ${HEADERS})
target_include_directories(qwtplot3d PUBLIC ${Qt5Core_INCLUDE_DIRS} ${Qt5OpenGL_INCLUDE_DIRS} ${Qt5Widgets_INCLUDE_DIRS} ${OPENGL_INCLUDE_DIRS})
target_link_libraries(qwtplot3d PUBLIC
        Qt5::Core Qt5::OpenGL Qt5::Widgets
        OpenGL::GL OpenGL::GLU PNG::PNG)

include(GNUInstallDirs)
install(TARGETS qwtplot3d
        EXPORT qwtplot3dTargets
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(FILES ${HEADERS}
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/qwtplot3d)
