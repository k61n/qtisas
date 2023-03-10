
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

    include/
    3rdparty/qtcolorpicker/src/
    ../core/
    ../matrix/
    ../scripting/)

set(SOURCES
    src/CollapsiveGroupBox.cpp
    src/ColorBox.cpp
    src/ColorButton.cpp
    src/ColorMapEditor.cpp
    src/DoubleSpinBox.cpp
    src/ExtensibleFileDialog.cpp
    src/LinearColorMap.cpp
    src/LineNumberDisplay.cpp
    src/PatternBox.cpp
    src/PenStyleBox.cpp
    src/SymbolBox.cpp
    src/SymbolDialog.cpp
    src/TextFormatButtons.cpp
    3rdparty/qtcolorpicker/src/qtcolorpicker.cpp)

set(HEADERS
    include/CollapsiveGroupBox.h
    include/ColorBox.h
    include/ColorButton.h
    include/ColorMapEditor.h
    include/DoubleSpinBox.h
    include/ExtensibleFileDialog.h
    include/LinearColorMap.h
    include/LineNumberDisplay.h
    include/PatternBox.h
    include/PenStyleBox.h
    include/SymbolBox.h
    include/SymbolDialog.h
    include/TextFormatButtons.h
    3rdparty/qtcolorpicker/src/qtcolorpicker.h)

QT4_WRAP_CPP(MOCS ${HEADERS})

add_library(lib STATIC ${SOURCES} ${MOCS})
target_link_libraries(lib PUBLIC
    ${GSL_LIBRARY} ${MUPARSER_LIBRARY} ${QWT_LIBRARY}
    Qt4::QtGui Qt4::Qt3Support)
