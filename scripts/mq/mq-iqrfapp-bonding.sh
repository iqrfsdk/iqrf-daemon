#!/bin/bash
# Script for testing IQRF APP on Linux machine
# IQRF APP is using Message queue MQ channel to communicate to IQRF daemon
# Tested on Raspberry PI 3, Raspbian Lite
# Tested on AAEON UP, UbiLinux

if [ ! -z $1 ]
then
# user address
	adr=$1
else
# auto address
	adr=00

fi

echo "Sending DPA request to bond node to the iqrf network"
sudo iqrfapp "{\"ctype\":\"dpa\",\"type\":\"raw\",\"msgid\":\"1\",\"timeout\":12000,\"request\":\"00.00.00.04.ff.ff.$adr.00\"}"
