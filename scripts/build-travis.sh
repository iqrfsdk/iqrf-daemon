#!/bin/bash
# Script for building IQRF daemon on Linux machine (Travis CI)
# Tested on Ubuntu 16.04
# External dependencies: IBM Paho, Oracle Java

set -e

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

cd ../..

# getting daemon
if [ ! -d "${DAEMON_DIRECTORY}" ]; then
	echo "Cloning daemon ..."
	git clone https://github.com/iqrfsdk/${DAEMON_DIRECTORY}.git
else
	cd ${DAEMON_DIRECTORY}
	echo "Pulling daemon ..."
	git pull origin
	cd ..
fi

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
# already fixed in paho master
#	if [ -d "${PAHO_DIRECTORY}" ]; then
#		echo "Applying patch ..."
#		cd ${PAHO_DIRECTORY}
#		git apply ../iqrf-daemon/daemon/cmake/modules/0001-Fixed-build-and-installation-for-Win-and-SSL.patch
#		cd ..
#	fi
else
	cd ${PAHO_DIRECTORY}
	echo "Pulling paho ..."
	git pull origin
	cd ..
fi

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
if [ -d "${DAEMON_DIRECTORY}" ]; then
	echo "Building daemon ..."
	cd ${DAEMON_DIRECTORY}/daemon
	bash buildMake.sh
	cd ../..
fi
