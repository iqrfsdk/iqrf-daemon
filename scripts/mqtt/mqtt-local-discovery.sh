#!/bin/bash
# Script for testing MQTT channel on Linux machine
# mosquitto_sub is using local MQTT broker to communicate to IQRF daemon
# Tested on Raspberry PI 3, Raspbian Lite
# Tested on AAEON UP, UbiLinux

#how long to wait for the discovery result in ms
if [ ! -z $1 ]
then
# user timeout
	timeout=$1
else
# infinite timeout
	timeout=0

fi

echo "Sending dpa request to discover iqrf network with txpower 6"
mosquitto_pub -t "Iqrf/DpaRequest" -m "{\"ctype\":\"dpa\",\"type\":\"raw\",\"msgid\":\"1\",\"timeout\":$timeout,\"request\":\"00.00.00.07.ff.ff.06.00\"}"
