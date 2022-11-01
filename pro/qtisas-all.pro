###############################################################
#### TARGET name: qtisas (without python), ##################
#### qtisas-py (with) #########################################
###############################################################
TARGET      = qtisas
TEMPLATE    = app

QT  += opengl network svg xml
QT  += qt3support

QMAKE_PROJECT_DEPTH =

CONFIG        += qt warn_on exceptions opengl thread
CONFIG        += assistant
CONFIG        -= debug
CONFIG        -= debug_and_release
CONFIG        += release

CONFIG += dan
CONFIG += compile
CONFIG += fittable
CONFIG += svd
CONFIG += jnse
CONFIG += ascii1d
CONFIG += yaml
CONFIG += tiff

#CONFIG += sasplugins


###############################################################
## BASIC PROJECT PROPERTIES ###################################
###############################################################
DEFINES        += QT_PLUGIN
DEFINES        += TEX_OUTPUT
DEFINES        += HAVE_ALGLIB
DEFINES        += HAVE_TAMUANOVA

dan:        DEFINES += DAN
compile:    DEFINES += COMPILE
fittable:   DEFINES += FITTABLE
svd:        DEFINES += SVD
jnse:       DEFINES += JNSE
ascii1d:    DEFINES += ASCII1D
sasplugins: DEFINES += SASPLUGINS
yaml:       DEFINES += YAMLYAML
tiff:       DEFINES += TIFFTIFF

###############################################################
#### Scripts: muParser  + (optional) Python ###################
###############################################################
SCRIPTING_LANGS += muParser
contains(py,yes){
    TARGET = qtisas-py
    SCRIPTING_LANGS += Python
    DEFINES            += SCRIPTING_PYTHON
    DEFINES            += SCRIPTING_CONSOLE
}

###############################################################
#### System-dependent ".pri" files ############################
###############################################################
linux-g++:      include(./qtisas-linux64.pri)
linux-g++-64:   include(./qtisas-linux64.pri)
mac:            include(./qtisas-mac.pri)
win32:          include(./qtisas-win.pri)

###############################################################
#### "Folder"-variables. $$SYS comes from "pri"-files, ########
#### platform-dependent #######################################
###############################################################
QTISAS_FOLDER	= ..
QTISAS_TMP	= $${QTISAS_FOLDER}/tmp/$${SYS}
QTISAS_LIB	= $${QTISAS_FOLDER}/lib/$${SYS}
QTISAS_BIN	= $${QTISAS_FOLDER}/bin/$${SYS}
QTISAS_INC	= $${QTISAS_FOLDER}/3rdparty

###############################################################
#### Qt-directories, redirection of outout ####################
###############################################################
DESTDIR 	    = $${QTISAS_BIN}
MOC_DIR        	= $${QTISAS_TMP}/$$TARGET
OBJECTS_DIR    	= $${QTISAS_TMP}/$$TARGET
SIP_DIR        	= $${QTISAS_TMP}/$$TARGET
UI_DIR          = $${QTISAS_TMP}/$$TARGET

###############################################################
## 3rd PARTY HEADER FILES SECTION #############################
###############################################################
INCLUDEPATH	        += $${QTISAS_INC}/zlib
INCLUDEPATH	        += $${QTISAS_INC}/muparser/include
INCLUDEPATH	        += $${QTISAS_INC}/qwt/src
INCLUDEPATH	        += $${QTISAS_INC}/qwtplot3d/include
INCLUDEPATH	        += $${QTISAS_INC}/qtexengine/src
INCLUDEPATH	        += $${QTISAS_INC}/alglib/src
INCLUDEPATH	        += $${QTISAS_INC}/tamu-anova
INCLUDEPATH	        += $${QTISAS_INC}/gsl/include
yaml:INCLUDEPATH    += $${QTISAS_INC}/yaml-cpp/include
tiff:INCLUDEPATH    += $${QTISAS_INC}/tiff/libtiff

###############################################################
## 3rd PARTY LIBRARIES FILES SECTION ##########################
###############################################################
LIBS        += $${QTISAS_LIB}/libgsl.a
LIBS        += $${QTISAS_LIB}/libgslcblas.a
LIBS		+= $${QTISAS_LIB}/libqtexengine.a
LIBS		+= $${QTISAS_LIB}/libqwt.a
!win32:LIBS += $${QTISAS_LIB}/libqwtplot3d.a
LIBS		+= $${QTISAS_LIB}/libalglib.a
LIBS		+= $${QTISAS_LIB}/libtamuanova.a
LIBS        += $${QTISAS_LIB}/libmuparser.a
yaml:LIBS   += $${QTISAS_LIB}/libyaml-cpp.a
tiff:LIBS   += $${QTISAS_LIB}/libtiff.a

###############################################################
## PROJECT FILES SECTION ######################################
###############################################################

###################### ICONS ##################################
INCLUDEPATH  += ../qtisas/icons/
RESOURCES     = ../qtisas/icons/icons/icons.qrc

###############################################################
##################### Compression (zlib-1.2.3) ################
###############################################################
SOURCES 	+= ../3rdparty/zlib/minigzip.c

###############################################################
################# Default Modules #############################
###############################################################
include(../qtisas/sans/common/common.pri)
include(../qtisas/src/analysis/analysis.pri)
include(../qtisas/src/core/core.pri)
include(../qtisas/src/lib/libqti.pri)
include(../qtisas/src/plot2D/plot2D.pri)
include(../qtisas/src/plot3D/plot3D.pri)
include(../qtisas/src/matrix/matrix.pri)
include(../qtisas/src/table/table.pri)
include(../qtisas/src/scripting/scripting.pri)

svd:        include(../qtisas/sans/svd/svd.pri)
jnse:       include(../qtisas/sans/jnse/jnse.pri)
dan:        include(../qtisas/sans/dan/dan.pri)
compile:    include(../qtisas/sans/compile/compile.pri)
fittable:   include(../qtisas/sans/fittable/fittable.pri)
ascii1d:    include(../qtisas/sans/ascii1d/ascii1d.pri)

###############################################################
#### Scripts: + (optional) Python #############################
###############################################################
contains(py,yes): include(./qtisas-python.pri)

###############################################################
# At the very end: add global include- and lib path ###########
###############################################################
unix: INCLUDEPATH 	+= $$SYS_INCLUDEPATH
unix: LIBS 			+= $$SYS_LIBS


###############################################################
unix:system {
    contains(DATE) {
    rm $$OBJECTS_DIR/ApplicationWindow.o
    }
}

win32 {
     TMPPATH=$$OBJECTS_DIR/ApplicationWindow.o
     TMPPATH=$$replace(TMPPATH,/,\\) 
     system(del $$TMPPATH)
}

