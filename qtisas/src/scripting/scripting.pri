###############################################################
##################### SCRIPTING LANGUAGES SECTION #############
###############################################################

INCLUDEPATH += $${QTISAS_FOLDER}/qtisas/src/scripting/

HEADERS  += $${QTISAS_FOLDER}/qtisas/src/scripting/customevents.h\
            $${QTISAS_FOLDER}/qtisas/src/scripting/FindReplaceDialog.h\
            $${QTISAS_FOLDER}/qtisas/src/scripting/MyParser.h\
            $${QTISAS_FOLDER}/qtisas/src/scripting/Note.h\
            $${QTISAS_FOLDER}/qtisas/src/scripting/PythonSyntaxHighlighter.h\
            $${QTISAS_FOLDER}/qtisas/src/scripting/ScriptingEnv.h\
            $${QTISAS_FOLDER}/qtisas/src/scripting/Script.h\
            $${QTISAS_FOLDER}/qtisas/src/scripting/ScriptEdit.h\
            $${QTISAS_FOLDER}/qtisas/src/scripting/ScriptingLangDialog.h\
            $${QTISAS_FOLDER}/qtisas/src/scripting/ScriptWindow.h\

SOURCES  += $${QTISAS_FOLDER}/qtisas/src/scripting/FindReplaceDialog.cpp\
            $${QTISAS_FOLDER}/qtisas/src/scripting/MyParser.cpp\
            $${QTISAS_FOLDER}/qtisas/src/scripting/Note.cpp\
            $${QTISAS_FOLDER}/qtisas/src/scripting/PythonSyntaxHighlighter.cpp\
            $${QTISAS_FOLDER}/qtisas/src/scripting/ScriptingEnv.cpp\
            $${QTISAS_FOLDER}/qtisas/src/scripting/Script.cpp\
            $${QTISAS_FOLDER}/qtisas/src/scripting/ScriptEdit.cpp\
            $${QTISAS_FOLDER}/qtisas/src/scripting/ScriptingLangDialog.cpp\
            $${QTISAS_FOLDER}/qtisas/src/scripting/ScriptWindow.cpp\

##################### Default: muParser v1.28 #################

contains(SCRIPTING_LANGS, muParser) {
  DEFINES += SCRIPTING_MUPARSER

  HEADERS += $${QTISAS_FOLDER}/qtisas/src/scripting/muParserScript.h \
             $${QTISAS_FOLDER}/qtisas/src/scripting/muParserScripting.h \

  SOURCES += $${QTISAS_FOLDER}/qtisas/src/scripting/muParserScript.cpp \
             $${QTISAS_FOLDER}/qtisas/src/scripting/muParserScripting.cpp
}

##################### PYTHON + SIP + PyQT #####################

