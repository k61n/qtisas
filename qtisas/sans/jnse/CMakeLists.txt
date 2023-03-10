
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
    jnse18.cpp)

set(HEADERS
    jnse18.h)

set(UI_SOURCES
    jnse.ui)

QT4_WRAP_CPP(MOCS ${HEADERS})

qt4_wrap_ui(UI_HEADERS ${UI_SOURCES})

add_library(jnse STATIC ${SOURCES} ${MOCS} ${UI_HEADERS})
target_link_libraries(jnse PUBLIC
    ${QWT_LIBRARY}
    Qt4::QtGui Qt4::Qt3Support)
