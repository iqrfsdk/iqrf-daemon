#!/bin/bash
# Script for downloading dependencies of IQRF daemon on Linux machine
# External dependencies: IBM Paho, Oracle Java

set -e

LIB_DIRECTORY=${1:-../..}
UTILS_DIRECTORY=cutils
LIBDPA_DIRECTORY=clibdpa
LIBCDC_DIRECTORY=clibcdc
LIBSPI_DIRECTORY=clibspi
PAHO_DIRECTORY=paho.mqtt.c

if [ ! -d "${LIB_DIRECTORY}" ]; then
	mkdir ${LIB_DIRECTORY}
fi
cd ${LIB_DIRECTORY}

# getting cutils, clibdpa, clibcdc, clibspi
for repository in ${UTILS_DIRECTORY} ${LIBDPA_DIRECTORY} ${LIBCDC_DIRECTORY} ${LIBSPI_DIRECTORY}; do
	if [ ! -d "${repository}" ]; then
		echo "Cloning ${repository} ..."
		git clone https://github.com/iqrfsdk/${repository}.git
	else
		cd ${repository}
		echo "Pulling ${repository} ..."
		git pull origin
		cd ..
	fi
done

# getting paho
if [ ! -d "${PAHO_DIRECTORY}" ]; then
	echo "Cloning paho ..."
	git clone https://github.com/eclipse/${PAHO_DIRECTORY}.git
else
	cd ${PAHO_DIRECTORY}
	echo "Pulling paho ..."
	git pull origin
fi
