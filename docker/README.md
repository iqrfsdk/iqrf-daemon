# IQRF in Docker

[https://hub.docker.com/u/iqrfsdk](https://hub.docker.com/u/iqrfsdk)

## Install your Docker environment

Follow the [guide](https://github.com/iqrfsdk/iqrf-daemon/blob/master/docker/INSTALL.md)

## Build your custom container

```Bash
git clone https://github.com/iqrfsdk/iqrf-daemon.git
cd iqrf-daemon/docker

EDIT YOUR CONFIGURATION IN
iqrf-daemon/docker/config1/IqrfInterface.json (tip: select your IQRF interface)
iqrf-daemon/docker/config1/MqttMessaging.json (tip: select your broker IP address)
...
```

### UP board

```Bash
docker build -f Dockerfile1C.amd64 -t iqrf-daemon .
```

### RPI board

```Bash
docker build -f Dockerfile1C.rpi -t iqrf-daemon .
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
docker network create --subnet 10.1.0.0/16 --gateway 10.1.0.1 --ip-range=10.1.1.0/24
--driver=bridge --label=host1network bridge01
```

### MQTT broker

```Bash
docker container run -d --name mqtt1broker -p 1883:1883 -p 9001:9001 --network=bridge01
--ip=10.1.1.1 --restart=always eclipse-mosquitto
```

### IQRF daemon

```Bash
docker container run -d --name iqrf1daemon --device /dev/spidev2.0:/dev/spidev2.0
--privileged --net bridge01 --ip 10.1.1.2 --restart=always iqrf-daemon
```

### IQRF daemon app

#### Build it for UP board

```Bash
cd iqrf-daemon/docker/apps/bash
docker build -f Dockerfile1C.amd64 -t iqrf-daemon-app .
```

#### Build it for RPI board

```Bash
cd iqrf-daemon/docker/apps/bash
docker build -f Dockerfile1C.rpi -t iqrf-daemon-app .
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

### Ready for IQRF Dashboard

#### Stop IQRF daemon app

```Bash
docker container stop iqrf1daemon-app
```

#### Check IQRF Node-RED app

Follow the [guide](https://github.com/iqrfsdk/iqrf-daemon-examples/tree/master/node-red)

### Ready for 2 IQRF coordinators/networks on single GW

- IQRF Mesh network1 via IQRF SPI (configuration in [config1](config1) folder)
- IQRF Mesh network2 via CDC IQRF (configuration in [config2](config2) folder)

#### Stop IQRF daemon and IQRF daemon app

```Bash
docker container stop iqrf1daemon-app
docker container stop iqrf1daemon
```

#### Build your custom containers

```Bash
cd iqrf-daemon/docker

EDIT YOUR CONFIGURATION IN
iqrf-daemon/docker/config1/IqrfInterface.json (tip: select your IQRF interface)
iqrf-daemon/docker/config1/MqttMessaging.json (tip: select your broker IP address)
...
iqrf-daemon/docker/config2/IqrfInterface.json (tip: select your IQRF interface)
iqrf-daemon/docker/config2/MqttMessaging.json (tip: select your broker IP address)
...
```

##### UP board

```Bash
docker build -f Dockerfile1C.amd64 -t iqrf-daemon-1c .
docker build -f Dockerfile2C.amd64 -t iqrf-daemon-2c .
```

##### RPI board

```Bash
docker build -f Dockerfile1C.rpi -t iqrf-daemon-1c .
docker build -f Dockerfile2C.rpi -t iqrf-daemon-2c .
```

#### Run IQRF daemons

```Bash
docker container run -d --name iqrf1daemon --device /dev/spidev2.0:/dev/spidev2.0
--privileged --net bridge01 --ip 10.1.1.2 --restart=always iqrf-daemon-1c
docker container run -d --name iqrf2daemon --device /dev/ttyACM0:/dev/ttyACM0
--privileged --net bridge01 --ip 10.1.1.3 --restart=always iqrf-daemon-2c
```

#### IQRF daemon app

##### Build it for UP board

```Bash
cd iqrf-daemon/docker/apps/bash
docker build -f Dockerfile2C.amd64 -t iqrf-daemon-app .
```

##### Build it for RPI board

```Bash
cd iqrf-daemon/docker/apps/bash
docker build -f Dockerfile2C.rpi -t iqrf-daemon-app .
```

##### Run it

```Bash
docker container run -d --name iqrf-daemon-app --net bridge01 --ip 10.1.1.4 --restart=always iqrf-daemon-app
```

#### Check that all is good

##### See all containers running

```Bash
ubuntu@ubuntu-up-gw01:~/iqrf-daemon/docker/apps/bash$ docker container ls
CONTAINER ID        IMAGE               COMMAND                  CREATED             STATUS              PORTS                                            NAMES
797d54f39b9a        iqrf-daemon-app     "/usr/bin/entry.sh..."   12 seconds ago      Up 10 seconds                                                        iqrf-daemon-app
cc6dfb14ba27        iqrf-daemon-2c      "/usr/bin/entry.sh..."   2 minutes ago       Up 2 minutes                                                         iqrf2daemon
6370837f3dca        iqrf-daemon-1c      "/usr/bin/entry.sh..."   2 minutes ago       Up 2 minutes                                                         iqrf1daemon
c807bea35569        eclipse-mosquitto   "/docker-entrypoin..."   3 minutes ago       Up 3 minutes        0.0.0.0:1883->1883/tcp, 0.0.0.0:9001->9001/tcp   mqtt1broker
```

##### Check the log from the app

```Bash
ubuntu@ubuntu-up-gw01:~/iqrf-daemon/docker/apps/bash$ docker container logs iqrf-daemon-app
sending request 1 to pulse red led on node 1
sending request 2 to pulse red led on node 1
sending request 3 to pulse red led on node 1
```

##### Listen for JSON response from 2C

```Bash
ubuntu@ubuntu-up-gw01:~/iqrf-daemon/docker/apps/bash$ ./mqtt-sub.sh
Listening for DPA responses from IQRF network:
Client mosqsub/1036-ubuntu-up- sending CONNECT
Client mosqsub/1036-ubuntu-up- received CONNACK
Client mosqsub/1036-ubuntu-up- sending SUBSCRIBE (Mid: 1, Topic: C1/Iqrf/DpaResponse, QoS: 0)
Client mosqsub/1036-ubuntu-up- sending SUBSCRIBE (Mid: 2, Topic: C2/Iqrf/DpaResponse, QoS: 0)
Client mosqsub/1036-ubuntu-up- received SUBACK
Subscribed (mid: 1): 0
Client mosqsub/1036-ubuntu-up- received SUBACK
Subscribed (mid: 2): 0
Client mosqsub/1036-ubuntu-up- received PUBLISH (d0, q0, r0, m0, 'C1/Iqrf/DpaResponse', ... (379 bytes))
{
    "ctype": "dpa",
    "type": "raw",
    "msgid": "1",
    "request": "01.00.06.03.ff.ff",
    "request_ts": "2017-10-16 17:02:52.669711",
    "confirmation": "01.00.06.03.ff.ff.ff.43.01.08.01",
    "confirmation_ts": "2017-10-16 17:02:52.718294",
    "response": "01.00.06.83.0f.00.00.44",
    "response_ts": "2017-10-16 17:02:52.964795",
    "status": "STATUS_NO_ERROR"
}
Client mosqsub/1036-ubuntu-up- received PUBLISH (d0, q0, r0, m0, 'C2/Iqrf/DpaResponse', ... (379 bytes))
{
    "ctype": "dpa",
    "type": "raw",
    "msgid": "1",
    "request": "01.00.06.03.ff.ff",
    "request_ts": "2017-10-16 17:02:52.683244",
    "confirmation": "01.00.06.03.ff.ff.ff.50.01.08.01",
    "confirmation_ts": "2017-10-16 17:02:52.723130",
    "response": "01.00.06.83.00.00.00.51",
    "response_ts": "2017-10-16 17:02:52.966805",
    "status": "STATUS_NO_ERROR"
}
```

## Feedback

Please, let us know if we miss anything!
Enjoy & spread the joy!
