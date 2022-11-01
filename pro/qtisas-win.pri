###############################################################
## OS: win ####################################################
###############################################################
SYS=winxp

winver = $$system(ver)

lst1 = $$split(winver, " ")
verstr=$$member(lst1, 3)
lst2 = $$split(verstr,".")
vernum=$$member(lst2, 0)

contains(vernum,11) SYS=win11
contains(vernum,10) SYS=win10
contains(vernum,6)  SYS=win

###############################################################
## date #######################################################
###############################################################
QTISAS_DATE	= $$system(DATE /T)

###############################################################
## post linking actions #######################################
###############################################################
QMAKE_POST_LINK=$$quote(mkdir -p ../bin/$${SYS}/$${QTISAS_DATE} )
QMAKE_POST_LINK+=$$quote( && cp -f ../bin/$${SYS}/$${TARGET}.exe ../bin/$${SYS}/$${QTISAS_DATE}/$${TARGET}.exe )

###############################################################
## CONFIG AND DEFINES #########################################
###############################################################
CONFIG			+= console
CONFIG			+= qaxcontainer
DEFINES			+= QT_DLL QT_THREAD_SUPPORT
QMAKE_LFLAGS 	+= -Wl,-enable-auto-import

###############################################################
## QwtPlot3D - use local copy till upstream catches up ########
###############################################################
contains(SYS,win)LIBS			+= ../bin/$${SYS}/qwtplot3d.dll
contains(SYS,win10) LIBS            += ../bin/$${SYS}/libqwtplot3d.a

###############################################################
## other libraries ############################################
###############################################################
DEFINES 		+= GL2PS_HAVE_LIBPNG

contains(SYS,win){
    INCLUDEPATH += c:/mingw/include
    LIBS        += c:/mingw/bin/libpng3.dll
}

contains(SYS,win10){
    INCLUDEPATH        += c:/mingw32/include
    LIBS             += c:/mingw32/bin/libpng3.dll
    LIBS            += c:/mingw32/bin/zlib1.dll
}



###############################################################
## RC file  ###################################################
###############################################################
RC_FILE 		= ../qtisas/icons/qtisas.rc

