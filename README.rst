QtiSAS
======

This software can be successfully build in Ubuntu 18.04, due to its previous
dependency on qt3/qt4.
For this following packages are necessary::

    $ sudo apt install autoconf build-essential git libpng-dev libpython3-dev \
	    libtiff-dev libtool pkg-config pyqt4-dev-tools python-qt4-dev python3 \
	    python3-pyqt4 python3-sip-dev qt4-dev-tools sip-dev wget zlib1g-dev

Since 32b3cb47 the git repo starts to include submodules, therefore
when cloning use --recurse-submodules flag::

    $ git clone --recurse-submodules https://iffgit.fz-juelich.de/kholostov/qtisas.git

To build the software modern cmake is necessary. This can be provided by your
IDE of choice, otherwise can be installed from Kitware repository::

    $ wget -O - https://apt.kitware.com/keys/kitware-archive-latest.asc | gpg --dearmor - | sudo tee /usr/share/keyrings/kitware-archive-keyring.gpg
    $ echo 'deb [signed-by=/usr/share/keyrings/kitware-archive-keyring.gpg] https://apt.kitware.com/ubuntu/ bionic main' | sudo tee /etc/apt/sources.list.d/kitware.list
    $ sudo apt update
    $ sudo apt install -y cmake

Use cmake to finally build the software::

    $ cd qtisas
    $ mkdir build
    $ cd build
    $ cmake .. -DCMAKE_BUILD_TYPE=Release -DWITH_PYTHON=On
    $ cmake --build . --parallel 8
    $ ../bin/qtisas

