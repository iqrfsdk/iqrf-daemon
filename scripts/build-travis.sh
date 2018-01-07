#!/bin/bash
# Script for building IQRF daemon on Linux machine (Travis CI)
# Tested on Ubuntu 16.04
# External dependencies: IBM Paho, Oracle Java

set -e

LIB_DIRECTORY=${1:-../..}
DAEMON_DIRECTORY=$PWD/../daemon
UTILS_DIRECTORY=cutils
LIBDPA_DIRECTORY=clibdpa
LIBCDC_DIRECTORY=clibcdc
LIBSPI_DIRECTORY=clibspi
PAHO_DIRECTORY=paho.mqtt.c

#DEBUG="Debug"
DEBUG=""

bash download-dependencies.sh ${LIB_DIRECTORY}

cd ${LIB_DIRECTORY}

# building paho
if [ -d "${PAHO_DIRECTORY}" ]; then
	echo "Building paho ..."
	cd ${PAHO_DIRECTORY}
	cmake -G "Unix Makefiles" -DPAHO_WITH_SSL=TRUE -DPAHO_BUILD_DOCUMENTATION=FALSE -DPAHO_BUILD_SAMPLES=TRUE .
	make
	make install
	ldconfig
	cd ..
fi

# building clibspi, clibcdc, cutils, clibdpa
for repository in ${LIBSPI_DIRECTORY} ${LIBCDC_DIRECTORY} ${UTILS_DIRECTORY} ${LIBDPA_DIRECTORY}; do
	if [ -d "${repository}" ]; then
		echo "Building ${repository} ..."
		cd ${repository}
		bash buildMake.sh ${DEBUG}
		cd ..
	fi
done

# building daemon
echo "Building daemon ..."
cd ${DAEMON_DIRECTORY}
bash buildMake.sh ${LIB_DIRECTORY} ${DEBUG}
