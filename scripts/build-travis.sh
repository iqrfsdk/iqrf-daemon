#!/bin/bash
# Script for building IQRF daemon on Linux machine (Travis CI)
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

bash download-dependencies.sh

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

# building libspi
if [ -d "${LIBSPI_DIRECTORY}" ]; then
	echo "Building libspi ..."
	cd ${LIBSPI_DIRECTORY}
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
bash buildMake.sh
