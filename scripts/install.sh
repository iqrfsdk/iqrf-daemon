#!/bin/bash
# Script for installing IQRF daemon on Linux machine
# Tested on Raspberry PI 3, Raspbian Lite
# Tested on AAEON UP, UbiLinux

DAEMON_DIRECTORY=iqrf-daemon/daemon/build/Unix_Makefiles

cd ../..

# installing daemon
if [ -d "${DAEMON_DIRECTORY}" ]; then
        echo "Installing daemon ..."
	cd ${DAEMON_DIRECTORY}
        sudo make install
else
	echo "IQRF daemon is not built yet!"
	echo "Run build script according to your platform first!"
fi
