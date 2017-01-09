#!/bin/bash
# Script for testing IQRF APP on Linux machine
# IQRF APP is using Message queue MQ channel to communicate to IQRF daemon
# Tested on Raspberry PI 3, Raspbian Lite
# Tested on AAEON UP, UbiLinux

echo "Sending DPA request to pulse red led on the node 1"
sudo /usr/local/bin/iqrfapp Raw 01 00 06 03 ff ff
