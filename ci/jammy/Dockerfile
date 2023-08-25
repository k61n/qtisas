# Version 0.0.1
FROM --platform=linux/amd64 ubuntu:jammy

WORKDIR /root

RUN apt update && apt upgrade -y && \
	apt install -y git wget \
	build-essential clang-tidy \
	libpng-dev libtiff-dev zlib1g-dev \
	qtbase5-dev libqt5svg5-dev \
	python3 libpython3-dev python3-pip python3-venv

#install cmake from ppa
RUN wget -O - https://apt.kitware.com/keys/kitware-archive-latest.asc 2>/dev/null | gpg --dearmor - | tee /usr/share/keyrings/kitware-archive-keyring.gpg >/dev/null && \
	echo 'deb [signed-by=/usr/share/keyrings/kitware-archive-keyring.gpg] https://apt.kitware.com/ubuntu/ jammy main' | tee /etc/apt/sources.list.d/kitware.list >/dev/null && \
	apt update && \
	apt install -y cmake

RUN python3 -m venv venv-qtisas && \
	. venv-qtisas/bin/activate && \
	pip3 install python-gitlab pyqt5 pyqt-builder sip && \
	deactivate

COPY files/ /
