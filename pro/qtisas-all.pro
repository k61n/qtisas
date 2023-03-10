
TARGET = qtisas
TEMPLATE = app

QT += opengl network svg xml qt3support

QMAKE_PROJECT_DEPTH =

CONFIG += qt warn_on exceptions opengl thread assistant
CONFIG -= debug debug_and_release
CONFIG += release

CONFIG += ascii1d dan compile fittable jnse svd tiff yaml

###############################################################
## BASIC PROJECT PROPERTIES ###################################
###############################################################
DEFINES += QT_PLUGIN TEX_OUTPUT HAVE_ALGLIB HAVE_TAMUANOVA

ascii1d:    DEFINES += ASCII1D
compile:    DEFINES += COMPILE
dan:        DEFINES += DAN
fittable:   DEFINES += FITTABLE
jnse:       DEFINES += JNSE
svd:        DEFINES += SVD
tiff:       DEFINES += TIFFTIFF
yaml:       DEFINES += YAMLYAML

###############################################################
#### Scripts: muParser  + (optional) Python ###################
###############################################################
SCRIPTING_LANGS += muParser
contains(py, yes) {
    TARGET = qtisas-py
    SCRIPTING_LANGS += Python
    DEFINES += SCRIPTING_PYTHON
    DEFINES += SCRIPTING_CONSOLE
}

###############################################################
#### "Folder"-variables. $$SYS comes from "pri"-files, ########
#### platform-dependent #######################################
###############################################################
QTISAS_FOLDER = $$PWD/..
QTISAS_TMP = $${QTISAS_FOLDER}/tmp/$${OS}-$${ARCH}
QTISAS_LIB = $${QTISAS_FOLDER}/libs/$${OS}-$${ARCH}
QTISAS_BIN = $${QTISAS_FOLDER}/bin/$${OS}-$${ARCH}
QTISAS_INC = $${QTISAS_FOLDER}/libs/$${OS}-$${ARCH}

###############################################################
#### System-dependent ".pri" files ############################
###############################################################
equals(OS, "Linux") {
    include($$PWD/qtisas-linux64.pri)
} else {
    equals(OS, "Darwin") {
        #include($$PWD/qtisas-mac.pri)
    } else {
        #include($$PWD/qtisas-win.pri)
    }
}

###############################################################
#### Qt-directories, redirection of outout ####################
###############################################################
DESTDIR = $${QTISAS_BIN}
MOC_DIR = $${QTISAS_TMP}/$$TARGET
OBJECTS_DIR = $${QTISAS_TMP}/$$TARGET
SIP_DIR = $${QTISAS_TMP}/$$TARGET
UI_DIR = $${QTISAS_TMP}/$$TARGET

###############################################################
## 3rd PARTY  #################################################
###############################################################
INCLUDEPATH += \
    $${QTISAS_INC}/alglib/include/alglib \
    $${QTISAS_INC}/gsl/include \
    $${QTISAS_INC}/muparser/include/muparser \
    $${QTISAS_INC}/qtexengine/include/qtexengine \
    $${QTISAS_INC}/qwt/include/qwt \
    $${QTISAS_INC}/qwtplot3d/include/qwtplot3d \
    $${QTISAS_INC}/tamuanova/include/tamuanova
tiff: INCLUDEPATH += $${QTISAS_INC}/tiff/include
yaml: INCLUDEPATH += $${QTISAS_INC}/yaml-cpp/include

LIBS += \
    $${QTISAS_LIB}/alglib/lib/libalglib.a \
    $${QTISAS_LIB}/gsl/lib/libgsl.a \
    $${QTISAS_LIB}/gsl/lib/libgslcblas.a \
    $${QTISAS_LIB}/minigzip/lib/libminigzip.a \
    $${QTISAS_LIB}/muparser/lib/libmuparser.a \
    $${QTISAS_LIB}/qtexengine/lib/libqtexengine.a \
    $${QTISAS_LIB}/qwt/lib/libqwt.a \
    $${QTISAS_LIB}/tamuanova/lib/libtamuanova.a
!win32: LIBS += $${QTISAS_LIB}/qwtplot3d/lib/libqwtplot3d.a
tiff:   LIBS += $${QTISAS_LIB}/tiff/lib/libtiff.a
yaml:   LIBS += $${QTISAS_LIB}/yaml-cpp/lib/libyaml-cpp.a

###############################################################
## Resources ##################################################
###############################################################

RESOURCES     = $$QTISAS_FOLDER/qtisas/icons/icons/icons.qrc

###############################################################
################# Modules #####################################
###############################################################

include($$QTISAS_FOLDER/qtisas/sans/common/common.pri)
include($$QTISAS_FOLDER/qtisas/src/analysis/analysis.pri)
include($$QTISAS_FOLDER/qtisas/src/core/core.pri)
include($$QTISAS_FOLDER/qtisas/src/lib/libqti.pri)
include($$QTISAS_FOLDER/qtisas/src/plot2D/plot2D.pri)
include($$QTISAS_FOLDER/qtisas/src/plot3D/plot3D.pri)
include($$QTISAS_FOLDER/qtisas/src/matrix/matrix.pri)
include($$QTISAS_FOLDER/qtisas/src/table/table.pri)

svd:        include($$QTISAS_FOLDER/qtisas/sans/svd/svd.pri)
jnse:       include($$QTISAS_FOLDER/qtisas/sans/jnse/jnse.pri)
dan:        include($$QTISAS_FOLDER/qtisas/sans/dan/dan.pri)
compile:    include($$QTISAS_FOLDER/qtisas/sans/compile/compile.pri)
fittable:   include($$QTISAS_FOLDER/qtisas/sans/fittable/fittable.pri)
ascii1d:    include($$QTISAS_FOLDER/qtisas/sans/ascii1d/ascii1d.pri)

include($$QTISAS_FOLDER/qtisas/src/scripting/scripting.pri)

###############################################################
#### Scripts: + (optional) Python #############################
###############################################################
contains(py,yes): include(./qtisas-python.pri)

###############################################################
# At the very end: add global include- and lib path ###########
###############################################################
unix: INCLUDEPATH 	+= $$SYS_INCLUDEPATH
unix: LIBS 			+= $$SYS_LIBS
