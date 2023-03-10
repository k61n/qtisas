
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)

set(OS ${CMAKE_SYSTEM_NAME})
set(ARCH ${CMAKE_SYSTEM_PROCESSOR})

find_package(Qt4 COMPONENTS QtCore QtOpenGL QtSvg Qt3Support REQUIRED)
find_library(GSL_LIBRARY NAMES gsl PATHS ${CMAKE_SOURCE_DIR}/libs/${OS}-${ARCH}/gsl/lib NO_DEFAULT_PATH)
find_library(MUPARSER_LIBRARY NAMES muparser PATHS ${CMAKE_SOURCE_DIR}/libs/${OS}-${ARCH}/muparser/lib NO_DEFAULT_PATH)
find_library(QTEXENGINE_LIBRARY NAMES qtexengine PATHS ${CMAKE_SOURCE_DIR}/libs/${OS}-${ARCH}/qtexengine/lib NO_DEFAULT_PATH)
find_library(QWT_LIBRARY NAMES qwt PATHS ${CMAKE_SOURCE_DIR}/libs/${OS}-${ARCH}/qwt/lib NO_DEFAULT_PATH)
find_library(QWTPLOT3D_LIBRARY NAMES qwtplot3d PATHS ${CMAKE_SOURCE_DIR}/libs/${OS}-${ARCH}/qwtplot3d/lib NO_DEFAULT_PATH)

include_directories(
    ${CMAKE_SOURCE_DIR}/libs/${OS}-${ARCH}/gsl/include/
    ${CMAKE_SOURCE_DIR}/libs/${OS}-${ARCH}/muparser/include/muparser/
    ${CMAKE_SOURCE_DIR}/libs/${OS}-${ARCH}/qtexengine/include/qtexengine/
    ${CMAKE_SOURCE_DIR}/libs/${OS}-${ARCH}/qwt/include/qwt/
    ${CMAKE_SOURCE_DIR}/libs/${OS}-${ARCH}/qwtplot3d/include/qwtplot3d/

    dialogs/
    ../analysis/
    ../core/
    ../lib/3rdparty/qtcolorpicker/src/
    ../lib/include/
    ../matrix/
    ../plot3D/
    ../table/
    ../scripting/)

set(SOURCES
    dialogs/AssociationsDialog.cpp
    dialogs/AxesDialog.cpp
    dialogs/ContourLinesEditor.cpp
    dialogs/CurveRangeDialog.cpp
    dialogs/CurvesDialog.cpp
    dialogs/EnrichmentDialog.cpp
    dialogs/ErrDialog.cpp
    dialogs/FunctionDialog.cpp
    dialogs/ImageExportDialog.cpp
    dialogs/LayerDialog.cpp
    dialogs/LineDialog.cpp
    dialogs/PlotDialog.cpp
    dialogs/TextDialog.cpp
    dialogs/TextEditor.cpp
    AddWidgetTool.cpp
    ArrowMarker.cpp
    BoxCurve.cpp
    CanvasPicker.cpp
    DataPickerTool.cpp
    EllipseWidget.cpp
    ErrorBarsCurve.cpp
    FrameWidget.cpp
    FunctionCurve.cpp
    Graph.cpp
    Grid.cpp
    ImageSymbol.cpp
    ImageWidget.cpp
    LegendWidget.cpp
    LineProfileTool.cpp
    LnScaleEngine.cpp
    Log2ScaleEngine.cpp
    LogitScaleEngine.cpp
    MultiLayer.cpp
    MultiPeakFitTool.cpp
    PieCurve.cpp
    PlotCurve.cpp
    ProbabilityScaleEngine.cpp
    QwtBarCurve.cpp
    QwtHistogram.cpp
    RangeSelectorTool.cpp
    ReciprocalScaleEngine.cpp
    RectangleWidget.cpp
    ScaleDraw.cpp
    ScaleEngine.cpp
    ScalePicker.cpp
    ScreenPickerTool.cpp
    SelectionMoveResizer.cpp
    Spectrogram.cpp
    SubtractLineTool.cpp
    TexWidget.cpp
    TitlePicker.cpp
    TranslateCurveTool.cpp
    VectorCurve.cpp)

set(HEADERS
    dialogs/AssociationsDialog.h
    dialogs/AxesDialog.h
    dialogs/ContourLinesEditor.h
    dialogs/CurveRangeDialog.h
    dialogs/CurvesDialog.h
    dialogs/EnrichmentDialog.h
    dialogs/ErrDialog.h
    dialogs/FunctionDialog.h
    dialogs/ImageExportDialog.h
    dialogs/LayerDialog.h
    dialogs/LineDialog.h
    dialogs/PlotDialog.h
    dialogs/TextDialog.h
    dialogs/TextEditor.h
    AddWidgetTool.h
    ArrowMarker.h
    BoxCurve.h
    CanvasPicker.h
    DataPickerTool.h
    EllipseWidget.h
    ErrorBarsCurve.h
    FrameWidget.h
    FunctionCurve.h
    Graph.h
    Grid.h
    ImageSymbol.h
    ImageWidget.h
    LegendWidget.h
    LineProfileTool.h
    LnScaleEngine.h
    Log2ScaleEngine.h
    LogitScaleEngine.h
    MultiLayer.h
    MultiPeakFitTool.h
    PieCurve.h
    PlotCurve.h
    PlotToolInterface.h
    ProbabilityScaleEngine.h
    QwtBarCurve.h
    QwtHistogram.h
    RangeSelectorTool.h
    ReciprocalScaleEngine.h
    RectangleWidget.h
    ScaleDraw.h
    ScaleEngine.h
    ScalePicker.h
    ScreenPickerTool.h
    SelectionMoveResizer.h
    Spectrogram.h
    SubtractLineTool.h
    TexWidget.h
    TitlePicker.h
    TranslateCurveTool.h
    VectorCurve.h)

QT4_WRAP_CPP(MOCS ${HEADERS})

add_definitions(-DHAVE_ALGLIB=1 -DTEX_OUTPUT=1)

add_library(plot2D STATIC ${SOURCES} ${MOCS})
target_link_libraries(plot2D PUBLIC lib plot3D
    ${GSL_LIBRARY} ${MUPARSER_LIBRARY} ${QTEXENGINE_LIBRARY} ${QWT_LIBRARY} ${QWTPLOT3D_LIBRARY}
    Qt4::QtCore Qt4::QtOpenGL Qt4::QtSvg Qt4::Qt3Support)
