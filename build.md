# QtiSAS

This software can be successfully build on systems having Qt library of version
5.12 and above.

## Linux

    git clone --recurse-submodules https://iffgit.fz-juelich.de/qtisas/qtisas.git
    cd qtisas
    mkdir build
    cd build
    cmake .. -DCMAKE_C_COMPILER=/usr/bin/gcc -DCMAKE_CXX_COMPILER=/usr/bin/g++ \
        -DCMAKE_BUILD_TYPE=Release -DWITH_PYTHON=ON
    cmake --build . --parallel $(nproc)
    ../bin/qtisas

Generate .deb package:

    cpack -G DEB

Generate .rpm package:

    cpack -G RPM

### Dependencies

#### Debian 12

    apt install -y cmake git wget \
        build-essential libglu1-mesa-dev \
        qt6-base-dev libqt6opengl6-dev libqt6svg6-dev \
        libpython3-dev pyqt6-dev python3-sipbuild python3-pyqtbuild sip-tools \
        libgsl-dev libhdf5-dev libmuparser-dev libtiff-dev libyaml-cpp-dev

#### Rocky Linux 9

    dnf install -y dnf-plugins-core
    dnf config-manager --set-enabled devel
    dnf install -y epel-release
    cd /etc/yum.repos.d/
    wget https://download.opensuse.org/repositories/home:kholostov/RockyLinux_9/home:kholostov.repo
    dnf up -y
    dnf install -y cmake git wget \
        mesa-libGLU-devel qt5-qtbase-devel qt5-qtsvg-devel \
        python3-devel python3-qt5-devel sip6 PyQt-builder \
        gsl-devel hdf5-devel muparser-devel libtiff-devel yaml-cpp-devel zlib-devel

## Windows

Install following apps:

    git - https://git-scm.com/download/win
    cmake - https://cmake.org/download/
    mingw, Qt5/6 - https://www.qt.io/download-open-source
    python - https://www.python.org/ftp/python/

And libs:

    gsl - https://github.com/k61n/gsl
    hdf5 - https://github.com/HDFGroup/hdf5
    muparser - https://github.com/beltoforion/muparser
    libtiff - https://gitlab.com/libtiff/libtiff
    yaml-cpp - https://github.com/jbeder/yaml-cpp
    zlib - https://github.com/madler/zlib

You can use MSYS or similar tools to obtain the libs. Or use our [precompiled 
libs](https://iffgit.fz-juelich.de/qtisas/win-libs/-/releases).

Use powershell to install few packages from pip:

    pip3.exe install pyqt6 pyqt-builder sip

Use PowerShell (as an example, you might have different paths):

    git.exe clone --recurse-submodules https://iffgit.fz-juelich.de/qtisas/qtisas.git
    cd qtisas
    mkdir build
    cd build
    cmake.exe .. -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release
        -DCMAKE_MAKE_PROGRAM="C:/Qt/Tools/mingw1120_64/bin/mingw32-make.exe"
        -DCMAKE_C_COMPILER="C:/Qt/Tools/mingw1120_64/bin/gcc.exe"
        -DCMAKE_CXX_COMPILER="C:/Qt/Tools/mingw1120_64/bin/g++.exe"
        -DGSL_ROOT="C:/Users/admin/LIBS/mingw1120_64/gsl"
        -DHDF5_ROOT="C:/Users/admin/LIBS/mingw1120_64/hdf5"
        -Dmuparser_ROOT="C:/Users/admin/LIBS/mingw1120_64/muparser"
        -DTIFF_ROOT="C:/Users/admin/LIBS/mingw1120_64/tiff"
        -Dyaml-cpp_ROOT="C:/Users/admin/LIBS/mingw1120_64/yaml_cpp"
        -DZLIB_ROOT="C:/Users/admin/LIBS/mingw1120_64/zlib"
        -DCMAKE_PREFIX_PATH="C:/Qt/6.7.2/mingw_64"
        -DPython3_ROOT_DIR="C:/Users/admin/AppData/Local/Programs/Python/Python312"
        -DWITH_PYTHON=ON
    cmake.exe --build . --parallel $env:NUMBER_OF_PROCESSORS

If all the ddls are available in the PATH, run bin/qtisas.exe.

## MacOS

Xcode command line tools is a prerequisite. Then a user might need brew or other
software packaging system. Once you have it:

    brew install cmake qt gsl hdf5 muparser libtiff yaml-cpp
    /usr/bin/pip3 install sip pyqt6

Clone the repository:

    git clone --recurse-submodules https://iffgit.fz-juelich.de/qtisas/qtisas.git

Build the software:

    cd qtisas
    mkdir build
    cd build
    cmake .. -DCMAKE_C_COMPILER=/usr/bin/clang -DCMAKE_CXX_COMPILER=/usr/bin/clang++
        -DGSL_ROOT=/opt/homebrew/opt/gsl
        -DHDF5_ROOT=/opt/homebrew/opt/hdf5
        -Dmuparser_ROOT=/opt/homebrew/opt/muparser
        -DTIFF_ROOT=/opt/homebrew/opt/libtiff
        -Dyaml-cpp_ROOT=/opt/homebrew/opt/yaml-cpp
        -DCMAKE_PREFIX_PATH=/opt/homebrew/opt/qt
        -DPython3_ROOT_DIR=/Applications/Xcode.app/Contents/Developer/Library/Frameworks/Python3.framework/Versions/Current
        -DCMAKE_BUILD_TYPE=Release -DWITH_PYTHON=ON
    cmake --build . --parallel $(sysctl -n hw.ncpu)
    open bin/qtisas.app
