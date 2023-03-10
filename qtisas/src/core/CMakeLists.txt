
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)

set(OS ${CMAKE_SYSTEM_NAME})
set(ARCH ${CMAKE_SYSTEM_PROCESSOR})

find_package(Qt4 COMPONENTS QtGui QtOpenGL QtSvg QtXml Qt3Support REQUIRED)
find_library(GSL_LIBRARY NAMES gsl PATHS ${CMAKE_SOURCE_DIR}/libs/${OS}-${ARCH}/gsl/lib NO_DEFAULT_PATH)
find_library(MUPARSER_LIBRARY NAMES muparser PATHS ${CMAKE_SOURCE_DIR}/libs/${OS}-${ARCH}/muparser/lib NO_DEFAULT_PATH)
find_library(QWT_LIBRARY NAMES qwt PATHS ${CMAKE_SOURCE_DIR}/libs/${OS}-${ARCH}/qwt/lib NO_DEFAULT_PATH)
find_library(QWTPLOT3D_LIBRARY NAMES qwtplot3d PATHS ${CMAKE_SOURCE_DIR}/libs/${OS}-${ARCH}/qwtplot3d/lib NO_DEFAULT_PATH)

include_directories(
    ${CMAKE_SOURCE_DIR}/libs/${OS}-${ARCH}/gsl/include/
    ${CMAKE_SOURCE_DIR}/libs/${OS}-${ARCH}/muparser/include/muparser/
    ${CMAKE_SOURCE_DIR}/libs/${OS}-${ARCH}/qwt/include/qwt/
    ${CMAKE_SOURCE_DIR}/libs/${OS}-${ARCH}/qwtplot3d/include/qwtplot3d/

    ../analysis/
    ../analysis/dialogs/
    ../lib/include/
    ../lib/3rdparty/qtcolorpicker/src/
    ../matrix/
    ../plot2D/
    ../plot2D/dialogs/
    ../plot3D/
    ../scripting/
    ../table/

    ${CMAKE_SOURCE_DIR}/qtisas/sans/ascii1d/
    ${CMAKE_SOURCE_DIR}/qtisas/sans/compile/
    ${CMAKE_SOURCE_DIR}/qtisas/sans/dan/
    ${CMAKE_SOURCE_DIR}/qtisas/sans/fittable/
    ${CMAKE_SOURCE_DIR}/qtisas/sans/genmin/include/
    ${CMAKE_SOURCE_DIR}/qtisas/sans/jnse/
    ${CMAKE_SOURCE_DIR}/qtisas/sans/svd/

    ${CMAKE_CURRENT_BINARY_DIR}/../../sans/ascii1d/
    ${CMAKE_CURRENT_BINARY_DIR}/../../sans/compile/
    ${CMAKE_CURRENT_BINARY_DIR}/../../sans/dan/
    ${CMAKE_CURRENT_BINARY_DIR}/../../sans/fittable/
    ${CMAKE_CURRENT_BINARY_DIR}/../../sans/jnse/
    ${CMAKE_CURRENT_BINARY_DIR}/../../sans/svd/)

set(SOURCES
    main.cpp
    ApplicationWindow.cpp
    ConfigDialog.cpp
    CreateBinMatrixDialog.cpp
    CustomActionDialog.cpp
    FindDialog.cpp
    Folder.cpp
    ImportASCIIDialog.cpp
    MdiSubWindow.cpp
    OpenProjectDialog.cpp
    PlotWizard.cpp
    QtiPlotApplication.cpp
    RenameWindowDialog.cpp)

set(HEADERS
    ApplicationWindow.h
    ConfigDialog.h
    CreateBinMatrixDialog.h
    CustomActionDialog.h
    FindDialog.h
    Folder.h
    globals.h
    ImportASCIIDialog.h
    ImportExportPlugin.h
    MdiSubWindow.h
    OpenProjectDialog.h
    PlotWizard.h
    QtiPlotApplication.h
    RenameWindowDialog.h)

QT4_WRAP_CPP(MOCS ${HEADERS})

add_definitions(-DASCII1D=1 -DDAN=1 -DCOMPILE=1 -DFITTABLE=1 -DHAVE_ALGLIB=1 -DJNSE=1 -DSVD=1)

add_library(core STATIC ${SOURCES} ${MOCS})
add_dependencies(core ascii1d analysis compile dan fittable jnse svd)
target_link_libraries(core PUBLIC analysis
    ${GSL_LIBRARY} ${MUPARSER_LIBRARY} ${QWT_LIBRARY} ${QWTPLOT3D_LIBRARY}
    Qt4::QtGui Qt4::QtOpenGL Qt4::QtSvg Qt4::QtXml Qt4::Qt3Support)