contains(SCRIPTING_LANGS, Python) {

  DEFINES += SCRIPTING_PYTHON

  HEADERS += $${QTISAS_FOLDER}/qtisas/src/scripting/PythonScript.h\
             $${QTISAS_FOLDER}/qtisas/src/scripting/PythonScripting.h\
			 $${SIP_DIR}/sipAPIqti.h

  SOURCES += $${QTISAS_FOLDER}/qtisas/src/scripting/PythonScript.cpp\
  			 $${QTISAS_FOLDER}/qtisas/src/scripting/PythonScripting.cpp\

			 
##################### SIP generated files #####################

  SOURCES += $${SIP_DIR}/sipqticmodule.cpp\
             $${SIP_DIR}/sipqtiApplicationWindow.cpp\
             $${SIP_DIR}/sipqtiQwtPlot.cpp\
             $${SIP_DIR}/sipqtiGraph.cpp\
             $${SIP_DIR}/sipqtiGraph3D.cpp\
             $${SIP_DIR}/sipqtiArrowMarker.cpp\
             $${SIP_DIR}/sipqtiFrameWidget.cpp\
			 $${SIP_DIR}/sipqtiEllipseWidget.cpp\
			 $${SIP_DIR}/sipqtiImageWidget.cpp\
			 $${SIP_DIR}/sipqtiLegendWidget.cpp\
			 $${SIP_DIR}/sipqtiRectangleWidget.cpp\
			 $${SIP_DIR}/sipqtiGrid.cpp\
			 $${SIP_DIR}/sipqtiQwtSymbol.cpp\
			 $${SIP_DIR}/sipqtiImageSymbol.cpp\
			 $${SIP_DIR}/sipqtiQwtPlotCurve.cpp\
			 $${SIP_DIR}/sipqtiBoxCurve.cpp\
			 $${SIP_DIR}/sipqtiPieCurve.cpp\
			 $${SIP_DIR}/sipqtiPlotCurve.cpp\
			 $${SIP_DIR}/sipqtiFunctionCurve.cpp\
			 $${SIP_DIR}/sipqtiDataCurve.cpp\
			 $${SIP_DIR}/sipqtiErrorBarsCurve.cpp\
			 $${SIP_DIR}/sipqtiVectorCurve.cpp\
			 $${SIP_DIR}/sipqtiQwtHistogram.cpp\
			 $${SIP_DIR}/sipqtiQwtPlotSpectrogram.cpp\
			 $${SIP_DIR}/sipqtiSpectrogram.cpp\
             $${SIP_DIR}/sipqtiMultiLayer.cpp\
             $${SIP_DIR}/sipqtiTable.cpp\
			 $${SIP_DIR}/sipqtiLinearColorMap.cpp\
			 $${SIP_DIR}/sipqtiQwtLinearColorMap.cpp\
             $${SIP_DIR}/sipqtiMatrix.cpp\
             $${SIP_DIR}/sipqtiMdiSubWindow.cpp\
             $${SIP_DIR}/sipqtiScriptEdit.cpp\
             $${SIP_DIR}/sipqtiNote.cpp\
             $${SIP_DIR}/sipqtiPythonScript.cpp\
             $${SIP_DIR}/sipqtiPythonScripting.cpp\
             $${SIP_DIR}/sipqtiFolder.cpp\
             $${SIP_DIR}/sipqtiFit.cpp \
             $${SIP_DIR}/sipqtiExponentialFit.cpp \
             $${SIP_DIR}/sipqtiTwoExpFit.cpp \
             $${SIP_DIR}/sipqtiThreeExpFit.cpp \
             $${SIP_DIR}/sipqtiSigmoidalFit.cpp \
			 $${SIP_DIR}/sipqtiLogisticFit.cpp \
             $${SIP_DIR}/sipqtiGaussAmpFit.cpp \
             $${SIP_DIR}/sipqtiLorentzFit.cpp \
             $${SIP_DIR}/sipqtiNonLinearFit.cpp \
             $${SIP_DIR}/sipqtiPluginFit.cpp \
             $${SIP_DIR}/sipqtiMultiPeakFit.cpp \
             $${SIP_DIR}/sipqtiPolynomialFit.cpp \
             $${SIP_DIR}/sipqtiLinearFit.cpp \
             $${SIP_DIR}/sipqtiGaussFit.cpp \
             $${SIP_DIR}/sipqtiFilter.cpp \
             $${SIP_DIR}/sipqtiDifferentiation.cpp \
             $${SIP_DIR}/sipqtiIntegration.cpp \
			 $${SIP_DIR}/sipqtiInterpolation.cpp \
			 $${SIP_DIR}/sipqtiSmoothFilter.cpp \
			 $${SIP_DIR}/sipqtiFFTFilter.cpp \
			 $${SIP_DIR}/sipqtiFFT.cpp \
			 $${SIP_DIR}/sipqtiCorrelation.cpp \
			 $${SIP_DIR}/sipqtiConvolution.cpp \
			 $${SIP_DIR}/sipqtiDeconvolution.cpp \
			 $${SIP_DIR}/sipqtiStatistics.cpp \
			 $${SIP_DIR}/sipqtiStatisticTest.cpp \
			 $${SIP_DIR}/sipqtitTest.cpp \
			 $${SIP_DIR}/sipqtiShapiroWilkTest.cpp \
			 $${SIP_DIR}/sipqtiChiSquareTest.cpp \
			 $${SIP_DIR}/sipqtiAnova.cpp \

exists(../../$${SIP_DIR}/sipqtiQList.cpp) {
  # SIP < 4.9
  SOURCES += $${SIP_DIR}/sipqtiQList.cpp
} else {
  SOURCES += \
			 $${SIP_DIR}/sipqtiQList0101ErrorBarsCurve.cpp\
			 $${SIP_DIR}/sipqtiQList0101Folder.cpp\
			 $${SIP_DIR}/sipqtiQList0101Graph.cpp\
			 $${SIP_DIR}/sipqtiQList0101MdiSubWindow.cpp\
			 $${SIP_DIR}/sipqtiQList0101ArrowMarker.cpp\
}
}
