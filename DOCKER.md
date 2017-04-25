# IQRF in Docker

[https://hub.docker.com/u/iqrfsdk](https://hub.docker.com/u/iqrfsdk)

## Install Docker CE

### Add Docker's official GPG key

```Bash
sudo apt-key adv --keyserver hkp://p80.pool.sks-keyservers.net:80 --recv-keys 58118E89F3A912897C070ADBF76221572C52609D
```

### Add repository to the source list

-	For Debian (amd64)

```Bash
echo "deb https://download.docker.com/linux/debian jessie stable" | sudo tee -a /etc/apt/sources.list
sudo apt-get update
```

-	For Ubuntu (amd64)

```Bash
echo "deb https://download.docker.com/linux/ubuntu xenial stable" | sudo tee -a /etc/apt/sources.list
sudo apt-get update
```

-	For Raspbian (armhf)

```Bash
echo "deb https://apt.dockerproject.org/repo raspbian-jessie main" | sudo tee -a /etc/apt/sources.list
sudo apt-get update
```

### Install the Docker

```Bash
sudo apt-get install docker-ce (docker-engine for Raspbian)
```

### Confirm that Docker service is up and running

```Bash
sudo systemctl status docker.service
â docker.service - Docker Application Container Engine
   Loaded: loaded (/lib/systemd/system/docker.service; enabled)
   Active: active (running) since Tue 2017-04-25 22:21:28 CEST; 1min 6s ago
     Docs: https://docs.docker.com
 Main PID: 23484 (dockerd)
   CGroup: /system.slice/docker.service
           ââ23484 /usr/bin/dockerd -H fd://
           ââ23491 docker-containerd -l unix:///var/run/docker/libcontainerd/docker-containerd.sock --metrics-interval=0 --start-timeout 2m --state-dir /var/run/dock...

Apr 25 22:21:28 ubilinux dockerd[23484]: time="2017-04-25T22:21:28.422059424+02:00" level=warning msg="Your kernel does not support cgroup rt runtime"
Apr 25 22:21:28 ubilinux dockerd[23484]: time="2017-04-25T22:21:28.423115042+02:00" level=info msg="Loading containers: start."
Apr 25 22:21:28 ubilinux dockerd[23484]: time="2017-04-25T22:21:28.464248327+02:00" level=info msg="Firewalld running: false"
Apr 25 22:21:28 ubilinux dockerd[23484]: time="2017-04-25T22:21:28.648868560+02:00" level=info msg="Default bridge (docker0) is assigned with an IP address...P address"
Apr 25 22:21:28 ubilinux dockerd[23484]: time="2017-04-25T22:21:28.750178656+02:00" level=info msg="Loading containers: done."
Apr 25 22:21:28 ubilinux dockerd[23484]: time="2017-04-25T22:21:28.790408003+02:00" level=info msg="Daemon has completed initialization"
Apr 25 22:21:28 ubilinux dockerd[23484]: time="2017-04-25T22:21:28.790509937+02:00" level=info msg="Docker daemon" commit=c6d412e graphdriver=overlay2 vers...17.03.1-ce
Apr 25 22:21:28 ubilinux dockerd[23484]: time="2017-04-25T22:21:28.818738176+02:00" level=info msg="API listen on /var/run/docker.sock"
Apr 25 22:21:28 ubilinux systemd[1]: Started Docker Application Container Engine.
Apr 25 22:21:28 ubilinux systemd[1]: [/lib/systemd/system/docker.service:24] Unknown lvalue 'Delegate' in section 'Service'
Hint: Some lines were ellipsized, use -l to show in full.
```

### Add user ubilinux to docker group (allow user to connect to docker engine)

```Bash
sudo usermod $(whoami) -aG docker
```

Terminal logout & login is necessary to update settings.

### Run quick test

```Bash
docker run hello-world

Unable to find image 'hello-world:latest' locally
latest: Pulling from library/hello-world
78445dd45222: Pull complete
Digest: sha256:c5515758d4c5e1e838e9cd307f6c6a0d620b5e07e6f927b07d05f6d12a1ac8d7
Status: Downloaded newer image for hello-world:latest

Hello from Docker!
This message shows that your installation appears to be working correctly.

To generate this message, Docker took the following steps:
 1. The Docker client contacted the Docker daemon.
 2. The Docker daemon pulled the "hello-world" image from the Docker Hub.
 3. The Docker daemon created a new container from that image which runs the
    executable that produces the output you are currently reading.
 4. The Docker daemon streamed that output to the Docker client, which sent it
    to your terminal.

To try something more ambitious, you can run an Ubuntu container with:
 $ docker run -it ubuntu bash

Share images, automate workflows, and more with a free Docker ID:
 https://cloud.docker.com/

For more examples and ideas, visit:
 https://docs.docker.com/engine/userguide/
```
