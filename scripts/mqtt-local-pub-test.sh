#!/bin/bash
# Script for testing MQTT channel on Linux machine
# mosquitto_pub is using MQTT local broker to communicate to IQRF daemon
# Tested on Raspberry PI 3, Raspbian Lite
# Tested on AAEON UP, UbiLinux

cnt=1
adrdec=1
adrhex=1

echo "sending requests to pulse red led"

while [ true ]
do

	for value in {1..20}
	do

		echo "req n.$cnt adr n.$adrhex"

#		mosquitto_pub -t "Iqrf/DpaRequest" -m "{\"ctype\":\"dpa\",\"type\":\"raw\",\"msgid\":\"$cnt\",\"timeout\":2000,\"request\":\"$adrhex.00.06.03.ff.ff\",\"request_ts\":\"\",\"confirmation\":\".\",\"confirmation_ts\":\"\",\"response\":\".\",\"response_ts\":\"\"}"
		mosquitto_pub -t "Iqrf/DpaRequest" -m "{\"ctype\":\"dpa\",\"type\":\"raw\",\"msgid\":\"$cnt\",\"request\":\"$adrhex.00.06.03.ff.ff\",\"request_ts\":\"\",\"confirmation\":\".\",\"confirmation_ts\":\"\",\"response\":\".\",\"response_ts\":\"\"}"

		cnt=$((cnt+1))

		adrdec=$((adrdec+1))
		if [ $adrdec -gt 20 ]
		then
			adrdec=1
		fi
		adrhex=`echo "ibase=10;obase=16;$adrdec"|bc`
	done

	sleep 60
done
