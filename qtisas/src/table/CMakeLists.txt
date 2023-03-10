
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)

set(OS ${CMAKE_SYSTEM_NAME})
set(ARCH ${CMAKE_SYSTEM_PROCESSOR})

find_package(Qt4 COMPONENTS QtGui Qt3Support REQUIRED)
find_library(GSL_LIBRARY NAMES gsl PATHS ${CMAKE_SOURCE_DIR}/libs/${OS}-${ARCH}/gsl/lib NO_DEFAULT_PATH)
find_library(MUPARSER_LIBRARY NAMES muparser PATHS ${CMAKE_SOURCE_DIR}/libs/${OS}-${ARCH}/muparser/lib NO_DEFAULT_PATH)
find_library(QWT_LIBRARY NAMES qwt PATHS ${CMAKE_SOURCE_DIR}/libs/${OS}-${ARCH}/qwt/lib NO_DEFAULT_PATH)

include_directories(
    ${CMAKE_SOURCE_DIR}/libs/${OS}-${ARCH}/gsl/include/
    ${CMAKE_SOURCE_DIR}/libs/${OS}-${ARCH}/muparser/include/muparser/
    ${CMAKE_SOURCE_DIR}/libs/${OS}-${ARCH}/qwt/include/qwt/

    ../core/
    ../lib/include/
    ../matrix/
    ../plot2D/
    ../plot2D/dialogs/
    ../scripting/)

set(SOURCES
    ExportDialog.cpp
    ExtractDataDialog.cpp
    SetColValuesDialog.cpp
    SortDialog.cpp
    Table.cpp
    TableDialog.cpp
    TableStatistics.cpp)

set(HEADERS
    ExportDialog.h
    ExtractDataDialog.h
    SetColValuesDialog.h
    SortDialog.h
    Table.h
    TableDialog.h
    TableStatistics.h)

QT4_WRAP_CPP(MOCS ${HEADERS})

add_library(table STATIC ${SOURCES} ${MOCS})
target_link_libraries(table PUBLIC
    ${GSL_LIBRARY} ${MUPARSER_LIBRARY} ${QWT_LIBRARY}
    Qt4::QtGui Qt4::Qt3Support)
