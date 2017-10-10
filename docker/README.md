# IQRF in Docker

[https://hub.docker.com/u/iqrfsdk](https://hub.docker.com/u/iqrfsdk)

## Install your Docker environment

Follow the [guide](https://github.com/iqrfsdk/iqrf-daemon/blob/master/docker/INSTALL.md)

## Build your custom container

```Bash
git clone https://github.com/iqrfsdk/iqrf-daemon.git
cd iqrf-daemon/docker

EDIT YOUR CONFIGURATION IN
iqrf-daemon/docker/configuration/IqrfInterface.json (help: select your IQRF interface)
iqrf-daemon/docker/configuration/MqttMessaging.json (help: select your broker IP address)
...
```

### UP board

```Bash
docker build -f Dockerfile.amd64 -t iqrf-daemon .
```

### RPI board

```Bash
docker build -f Dockerfile.rpi -t iqrf-daemon .
```

## Run your custom container

Make sure that your host is not running these services and if so stop and disable them:

```Bash
sudo systemctl stop mosquitto.service
sudo systemctl disable mosquitto.service

sudo systemctl stop iqrf-daemon.service
sudo systemctl disable iqrf-daemon.service
```

### Dedicated network for your containers

```Bash
docker network create --subnet 10.1.0.0/16 --gateway 10.1.0.1 --ip-range=10.1.1.0/24 \ 
--driver=bridge --label=host1network bridge01
```

### MQTT broker

```Bash
docker container run -d --name mqtt1broker -p 1883:1883 -p 9001:9001 --network=bridge01 \
--ip=10.1.1.1 --restart=always eclipse-mosquitto
```

### IQRF daemon

```Bash
docker container run -d --name iqrf1daemon --device /dev/spidev2.0:/dev/spidev2.0 \
--privileged --net bridge01 --ip 10.1.1.2 --restart=always iqrf-daemon
```

### IQRF daemon app

#### Build it for UP board

```Bash
cd iqrf-daemon/docker/apps/bash
docker build -f Dockerfile.amd64 -t iqrf-daemon-app .
```

#### Build it for RPI board

```Bash
cd iqrf-daemon/docker/apps/bash
docker build -f Dockerfile.rpi -t iqrf-daemon-app .
```

#### Run it

```Bash
docker container run -d --name iqrf1daemon-app --net bridge01 --ip 10.1.1.3 --restart=always iqrf-daemon-app
```

### Check that all is good

#### See all containers running

```Bash
ubilinux@ubilinux:~/iqrf-daemon/docker/apps/bash$ docker container ls
CONTAINER ID        IMAGE               COMMAND                  CREATED             STATUS              PORTS                                            NAMES
d11255a0456e        iqrf-daemon-app     "/usr/bin/entry.sh..."   6 seconds ago       Up 6 seconds                                                         iqrf1daemon-app
91bef707acc9        iqrf-daemon         "/usr/bin/entry.sh..."   4 hours ago         Up 4 hours                                                           iqrf1daemon
01eda0695d7f        eclipse-mosquitto   "/docker-entrypoin..."   4 hours ago         Up 4 hours          0.0.0.0:1883->1883/tcp, 0.0.0.0:9001->9001/tcp   mqtt1broker
```

#### Check the log from the app

```Bash
ubilinux@ubilinux:~/iqrf-daemon/docker$ docker container logs iqrf1daemon-app
sending request 1 to pulse red led on node 1
sending request 2 to pulse red led on node 1
sending request 3 to pulse red led on node 1
```

## Feedback

Please, let us know if we miss anything!
Enjoy & spread the joy!
