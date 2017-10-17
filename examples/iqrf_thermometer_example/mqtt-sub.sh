#!/bin/bash
# Script for testing MQTT channel on Linux machine
# mosquitto_sub is using local MQTT broker to communicate to IQRF daemon
# Tested on Raspberry PI 3, Raspbian Lite
# Tested on AAEON UP, UbiLinux

echo "Listening for DPA responses from IQRF network:"
mosquitto_sub -v -t Iqrf/DpaResponse | xargs -d $'\n' -L1 bash -c 'date "+%Y-%m-%d %T.%3N $0"'
