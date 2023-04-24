FROM --platform=linux/amd64 ubuntu:bionic

WORKDIR /root

RUN apt update && apt upgrade -y && \
	apt install -y autoconf build-essential git libpng-dev libpython3-dev \
	libtiff-dev libtool pkg-config pyqt4-dev-tools python-qt4-dev python3 \
	python3-pyqt4 python3-sip-dev qt4-dev-tools sip-dev wget zlib1g-dev

#install cmake from ppa
RUN wget -O - https://apt.kitware.com/keys/kitware-archive-latest.asc 2>/dev/null | gpg --dearmor - | tee /usr/share/keyrings/kitware-archive-keyring.gpg >/dev/null && \
	echo 'deb [signed-by=/usr/share/keyrings/kitware-archive-keyring.gpg] https://apt.kitware.com/ubuntu/ bionic main' | tee /etc/apt/sources.list.d/kitware.list >/dev/null && \
	apt update && \
	apt install -y cmake