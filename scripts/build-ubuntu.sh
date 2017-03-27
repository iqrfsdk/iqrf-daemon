#!/bin/bash
# Script for building IQRF daemon on Linux machine
# Tested on Ubuntu 16.04
# External dependencies: IBM Paho, Oracle Java

set -e

LIB_DIRECTORY=../lib
DAEMON_DIRECTORY=iqrf-daemon
UTILS_DIRECTORY=cutils
LIBDPA_DIRECTORY=clibdpa
LIBCDC_DIRECTORY=clibcdc
LIBSPI_DIRECTORY=clibspi
PAHO_DIRECTORY=paho.mqtt.c

export JAVA_HOME="/usr/lib/jvm/java-8-oracle"
#echo "JAVA_HOME=\"/usr/lib/jvm/java-8-oracle\"" >> ${HOME}/.profile

export JAVA_INCLUDE_PATH=${JAVA_HOME}/include
#echo "JAVA_INCLUDE_PATH=\"/usr/lib/jvm/java-8-oracle/include\"" >> ${HOME}/.profile

export JAVA_INCLUDE_PATH2=${JAVA_INCLUDE_PATH}/linux
#echo "JAVA_INCLUDE_PATH2=\"/usr/lib/jvm/java-8-oracle/include/linux\"" >> ${HOME}/.profile

bash download-dependencies.sh

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
	if [! grep -q "ppa:webupd8team/java" /etc/apt/sources.list /etc/apt/sources.list.d/*]; then
		sudo add-apt-repository ppa:webupd8team/java
		sudo apt-get update
	fi
	sudo apt-get install oracle-java8-installer
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
fi

# building daemon
echo "Building daemon ..."
cd ../../daemon
bash buildMake.sh
