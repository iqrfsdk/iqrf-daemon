#!/bin/bash
# Script for testing MQTT channel on Linux machine
# mosquitto_pub is using MQTT local broker to communicate to IQRF daemon
# Tested on Raspberry PI 3, Raspbian Lite
# Tested on AAEON UP, UbiLinux

cnt=1
adr=1

echo "sending requests to pulse red led"

while [ true ]
do

	for value in {1..5}
	do

		echo "req n.$cnt adr n.$adr"

		mosquitto_pub -t "Iqrf/DpaRequest" -m "{\"ctype\":\"dpa\",\"type\":\"raw\",\"msgid\":\"$cnt\",\"timeout\":2000,\"request\":\"$adr.00.06.03.ff.ff\",\"request_ts\":\"\",\"confirmation\":\".\",\"confirmation_ts\":\"\",\"response\":\".\",\"response_ts\":\"\"}"

		cnt=$((cnt+1))

		adr=$((adr+1))
		if [ $adr -gt 5 ]
		then
			adr=1
		fi
		printf -v adr "%x" "$adr"
	done

	sleep 10
done
