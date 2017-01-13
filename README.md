# IQRF GW daemon

IQRF GW daemon with the multiple communication channels - UDP/MQ/MQTT.

#How to compile the daemon for AAEON UP board

cd scripts
./build-rpi.sh

#How to compile the daemon for Raspberry PI

cd scripts
./build-rpi.sh

#How to install the daemon

cd scripts
./install.sh

#How to run the daemon

cd scripts
./run.sh

#How to test UDP channel

Use IQRF IDE and follow the instruction for UDP connection.

#How to test MQ channel

cd scripts
./mq-iqrfapp-test.sh

#How to test MQTT channel

sudo apt-get install mosquitto

cd scripts
./mqtt-local-pub-test.sh
