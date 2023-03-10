
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)

set(OS ${CMAKE_SYSTEM_NAME})
set(ARCH ${CMAKE_SYSTEM_PROCESSOR})

find_package(Qt4 COMPONENTS QtGui Qt3Support REQUIRED)
find_library(GSL_LIBRARY NAMES gsl PATHS ${CMAKE_SOURCE_DIR}/libs/${OS}-${ARCH}/gsl/lib NO_DEFAULT_PATH)
find_library(QWT_LIBRARY NAMES qwt PATHS ${CMAKE_SOURCE_DIR}/libs/${OS}-${ARCH}/qwt/lib NO_DEFAULT_PATH)

include_directories(
    ${CMAKE_SOURCE_DIR}/libs/${OS}-${ARCH}/gsl/include/
    ${CMAKE_SOURCE_DIR}/libs/${OS}-${ARCH}/qwt/include/qwt/

    ${CMAKE_SOURCE_DIR}/qtisas/src/core/
    ${CMAKE_SOURCE_DIR}/qtisas/src/lib/include/
    ${CMAKE_SOURCE_DIR}/qtisas/src/lib/3rdparty/qtcolorpicker/src/
    ${CMAKE_SOURCE_DIR}/qtisas/src/plot2D/
    ${CMAKE_SOURCE_DIR}/qtisas/src/plot2D/dialogs/
    ${CMAKE_SOURCE_DIR}/qtisas/src/scripting/
    ${CMAKE_SOURCE_DIR}/qtisas/src/table/

    ../genmin/include/
    ../jnse/

    ${CMAKE_CURRENT_BINARY_DIR}/../compile/
    ${CMAKE_CURRENT_BINARY_DIR}/../jnse/)

set(SOURCES
    ../genmin/src/genmin.cc
    ../genmin/src/get_options.cc
    ../genmin/src/localMinGSL.cc
    ../genmin/src/multi_population.cc
    ../genmin/src/problem.cc
    ../genmin/src/program.cc
    ../genmin/src/rlsprogram.cc
    ../genmin/src/rule.cc
    ../genmin/src/symbol.cc
    ../genmin/src/tolmin.cc
    fittable18.cpp
    fittable-data.cpp
    fittable-explorer.cpp
    fittable-fitting.cpp
    fittable-fitting-fitting.cpp
    fittable-init.cpp
    fittable-simulate.cpp
    fittable-simulate-simulate.cpp
    fitting.cpp
    glaguerre.cpp
    glegendre.cpp)

set(HEADERS
    ../genmin/include/f2c.h
    ../genmin/include/genmin.h
    ../genmin/include/get_options.h
    ../genmin/include/multi_population.h
    ../genmin/include/problem.h
    ../genmin/include/program.h
    ../genmin/include/rlsprogram.h
    ../genmin/include/rule.h
    ../genmin/include/symbol.h
    ../genmin/include/tolmin.h
    fittable18.h
    fitting.h)

QT4_WRAP_CPP(MOCS ${HEADERS})

set(UI_SOURCES
    fittable.ui)

add_definitions(-DFITTABLE=1 -DJNSE=1)

qt4_wrap_ui(UI_HEADERS ${UI_SOURCES})

add_library(fittable STATIC ${SOURCES} ${MOCS} ${UI_HEADERS})
add_dependencies(fittable compile jnse)
target_link_libraries(fittable PUBLIC
    ${GSL_LIBRARY} ${QWT_LIBRARY}
    Qt4::QtGui Qt4::Qt3Support)
