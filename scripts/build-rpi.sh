#!/bin/bash
# Script for building IQRF daemon on Linux machine
# Tested on Raspberry PI 3, Raspbian Lite
# External dependencies: IBM Paho, Oracle Java

DAEMON_DIRECTORY=iqrf-daemon
UTILS_DIRECTORY=cutils
LIBDPA_DIRECTORY=clibdpa
LIBCDC_DIRECTORY=clibcdc
LIBSPI_DIRECTORY=clibspi
PAHO_DIRECTORY=paho.mqtt.c

export JAVA_HOME=/usr/lib/jvm/jdk-8-oracle-arm32-vfp-hflt
export JAVA_INCLUDE_PATH=${JAVA_HOME}/include
export JAVA_INCLUDE_PATH2=${JAVA_INCLUDE_PATH}/linux

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
        if [ -d "${PAHO_DIRECTORY}" ]; then
                echo "Applying patch ..."
                cd ${PAHO_DIRECTORY}
                git apply ../iqrf-daemon/daemon/cmake/modules/0001-Fixed-build-and-installation-for-Win-and-SSL.patch
                cd ..
        fi
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
	sudo apt-get install build-essential gcc make cmake
	sudo apt-get install libssl-dev
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
        sudo apt-get install oracle-java8-jdk
	chmod +x buildMake.sh
	./buildMake.sh
	cd ..
fi

# building libcdc
if [ -d "${LIBCDC_DIRECTORY}" ]; then
	echo "Building libcdc ..."
        cd ${LIBCDC_DIRECTORY}
        chmod +x buildMake.sh
        ./buildMake.sh
	cd ..
fi

# building cutils
if [ -d "${UTILS_DIRECTORY}" ]; then
        echo "Building utils ..."
	cd ${UTILS_DIRECTORY}
        chmod +x buildMake.sh
        ./buildMake.sh
	cd ..
fi

# building libdpa
if [ -d "${LIBDPA_DIRECTORY}" ]; then
        echo "Building libdpa ..."
        cd ${LIBDPA_DIRECTORY}
        chmod +x buildMake.sh
        ./buildMake.sh
        cd ..
fi

# building daemon
if [ -d "${DAEMON_DIRECTORY}" ]; then
        echo "Building daemon ..."
	cd ${DAEMON_DIRECTORY}/daemon
        chmod +x buildMake.sh
        ./buildMake.sh
	cd ../..
fi
