#!/bin/bash
# Script for installing IQRF daemon service
#
# Service interface for later control
# sudo service IQRF start/stop/status
#
# Tested on Raspberry PI 3, Raspbian Lite

#BIN
DAEMON_BIN=/usr/local/bin/iqrf_startup

#SPI
INTERFACE=/dev/spidev0.0

#CDC
#INTERFACE=/dev/ttyACM0

# running daemon
if [ -s "${DAEMON_BIN}" ]; then
        echo "Running daemon ..."
        sudo ./daemonize.sh IQRF ${DAEMON_BIN} ${INTERFACE}
fi
