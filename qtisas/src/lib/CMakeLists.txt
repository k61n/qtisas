
set(SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/CollapsibleGroupBox.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ColorBox.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ColorButton.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ColorMapEditor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/DoubleSpinBox.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ExtensibleFileDialog.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/LinearColorMap.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/LineNumberDisplay.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/PatternBox.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/PenStyleBox.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/SymbolBox.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/SymbolDialog.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/TextFormatButtons.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/qtcolorpicker/src/qtcolorpicker.cpp)

set(HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/include/CollapsibleGroupBox.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/ColorBox.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/ColorButton.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/ColorMapEditor.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/DoubleSpinBox.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/ExtensibleFileDialog.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/LinearColorMap.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/LineNumberDisplay.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/PatternBox.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/PenStyleBox.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/SymbolBox.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/SymbolDialog.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/TextFormatButtons.h
    ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/qtcolorpicker/src/qtcolorpicker.h)

set(INCLUDEDIRS
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/qtcolorpicker/src)

set(lib_SOURCES ${SOURCES} PARENT_SCOPE)
set(lib_HEADERS ${HEADERS} PARENT_SCOPE)
set(lib_INCLUDEDIRS ${INCLUDEDIRS} PARENT_SCOPE)
