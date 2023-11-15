# Version 0.0.3
FROM --platform=linux/amd64 rockylinux:9.2

WORKDIR /root

RUN dnf up -y

RUN dnf install -y dnf-plugins-core && \
    dnf config-manager --set-enabled devel && \
    dnf install -y epel-release && \
    dnf up -y

RUN dnf install -y git wget \
    g++ libtool mesa-libGLU-devel hdf5-devel libpng-devel libtiff-devel zlib-devel \
    qt5-qtbase-devel qt5-qtsvg-devel \
    python3 python3-devel python3-qt5-devel sip6

RUN wget https://github.com/Kitware/CMake/releases/download/v3.27.1/cmake-3.27.1-linux-x86_64.tar.gz && \
    tar -xvf cmake-3.27.1-linux-x86_64.tar.gz

ENV PATH=/root/cmake-3.27.1-linux-x86_64/bin:$PATH
