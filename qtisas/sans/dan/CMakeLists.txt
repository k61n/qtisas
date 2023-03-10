
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)

set(OS ${CMAKE_SYSTEM_NAME})
set(ARCH ${CMAKE_SYSTEM_PROCESSOR})

find_package(Qt4 COMPONENTS QtGui QtXml Qt3Support REQUIRED)
find_library(GSL_LIBRARY NAMES gsl PATHS ${CMAKE_SOURCE_DIR}/libs/${OS}-${ARCH}/gsl/lib NO_DEFAULT_PATH)
find_library(MUPARSER_LIBRARY NAMES muparser PATHS ${CMAKE_SOURCE_DIR}/libs/${OS}-${ARCH}/muparser/lib NO_DEFAULT_PATH)
find_library(QTEXENGINE_LIBRARY NAMES qtexengine PATHS ${CMAKE_SOURCE_DIR}/libs/${OS}-${ARCH}/qtexengine/lib NO_DEFAULT_PATH)
find_library(QWT_LIBRARY NAMES qwt PATHS ${CMAKE_SOURCE_DIR}/libs/${OS}-${ARCH}/qwt/lib NO_DEFAULT_PATH)
find_library(TIFF_LIBRARY NAMES tiff PATHS ${CMAKE_SOURCE_DIR}/libs/${OS}-${ARCH}/tiff/lib NO_DEFAULT_PATH)
find_library(YAML-CPP_LIBRARY NAMES yaml-cpp PATHS ${CMAKE_SOURCE_DIR}/libs/${OS}-${ARCH}/yaml-cpp/lib NO_DEFAULT_PATH)

include_directories(
    ${CMAKE_SOURCE_DIR}/libs/${OS}-${ARCH}/gsl/include/
    ${CMAKE_SOURCE_DIR}/libs/${OS}-${ARCH}/muparser/include/muparser/
    ${CMAKE_SOURCE_DIR}/libs/${OS}-${ARCH}/qtexengine/include/qtexengine/
    ${CMAKE_SOURCE_DIR}/libs/${OS}-${ARCH}/qwt/include/qwt/
    ${CMAKE_SOURCE_DIR}/libs/${OS}-${ARCH}/tiff/include/
    ${CMAKE_SOURCE_DIR}/libs/${OS}-${ARCH}/yaml-cpp/include/

    ../../src/core/
    ../../src/lib/include/
    ../../src/matrix/
    ../../src/plot2D/
    ../../src/plot2D/dialogs/
    ../../src/scripting/
    ../../src/table/

    ${CMAKE_CURRENT_BINARY_DIR}/../compile/)

set(SOURCES
    dan18.cpp
    dan18-init.cpp
    dan18-mask.cpp
    dan18-merge1d.cpp
    dan18-options.cpp
    dan18-options-header.cpp
    dan18-process.cpp
    dan18-process-dandan.cpp
    dan18-process-matrix.cpp
    dan18-rawdata-addfiles.cpp
    dan18-rawdata-extractor.cpp
    dan18-rawdata-realtime.cpp
    dan18-rawdata-tof.cpp
    dan18-rawdata-tools.cpp
    dan18-sens.cpp
    dan18-settings.cpp
    dan-init-interface.cpp
    deadTimeRoot.cpp)

set(HEADERS
    dan18.h
    deadTimeRoot.h)

QT4_WRAP_CPP(MOCS ${HEADERS})

set(UI_SOURCES
    dan.ui)

qt4_wrap_ui(UI_HEADERS ${UI_SOURCES})

add_definitions(-DTIFFTIFF=1 -DYAMLYAML=1)

add_library(dan STATIC ${SOURCES} ${MOCS} ${UI_HEADERS})
add_dependencies(dan common compile)
target_link_libraries(dan PUBLIC common
    ${GSL_LIBRARY} ${MUPARSER_LIBRARY} ${QTEXENGINE_LIBRARY} ${QWT_LIBRARY} ${TIFF_LIBRARY} ${YAML-CPP_LIBRARY}
    Qt4::QtGui Qt4::QtXml Qt4::Qt3Support)
