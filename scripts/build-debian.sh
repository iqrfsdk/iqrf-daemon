#!/bin/bash
# Script for building IQRF daemon on Linux machine
# Tested on AAEON UP, UbiLinux
# External dependencies: IBM Paho, Oracle Java

set -e

LIB_DIRECTORY=${1:-../..}
DAEMON_DIRECTORY=iqrf-daemon
UTILS_DIRECTORY=cutils
LIBDPA_DIRECTORY=clibdpa
LIBCDC_DIRECTORY=clibcdc
LIBSPI_DIRECTORY=clibspi
PAHO_DIRECTORY=paho.mqtt.c

export JAVA_HOME=/opt/jdk/jdk1.8.0_112
export JAVA_INCLUDE_PATH=${JAVA_HOME}/include
export JAVA_INCLUDE_PATH2=${JAVA_INCLUDE_PATH}/linux

bash download-dependencies.sh ${LIB_DIRECTORY}

cd ${LIB_DIRECTORY}

# building paho
if [ -d "${PAHO_DIRECTORY}" ]; then
	echo "Building paho ..."
	cd ${PAHO_DIRECTORY}
	sudo apt-get install build-essential gcc make cmake libssl-dev
	cmake -G "Unix Makefiles" -DPAHO_WITH_SSL=TRUE -DPAHO_BUILD_DOCUMENTATION=FALSE -DPAHO_BUILD_SAMPLES=TRUE .
	make
	sudo make install
	sudo ldconfig
	cd ..
fi

# building libspi
if [ -d "${LIBSPI_DIRECTORY}" ]; then
	echo "Building libspi ..."
	cd ${LIBSPI_DIRECTORY}
	if [ ! -d "${JAVA_HOME}" ]; then
		echo "Getting and installing Oracle Java for IQRF libraries Java stubs"
		wget --no-check-certificate --header "Cookie: oraclelicense=accept-securebackup-cookie" http://download.oracle.com/otn-pub/java/jdk/8u112-b15/jdk-8u112-linux-x64.tar.gz
		sudo mkdir /opt/jdk
		sudo tar -zxf jdk-8u112-linux-x64.tar.gz -C /opt/jdk
		sudo update-alternatives --install /usr/bin/java java /opt/jdk/jdk1.8.0_112/bin/java 100
		sudo update-alternatives --install /usr/bin/javac javac /opt/jdk/jdk1.8.0_112/bin/javac 100
		rm -rf jdk-8u112-linux-x64.tar.gz
	fi
	bash buildMake.sh
	cd ..
fi

# building libcdc
if [ -d "${LIBCDC_DIRECTORY}" ]; then
	echo "Building libcdc ..."
	cd ${LIBCDC_DIRECTORY}
	bash buildMake.sh
	cd ..
fi

# building cutils
if [ -d "${UTILS_DIRECTORY}" ]; then
	echo "Building utils ..."
	cd ${UTILS_DIRECTORY}
	bash buildMake.sh
	cd ..
fi

# building libdpa
if [ -d "${LIBDPA_DIRECTORY}" ]; then
	echo "Building libdpa ..."
	cd ${LIBDPA_DIRECTORY}
	bash buildMake.sh
	cd ..
fi

# building daemon
echo "Building daemon ..."
cd ../daemon
bash buildMake.sh ${LIB_DIRECTORY}
