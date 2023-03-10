
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)

set(OS ${CMAKE_SYSTEM_NAME})
set(ARCH ${CMAKE_SYSTEM_PROCESSOR})

find_package(OpenGL REQUIRED)
find_package(Qt4 COMPONENTS QtGui QtOpenGL Qt3Support REQUIRED)
find_library(GSL_LIBRARY NAMES gsl PATHS ${CMAKE_SOURCE_DIR}/libs/${OS}-${ARCH}/gsl/lib NO_DEFAULT_PATH)
find_library(MUPARSER_LIBRARY NAMES muparser PATHS ${CMAKE_SOURCE_DIR}/libs/${OS}-${ARCH}/muparser/lib NO_DEFAULT_PATH)
find_library(QWT_LIBRARY NAMES qwt PATHS ${CMAKE_SOURCE_DIR}/libs/${OS}-${ARCH}/qwt/lib NO_DEFAULT_PATH)
find_library(QWTPLOT3D_LIBRARY NAMES qwtplot3d PATHS ${CMAKE_SOURCE_DIR}/libs/${OS}-${ARCH}/qwtplot3d/lib NO_DEFAULT_PATH)

include_directories(
    ${CMAKE_SOURCE_DIR}/libs/${OS}-${ARCH}/gsl/include/
    ${CMAKE_SOURCE_DIR}/libs/${OS}-${ARCH}/muparser/include/muparser/
    ${CMAKE_SOURCE_DIR}/libs/${OS}-${ARCH}/qwt/include/qwt/
    ${CMAKE_SOURCE_DIR}/libs/${OS}-${ARCH}/qwtplot3d/include/qwtplot3d/

    ../core/
    ../lib/3rdparty/qtcolorpicker/src/
    ../lib/include/
    ../matrix/
    ../plot2D/
    ../plot2D/dialogs/
    ../scripting/
    ../table/)

set(SOURCES
    Bar.cpp
    ColorMapPreviewDialog.cpp
    Cone3D.cpp
    Graph3D.cpp
    Plot3DDialog.cpp
    SurfaceDialog.cpp)

set(HEADERS
    Bar.h
    ColorMapPreviewDialog.h
    Cone3D.h
    Graph3D.h
    Plot3DDialog.h
    SurfaceDialog.h)

QT4_WRAP_CPP(MOCS ${HEADERS})

add_library(plot3D STATIC ${SOURCES} ${MOCS})
target_link_libraries(plot3D PUBLIC
    ${GSL_LIBRARY} ${MUPARSER_LIBRARY} ${QWT_LIBRARY} ${QWTPLOT3D_LIBRARY}
    Qt4::QtGui Qt4::QtOpenGL Qt4::Qt3Support
    OpenGL::GLU)
