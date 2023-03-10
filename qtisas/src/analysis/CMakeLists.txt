
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)

set(OS ${CMAKE_SYSTEM_NAME})
set(ARCH ${CMAKE_SYSTEM_PROCESSOR})

find_package(Qt4 COMPONENTS QtCore QtGui QtOpenGL QtXml Qt3Support REQUIRED)
find_library(ALGLIB_LIBRARY NAMES alglib PATHS ${CMAKE_SOURCE_DIR}/libs/${OS}-${ARCH}/alglib/lib NO_DEFAULT_PATH)
find_library(GSL_LIBRARY NAMES gsl PATHS ${CMAKE_SOURCE_DIR}/libs/${OS}-${ARCH}/gsl/lib NO_DEFAULT_PATH)
find_library(MUPARSER_LIBRARY NAMES muparser PATHS ${CMAKE_SOURCE_DIR}/libs/${OS}-${ARCH}/muparser/lib NO_DEFAULT_PATH)
find_library(QWT_LIBRARY NAMES qwt PATHS ${CMAKE_SOURCE_DIR}/libs/${OS}-${ARCH}/qwt/lib NO_DEFAULT_PATH)
find_library(QWTPLOT3D_LIBRARY NAMES qwtplot3d PATHS ${CMAKE_SOURCE_DIR}/libs/${OS}-${ARCH}/qwtplot3d/lib NO_DEFAULT_PATH)
find_library(TAMUANOVA_LIBRARY NAMES tamuanova PATHS ${CMAKE_SOURCE_DIR}/libs/${OS}-${ARCH}/tamuanova/lib NO_DEFAULT_PATH)

include_directories(
    ${CMAKE_SOURCE_DIR}/libs/${OS}-${ARCH}/alglib/include/alglib/
    ${CMAKE_SOURCE_DIR}/libs/${OS}-${ARCH}/gsl/include/
    ${CMAKE_SOURCE_DIR}/libs/${OS}-${ARCH}/muparser/include/muparser/
    ${CMAKE_SOURCE_DIR}/libs/${OS}-${ARCH}/qwt/include/qwt/
    ${CMAKE_SOURCE_DIR}/libs/${OS}-${ARCH}/qwtplot3d/include/qwtplot3d/
    ${CMAKE_SOURCE_DIR}/libs/${OS}-${ARCH}/tamuanova/include/tamuanova/

    dialogs/
    ../core/
    ../lib/include/
    ../lib/3rdparty/qtcolorpicker/src
    ../matrix/
    ../plot2D/
    ../plot2D/dialogs
    ../scripting/
    ../table/)

set(SOURCES
    dialogs/AnovaDialog.cpp
    dialogs/BaselineDialog.cpp
    dialogs/ExpDecayDialog.cpp
    dialogs/FFTDialog.cpp
    dialogs/FilterDialog.cpp
    dialogs/FitDialog.cpp
    dialogs/FrequencyCountDialog.cpp
    dialogs/GriddingDialog.cpp
    dialogs/IntDialog.cpp
    dialogs/IntegrationDialog.cpp
    dialogs/InterpolationDialog.cpp
    dialogs/PolynomFitDialog.cpp
    dialogs/SmoothCurveDialog.cpp
    dialogs/StudentTestDialog.cpp
    dialogs/SubtractDataDialog.cpp
    Anova.cpp
    ChiSquareTest.cpp
    Convolution.cpp
    Correlation.cpp
    Differentiation.cpp
    ExponentialFit.cpp
    FFT.cpp
    fft2D.cpp
    FFTFilter.cpp
    Filter.cpp
    Fit.cpp
    fit_gsl.cpp
    FitModelHandler.cpp
    Integration.cpp
    Interpolation.cpp
    LogisticFit.cpp
    MultiPeakFit.cpp
    NonLinearFit.cpp
    PluginFit.cpp
    PolynomialFit.cpp
    ShapiroWilkTest.cpp
    SigmoidalFit.cpp
    SmoothFilter.cpp
    Statistics.cpp
    StatisticTest.cpp
    tTest.cpp
    lowess.c)

set(HEADERS
    dialogs/AnovaDialog.h
    dialogs/BaselineDialog.h
    dialogs/ExpDecayDialog.h
    dialogs/FFTDialog.h
    dialogs/FilterDialog.h
    dialogs/FitDialog.h
    dialogs/FrequencyCountDialog.h
    dialogs/GriddingDialog.h
    dialogs/IntDialog.h
    dialogs/IntegrationDialog.h
    dialogs/InterpolationDialog.h
    dialogs/PolynomFitDialog.h
    dialogs/SmoothCurveDialog.h
    dialogs/StudentTestDialog.h
    dialogs/SubtractDataDialog.h
    Anova.h
    ChiSquareTest.h
    Convolution.h
    Correlation.h
    Differentiation.h
    ExponentialFit.h
    FFT.h
    fft2D.h
    FFTFilter.h
    Filter.h
    Fit.h
    fit_gsl.h
    FitModelHandler.h
    Integration.h
    Interpolation.h
    LogisticFit.h
    MultiPeakFit.h
    NonLinearFit.h
    PluginFit.h
    PolynomialFit.h
    ShapiroWilkTest.h
    SigmoidalFit.h
    SmoothFilter.h
    Statistics.h
    StatisticTest.h
    tTest.h)

QT4_WRAP_CPP(MOCS ${HEADERS})

add_definitions(-DHAVE_TAMUANOVA=1)

add_library(analysis STATIC ${SOURCES} ${MOCS})
target_link_libraries(analysis PUBLIC
    ${ALGLIB_LIBRARY} ${GSL_LIBRARY} ${MUPARSER_LIBRARY} ${QWT_LIBRARY} ${QWTPLOT3D_LIBRARY} ${TAMUANOVA_LIBRARY}
    Qt4::QtCore Qt4::QtGui Qt4::QtOpenGL Qt4::QtXml Qt4::Qt3Support)
