#!/bin/bash
# Script for testing IQRF APP on Linux machine
# IQRF APP is using Message queue MQ channel to communicate to IQRF daemon
# Tested on Raspberry PI 3, Raspbian Lite
# Tested on AAEON UP, UbiLinux

# timeout:0 means waiting until response comes
echo "Sending DPA request to discovere the iqrf network with txpower 6"
sudo iqrfapp "{\"ctype\":\"dpa\",\"type\":\"raw\",\"msgid\":\"1\",\"timeout\":0,\"request\":\"00.00.00.07.ff.ff.06.00\"}"
