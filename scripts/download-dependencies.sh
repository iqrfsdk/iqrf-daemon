#!/bin/bash
# Script for downloading dependencies of IQRF daemon on Linux machine
# External dependencies: IBM Paho, Oracle Java

set -e

LIB_DIRECTORY=../libs
DAEMON_DIRECTORY=iqrf-daemon
UTILS_DIRECTORY=cutils
LIBDPA_DIRECTORY=clibdpa
LIBCDC_DIRECTORY=clibcdc
LIBSPI_DIRECTORY=clibspi
PAHO_DIRECTORY=paho.mqtt.c

if [ ! -d "${LIB_DIRECTORY}" ]; then
	mkdir ${LIB_DIRECTORY}
fi
cd ${LIB_DIRECTORY}


# getting utils
if [ ! -d "${UTILS_DIRECTORY}" ]; then
	echo "Cloning utils ..."
	git clone https://github.com/iqrfsdk/${UTILS_DIRECTORY}.git
else
	cd ${UTILS_DIRECTORY}
	echo "Pulling utils ..."
	git pull origin
	cd ..
fi

# getting libdpa
if [ ! -d "${LIBDPA_DIRECTORY}" ]; then
	echo "Cloning libdpa ..."
	git clone https://github.com/iqrfsdk/${LIBDPA_DIRECTORY}.git
else
	cd ${LIBDPA_DIRECTORY}
	echo "Pulling libdpa ..."
	git pull origin
	cd ..
fi

# getting libcdc
if [ ! -d "${LIBCDC_DIRECTORY}" ]; then
	echo "Cloning libcdc ..."
	git clone https://github.com/iqrfsdk/${LIBCDC_DIRECTORY}.git
else
	cd ${LIBCDC_DIRECTORY}
	echo "Pulling libcdc ..."
	git pull origin
	cd ..
fi

# getting libspi
if [ ! -d "${LIBSPI_DIRECTORY}" ]; then
	echo "Cloning libspi ..."
	git clone https://github.com/iqrfsdk/${LIBSPI_DIRECTORY}.git
else
	cd ${LIBSPI_DIRECTORY}
	echo "Pulling libspi ..."
	git pull origin
	cd ..
fi

# getting paho
if [ ! -d "${PAHO_DIRECTORY}" ]; then
	echo "Cloning paho ..."
	git clone https://github.com/eclipse/${PAHO_DIRECTORY}.git
else
	cd ${PAHO_DIRECTORY}
	echo "Pulling paho ..."
	git pull origin
fi
