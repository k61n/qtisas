
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)

set(OS ${CMAKE_SYSTEM_NAME})
set(ARCH ${CMAKE_SYSTEM_PROCESSOR})

find_package(Qt4 COMPONENTS QtGui QtSvg Qt3Support REQUIRED)
find_library(ALGLIB_LIBRARY NAMES alglib PATHS ${CMAKE_SOURCE_DIR}/libs/${OS}-${ARCH}/alglib/lib NO_DEFAULT_PATH)
find_library(GSL_LIBRARY NAMES gsl PATHS ${CMAKE_SOURCE_DIR}/libs/${OS}-${ARCH}/gsl/lib NO_DEFAULT_PATH)
find_library(MUPARSER_LIBRARY NAMES muparser PATHS ${CMAKE_SOURCE_DIR}/libs/${OS}-${ARCH}/muparser/lib NO_DEFAULT_PATH)
find_library(QWT_LIBRARY NAMES qwt PATHS ${CMAKE_SOURCE_DIR}/libs/${OS}-${ARCH}/qwt/lib NO_DEFAULT_PATH)

include_directories(
    ${CMAKE_SOURCE_DIR}/libs/${OS}-${ARCH}/alglib/include/alglib/
    ${CMAKE_SOURCE_DIR}/libs/${OS}-${ARCH}/gsl/include/
    ${CMAKE_SOURCE_DIR}/libs/${OS}-${ARCH}/muparser/include/muparser/
    ${CMAKE_SOURCE_DIR}/libs/${OS}-${ARCH}/qwt/include/qwt/

    ../analysis/
    ../core/
    ../lib/include/
    ../plot2D/
    ../plot2D/dialogs/
    ../scripting/
    ../table/)

set(SOURCES
    ColorMapDialog.cpp
    Matrix.cpp
    MatrixCommand.cpp
    MatrixDialog.cpp
    MatrixModel.cpp
    MatrixResamplingDialog.cpp
    MatrixSizeDialog.cpp
    MatrixValuesDialog.cpp)

set(HEADERS
    ColorMapDialog.h
    Matrix.h
    MatrixCommand.h
    MatrixDialog.h
    MatrixModel.h
    MatrixResamplingDialog.h
    MatrixSizeDialog.h
    MatrixValuesDialog.h)

QT4_WRAP_CPP(MOCS ${HEADERS})

add_definitions(-DHAVE_ALGLIB=1)

add_library(matrix STATIC ${SOURCES} ${MOCS})
#add_dependencies(matrix scripting)
target_link_libraries(matrix PUBLIC
    ${ALGLIB_LIBRARY} ${GSL_LIBRARY} ${MUPARSER_LIBRARY} ${QWT_LIBRARY}
    Qt4::QtGui Qt4::QtSvg Qt4::Qt3Support)
