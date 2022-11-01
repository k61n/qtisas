###############################################################
################# Module: Analysis ##############################
###############################################################

INCLUDEPATH += $${QTISAS_FOLDER}/qtisas/src/analysis
INCLUDEPATH += $${QTISAS_FOLDER}/qtisas/src/analysis/dialogs

HEADERS += $${QTISAS_FOLDER}/qtisas/src/analysis/ChiSquareTest.h \
			   $${QTISAS_FOLDER}/qtisas/src/analysis/Convolution.h \
			   $${QTISAS_FOLDER}/qtisas/src/analysis/Correlation.h \
			   $${QTISAS_FOLDER}/qtisas/src/analysis/Differentiation.h \
			   $${QTISAS_FOLDER}/qtisas/src/analysis/ExponentialFit.h \
			   $${QTISAS_FOLDER}/qtisas/src/analysis/FFTFilter.h \
			   $${QTISAS_FOLDER}/qtisas/src/analysis/FFT.h \
			   $${QTISAS_FOLDER}/qtisas/src/analysis/Filter.h \
			   $${QTISAS_FOLDER}/qtisas/src/analysis/Fit.h \
			   $${QTISAS_FOLDER}/qtisas/src/analysis/FitModelHandler.h \
			   $${QTISAS_FOLDER}/qtisas/src/analysis/Integration.h \
			   $${QTISAS_FOLDER}/qtisas/src/analysis/Interpolation.h \
			   $${QTISAS_FOLDER}/qtisas/src/analysis/LogisticFit.h \
			   $${QTISAS_FOLDER}/qtisas/src/analysis/MultiPeakFit.h \
			   $${QTISAS_FOLDER}/qtisas/src/analysis/NonLinearFit.h \
			   $${QTISAS_FOLDER}/qtisas/src/analysis/PluginFit.h \
			   $${QTISAS_FOLDER}/qtisas/src/analysis/PolynomialFit.h \
			   $${QTISAS_FOLDER}/qtisas/src/analysis/ShapiroWilkTest.h \
			   $${QTISAS_FOLDER}/qtisas/src/analysis/SigmoidalFit.h \
			   $${QTISAS_FOLDER}/qtisas/src/analysis/SmoothFilter.h \
			   $${QTISAS_FOLDER}/qtisas/src/analysis/Statistics.h \
			   $${QTISAS_FOLDER}/qtisas/src/analysis/StatisticTest.h \
			   $${QTISAS_FOLDER}/qtisas/src/analysis/tTest.h \
			   $${QTISAS_FOLDER}/qtisas/src/analysis/fft2D.h \
			   $${QTISAS_FOLDER}/qtisas/src/analysis/fit_gsl.h \

SOURCES += $${QTISAS_FOLDER}/qtisas/src/analysis/ChiSquareTest.cpp \
			   $${QTISAS_FOLDER}/qtisas/src/analysis/Convolution.cpp \
			   $${QTISAS_FOLDER}/qtisas/src/analysis/Correlation.cpp \
			   $${QTISAS_FOLDER}/qtisas/src/analysis/Differentiation.cpp \
			   $${QTISAS_FOLDER}/qtisas/src/analysis/ExponentialFit.cpp \
			   $${QTISAS_FOLDER}/qtisas/src/analysis/FFTFilter.cpp \
			   $${QTISAS_FOLDER}/qtisas/src/analysis/FFT.cpp \
			   $${QTISAS_FOLDER}/qtisas/src/analysis/Filter.cpp \
			   $${QTISAS_FOLDER}/qtisas/src/analysis/Fit.cpp \
			   $${QTISAS_FOLDER}/qtisas/src/analysis/FitModelHandler.cpp \
			   $${QTISAS_FOLDER}/qtisas/src/analysis/Integration.cpp \
			   $${QTISAS_FOLDER}/qtisas/src/analysis/Interpolation.cpp \
			   $${QTISAS_FOLDER}/qtisas/src/analysis/LogisticFit.cpp \
			   $${QTISAS_FOLDER}/qtisas/src/analysis/MultiPeakFit.cpp \
			   $${QTISAS_FOLDER}/qtisas/src/analysis/NonLinearFit.cpp \
			   $${QTISAS_FOLDER}/qtisas/src/analysis/PluginFit.cpp \
			   $${QTISAS_FOLDER}/qtisas/src/analysis/PolynomialFit.cpp \
			   $${QTISAS_FOLDER}/qtisas/src/analysis/ShapiroWilkTest.cpp \
			   $${QTISAS_FOLDER}/qtisas/src/analysis/SigmoidalFit.cpp \
			   $${QTISAS_FOLDER}/qtisas/src/analysis/SmoothFilter.cpp \
			   $${QTISAS_FOLDER}/qtisas/src/analysis/Statistics.cpp \
			   $${QTISAS_FOLDER}/qtisas/src/analysis/StatisticTest.cpp \
			   $${QTISAS_FOLDER}/qtisas/src/analysis/tTest.cpp \
			   $${QTISAS_FOLDER}/qtisas/src/analysis/fft2D.cpp \
			   $${QTISAS_FOLDER}/qtisas/src/analysis/fit_gsl.cpp \

