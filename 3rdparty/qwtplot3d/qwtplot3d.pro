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

# pro file for building the makefile for qwtplot3d

include (qwtplot3d.pri)
QT              += opengl
CONFIG          += qt warn_on opengl thread release

#######################
# NEW :: QTISAS
#######################
MOC_DIR		= ../../tmp/$${SYS}/qwtplot3d
OBJECTS_DIR	= ../../tmp/$${SYS}/qwtplot3d
win32:DESTDIR		= ../../bin/$${SYS}
unix:DESTDIR           = ../../lib/$${SYS}

INCLUDEPATH      = include
DEPENDPATH       = include src
#######################
# NEW :: QTISAS
#######################


linux-g++: CONFIG    += static
unix:CONFIG     += staticlib


contains(SYS,win){
    #win32:CONFIG    += static
    #win32:CONFIG    += dll
    #win32:CONFIG     += staticlib
    win32:CONFIG    += exceptions
    win32:dll:DEFINES    += QT_DLL QWT3D_DLL QWT3D_MAKEDLL
}
contains(SYS,win10){
    win32:CONFIG    += static
    #win32:CONFIG    += dll
    #win32:CONFIG     += staticlib
    #    win32:CONFIG    += exceptions
    #win32:dll:DEFINES    += QT_DLL QWT3D_DLL QWT3D_MAKEDLL

}



win32:QMAKE_CXXFLAGS += $$QMAKE_CFLAGS_STL

# Comment the next line, if you do not want debug message output
#DEFINES -= QT_NO_DEBUG_OUTPUT

#linux-g++:TMAKE_CXXFLAGS += -fno-exceptions
unix:VERSION     = 0.3.0

win32:DEFINES     += GL2PS_HAVE_LIBPNG
INCLUDEPATH += ../zlib/

contains(SYS,win){
win32: INCLUDEPATH += c:/mingw/include
win32: LIBS        += c:/mingw/bin/libpng3.dll
}
contains(SYS,win10){
    win32: INCLUDEPATH += c:/mingw32/include
    win32: LIBS        += c:/mingw32/lib/libz.a
    win32: LIBS        += c:/mingw32/bin/libpng3.dll
}
linux-g++:INCLUDEPATH    += /usr/include
mac: INCLUDEPATH      += /opt/X11/include

unix: LIBS += -lpng

#win32:LIBS        += c:/mingw/lib/libpng.dll.a
#LIBS         += -lpng

	
# install
target.path = lib
INSTALLS += target
