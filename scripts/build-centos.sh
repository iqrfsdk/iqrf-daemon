#!/bin/bash
# Script for building IQRF daemon on Linux machine
# Tested on CentOS 7.3
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

export JAVA_HOME=/usr/java/jdk1.8.0_121
export JAVA_INCLUDE_PATH=${JAVA_HOME}/include
export JAVA_INCLUDE_PATH2=${JAVA_INCLUDE_PATH}/linux

sudo yum -q -y install wget git gcc make cmake openssl-devel

if [ ! -d "${JAVA_HOME}" ]; then
    echo "Getting and installing Oracle Java for IQRF libraries Java stubs"
    wget -q --no-check-certificate --header "Cookie: oraclelicense=accept-securebackup-cookie" http://download.oracle.com/otn-pub/java/jdk/8u121-b13/e9e7ea248e2c4826b92b3f075a80e441/jdk-8u121-linux-x64.rpm
    sudo yum -q -y localinstall jdk-8u121-linux-x64.rpm
    rm jdk-8u121-linux-x64.rpm
fi

bash download-dependencies.sh ${LIB_DIRECTORY}

cd ${LIB_DIRECTORY}

# building paho
if [ -d "${PAHO_DIRECTORY}" ]; then
        echo "Building paho ..."
        cd ${PAHO_DIRECTORY}
        cmake -G "Unix Makefiles" -DPAHO_WITH_SSL=TRUE -DPAHO_BUILD_DOCUMENTATION=FALSE -DPAHO_BUILD_SAMPLES=TRUE .
        make
        sudo make install
sudo cat <<EOF > /etc/ld.so.conf.d/local.conf
/usr/local/lib
/usr/local/lib64
EOF
        sudo ldconfig
        cd ..
fi

# building libspi
if [ -d "${LIBSPI_DIRECTORY}" ]; then
        echo "Building libspi ..."
        cd ${LIBSPI_DIRECTORY}
        bash buildMake.sh ${DEBUG}
        cd ..
fi

# building clibcdc, cutils, clibdpa
for repository in ${LIBCDC_DIRECTORY} ${UTILS_DIRECTORY} ${LIBDPA_DIRECTORY}; do
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
