# QtiSAS

Qtisas can be successfully build on systems having Qt library of version 5.15
and above, Qwt of version 6.3 and above. Minimal python version is 3.9.2.

## Linux

    git clone --recurse-submodules https://iffgit.fz-juelich.de/qtisas/qtisas.git
    cd qtisas
    mkdir build
    cd build
    cmake .. -DCMAKE_BUILD_TYPE=Release
    cmake --build . --parallel $(nproc)
    ../bin/qtisas

### Dependencies

#### Arch

    pacman -Syu cmake gcc ninja \
        glu gl2ps gsl hdf5 muparser libtiff qwt yaml-cpp \
        qt6-base qt6-svg pyqt-builder python-pyqt6 python-pyqt6-sip sip

Most Linux distros don't provide the upstream qwt. It needs to be built, or it
can be also installed from our repositories.

#### Debian 13

    curl -fsSL https://download.opensuse.org/repositories/home:kholostov/Debian_13/Release.key \
        | gpg --dearmor \
        | tee /etc/apt/trusted.gpg.d/home_kholostov.gpg \
        > /dev/null
    echo "deb [signed-by=/etc/apt/trusted.gpg.d/home_kholostov.gpg] http://download.opensuse.org/repositories/home:/kholostov/Debian_13/ /" \
        | tee /etc/apt/sources.list.d/home:kholostov.list
    apt install libqwt-dev

    apt install -y cmake ninja-build git wget \
        build-essential libgl2ps-dev libgsl-dev libhdf5-dev libmuparser-dev libtiff-dev libyaml-cpp-dev \
        libglu1-mesa-dev qt6-base-dev libqt6opengl6-dev libqt6svg6-dev \
        libpython3-dev pyqt6-dev python3-sipbuild python3-pyqtbuild sip-tools

#### Rocky Linux 10

Rocky Linux systems lack muParser library too.

    dnf install -y dnf-plugins-core
    dnf config-manager --set-enabled devel
    dnf install -y epel-release
    cd /etc/yum.repos.d/
    wget https://download.opensuse.org/repositories/home:kholostov/RockyLinux_10/home:kholostov.repo
    dnf up -y
    dnf install -y muParser-devel qwt-devel

    dnf install -y cmake ninja-build git \
        mesa-libGLU-devel \
        gl2ps-devel gsl-devel hdf5-devel libtiff-devel yaml-cpp-devel zlib-devel \
        qt6-qtbase-devel qt6-qtsvg-devel \
        python3-devel python3-pyqt6-devel sip6 PyQt-builder

#### Systems with Python3 < 3.11

Require tomli module.

    apt install python3-tomli

or

    dnf install python3-tomli

## Windows

Install following apps:

    git - https://git-scm.com/download/win
    cmake - https://cmake.org/download/
    Qt6 msvc2022 - https://www.qt.io/download-open-source
    python - https://www.python.org/ftp/python/
    MSVC2022 - https://visualstudio.microsoft.com/downloads/

And libs:

    gl2ps - https://gitlab.onelab.info/gl2ps/gl2ps.git
    gsl - https://git.savannah.gnu.org/git/gsl.git
    hdf5 - https://github.com/HDFGroup/hdf5
    libjpeg-turbo - https://github.com/libjpeg-turbo/libjpeg-turbo
    libtiff - https://gitlab.com/libtiff/libtiff
    muParser - https://github.com/beltoforion/muparser
    qwt - https://sourceforge.net/projects/qwt/
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
        -DBUILD_SHARED_LIBS=ON
        -DCMAKE_BUILD_TYPE=Release
        -Dgl2ps_ROOT="C:/qtisas_libs/msvc2022_64/gl2ps"
        -DGSL_ROOT="C:/qtisas_libs/msvc2022_64/gsl"
        -DHDF5_ROOT="C:/qtisas_libs/msvc2022_64/hdf5"
        -Dmuparser_ROOT="C:/qtisas_libs/msvc2022_64/muparser"
        -Dqwt_ROOT="C:/qtisas_libs/msvc2022_64/qwt"
        -DTIFF_ROOT="C:/qtisas_libs/msvc2022_64/tiff"
        -Dyaml-cpp_ROOT="C:/qtisas_libs/msvc2022_64/yaml_cpp"
        -DZLIB_ROOT="C:/qtisas_libs/msvc2022_64/zlib"
        -DCMAKE_PREFIX_PATH="C:/Qt/6.11.1/msvc2022_64"
        -DPython3_ROOT_DIR="C:/Users/admin/AppData/Local/Programs/Python/Python314"
    cmake --build . --config Release

If all the dlls are available in the PATH, run bin/qtisas.exe.

## MacOS

Xcode command line tools is a prerequisite. Then a user might need brew or other
software packaging system. Once you have it:

    brew install cmake ninja qt pyqt gl2ps gsl hdf5 muparser libtiff qwt yaml-cpp

Clone the repository:

    git clone --recurse-submodules https://iffgit.fz-juelich.de/qtisas/qtisas.git

Build the software:

    cd qtisas
    mkdir build
    cd build
    cmake ..
        -DCMAKE_BUILD_TYPE=Release
        -DCMAKE_PREFIX_PATH="/opt/homebrew/opt/qt;/opt/homebrew/opt/"
    cmake --build . --parallel $(sysctl -n hw.ncpu)
    open bin/qtisas.app
