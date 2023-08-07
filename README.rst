QtiSAS
======

Ubuntu
------

This software can be successfully build on systems having Qt5 library, such as
Ubuntu 18.04 and so on.
For this following packages are necessary::

    sudo apt install git wget \
        autoconf build-essential libtool pkg-config \
        libpng-dev libtiff-dev zlib1g-dev \
        qtbase5-dev libqt5svg5-dev \
        python3 libpython3-dev pyqt5-dev pyqt5-dev-tools sip-dev python3-sip-dev

Since 32b3cb47 the git repo includes submodules, therefore when cloning use
--recurse-submodules flag::

    git clone --recurse-submodules https://iffgit.fz-juelich.de/kholostov/qtisas.git

To build the software modern cmake is necessary. This can be provided by your
IDE of choice, otherwise it can be installed from Kitware repository::

    wget -O - https://apt.kitware.com/keys/kitware-archive-latest.asc | gpg --dearmor - | sudo tee /usr/share/keyrings/kitware-archive-keyring.gpg
    echo 'deb [signed-by=/usr/share/keyrings/kitware-archive-keyring.gpg] https://apt.kitware.com/ubuntu/ bionic main' | sudo tee /etc/apt/sources.list.d/kitware.list
    sudo apt update
    sudo apt install -y cmake

Use cmake to finally build the software::

    cd qtisas
    mkdir build
    cd build
    cmake .. -DCMAKE_C_COMPILER=/usr/bin/gcc -DCMAKE_CXX_COMPILER=/usr/bin/g++ \
        -DCMAKE_BUILD_TYPE=Release -DWITH_PYTHON=ON
    cmake --build . --parallel 8
    ../bin/qtisas

Rocky Linux 9
-------------

Following pacakges are to be installed::

    sudo yum install -y git nano wget \
    	g++ libtool mesa-libGLU-devel libpng-devel libtiff-devel zlib-devel \
    	qt5-qtbase-devel qt5-qtsvg-devel python3 python3-devel sip6

In order to enable python support in qtisas following packages are needed from
pip::

    pip3 install pyqt5 pyqt-builder sip

After that make sure to implement following workarounds::

    sudo ln -s /usr/bin/qmake-qt5 /usr/bin/qmake
    sudo ln -s $(dirname $(dirname $(find /usr -name QtCoremod.sip))) /usr/share/sip/PyQt

Install modern cmake::

    wget https://github.com/Kitware/CMake/releases/download/v3.27.1/cmake-3.27.1-linux-x86_64.tar.gz && \
    tar -xvf cmake-3.27.1-linux-x86_64.tar.gz

Edit bashrc to add cmake to the PATH::

    nano ~/.bashrc

And add this line at the end of the file, save and reloggin::

    PATH=~/cmake-3.27.1-linux-x86_64/bin:$PATH

Clone the repository::

    git clone --recurse-submodules https://iffgit.fz-juelich.de/kholostov/qtisas.git

Use cmake to finally build the software::

    cd qtisas
    mkdir build
    cd build
    cmake .. -DCMAKE_C_COMPILER=/usr/bin/gcc -DCMAKE_CXX_COMPILER=/usr/bin/g++ \
        -DCMAKE_BUILD_TYPE=Release -DWITH_PYTHON=ON
    cmake --build . --parallel 8
    ../bin/qtisas

MacOS
-----

Xcode command line tools are prerequisite. Then a user might need brew or other
software packaging system. Once you have it::

    brew install autoconf automake cmake libtool make pkg-config \
        qt@5 libtiff python3 pyqt@5 pyqt-builder sip scipy

Clone the repository::

    git clone --recurse-submodules https://iffgit.fz-juelich.de/kholostov/qtisas.git

Then let's finally build the software::

    cd qtisas
    mkdir build
    cd build
    cmake .. -DCMAKE_C_COMPILER=/usr/bin/clang -DCMAKE_CXX_COMPILER=/usr/bin/clang++ \
        -DCMAKE_PREFIX_PATH=/opt/homebrew/opt/qt@5 \
        -DPython3_ROOT_DIR=/opt/homebrew/opt/python@3 \
        -DCMAKE_BUILD_TYPE=Release -DWITH_PYTHON=ON
    cmake --build . --parallel 8
    ../bin/qtisas