HEADERS += $${QTISAS_FOLDER}/qtisas/src/analysis/dialogs/AnovaDialog.h \
			   $${QTISAS_FOLDER}/qtisas/src/analysis/dialogs/BaselineDialog.h \
			   $${QTISAS_FOLDER}/qtisas/src/analysis/dialogs/ExpDecayDialog.h \
			   $${QTISAS_FOLDER}/qtisas/src/analysis/dialogs/FFTDialog.h \
			   $${QTISAS_FOLDER}/qtisas/src/analysis/dialogs/FitDialog.h \
			   $${QTISAS_FOLDER}/qtisas/src/analysis/dialogs/FilterDialog.h \
			   $${QTISAS_FOLDER}/qtisas/src/analysis/dialogs/FrequencyCountDialog.h \
			   $${QTISAS_FOLDER}/qtisas/src/analysis/dialogs/IntDialog.h \
			   $${QTISAS_FOLDER}/qtisas/src/analysis/dialogs/IntegrationDialog.h \
			   $${QTISAS_FOLDER}/qtisas/src/analysis/dialogs/InterpolationDialog.h \
			   $${QTISAS_FOLDER}/qtisas/src/analysis/dialogs/PolynomFitDialog.h \
			   $${QTISAS_FOLDER}/qtisas/src/analysis/dialogs/SmoothCurveDialog.h \
			   $${QTISAS_FOLDER}/qtisas/src/analysis/dialogs/SubtractDataDialog.h \
			   $${QTISAS_FOLDER}/qtisas/src/analysis/dialogs/StudentTestDialog.h \

SOURCES += $${QTISAS_FOLDER}/qtisas/src/analysis/dialogs/AnovaDialog.cpp \
		   $${QTISAS_FOLDER}/qtisas/src/analysis/dialogs/BaselineDialog.cpp \
			   $${QTISAS_FOLDER}/qtisas/src/analysis/dialogs/ExpDecayDialog.cpp \
			   $${QTISAS_FOLDER}/qtisas/src/analysis/dialogs/FFTDialog.cpp \
			   $${QTISAS_FOLDER}/qtisas/src/analysis/dialogs/FitDialog.cpp \
			   $${QTISAS_FOLDER}/qtisas/src/analysis/dialogs/FilterDialog.cpp \
			   $${QTISAS_FOLDER}/qtisas/src/analysis/dialogs/FrequencyCountDialog.cpp \
			   $${QTISAS_FOLDER}/qtisas/src/analysis/dialogs/IntDialog.cpp \
			   $${QTISAS_FOLDER}/qtisas/src/analysis/dialogs/IntegrationDialog.cpp \
			   $${QTISAS_FOLDER}/qtisas/src/analysis/dialogs/InterpolationDialog.cpp \
			   $${QTISAS_FOLDER}/qtisas/src/analysis/dialogs/PolynomFitDialog.cpp \
			   $${QTISAS_FOLDER}/qtisas/src/analysis/dialogs/SmoothCurveDialog.cpp \
			   $${QTISAS_FOLDER}/qtisas/src/analysis/dialogs/SubtractDataDialog.cpp \
			   $${QTISAS_FOLDER}/qtisas/src/analysis/dialogs/StudentTestDialog.cpp \


HEADERS += $${QTISAS_FOLDER}/qtisas/src/analysis/dialogs/GriddingDialog.h
SOURCES += $${QTISAS_FOLDER}/qtisas/src/analysis/dialogs/GriddingDialog.cpp


HEADERS += $${QTISAS_FOLDER}/qtisas/src/analysis/Anova.h
SOURCES += $${QTISAS_FOLDER}/qtisas/src/analysis/Anova.cpp

