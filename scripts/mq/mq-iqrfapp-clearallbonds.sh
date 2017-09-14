#!/bin/bash
# Script for testing IQRF APP on Linux machine
# IQRF APP is using Message queue MQ channel to communicate to IQRF daemon
# Tested on Raspberry PI 3, Raspbian Lite
# Tested on AAEON UP, UbiLinux

echo "Sending DPA request to clear coordinator"
sudo iqrfapp "{\"ctype\":\"dpa\",\"type\":\"raw\",\"msgid\":\"1\",\"timeout\":1000,\"request\":\"00.00.00.03.ff.ff\"}"
