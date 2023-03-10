
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)

set(OS ${CMAKE_SYSTEM_NAME})
set(ARCH ${CMAKE_SYSTEM_PROCESSOR})

find_package(Qt4 COMPONENTS QtGui Qt3Support REQUIRED)
find_library(QWT_LIBRARY NAMES qwt PATHS ${CMAKE_SOURCE_DIR}/libs/${OS}-${ARCH}/qwt/lib NO_DEFAULT_PATH)

include_directories(
    ${CMAKE_SOURCE_DIR}/libs/${OS}-${ARCH}/qwt/include/qwt/

    ../../src/core/
    ../../src/lib/include/
    ../../src/plot2D/
    ../../src/plot2D/dialogs/
    ../../src/scripting/
    ../../src/table/)

set(SOURCES
    compile18.cpp
    compile-code-menu.cpp
    compile-explorer.cpp
    compile-info.cpp
    compile-init.cpp
    compile-parameters.cpp
    compile-settings.cpp
    highlighter.cpp)

set(HEADERS
    compile18.h
    highlighter.h)

set(UI_SOURCES
    compile.ui)

QT4_WRAP_CPP(MOCS ${HEADERS})

qt4_wrap_ui(UI_HEADERS ${UI_SOURCES})

add_library(compile STATIC ${SOURCES} ${MOCS} ${UI_HEADERS})
target_link_libraries(compile PUBLIC
    ${QWT_LIBRARY}
    Qt4::QtGui Qt4::Qt3Support)
