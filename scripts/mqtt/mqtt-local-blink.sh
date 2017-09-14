#!/bin/bash
# Script for testing MQTT channel on Linux machine
# mosquitto_pub is using MQTT local broker to communicate to IQRF daemon
# Tested on Raspberry PI 3, Raspbian Lite
# Tested on AAEON UP, UbiLinux

echo "sending requests to pulse red led on node 1"
mosquitto_pub -t "Iqrf/DpaRequest" -m "{\"ctype\":\"dpa\",\"type\":\"raw\",\"msgid\":\"1\",\"timeout\":1000,\"request\":\"01.00.06.03.ff.ff\",\"request_ts\":\"\",\"confirmation\":\".\",\"confirmation_ts\":\"\",\"response\":\".\",\"response_ts\":\"\"}"
