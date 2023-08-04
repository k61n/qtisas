# Version 0.0.1
FROM --platform=linux/amd64 rockylinux:9.2

WORKDIR /root

RUN yum upgrade -y && \
	yum install -y git wget \
	g++ libtool mesa-libGLU-devel libpng-devel libtiff-devel zlib-devel \
	qt5-qtbase-devel qt5-qtsvg-devel python3 python3-devel sip6 && \
	pip3 install pyqt5 pyqt-builder sip

RUN ln -s /usr/bin/qmake-qt5 /usr/bin/qmake && \
    mkdir -p /usr/share/sip && \
	ln -s $(dirname $(dirname $(find /usr -name QtCoremod.sip))) /usr/share/sip/PyQt

RUN wget https://github.com/Kitware/CMake/releases/download/v3.27.1/cmake-3.27.1-linux-x86_64.tar.gz && \
	tar -xvf cmake-3.27.1-linux-x86_64.tar.gz

ENV PATH=/root/cmake-3.27.1-linux-x86_64/bin:$PATH
