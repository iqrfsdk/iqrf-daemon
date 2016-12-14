#!/bin/bash
# Script for building IQRF daemon on Linux machine
# Tested on AAEON UP, UbiLinux
# External dependencies: IBM Paho, Oracle Java

DAEMON_DIRECTORY=iqrf-daemon
UTILS_DIRECTORY=cutils
LIBDPA_DIRECTORY=clibdpa
LIBCDC_DIRECTORY=clibcdc
LIBSPI_DIRECTORY=clibspi
PAHO_DIRECTORY=paho.mqtt.c

export JAVA_HOME=/opt/jdk/jdk1.8.0_112
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
	if [ ! -d "${JAVA_HOME}" ]; then
        	echo "Getting and installing Oracle Java for IQRF libraries Java stubs"
		wget --no-check-certificate --header "Cookie: oraclelicense=accept-securebackup-cookie" http://download.oracle.com/otn-pub/java/jdk/8u112-b15/jdk-8u112-linux-x64.tar.gz
		sudo mkdir /opt/jdk
		sudo tar -zxf jdk-8u112-linux-x64.tar.gz -C /opt/jdk
		sudo update-alternatives --install /usr/bin/java java /opt/jdk/jdk1.8.0_112/bin/java 100
		sudo update-alternatives --install /usr/bin/javac javac /opt/jdk/jdk1.8.0_112/bin/javac 100
		rm -rf jdk-8u112-linux-x64.tar.gz
	fi
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
