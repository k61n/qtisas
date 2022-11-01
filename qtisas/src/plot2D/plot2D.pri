###############################################################
################# Module: Plot 2D #############################
###############################################################

    INCLUDEPATH += $${QTISAS_FOLDER}/qtisas/src/plot2D/
    INCLUDEPATH += $${QTISAS_FOLDER}/qtisas/src/plot2D/dialogs/

    HEADERS += $${QTISAS_FOLDER}/qtisas/src/plot2D/AddWidgetTool.h \
			   $${QTISAS_FOLDER}/qtisas/src/plot2D/ArrowMarker.h \
			   $${QTISAS_FOLDER}/qtisas/src/plot2D/BoxCurve.h \
			   $${QTISAS_FOLDER}/qtisas/src/plot2D/CanvasPicker.h \
			   $${QTISAS_FOLDER}/qtisas/src/plot2D/DataPickerTool.h \
			   $${QTISAS_FOLDER}/qtisas/src/plot2D/EllipseWidget.h \
			   $${QTISAS_FOLDER}/qtisas/src/plot2D/ErrorBarsCurve.h \
			   $${QTISAS_FOLDER}/qtisas/src/plot2D/FrameWidget.h \
			   $${QTISAS_FOLDER}/qtisas/src/plot2D/FunctionCurve.h \
			   $${QTISAS_FOLDER}/qtisas/src/plot2D/Graph.h \
			   $${QTISAS_FOLDER}/qtisas/src/plot2D/Grid.h \
			   $${QTISAS_FOLDER}/qtisas/src/plot2D/ImageSymbol.h \
			   $${QTISAS_FOLDER}/qtisas/src/plot2D/ImageWidget.h \
			   $${QTISAS_FOLDER}/qtisas/src/plot2D/LegendWidget.h \
			   $${QTISAS_FOLDER}/qtisas/src/plot2D/LineProfileTool.h \
			   $${QTISAS_FOLDER}/qtisas/src/plot2D/LnScaleEngine.h \
			   $${QTISAS_FOLDER}/qtisas/src/plot2D/Log2ScaleEngine.h \
			   $${QTISAS_FOLDER}/qtisas/src/plot2D/LogitScaleEngine.h \
			   $${QTISAS_FOLDER}/qtisas/src/plot2D/MultiLayer.h \
			   $${QTISAS_FOLDER}/qtisas/src/plot2D/MultiPeakFitTool.h \
			   $${QTISAS_FOLDER}/qtisas/src/plot2D/PieCurve.h \
			   $${QTISAS_FOLDER}/qtisas/src/plot2D/PlotCurve.h \
			   $${QTISAS_FOLDER}/qtisas/src/plot2D/PlotToolInterface.h \
			   $${QTISAS_FOLDER}/qtisas/src/plot2D/ProbabilityScaleEngine.h \
			   $${QTISAS_FOLDER}/qtisas/src/plot2D/QwtBarCurve.h \
			   $${QTISAS_FOLDER}/qtisas/src/plot2D/QwtHistogram.h \
			   $${QTISAS_FOLDER}/qtisas/src/plot2D/RangeSelectorTool.h \
			   $${QTISAS_FOLDER}/qtisas/src/plot2D/ReciprocalScaleEngine.h \
			   $${QTISAS_FOLDER}/qtisas/src/plot2D/RectangleWidget.h \
			   $${QTISAS_FOLDER}/qtisas/src/plot2D/ScaleDraw.h \
			   $${QTISAS_FOLDER}/qtisas/src/plot2D/ScaleEngine.h \
			   $${QTISAS_FOLDER}/qtisas/src/plot2D/ScalePicker.h \
			   $${QTISAS_FOLDER}/qtisas/src/plot2D/ScreenPickerTool.h \
			   $${QTISAS_FOLDER}/qtisas/src/plot2D/SelectionMoveResizer.h \
			   $${QTISAS_FOLDER}/qtisas/src/plot2D/Spectrogram.h \
			   $${QTISAS_FOLDER}/qtisas/src/plot2D/SubtractLineTool.h \
			   $${QTISAS_FOLDER}/qtisas/src/plot2D/TexWidget.h \
			   $${QTISAS_FOLDER}/qtisas/src/plot2D/TitlePicker.h \
			   $${QTISAS_FOLDER}/qtisas/src/plot2D/TranslateCurveTool.h \
			   $${QTISAS_FOLDER}/qtisas/src/plot2D/VectorCurve.h \

    SOURCES += $${QTISAS_FOLDER}/qtisas/src/plot2D/AddWidgetTool.cpp \
			   $${QTISAS_FOLDER}/qtisas/src/plot2D/ArrowMarker.cpp \
			   $${QTISAS_FOLDER}/qtisas/src/plot2D/BoxCurve.cpp \
			   $${QTISAS_FOLDER}/qtisas/src/plot2D/CanvasPicker.cpp \
			   $${QTISAS_FOLDER}/qtisas/src/plot2D/DataPickerTool.cpp \
			   $${QTISAS_FOLDER}/qtisas/src/plot2D/EllipseWidget.cpp \
			   $${QTISAS_FOLDER}/qtisas/src/plot2D/ErrorBarsCurve.cpp \
			   $${QTISAS_FOLDER}/qtisas/src/plot2D/FrameWidget.cpp \
			   $${QTISAS_FOLDER}/qtisas/src/plot2D/FunctionCurve.cpp \
			   $${QTISAS_FOLDER}/qtisas/src/plot2D/Graph.cpp \
			   $${QTISAS_FOLDER}/qtisas/src/plot2D/Grid.cpp \
			   $${QTISAS_FOLDER}/qtisas/src/plot2D/ImageSymbol.cpp \
			   $${QTISAS_FOLDER}/qtisas/src/plot2D/ImageWidget.cpp \
			   $${QTISAS_FOLDER}/qtisas/src/plot2D/LegendWidget.cpp \
			   $${QTISAS_FOLDER}/qtisas/src/plot2D/LineProfileTool.cpp \
			   $${QTISAS_FOLDER}/qtisas/src/plot2D/LnScaleEngine.cpp \
			   $${QTISAS_FOLDER}/qtisas/src/plot2D/Log2ScaleEngine.cpp \
			   $${QTISAS_FOLDER}/qtisas/src/plot2D/LogitScaleEngine.cpp \
			   $${QTISAS_FOLDER}/qtisas/src/plot2D/MultiLayer.cpp \
			   $${QTISAS_FOLDER}/qtisas/src/plot2D/MultiPeakFitTool.cpp \
			   $${QTISAS_FOLDER}/qtisas/src/plot2D/PieCurve.cpp \
			   $${QTISAS_FOLDER}/qtisas/src/plot2D/PlotCurve.cpp \
			   $${QTISAS_FOLDER}/qtisas/src/plot2D/ProbabilityScaleEngine.cpp \
			   $${QTISAS_FOLDER}/qtisas/src/plot2D/QwtBarCurve.cpp \
			   $${QTISAS_FOLDER}/qtisas/src/plot2D/QwtHistogram.cpp \
			   $${QTISAS_FOLDER}/qtisas/src/plot2D/RangeSelectorTool.cpp \
			   $${QTISAS_FOLDER}/qtisas/src/plot2D/ReciprocalScaleEngine.cpp \
			   $${QTISAS_FOLDER}/qtisas/src/plot2D/RectangleWidget.cpp \
			   $${QTISAS_FOLDER}/qtisas/src/plot2D/ScaleDraw.cpp \
			   $${QTISAS_FOLDER}/qtisas/src/plot2D/ScaleEngine.cpp \
			   $${QTISAS_FOLDER}/qtisas/src/plot2D/ScalePicker.cpp \
			   $${QTISAS_FOLDER}/qtisas/src/plot2D/ScreenPickerTool.cpp \
			   $${QTISAS_FOLDER}/qtisas/src/plot2D/SelectionMoveResizer.cpp \
			   $${QTISAS_FOLDER}/qtisas/src/plot2D/Spectrogram.cpp \
			   $${QTISAS_FOLDER}/qtisas/src/plot2D/SubtractLineTool.cpp \
			   $${QTISAS_FOLDER}/qtisas/src/plot2D/TexWidget.cpp \
			   $${QTISAS_FOLDER}/qtisas/src/plot2D/TitlePicker.cpp \
			   $${QTISAS_FOLDER}/qtisas/src/plot2D/TranslateCurveTool.cpp \
			   $${QTISAS_FOLDER}/qtisas/src/plot2D/VectorCurve.cpp \

	HEADERS += $${QTISAS_FOLDER}/qtisas/src/plot2D/dialogs/AssociationsDialog.h \
			   $${QTISAS_FOLDER}/qtisas/src/plot2D/dialogs/AxesDialog.h \
			   $${QTISAS_FOLDER}/qtisas/src/plot2D/dialogs/ContourLinesEditor.h \
			   $${QTISAS_FOLDER}/qtisas/src/plot2D/dialogs/CurvesDialog.h \
			   $${QTISAS_FOLDER}/qtisas/src/plot2D/dialogs/CurveRangeDialog.h \
			   $${QTISAS_FOLDER}/qtisas/src/plot2D/dialogs/EnrichmentDialog.h \
			   $${QTISAS_FOLDER}/qtisas/src/plot2D/dialogs/ErrDialog.h \
			   $${QTISAS_FOLDER}/qtisas/src/plot2D/dialogs/FunctionDialog.h \
			   $${QTISAS_FOLDER}/qtisas/src/plot2D/dialogs/ImageExportDialog.h \
			   $${QTISAS_FOLDER}/qtisas/src/plot2D/dialogs/LayerDialog.h \
			   $${QTISAS_FOLDER}/qtisas/src/plot2D/dialogs/LineDialog.h \
		       $${QTISAS_FOLDER}/qtisas/src/plot2D/dialogs/PlotDialog.h \
			   $${QTISAS_FOLDER}/qtisas/src/plot2D/dialogs/TextDialog.h \
			   $${QTISAS_FOLDER}/qtisas/src/plot2D/dialogs/TextEditor.h \

	SOURCES += $${QTISAS_FOLDER}/qtisas/src/plot2D/dialogs/AssociationsDialog.cpp \
			   $${QTISAS_FOLDER}/qtisas/src/plot2D/dialogs/AxesDialog.cpp \
			   $${QTISAS_FOLDER}/qtisas/src/plot2D/dialogs/ContourLinesEditor.cpp \
			   $${QTISAS_FOLDER}/qtisas/src/plot2D/dialogs/CurvesDialog.cpp \
			   $${QTISAS_FOLDER}/qtisas/src/plot2D/dialogs/CurveRangeDialog.cpp \
			   $${QTISAS_FOLDER}/qtisas/src/plot2D/dialogs/EnrichmentDialog.cpp \
			   $${QTISAS_FOLDER}/qtisas/src/plot2D/dialogs/ErrDialog.cpp \
			   $${QTISAS_FOLDER}/qtisas/src/plot2D/dialogs/FunctionDialog.cpp \
			   $${QTISAS_FOLDER}/qtisas/src/plot2D/dialogs/ImageExportDialog.cpp \
			   $${QTISAS_FOLDER}/qtisas/src/plot2D/dialogs/LayerDialog.cpp \
			   $${QTISAS_FOLDER}/qtisas/src/plot2D/dialogs/LineDialog.cpp \
		       $${QTISAS_FOLDER}/qtisas/src/plot2D/dialogs/PlotDialog.cpp \
			   $${QTISAS_FOLDER}/qtisas/src/plot2D/dialogs/TextDialog.cpp \
			   $${QTISAS_FOLDER}/qtisas/src/plot2D/dialogs/TextEditor.cpp \
