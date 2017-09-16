# Building IQRF daemon from the source

[![Build Status](https://travis-ci.org/iqrfsdk/iqrf-daemon.svg?branch=master)](https://travis-ci.org/iqrfsdk/iqrf-daemon)

## Install the daemon from the repository

Follow the instalation [guide](https://github.com/iqrfsdk/iqrf-daemon/blob/master/README.md)

## Download the daemon from the github

```Bash
mkdir iqrf
cd iqrf/
git clone https://github.com/iqrfsdk/iqrf-daemon.git
cd iqrf-daemon
```

## Compile the daemon for AAEON UP board

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

## Compile the daemon for Raspberry PI

```Bash
cd scripts/
./build-raspbian.sh
```

## Check possible changes in configuration files

```Bash
cd ../daemon/iqrf_startup/configuration
```
Compare main and each component's JSON file with files in /etc/iqrf-daemon.

## Update iqrf-daemon and iqrfapp

```Bash
cd scripts/
./update.sh
```
