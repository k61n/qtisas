message("------------------------------------------------------")
message("--- Option 1 :: 'qmake'        :: without python -----")
message("--- Option 2 :: 'qmake py=yes' :: with___ python -----")
message("------------------------------------------------------")

TEMPLATE = subdirs

CONFIG	+= ordered
CONFIG  -= debug
CONFIG  += release

# remove old makefiles
unix:system(chmod 777 ./scripts/clean-for-unix)
unix:system(./scripts/clean-for-unix)
win32:    system(call .\scripts\clean-for-windows.bat)

linux-g++: SYS=linux64
linux-g++-64: SYS=linux64
mac: SYS=mac

win32: SYS=winxp
win32{
winver = $$system(ver)
lst1 = $$split(winver, " ")
verstr=$$member(lst1, 3)
lst2 = $$split(verstr,".")
vernum=$$member(lst2, 0)
contains(vernum,11) SYS=win11
contains(vernum,10) SYS=win10
contains(vernum,6)  SYS=win
}
# mac relevant compilation
SYSVERSION=
mac: SYSVERSION=$$system(sw_vers -productVersion)

unix{
system(mkdir -p lib)
system(mkdir -p ./lib/$$SYS$$SYSVERSION)

# build alglib
system(chmod 777 ./scripts/build-alglib)
system(./scripts/build-alglib $$SYS$$SYSVERSION ./3rdparty/alglib/)

# build gsl
system(chmod 777 ./scripts/build-gsl)
system(./scripts/build-gsl $$SYS$$SYSVERSION ./3rdparty/gsl/ $$SYS)

# build muparser
system(chmod 777 ./scripts/build-muparser)
system(./scripts/build-muparser $$SYS$$SYSVERSION ./3rdparty/muparser/)

# build anova libriry
system(chmod 777 ./scripts/build-anova)
system(./scripts/build-anova $$SYS$$SYSVERSION ./3rdparty/tamu-anova/)

# build yaml-cpp
system(chmod 777 ./scripts/build-yaml)
system(./scripts/build-yaml $$SYS$$SYSVERSION ./3rdparty/yaml-cpp/)

# build tiff
system(chmod 777 ./scripts/build-tiff)
system(./scripts/build-tiff $$SYS$$SYSVERSION ./3rdparty/tiff/)
}

SUBDIRS =

exists( ./lib/$${SYS}$${SYSVERSION}/libqwt.a ){
    message("existing: libqwt.a")
}else{
    SUBDIRS += 	3rdparty/qwt
}
exists( ./lib/$${SYS}$${SYSVERSION}/libqwtplot3d.a  ){
    message("existing: libqwtplot3d.a ")
}else{
    SUBDIRS +=	3rdparty/qwtplot3d
}
exists( ./lib/$${SYS}$${SYSVERSION}/libqtexengine.a ){
    message("existing: libqtexengine.a")
}else{
    SUBDIRS +=	3rdparty/qtexengine
}

SUBDIRS +=	pro/qtisas-all.pro



