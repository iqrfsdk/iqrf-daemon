# Install Docker CE

## Step 1: Add Docker's official GPG key, add repository to the source list and install it

-	For Debian (amd64)

```Bash
sudo apt-get install curl apt-transport-https   
curl -fsSL https://download.docker.com/linux/debian/gpg | sudo apt-key add -
echo "deb https://download.docker.com/linux/debian stretch stable" | sudo tee -a /etc/apt/sources.list
sudo apt-get update
sudo apt-get install docker-ce
```

-	For Ubuntu (amd64)

```Bash
sudo apt-get install curl apt-transport-https
curl -fsSL https://download.docker.com/linux/debian/gpg | sudo apt-key add -
echo "deb https://download.docker.com/linux/ubuntu xenial stable" | sudo tee -a /etc/apt/sources.list
sudo apt-get update
sudo apt-get install docker-ce
```

-	For Raspbian (armhf)

```Bash
curl -fsSL get.docker.com -o get-docker.sh
chmod +x get-docker.sh
sudo sh get-docker.sh
```

## Step 2: Confirm that Docker service is up and running

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
...
```

## Step 3: Add user $(whoami) to the Docker group

```Bash
sudo usermod $(whoami) -aG docker
```

**Terminal logout & login is necessary to update settings.**

## Optional: Docker Machine && Compose

Follow the links:

```Bash
https://docs.docker.com/machine/install-machine/
https://docs.docker.com/compose/install/#install-compose
```

## Optional: Run quick test

```Bash
docker run hello-world (amd64) or
docker run hypriot/armhf-hello-world (armhf)

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

Enjoy and spread the joy!
