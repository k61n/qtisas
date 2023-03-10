message("------------------------------------------------------")
message("--- Option 1 :: 'qmake'        :: without python -----")
message("--- Option 2 :: 'qmake py=yes' :: with___ python -----")
message("------------------------------------------------------")

TEMPLATE = subdirs

CONFIG	+= ordered
CONFIG  -= debug
CONFIG  += release silent
QMAKE_CXXFLAGS_WARN_ON += -w

OS = $$system(uname -s)
ARCH = $$system(uname -m)

DEFINES += OS
DEFINES += ARCH

equals(OS, "Linux") {
    CORES = $$system("nproc")
} else:equals(OS, "Darwin") {
    CORES = $$system("sysctl -n hw.ncpu")
} else {
    CORES = 1
}

isEmpty(system("[ -d libs/$$OS-$$ARCH ]")) {
    system(mkdir -p libs/$$OS-$$ARCH)
}
system(chmod 777 scripts/buildlib.sh)
DEPS += alglib minigzip muparser qtexengine qwt qwtplot3d tamuanova tiff yaml-cpp
for(dep, DEPS) {
    system(./scripts/buildlib.sh $$OS $$ARCH $$CORES $$dep 3rdparty/$$dep/)
}
system(./scripts/build-gsl.sh $$OS $$ARCH $$CORES 3rdparty/gsl/)

include(pro/qtisas-all.pro)
