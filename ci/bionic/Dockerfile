# Version 0.0.5
FROM --platform=linux/amd64 ubuntu:bionic

WORKDIR /root

RUN apt update && apt upgrade -y && \
	apt install -y git wget \
	build-essential libpng-dev libtiff-dev zlib1g-dev \
	qtbase5-dev libqt5svg5-dev \
	python3 libpython3-dev pyqt5-dev pyqt5-dev-tools sip-dev python3-sip-dev

#install cmake from ppa
RUN wget -O - https://apt.kitware.com/keys/kitware-archive-latest.asc 2>/dev/null | gpg --dearmor - | tee /usr/share/keyrings/kitware-archive-keyring.gpg >/dev/null && \
	echo 'deb [signed-by=/usr/share/keyrings/kitware-archive-keyring.gpg] https://apt.kitware.com/ubuntu/ bionic main' | tee /etc/apt/sources.list.d/kitware.list >/dev/null && \
	apt update && \
	apt install -y cmake
