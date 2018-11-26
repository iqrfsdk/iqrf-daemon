# How to install the daemon

[http://repos.iqrfsdk.org/](http://repos.iqrfsdk.org/)

-   iqrf-daemon_1.1.0-x_amd64.deb
-   iqrf-daemon_1.1.0-x_armhf.deb
-   libpaho.mqtt.c_1.2.0-x_amd64.deb
-   libpaho.mqtt.c_1.2.0-x_armhf.deb

## Download public key to verify the packages from the repository

```Bash
sudo apt-get install dirmngr
sudo apt-key adv --keyserver keyserver.ubuntu.com --recv-keys 9C076FCC7AB8F2E43C2AB0E73241B9B7B4BD8F8E
```

## Add repository to the source list

-	For Debian (amd64)

```Bash
echo "deb http://repos.iqrfsdk.org/debian stretch stable" | sudo tee -a /etc/apt/sources.list
sudo apt-get update
```

-	For Ubuntu (amd64)

```Bash
echo "deb http://repos.iqrfsdk.org/ubuntu/xenial xenial stable" | sudo tee -a /etc/apt/sources.list
sudo apt-get update
```

-	For Raspbian (armhf)

```Bash
echo "deb http://repos.iqrfsdk.org/raspbian stretch stable" | sudo tee -a /etc/apt/sources.list
sudo apt-get update
```

-	For Armbian (armhf)

```Bash
echo "deb http://repos.iqrfsdk.org/armbian xenial stable" | sudo tee -a /etc/apt/sources.list
sudo apt-get update
```

## Install or update the daemon

```Bash
sudo apt-get install iqrf-daemon
```
or

```Bash
sudo apt-get update
sudo apt-get install --only-upgrade iqrf-daemon
```

## Check the daemon service status

```Bash
systemctl status iqrf-daemon.service

● iqrf-daemon.service - IQRF daemon iqrf_startup
   Loaded: loaded (/lib/systemd/system/iqrf-daemon.service; enabled; vendor preset: enabled)
   Active: active (running) since Čt 2017-03-30 23:46:50 CEST; 2min 58s ago
 Main PID: 9540 (iqrf_startup)
   CGroup: /system.slice/iqrf-daemon.service
           └─9540 /usr/bin/iqrf_startup /etc/iqrf-daemon/config.json
```

## Set your configuration 

CK-USB-04A or GW-USB-06 devices must be switched to USB CDC IQRF mode using IDE
menu: Tools/USB Classes/Switch to CDC IQRF mode

Follow the guidance [here](https://github.com/iqrfsdk/iqrf-daemon/wiki/Configuration)

Configure/check mainly following components: 

- config.json           (tip: enable/disable daemon components)
- IqrfInterface.json    (tip: configure your IQRF interface - SPI/CDC)
- TracerFile.json       (tip: check your logging)
- MqttMessaging.json    (tip: configure your MQTT brokers - you can enable second mqtt instance)
- BaseService.json      (tip: see names of running instances of basic service - needed for scheduler)
- Scheduler.json        (tip: configure your regural DPA tasks if any)

and restart the service:

```Bash
sudo systemctl restart iqrf-daemon.service
```

## Content of iqrf-daemon package

```Bash
dpkg -L iqrf-daemon

/.
/usr
/usr/bin
/usr/bin/iqrfapp
/usr/bin/iqrf_startup
/usr/bin/tracefile_add_timestamp.sh
/usr/share
/usr/share/doc
/usr/share/doc/iqrf-daemon
/usr/share/doc/iqrf-daemon/copyright
/usr/share/doc/iqrf-daemon/changelog.gz
/lib
/lib/systemd
/lib/systemd/system
/lib/systemd/system/iqrf-daemon.service
/etc
/etc/iqrf-daemon
/etc/iqrf-daemon/UdpMessaging.json
/etc/iqrf-daemon/config.json
/etc/iqrf-daemon/MqMessaging.json
/etc/iqrf-daemon/TracerFile.json
/etc/iqrf-daemon/JsonSerializer.json
/etc/iqrf-daemon/IqrfInterface.json
/etc/iqrf-daemon/SimpleSerializer.json
/etc/iqrf-daemon/MqttMessaging.json
/etc/iqrf-daemon/Scheduler.json
/etc/iqrf-daemon/iqrfapp.json
/etc/iqrf-daemon/BaseService.json
```

## Content of libpaho.mqtt.c package

```Bash
dpkg -L libpaho.mqtt.c

/.
/usr
/usr/lib
/usr/lib/libpaho-mqtt3c.so.1.2.0
/usr/lib/libpaho-mqtt3as.so.1.2.0
/usr/lib/libpaho-mqtt3cs.so.1.2.0
/usr/lib/libpaho-mqtt3a.so.1.2.0
/usr/bin
/usr/bin/MQTTVersion
/usr/share
/usr/share/doc
/usr/share/doc/libpaho.mqtt.c
/usr/share/doc/libpaho.mqtt.c/copyright
/usr/share/doc/libpaho.mqtt.c/changelog.gz
```
