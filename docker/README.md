# IQRF in Docker

[https://hub.docker.com/u/iqrfsdk](https://hub.docker.com/u/iqrfsdk)

## Install your Docker environment

Follow the [guide](https://github.com/iqrfsdk/iqrf-daemon/blob/master/docker/INSTALL.md)

## Build your own custom container

```Bash
git clone https://github.com/iqrfsdk/iqrf-daemon.git
cd iqrf-daemon/docker

EDIT YOUR CONFIGURATION IN
... iqrf-daemon/docker/configuration/*.json ...

docker build -t iqrf-daemon -f Dockerfile.amd64 .
```

## Run your custom container

```Bash
docker container run iqrf-daemon
```
