# Thermometer service

- custom dpa logic 
- custom json messages

## Build the base daemon first

```Bash
cd ~/iqrf-daemon/scripts
sudo ./build-raspbian.sh ../libs
```

## Build the example

```Bash
cd ~/iqrf-daemon/examples/iqrf_thermometer_example
sudo ./buildMake.sh ../../libs
```

## Run the example

```Bash
cd ~/iqrf-daemon/examples/iqrf_thermometer_example/build/Unix_Makefiles/bin
sudo ./ThermometerStartup configuration/config.json
```

## Check responses

```Bash
cd ~/iqrf-daemon/examples/iqrf_thermometer_example
./mqtt-sub.sh
```

- custom json response every 5s

```Bash
2017-10-17 22:48:28.238 Iqrf/DpaResponse {
2017-10-17 22:48:28.249     "service": "ThermometerService",
2017-10-17 22:48:28.260     "Thermometers": [
2017-10-17 22:48:28.272         "27.500000",
2017-10-17 22:48:28.284         "27.500000"
2017-10-17 22:48:28.295     ]
2017-10-17 22:48:28.306 }
2017-10-17 22:48:33.233 Iqrf/DpaResponse {
2017-10-17 22:48:33.245     "service": "ThermometerService",
2017-10-17 22:48:33.257     "Thermometers": [
2017-10-17 22:48:33.268         "28.000000",
2017-10-17 22:48:33.280         "28.000000"
2017-10-17 22:48:33.292     ]
2017-10-17 22:48:33.304 }
```

## Confirm the change of reading period via messaging

- change the period to 30s

```Bash
cd ~/iqrf-daemon/examples/iqrf_thermometer_example
./mqtt-pub.sh
```

```Bash
2017-10-17 22:48:34.863 Iqrf/DpaResponse {
2017-10-17 22:48:34.875     "service": "ThermometerService",
2017-10-17 22:48:34.887     "period": 30,
2017-10-17 22:48:34.900     "status": "OK"
2017-10-17 22:48:34.914 }
2017-10-17 22:48:35.396 Iqrf/DpaResponse {
2017-10-17 22:48:35.408     "service": "ThermometerService",
2017-10-17 22:48:35.419     "Thermometers": [
2017-10-17 22:48:35.432         "27.500000",
2017-10-17 22:48:35.443         "28.000000"
2017-10-17 22:48:35.455     ]
2017-10-17 22:48:35.466 }
2017-10-17 22:49:05.408 Iqrf/DpaResponse {
2017-10-17 22:49:05.419     "service": "ThermometerService",
2017-10-17 22:49:05.431     "Thermometers": [
2017-10-17 22:49:05.443         "28.000000",
2017-10-17 22:49:05.455         "27.500000"
2017-10-17 22:49:05.467     ]
2017-10-17 22:49:05.479 }
```

## Feedback

Please, let us know if we miss anything!
Enjoy & spread the joy
