#!/bin/bash
# Script for running IQRF daemon on Linux machine
# Tested on Raspberry PI 3, Raspbian Lite
# Tested on AAEON UP, UbiLinux

DAEMON_DIRECTORY=/usr/local/bin

# running daemon
echo "Running daemon in Screen ..."
sudo apt-get install screen
cd ${DAEMON_DIRECTORY}
sudo screen -AmdS iqrf iqrf_startup /usr/local/bin/configuration/config.json
echo "IQRF daemon is running ..." 
echo "Use this command to see running process: sudo screen -r iqrf"
