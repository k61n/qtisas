# QtiSAS

This software can be successfully build on systems having Qt5 library and modern
cmake.

## Linux

Since 32b3cb47 the git repo includes submodules, therefore when cloning use
--recurse-submodules flag:

    git clone --recurse-submodules https://iffgit.fz-juelich.de/qtisas/qtisas.git

Use cmake to build the software:

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

### Prepare build environment

We need right dependencies and modern cmake.

#### Ubuntu 18.04

Dependencies:

    sudo apt install git wget
        build-essential libpng-dev libtiff-dev zlib1g-dev
        qtbase5-dev libqt5svg5-dev
        python3 libpython3-dev pyqt5-dev pyqt5-dev-tools sip-dev python3-sip-dev

Activate Kitware repository to install cmake:

    wget -O - https://apt.kitware.com/keys/kitware-archive-latest.asc | gpg --dearmor - | sudo tee /usr/share/keyrings/kitware-archive-keyring.gpg
    echo 'deb [signed-by=/usr/share/keyrings/kitware-archive-keyring.gpg] https://apt.kitware.com/ubuntu/ bionic main' | sudo tee /etc/apt/sources.list.d/kitware.list
    sudo apt update
    sudo apt install -y cmake

#### Rocky Linux 8

Enable `powertools` repository that contains
`python3-qt5-devel` package::

    dnf install -y dnf-plugins-core
    dnf config-manager --set-enabled powertools
    dnf update

Dependencies:

    dnf install -y git wget \
        mesa-libGLU-devel libpng-devel libtiff-devel zlib-devel
        qt5-qtbase-devel qt5-qtsvg-devel
        python3-qt5-devel

Cmake:

    wget https://github.com/Kitware/CMake/releases/download/v3.27.1/cmake-3.27.1-linux-x86_64.tar.gz
    tar -xvf cmake-3.27.1-linux-x86_64.tar.gz
    PATH=~/cmake-3.27.1-linux-x86_64/bin:$PATH
    exec bash

#### Rocky Linux 9

Enable `devel` repository that contains `python3-qt5-devel` and `sip6`
packages:

    dnf install -y dnf-plugins-core
    dnf config-manager --set-enabled devel
    dnf update

Dependencies:

    dnf install -y git wget \
        mesa-libGLU-devel libpng-devel libtiff-devel zlib-devel
        qt5-qtbase-devel qt5-qtsvg-devel python3 python3-devel sip6

Cmake:

    wget https://github.com/Kitware/CMake/releases/download/v3.27.1/cmake-3.27.1-linux-x86_64.tar.gz && \
    tar -xvf cmake-3.27.1-linux-x86_64.tar.gz
    PATH=~/cmake-3.27.1-linux-x86_64/bin:$PATH
    exec bash

## Windows

Install following apps:

    git - https://git-scm.com/download/win
    cmake - https://cmake.org/download/
    mingw, Qt5 - https://www.qt.io/download-open-source
    zlib - https://gnuwin32.sourceforge.net/packages/zlib.htm
    libpng - https://gnuwin32.sourceforge.net/packages/libpng.htm
    libtiff - https://gnuwin32.sourceforge.net/packages/tiff.htm
    python - https://www.python.org/ftp/python/

Since zlib, libpng and libtiff for windows are most easily obtained as 32 bit
versions, the qtisas is build also as 32 bit application.
Using qt installer MinGW 32-bit qt5 version as well as 32-bit MinGW itself
should be downloaded. Python version should be 32-bit as well.

Add to the path:

    C:\Program Files\Git\cmd
    C:\Qt\Tools\mingw810_32\bin
    C:\Program Files (x86)\CMake\bin
    C:\Program Files (x86)\GnuWin32\bin
    C:\Qt\5.15.2\mingw81_32
    C:\Users\kk\AppData\Local\Programs\Python\Python311-32
    C:\Users\kk\AppData\Local\Programs\Python\Python311-32\Scripts
    bin folders of 3rdparty libs in qtisas\libs\Windows-...

Use powershell to install few packages from pip:

    pip3.exe install pyqt5 pyqt-builder sip

Use PowerShell::

    git.exe clone --recurse-submodules https://iffgit.fz-juelich.de/qtisas/qtisas.git
    cd qtisas
    mkdir build
    cd build
    cmake.exe .. -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release
        -DCMAKE_MAKE_PROGRAM="C:/Qt/Tools/mingw810_32/bin/mingw32-make.exe"
        -DCMAKE_C_COMPILER="C:/Qt/Tools/mingw810_32/bin/gcc.exe"
        -DCMAKE_CXX_COMPILER="C:/Qt/Tools/mingw810_32/bin/g++.exe"
        -DTIFF_ROOT="C:/Program Files (x86)/GnuWin32"
        -DZLIB_ROOT="C:/Program Files (x86)/GnuWin32"
        -DPNG_ROOT="C:/Program Files (x86)/GnuWin32"
        -DCMAKE_PREFIX_PATH="C:/Qt/5.15.2/mingw81_32"
        -DPython3_ROOT_DIR="C:\Users\kk\AppData\Local\Programs\Python\Python311-32"
        -DWITH_PYTHON=ON
    cmake.exe --build . --parallel $env:NUMBER_OF_PROCESSORS

If everything is available in path, run bin/qtisas.exe.

## MacOS

Xcode command line tools are prerequisite. Then a user might need brew or other
software packaging system. Once you have it:

    brew install cmake qt@5 libtiff python3 pyqt@5 pyqt-builder sip scipy

Clone the repository:

    git clone --recurse-submodules https://iffgit.fz-juelich.de/qtisas/qtisas.git

Build the software:

    cd qtisas
    mkdir build
    cd build
    cmake .. -DCMAKE_C_COMPILER=/usr/bin/clang -DCMAKE_CXX_COMPILER=/usr/bin/clang++
        -DCMAKE_PREFIX_PATH=/opt/homebrew/opt/qt@5
        -DPython3_ROOT_DIR=/opt/homebrew/opt/python@3
        -DCMAKE_BUILD_TYPE=Release -DWITH_PYTHON=ON
    cmake --build . --parallel $(sysctl -n hw.ncpu)
    open bin/qtisas.app
