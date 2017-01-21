# IQRF GW daemon

IQRF GW daemon with the multiple communication channels - UDP/MQ/MQTT.

![See the daemon architecture](/doc/iqrf-linux-gw.png)

## How to compile the daemon for AAEON UP board

```Bash
cd scripts/
./build-up.sh
```

## How to compile the daemon for Raspberry PI

```Bash
cd scripts/
./build-rpi.sh
```

## How to install the daemon

```Bash
cd scripts/
./install.sh
```

## How to configure the daemon

```Bash
cd /usr/local/bin/configuration/
```
Configure main and each component's JSON file.

## How to run the daemon

```Bash
cd scripts/
./run.sh
```

## How to test UDP channel

Use IQRF IDE and follow the instruction for UDP connection.
Then use IQMESH Network Manager.

## How to test MQ channel

```Bash
cd scripts/
./mq-iqrfapp-test.sh
```

## How to test MQTT channel

```Bash
sudo apt-get install mosquitto

cd scripts/
./mqtt-local-pub-test.sh
```
