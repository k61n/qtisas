
set(SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/dialogs/AnovaDialog.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/dialogs/BaselineDialog.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/dialogs/ExpDecayDialog.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/dialogs/FFTDialog.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/dialogs/FilterDialog.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/dialogs/FitDialog.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/dialogs/FrequencyCountDialog.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/dialogs/GriddingDialog.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/dialogs/IntDialog.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/dialogs/IntegrationDialog.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/dialogs/InterpolationDialog.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/dialogs/PolynomFitDialog.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/dialogs/SmoothCurveDialog.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/dialogs/StudentTestDialog.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/dialogs/SubtractDataDialog.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Anova.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ChiSquareTest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Convolution.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Correlation.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Differentiation.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ExponentialFit.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/FFT.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/fft2D.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/FFTFilter.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Filter.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Fit.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/fit_gsl.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Integration.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Interpolation.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/LogisticFit.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/MultiPeakFit.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/NonLinearFit.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/PluginFit.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/PolynomialFit.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ShapiroWilkTest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/SigmoidalFit.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/SmoothFilter.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Statistics.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/StatisticTest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/tTest.cpp)

set(HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/dialogs/AnovaDialog.h
    ${CMAKE_CURRENT_SOURCE_DIR}/dialogs/BaselineDialog.h
    ${CMAKE_CURRENT_SOURCE_DIR}/dialogs/ExpDecayDialog.h
    ${CMAKE_CURRENT_SOURCE_DIR}/dialogs/FFTDialog.h
    ${CMAKE_CURRENT_SOURCE_DIR}/dialogs/FilterDialog.h
    ${CMAKE_CURRENT_SOURCE_DIR}/dialogs/FitDialog.h
    ${CMAKE_CURRENT_SOURCE_DIR}/dialogs/FrequencyCountDialog.h
    ${CMAKE_CURRENT_SOURCE_DIR}/dialogs/GriddingDialog.h
    ${CMAKE_CURRENT_SOURCE_DIR}/dialogs/IntDialog.h
    ${CMAKE_CURRENT_SOURCE_DIR}/dialogs/IntegrationDialog.h
    ${CMAKE_CURRENT_SOURCE_DIR}/dialogs/InterpolationDialog.h
    ${CMAKE_CURRENT_SOURCE_DIR}/dialogs/PolynomFitDialog.h
    ${CMAKE_CURRENT_SOURCE_DIR}/dialogs/SmoothCurveDialog.h
    ${CMAKE_CURRENT_SOURCE_DIR}/dialogs/StudentTestDialog.h
    ${CMAKE_CURRENT_SOURCE_DIR}/dialogs/SubtractDataDialog.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Anova.h
    ${CMAKE_CURRENT_SOURCE_DIR}/ChiSquareTest.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Convolution.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Correlation.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Differentiation.h
    ${CMAKE_CURRENT_SOURCE_DIR}/ExponentialFit.h
    ${CMAKE_CURRENT_SOURCE_DIR}/FFT.h
    ${CMAKE_CURRENT_SOURCE_DIR}/fft2D.h
    ${CMAKE_CURRENT_SOURCE_DIR}/FFTFilter.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Filter.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Fit.h
    ${CMAKE_CURRENT_SOURCE_DIR}/fit_gsl.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Integration.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Interpolation.h
    ${CMAKE_CURRENT_SOURCE_DIR}/LogisticFit.h
    ${CMAKE_CURRENT_SOURCE_DIR}/lowess.h
    ${CMAKE_CURRENT_SOURCE_DIR}/MultiPeakFit.h
    ${CMAKE_CURRENT_SOURCE_DIR}/NonLinearFit.h
    ${CMAKE_CURRENT_SOURCE_DIR}/PluginFit.h
    ${CMAKE_CURRENT_SOURCE_DIR}/PolynomialFit.h
    ${CMAKE_CURRENT_SOURCE_DIR}/ShapiroWilkTest.h
    ${CMAKE_CURRENT_SOURCE_DIR}/SigmoidalFit.h
    ${CMAKE_CURRENT_SOURCE_DIR}/SmoothFilter.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Statistics.h
    ${CMAKE_CURRENT_SOURCE_DIR}/StatisticTest.h
    ${CMAKE_CURRENT_SOURCE_DIR}/tTest.h)

set(INCLUDEDIRS
    ${CMAKE_CURRENT_SOURCE_DIR}/
    ${CMAKE_CURRENT_SOURCE_DIR}/dialogs)

set(analysis_SOURCES ${SOURCES} PARENT_SCOPE)
set(analysis_HEADERS ${HEADERS} PARENT_SCOPE)
set(analysis_INCLUDEDIRS ${INCLUDEDIRS} PARENT_SCOPE)
