
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)

set(OS ${CMAKE_SYSTEM_NAME})
set(ARCH ${CMAKE_SYSTEM_PROCESSOR})

find_package(Qt4 COMPONENTS QtGui Qt3Support REQUIRED)
find_library(GSL_LIBRARY NAMES gsl PATHS ${CMAKE_SOURCE_DIR}/libs/${OS}-${ARCH}/gsl/lib NO_DEFAULT_PATH)
find_library(QWT_LIBRARY NAMES qwt PATHS ${CMAKE_SOURCE_DIR}/libs/${OS}-${ARCH}/qwt/lib NO_DEFAULT_PATH)

include_directories(
    ${CMAKE_SOURCE_DIR}/libs/${OS}-${ARCH}/gsl/include/
    ${CMAKE_SOURCE_DIR}/libs/${OS}-${ARCH}/qwt/include/qwt

    ../../src/core/
    ../../src/lib/include/
    ../../src/plot2D/
    ../../src/plot2D/dialogs/
    ../../src/scripting/
    ../../src/table/)

set(SOURCES
    svd.cpp)

set(HEADERS
    svd.h)

set(UI_SOURCES
    svd.ui)

QT4_WRAP_CPP(MOCS ${HEADERS})

qt4_wrap_ui(UI_HEADERS ${UI_SOURCES})

add_library(svd STATIC ${SOURCES} ${MOCS} ${UI_HEADERS})
target_link_libraries(svd PUBLIC
    ${GSL_LIBRARY} ${QWT_LIBRARY}
    Qt4::QtGui Qt4::Qt3Support)
