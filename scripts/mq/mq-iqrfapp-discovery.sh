#!/bin/bash
# Script for testing IQRF APP on Linux machine
# IQRF APP is using Message queue MQ channel to communicate to IQRF daemon
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

echo "Sending DPA request to discover the iqrf network with txpower 6"
sudo iqrfapp "{\"ctype\":\"dpa\",\"type\":\"raw\",\"msgid\":\"1\",\"timeout\":$timeout,\"request\":\"00.00.00.07.ff.ff.06.00\"}"
