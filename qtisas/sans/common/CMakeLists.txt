
set(OS ${CMAKE_SYSTEM_NAME})
set(ARCH ${CMAKE_SYSTEM_PROCESSOR})

find_package(Qt4 COMPONENTS QtCore QtGui Qt3Support REQUIRED)
find_library(QWT_LIBRARY NAMES qwt PATHS ${CMAKE_SOURCE_DIR}/libs/${OS}-${ARCH}/qwt/lib NO_DEFAULT_PATH)

include_directories(
    ${CMAKE_SOURCE_DIR}/libs/${OS}-${ARCH}/qwt/include/qwt)

set(SOURCES
    combobox-in-table.cpp)

set(HEADERS
    combobox-in-table.h
    msleep.h)

QT4_WRAP_CPP(MOCS ${HEADERS})

add_library(common STATIC ${SOURCES} ${MOCS})
target_link_libraries(common PUBLIC
    ${QWT_LIBRARY}
    Qt4::QtCore Qt4::QtGui Qt4::Qt3Support)
