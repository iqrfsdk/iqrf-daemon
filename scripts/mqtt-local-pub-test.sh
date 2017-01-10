#!/bin/bash
# Script for testing MQTT channel on Linux machine
# mosquitto_pub is using MQTT local broker to communicate to IQRF daemon
# Tested on Raspberry PI 3, Raspbian Lite
# Tested on AAEON UP, UbiLinux

echo "Sending DPA request to pulse red led on the node 1"
/usr/local/bin/mosquitto_pub -t "Iqrf/DpaRequest" -m "{\"Type\":\"Raw\",\"Request\":\"01 00 06 03 ff ff\",\"Timeout\":0}"
