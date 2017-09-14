#!/bin/bash
# Script for testing MQTT channel on Linux machine
# mosquitto_sub is using local MQTT broker to communicate to IQRF daemon
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
mosquitto_pub -t "Iqrf/DpaRequest" -m "{\"ctype\":\"conf\",\"type\":\"mode\",\"cmd\":\"$cmd\"}"
