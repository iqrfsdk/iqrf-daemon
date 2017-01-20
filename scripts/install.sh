#!/bin/bash
# Script for installing IQRF daemon on Linux machine
# Tested on Raspberry PI 3, Raspbian Lite
# Tested on AAEON UP, UbiLinux

CMAKE_DIRECTORY=iqrf-daemon/daemon/build/Unix_Makefiles
CONFIG_DIRECTORY=/usr/local/bin/configuration

cd ../..

# installing daemon
if [ -d "${CMAKE_DIRECTORY}" ]; then
        echo "Installing daemon ..."
	cd ${CMAKE_DIRECTORY}
        sudo make install
	# copy only if there is not directory
	if [ ! -d "${CONFIG_DIRECTORY}" ]; then
		echo "Copying the configuration directory ..."
		cd ../..
		sudo cp -r iqrf_startup/configuration /usr/local/bin
	fi
else
	echo "IQRF daemon is not built yet!"
	echo "Run build script according to your platform first!"
fi
