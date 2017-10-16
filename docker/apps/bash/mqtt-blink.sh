#!/bin/bash
# Script for testing MQTT channel on Linux machine
# mosquitto_pub is using MQTT local broker to communicate to IQRF daemon
# Tested on Raspberry PI 3, Raspbian Lite
# Tested on AAEON UP, UbiLinux

# node address
if [ ! -z $1 ]; then
# user address
        node=$1
else
# auto address
        node=01
fi

# sleep
if [ ! -z $2 ]; then
# user time
        time=$2
else
# auto time
        time=10
fi

# coordinators
if [ ! -z $3 ]; then
# number of coordinators
        coordinator=$3
else
# one coordinator
        coordinator=1
fi

# packet counter
cnt=1

# end loop
while [ true ]
do
	# echo
	echo "sending request $cnt to pulse red led on node $node"

	mosquitto_pub -h 10.1.1.1  -t "C1/Iqrf/DpaRequest" -m "{\"ctype\":\"dpa\",\"type\":\"raw\",\"msgid\":\"$cnt\",\"request\":\"$node.00.06.03.ff.ff\",\"request_ts\":\"\",\"confirmation\":\".\",\"confirmation_ts\":\"\",\"response\":\".\",\"response_ts\":\"\"}"
	
	if [ $coordinator == 2 ]; then
		mosquitto_pub -h 10.1.1.1  -t "C2/Iqrf/DpaRequest" -m "{\"ctype\":\"dpa\",\"type\":\"raw\",\"msgid\":\"$cnt\",\"request\":\"$node.00.06.03.ff.ff\",\"request_ts\":\"\",\"confirmation\":\".\",\"confirmation_ts\":\"\",\"response\":\".\",\"response_ts\":\"\"}"
	fi

	# counter
	cnt=$((cnt+1))
	
	# sleep
	sleep $time
done
