# Version 0.0.2
FROM iffregistry.fz-juelich.de/qtisas/qtisas:jammy-0.0.4

WORKDIR /root

RUN apt install -y clang-tidy clang-format gawk

RUN python3 -m venv venv-qtisas && \
    . venv-qtisas/bin/activate && \
    pip3 install python-gitlab pyqt5 pyqt-builder sip && \
    deactivate

# build qtisas libs
RUN git clone --recurse-submodules https://iffgit.fz-juelich.de/qtisas/qtisas.git && \
    mkdir qtisas/build && cd qtisas/build && \
    cmake .. -DCMAKE_C_COMPILER=/usr/bin/gcc -DCMAKE_CXX_COMPILER=/usr/bin/g++ \
        -DCMAKE_BUILD_TYPE=Release -DWITH_PYTHON=OFF && \
    cd /root/qtisas/libs && \
    tar -cJvf /root/Linux-x86_64.tar.xz Linux-x86_64 && \
    cd /root && rm -rf qtisas

COPY files/ /
