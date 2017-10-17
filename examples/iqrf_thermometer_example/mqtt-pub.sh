#!/bin/bash
# Script for testing MQTT channel on Linux machine
# mosquitto_pub is using MQTT local broker to communicate to IQRF daemon
# Tested on Raspberry PI 3, Raspbian Lite
# Tested on AAEON UP, UbiLinux

echo "sending custom json request to set reading period"
mosquitto_pub -d -t "Iqrf/DpaRequest" -m "{\"service\":\"ThermometerService\",\"period\":30}"
