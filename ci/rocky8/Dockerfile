# Version 0.0.3
FROM --platform=linux/amd64 rockylinux:8.8

WORKDIR /root

RUN dnf up -y

RUN dnf install -y dnf-plugins-core && \
    dnf config-manager --set-enabled powertools && \
    dnf install -y epel-release && \
    dnf up -y

RUN dnf install -y git wget \
    mesa-libGLU-devel hdf5-devel libpng-devel libtiff-devel yaml-cpp-devel zlib-devel \
    qt5-qtbase-devel qt5-qtsvg-devel \
    python3-qt5-devel

RUN wget https://github.com/Kitware/CMake/releases/download/v3.27.1/cmake-3.27.1-linux-x86_64.tar.gz && \
    tar -xvf cmake-3.27.1-linux-x86_64.tar.gz

ENV PATH=/root/cmake-3.27.1-linux-x86_64/bin:$PATH
