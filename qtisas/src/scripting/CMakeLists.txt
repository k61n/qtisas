
set(SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/FindReplaceDialog.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/muParserScript.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/muParserScripting.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/MyParser.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Note.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/PythonSyntaxHighlighter.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Script.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ScriptEdit.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ScriptingEnv.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ScriptingLangDialog.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ScriptWindow.cpp
)

set(HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/customevents.h
    ${CMAKE_CURRENT_SOURCE_DIR}/FindReplaceDialog.h
    ${CMAKE_CURRENT_SOURCE_DIR}/muParserScript.h
    ${CMAKE_CURRENT_SOURCE_DIR}/muParserScripting.h
    ${CMAKE_CURRENT_SOURCE_DIR}/MyParser.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Note.h
    ${CMAKE_CURRENT_SOURCE_DIR}/PythonSyntaxHighlighter.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Script.h
    ${CMAKE_CURRENT_SOURCE_DIR}/ScriptEdit.h
    ${CMAKE_CURRENT_SOURCE_DIR}/ScriptingEnv.h
    ${CMAKE_CURRENT_SOURCE_DIR}/ScriptingLangDialog.h
    ${CMAKE_CURRENT_SOURCE_DIR}/ScriptWindow.h)

if(WITH_PYTHON)
    set(SOURCES ${SOURCES}
        ${CMAKE_CURRENT_SOURCE_DIR}/PythonScript.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/PythonScripting.cpp
        ${CMAKE_BINARY_DIR}/sip/qti/sipqtiAnova.cpp
        ${CMAKE_BINARY_DIR}/sip/qti/sipqtiApplicationWindow.cpp
        ${CMAKE_BINARY_DIR}/sip/qti/sipqtiArrowMarker.cpp
        ${CMAKE_BINARY_DIR}/sip/qti/sipqtiBoxCurve.cpp
        ${CMAKE_BINARY_DIR}/sip/qti/sipqtiChiSquareTest.cpp
        ${CMAKE_BINARY_DIR}/sip/qti/sipqticmodule.cpp
        ${CMAKE_BINARY_DIR}/sip/qti/sipqtiConvolution.cpp
        ${CMAKE_BINARY_DIR}/sip/qti/sipqtiCorrelation.cpp
        ${CMAKE_BINARY_DIR}/sip/qti/sipqtiDataCurve.cpp
        ${CMAKE_BINARY_DIR}/sip/qti/sipqtiDeconvolution.cpp
        ${CMAKE_BINARY_DIR}/sip/qti/sipqtiDifferentiation.cpp
        ${CMAKE_BINARY_DIR}/sip/qti/sipqtiEllipseWidget.cpp
        ${CMAKE_BINARY_DIR}/sip/qti/sipqtiErrorBarsCurve.cpp
        ${CMAKE_BINARY_DIR}/sip/qti/sipqtiExponentialFit.cpp
        ${CMAKE_BINARY_DIR}/sip/qti/sipqtiFFT.cpp
        ${CMAKE_BINARY_DIR}/sip/qti/sipqtiFFTFilter.cpp
        ${CMAKE_BINARY_DIR}/sip/qti/sipqtiFilter.cpp
        ${CMAKE_BINARY_DIR}/sip/qti/sipqtiFit.cpp
        ${CMAKE_BINARY_DIR}/sip/qti/sipqtiFolder.cpp
        ${CMAKE_BINARY_DIR}/sip/qti/sipqtiFrameWidget.cpp
        ${CMAKE_BINARY_DIR}/sip/qti/sipqtiFunctionCurve.cpp
        ${CMAKE_BINARY_DIR}/sip/qti/sipqtiGaussAmpFit.cpp
        ${CMAKE_BINARY_DIR}/sip/qti/sipqtiGaussFit.cpp
        ${CMAKE_BINARY_DIR}/sip/qti/sipqtiGraph3D.cpp
        ${CMAKE_BINARY_DIR}/sip/qti/sipqtiGraph.cpp
        ${CMAKE_BINARY_DIR}/sip/qti/sipqtiGrid.cpp
        ${CMAKE_BINARY_DIR}/sip/qti/sipqtiImageSymbol.cpp
        ${CMAKE_BINARY_DIR}/sip/qti/sipqtiImageWidget.cpp
        ${CMAKE_BINARY_DIR}/sip/qti/sipqtiIntegration.cpp
        ${CMAKE_BINARY_DIR}/sip/qti/sipqtiInterpolation.cpp
        ${CMAKE_BINARY_DIR}/sip/qti/sipqtiLegendWidget.cpp
        ${CMAKE_BINARY_DIR}/sip/qti/sipqtiLinearColorMap.cpp
        ${CMAKE_BINARY_DIR}/sip/qti/sipqtiLinearFit.cpp
        ${CMAKE_BINARY_DIR}/sip/qti/sipqtiLogisticFit.cpp
        ${CMAKE_BINARY_DIR}/sip/qti/sipqtiLorentzFit.cpp
        ${CMAKE_BINARY_DIR}/sip/qti/sipqtiMatrix.cpp
        ${CMAKE_BINARY_DIR}/sip/qti/sipqtiMdiSubWindow.cpp
        ${CMAKE_BINARY_DIR}/sip/qti/sipqtiMultiLayer.cpp
        ${CMAKE_BINARY_DIR}/sip/qti/sipqtiMultiPeakFit.cpp
        ${CMAKE_BINARY_DIR}/sip/qti/sipqtiNonLinearFit.cpp
        ${CMAKE_BINARY_DIR}/sip/qti/sipqtiNote.cpp
        ${CMAKE_BINARY_DIR}/sip/qti/sipqtiPieCurve.cpp
        ${CMAKE_BINARY_DIR}/sip/qti/sipqtiPlotCurve.cpp
        ${CMAKE_BINARY_DIR}/sip/qti/sipqtiPluginFit.cpp
        ${CMAKE_BINARY_DIR}/sip/qti/sipqtiPolynomialFit.cpp
        ${CMAKE_BINARY_DIR}/sip/qti/sipqtiPythonScript.cpp
        ${CMAKE_BINARY_DIR}/sip/qti/sipqtiPythonScripting.cpp
        ${CMAKE_BINARY_DIR}/sip/qti/sipqtiQList0101ArrowMarker.cpp
        ${CMAKE_BINARY_DIR}/sip/qti/sipqtiQList0101ErrorBarsCurve.cpp
        ${CMAKE_BINARY_DIR}/sip/qti/sipqtiQList0101Folder.cpp
        ${CMAKE_BINARY_DIR}/sip/qti/sipqtiQList0101Graph.cpp
        ${CMAKE_BINARY_DIR}/sip/qti/sipqtiQList0101MdiSubWindow.cpp
        ${CMAKE_BINARY_DIR}/sip/qti/sipqtiQwtHistogram.cpp
        ${CMAKE_BINARY_DIR}/sip/qti/sipqtiQwtLinearColorMap.cpp
        ${CMAKE_BINARY_DIR}/sip/qti/sipqtiQwtPlot.cpp
        ${CMAKE_BINARY_DIR}/sip/qti/sipqtiQwtPlotCurve.cpp
        ${CMAKE_BINARY_DIR}/sip/qti/sipqtiQwtPlotSpectrogram.cpp
        ${CMAKE_BINARY_DIR}/sip/qti/sipqtiQwtSymbol.cpp
        ${CMAKE_BINARY_DIR}/sip/qti/sipqtiRectangleWidget.cpp
        ${CMAKE_BINARY_DIR}/sip/qti/sipqtiScriptEdit.cpp
        ${CMAKE_BINARY_DIR}/sip/qti/sipqtiShapiroWilkTest.cpp
        ${CMAKE_BINARY_DIR}/sip/qti/sipqtiSigmoidalFit.cpp
        ${CMAKE_BINARY_DIR}/sip/qti/sipqtiSmoothFilter.cpp
        ${CMAKE_BINARY_DIR}/sip/qti/sipqtiSpectrogram.cpp
        ${CMAKE_BINARY_DIR}/sip/qti/sipqtiStatistics.cpp
        ${CMAKE_BINARY_DIR}/sip/qti/sipqtiStatisticTest.cpp
        ${CMAKE_BINARY_DIR}/sip/qti/sipqtiTable.cpp
        ${CMAKE_BINARY_DIR}/sip/qti/sipqtiThreeExpFit.cpp
        ${CMAKE_BINARY_DIR}/sip/qti/sipqtitTest.cpp
        ${CMAKE_BINARY_DIR}/sip/qti/sipqtiTwoExpFit.cpp
        ${CMAKE_BINARY_DIR}/sip/qti/sipqtiVectorCurve.cpp)
    
    set(HEADERS ${HEADERS}
        ${CMAKE_CURRENT_SOURCE_DIR}/PythonScript.h
        ${CMAKE_CURRENT_SOURCE_DIR}/PythonScripting.h
        ${CMAKE_BINARY_DIR}/sip/qti/sipAPIqti.h)
endif()

set(INCLUDEDIRS
    ${CMAKE_CURRENT_SOURCE_DIR}/)

set(scripting_SOURCES ${SOURCES} PARENT_SCOPE)
set(scripting_HEADERS ${HEADERS} PARENT_SCOPE)
set(scripting_INCLUDEDIRS ${INCLUDEDIRS} PARENT_SCOPE)
