#!/bin/bash
# Script for running IQRF daemon on Linux machine
# Tested on Raspberry PI 3, Raspbian Lite
# Tested on AAEON UP, UbiLinux

DAEMON_DIRECTORY=/usr/local/bin

# running daemon
echo "Running daemon ..."
cd  ${DAEMON_DIRECTORY}
sudo ./iqrf_startup  ./configuration/config.json
