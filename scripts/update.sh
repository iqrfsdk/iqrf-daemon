#!/bin/bash
#
# Install daemon package from IQRFSDK repository!
# Update IQRF daemon and IQRF app on Linux machine after your compilation from source using this script!
# Check if there are not diferences in configuration files!
#
# Tested on Raspberry PI 3, Raspbian Lite
# Tested on AAEON UP, UbiLinux

DAEMON_BIN_DIR=iqrf-daemon/daemon/build/Unix_Makefiles/bin
IQRFAPP_BIN_DIR=iqrf-daemon/applications/iqrfapp/build/Unix_Makefiles

cd ../..

# installing daemon
if [ -d "${DAEMON_BIN_DIR}" ]; then
        echo "Updating daemon ..."
	cd ${DAEMON_BIN_DIR}

	sudo systemctl stop iqrf-daemon.service
        sudo cp iqrf_startup /usr/bin

	cd ../../../../..

	sudo systemctl start iqrf-daemon.service
	sleep 1
	sudo systemctl status iqrf-daemon.service
else
	echo "IQRF daemon is not built yet!"
	echo "Run build script according to your platform first!"
fi

#updating iqrfapp
if [ -d "${IQRFAPP_BIN_DIR}" ]; then
        echo "Updating iqrfapp ..."
        cd ${IQRFAPP_BIN_DIR}

	sudo cp iqrfapp /usr/bin
else
	echo "IQRF app is not built yet!"
	echo "Run build script according to your platform first!"
fi
