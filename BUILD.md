# Building IQRF daemon from the source

[![Build Status](https://travis-ci.org/iqrfsdk/iqrf-daemon.svg?branch=master)](https://travis-ci.org/iqrfsdk/iqrf-daemon)

## How to download the daemon

```Bash
mkdir iqrf
cd iqrf/
git clone https://github.com/iqrfsdk/iqrf-daemon.git
cd iqrf-daemon
```

## How to compile the daemon for AAEON UP board

### Ubilinux

```Bash
cd scripts/
./build-debian.sh
```

### Ubuntu

```Bash
cd scripts/
./build-ubuntu.sh
```

## How to compile the daemon for Raspberry PI

```Bash
cd scripts/
./build-raspbian.sh
```

## How to install the daemon to /usr/local/bin

```Bash
cd scripts/
./install.sh
```

## How to configure the daemon

```Bash
cd /usr/local/bin/configuration/
```
Configure main and each component's JSON file.

## How to install the daemon as a systemd service

```Bash
cd scripts/
sudo ./install-systemd.sh
```

## How to start/stop/restart/status the daemon

```Bash
sudo systemctl start/stop/restart/status IQRF
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
sudo apt-get install mosquitto-clients

cd scripts/
./mqtt-local-pub-test.sh
```
