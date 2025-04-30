# QtiSAS

Qtisas can be successfully build on systems having Qt library of version 5.15
and above. Minimal python version is 3.9.2.

## Linux

    git clone --recurse-submodules https://iffgit.fz-juelich.de/qtisas/qtisas.git
    cd qtisas
    mkdir build
    cd build
    cmake .. -DCMAKE_BUILD_TYPE=Release -DWITH_PYTHON=ON
    cmake --build . --parallel $(nproc)
    ../bin/qtisas

### Dependencies

#### Debian 12

    apt install -y cmake git wget
        build-essential libglu1-mesa-dev
        qt6-base-dev libqt6opengl6-dev libqt6svg6-dev
        libpython3-dev pyqt6-dev python3-sipbuild python3-pyqtbuild sip-tools
        libgl2ps-dev libgsl-dev libhdf5-dev libmuparser-dev libtiff-dev libyaml-cpp-dev

#### Rocky Linux 9

    dnf install -y dnf-plugins-core
    dnf config-manager --set-enabled devel
    dnf install -y epel-release
    cd /etc/yum.repos.d/
    wget https://download.opensuse.org/repositories/home:kholostov/RockyLinux_9/home:kholostov.repo
    dnf up -y
    dnf install -y cmake git wget
        mesa-libGLU-devel qt5-qtbase-devel qt5-qtsvg-devel
        python3-devel python3-qt5-devel sip6 PyQt-builder
        gl2ps-devel gsl-devel hdf5-devel muparser-devel libtiff-devel yaml-cpp-devel zlib-devel

## Windows

Install following apps:

    git - https://git-scm.com/download/win
    cmake - https://cmake.org/download/
    Qt6 msvc2022 - https://www.qt.io/download-open-source
    python - https://www.python.org/ftp/python/
    MSVC2022 - https://visualstudio.microsoft.com/downloads/

And libs:

    gl2ps - https://gitlab.onelab.info/gl2ps/gl2ps.git
    gsl - https://github.com/k61n/gsl
    hdf5 - https://github.com/HDFGroup/hdf5
    libjpeg-turbo - https://github.com/libjpeg-turbo/libjpeg-turbo
    libtiff - https://gitlab.com/libtiff/libtiff
    muparser - https://github.com/beltoforion/muparser
    xz - https://github.com/tukaani-project/xz
    yaml-cpp - https://github.com/jbeder/yaml-cpp
    zlib - https://github.com/madler/zlib
    zstd - https://github.com/facebook/zstd

You can use our [precompiled libs](https://iffgit.fz-juelich.de/qtisas/win-libs/-/releases).

Use PowerShell (as an example, you might have different paths):

    git clone --recurse-submodules https://iffgit.fz-juelich.de/qtisas/qtisas.git
    cd qtisas
    mkdir build
    cd build
    cmake ..
        -DCMAKE_BUILD_TYPE=Release
        -Dgl2ps_ROOT="C:/qtisas_libs/msvc2022_64/gl2ps"
        -DGSL_ROOT="C:/qtisas_libs/msvc2022_64/gsl"
        -DHDF5_ROOT="C:/qtisas_libs/msvc2022_64/hdf5"
        -Dmuparser_ROOT="C:/qtisas_libs/msvc2022_64/muparser"
        -DTIFF_ROOT="C:/qtisas_libs/msvc2022_64/tiff"
        -Dyaml-cpp_ROOT="C:/qtisas_libs/msvc2022_64/yaml_cpp"
        -DZLIB_ROOT="C:/qtisas_libs/msvc2022_64/zlib"
        -DCMAKE_PREFIX_PATH="C:/Qt/6.8.3/msvc2022_64"
        -DWITH_PYTHON=ON
        -DPython3_ROOT_DIR="C:/Users/admin/AppData/Local/Programs/Python/Python312"
    cmake --build . --config Release

If all the dlls are available in the PATH, run bin/qtisas.exe.

## MacOS

Xcode command line tools is a prerequisite. Then a user might need brew or other
software packaging system. Once you have it:

    brew install cmake qt gl2ps gsl hdf5 muparser libtiff yaml-cpp
    /usr/bin/pip3 install pyqt6

Optionally it is also possible to build against python from brew:

    brew install python pyqt

Clone the repository:

    git clone --recurse-submodules https://iffgit.fz-juelich.de/qtisas/qtisas.git

Build the software:

    cd qtisas
    mkdir build
    cd build
    cmake ..
        -DCMAKE_BUILD_TYPE=Release
        -Dgl2ps_ROOT=/opt/homebrew/opt/gl2ps
        -DGSL_ROOT=/opt/homebrew/opt/gsl
        -DHDF5_ROOT=/opt/homebrew/opt/hdf5
        -Dmuparser_ROOT=/opt/$BREW/opt/muparser
        -DTIFF_ROOT=/opt/homebrew/opt/libtiff
        -Dyaml-cpp_ROOT=/opt/homebrew/opt/yaml-cpp
        -DCMAKE_PREFIX_PATH=/opt/homebrew/opt/qt
        -DWITH_PYTHON=ON
        -DPython3_ROOT_DIR=/Applications/Xcode.app/Contents/Developer/Library/Frameworks/Python3.framework/Versions/Current
    cmake --build . --parallel $(sysctl -n hw.ncpu)
    open bin/qtisas.app
