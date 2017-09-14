#!/bin/bash
# Script for testing IQRF APP on Linux machine
# IQRF APP is using Message queue MQ channel to communicate to IQRF daemon
# Tested on Raspberry PI 3, Raspbian Lite
# Tested on AAEON UP, UbiLinux

#operational ... MQ and MQTT channels are enabled
#service ... UDP channel is enabled (IQRF IDE can be used)
#forwarding ... trafic via MQ and MQTT channels is forwarded to UDP channel
if [ ! -z $1 ]
then
# user selected mode
	if [ $1 == "operational" ]
	then
		cmd=$1
	elif [ $1 == "service" ]
	then
		cmd=$1
	elif [ $1 == "forwarding" ]
	then
		cmd=$1
	else
		cmd="operational"
	fi
else
# mode operational by default
	cmd="operational"

fi

echo "Sending conf request to change gw mode to $cmd"
sudo iqrfapp "{\"ctype\":\"conf\",\"type\":\"mode\",\"cmd\":\"$cmd\"}"
