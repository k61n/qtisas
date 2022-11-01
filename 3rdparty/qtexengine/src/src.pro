# qmake project file for building the QTeXEngine libraries

mac:   SYS=mac$$system(sw_vers -productVersion)
linux-g++: SYS = linux64
linux-g++-64: SYS = linux64
###############################################################
## OS: win ####################################################
###############################################################
win32{
    SYS=winxp
    
    winver = $$system(ver)
    
    lst1 = $$split(winver, " ")
    verstr=$$member(lst1, 3)
    lst2 = $$split(verstr,".")
    vernum=$$member(lst2, 0)
    
    contains(vernum,11) SYS=win11
    contains(vernum,10) SYS=win10
    contains(vernum,6)  SYS=win
}
include( ../config.pri )

TARGET   = qtexengine
TEMPLATE     = lib

#######################
# NEW :: QTIKWS12
#######################
MOC_DIR		= ../../../tmp/$${SYS}/qtexengine
OBJECTS_DIR	= ../../../tmp/$${SYS}/qtexengine
DESTDIR		= ../../../lib/$${SYS}

contains(CONFIG, QTeXEngineDll) {
    CONFIG  += dll
    DEFINES += QTEXENGINE_DLL QTEXENGINE_DLL_BUILD
} else {
    CONFIG  += staticlib
}

HEADERS  = QTeXEngine.h
SOURCES += QTeXPaintEngine.cpp
SOURCES += QTeXPaintDevice.cpp
