cmake_minimum_required(VERSION 3.10)

project(alglib)

set(CMAKE_CXX_STANDARD 17)

set(SOURCES
    src/ablas.cpp
    src/ablasf.cpp
    src/airyf.cpp
    src/ap.cpp
    src/apserv.cpp
    src/autogk.cpp
    src/bdss.cpp
    src/bdsvd.cpp
    src/bessel.cpp
    src/betaf.cpp
    src/binomialdistr.cpp
    src/blas.cpp
    src/chebyshev.cpp
    src/chisquaredistr.cpp
    src/conv.cpp
    src/corr.cpp
    src/correlation.cpp
    src/correlationtests.cpp
    src/creflections.cpp
    src/dawson.cpp
    src/densesolver.cpp
    src/descriptivestatistics.cpp
    src/dforest.cpp
    src/elliptic.cpp
    src/estnorm.cpp
    src/evd.cpp
    src/expintegrals.cpp
    src/fdistr.cpp
    src/fft.cpp
    src/fht.cpp
    src/fresnel.cpp
    src/ftbase.cpp
    src/gammafunc.cpp
    src/gkq.cpp
    src/gq.cpp
    src/hblas.cpp
    src/hermite.cpp
    src/hqrnd.cpp
    src/hsschur.cpp
    src/ialglib.cpp
    src/ibetaf.cpp
    src/idwint.cpp
    src/igammaf.cpp
    src/inverseupdate.cpp
    src/jacobianelliptic.cpp
    src/jarquebera.cpp
    src/kmeans.cpp
    src/laguerre.cpp
    src/lda.cpp
    src/ldlt.cpp
    src/legendre.cpp
    src/linmin.cpp
    src/linreg.cpp
    src/logit.cpp
    src/lsfit.cpp
    src/mannwhitneyu.cpp
    src/matdet.cpp
    src/matgen.cpp
    src/matinv.cpp
    src/minasa.cpp
    src/mincg.cpp
    src/minlbfgs.cpp
    src/minlm.cpp
    src/mlpbase.cpp
    src/mlpe.cpp
    src/mlptrain.cpp
    src/nearestneighbor.cpp
    src/nearunityunit.cpp
    src/normaldistr.cpp
    src/odesolver.cpp
    src/ortfac.cpp
    src/pca.cpp
    src/poissondistr.cpp
    src/polint.cpp
    src/psif.cpp
    src/pspline.cpp
    src/ratint.cpp
    src/ratinterpolation.cpp
    src/rcond.cpp
    src/reflections.cpp
    src/rotations.cpp
    src/safesolve.cpp
    src/sblas.cpp
    src/schur.cpp
    src/sdet.cpp
    src/sinverse.cpp
    src/spdgevd.cpp
    src/spline1d.cpp
    src/spline2d.cpp
    src/spline3.cpp
    src/srcond.cpp
    src/ssolve.cpp
    src/stest.cpp
    src/studenttdistr.cpp
    src/studentttests.cpp
    src/svd.cpp
    src/trfac.cpp
    src/trigintegrals.cpp
    src/trlinsolve.cpp
    src/tsort.cpp
    src/variancetests.cpp
    src/wsr.cpp
    src/xblas.cpp)

set(HEADERS
    include/ablas.h
    include/ablasf.h
    include/airyf.h
    include/ap.h
    include/apserv.h
    include/apvt.h
    include/autogk.h
    include/bdss.h
    include/bdsvd.h
    include/bessel.h
    include/betaf.h
    include/binomialdistr.h
    include/blas.h
    include/chebyshev.h
    include/chisquaredistr.h
    include/conv.h
    include/corr.h
    include/correlation.h
    include/correlationtests.h
    include/creflections.h
    include/dawson.h
    include/densesolver.h
    include/descriptivestatistics.h
    include/dforest.h
    include/elliptic.h
    include/estnorm.h
    include/evd.h
    include/expintegrals.h
    include/fdistr.h
    include/fft.h
    include/fht.h
    include/fresnel.h
    include/ftbase.h
    include/gammafunc.h
    include/gkq.h
    include/gq.h
    include/hblas.h
    include/hermite.h
    include/hqrnd.h
    include/hsschur.h
    include/ialglib.h
    include/ibetaf.h
    include/idwint.h
    include/igammaf.h
    include/inverseupdate.h
    include/jacobianelliptic.h
    include/jarquebera.h
    include/kmeans.h
    include/laguerre.h
    include/lda.h
    include/ldlt.h
    include/legendre.h
    include/linmin.h
    include/linreg.h
    include/logit.h
    include/lsfit.h
    include/mannwhitneyu.h
    include/matdet.h
    include/matgen.h
    include/matinv.h
    include/minasa.h
    include/mincg.h
    include/minlbfgs.h
    include/minlm.h
    include/mlpbase.h
    include/mlpe.h
    include/mlptrain.h
    include/nearestneighbor.h
    include/nearunityunit.h
    include/normaldistr.h
    include/odesolver.h
    include/ortfac.h
    include/pca.h
    include/poissondistr.h
    include/polint.h
    include/psif.h
    include/pspline.h
    include/ratint.h
    include/ratinterpolation.h
    include/rcond.h
    include/reflections.h
    include/rotations.h
    include/safesolve.h
    include/sblas.h
    include/schur.h
    include/sdet.h
    include/sinverse.h
    include/spdgevd.h
    include/spline1d.h
    include/spline2d.h
    include/spline3.h
    include/srcond.h
    include/ssolve.h
    include/stdafx.h
    include/stest.h
    include/studenttdistr.h
    include/studentttests.h
    include/svd.h
    include/trfac.h
    include/trigintegrals.h
    include/trlinsolve.h
    include/tsort.h
    include/variancetests.h
    include/wsr.h
    include/xblas.h)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

if (MSVC)
    add_compile_options($<$<CXX_COMPILER_ID:MSVC>:/MP>)
endif ()

add_library(alglib_static STATIC ${SOURCES} ${HEADERS})
if (MSVC AND BUILD_SHARED_LIBS)
    set_target_properties(alglib_static PROPERTIES OUTPUT_NAME "alglib_static")
else ()
    set_target_properties(alglib_static PROPERTIES OUTPUT_NAME "alglib")
endif ()

if (BUILD_SHARED_LIBS)
    add_library(alglib_shared SHARED $<TARGET_OBJECTS:alglib_static>)
    set_target_properties(alglib_shared PROPERTIES OUTPUT_NAME "alglib")
endif ()

include(GNUInstallDirs)
install(TARGETS alglib_static
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
if (BUILD_SHARED_LIBS)
    install(TARGETS alglib_shared
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
endif ()
install(FILES ${HEADERS}
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/alglib)
