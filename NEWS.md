# IQRF Gateway Daemon

## Changes for iqrf-daemon

**1.0.0**

News:
- dpa raw-hdp rdata introduced instead of both req_data and res_data
- new format for timestamp according to ISO 8601
- dpa raw binary separator defaulted to "."
- improved api comments
- improved traces for release version
- more attempts to open iqrf interface if fails
- configuration version of the daemon set to v1.0
- compatible with prepared iqrf repository
- deb packages for up, up2 and rpi boards 

- iqrf gateway daemon [webapp](https://github.com/iqrfsdk/iqrf-daemon-webapp) follows up in development with many new features such as: 4 clouds support and bonding and discovery in iqmesh network manager

- https://repos.iqrfsdk.org/ stable version introduced with new signing key
- https://apidocs.iqrfsdk.org/ documentation introduced
- https://github.com/iqrfsdk/iqrf-daemon-examples simple examples for python and node.js added
- https://github.com/iqrfsdk/iot-starter-kit step by step guide for gw prepared
- https://github.com/iqrfsdk/iqrf-gateway docker image for demo gw introduced

Fixes:
- dpa raw-hdp response parsing

**0.8.0-3**

Fixes:
- dependance on libssl1.0.2 for debian stretch packages

**0.8.0-2**

Fixes:
- config version set back to v0.0 

**0.8.0-1**

News:
- daemon [roadmap](https://github.com/iqrfsdk/iqrf-daemon/projects?query=is%3Aopen+sort%3Aname-asc) updated
- daemon [webapp](https://github.com/iqrfsdk/iqrf-daemon-webapp) follows up in development
- rehivetech management [system](https://management.rehivetech.com) adds user selected port tunnel (webapp 80 port can be also tunneled now)
- dpa timing reworked, frc timing not yet implemented
- iqrfapp reworked
- udp channel parameters for iqrf ide
- ready state for cutils IChannel
- asynchronous messages support
- two custom services
- packages for debian stretch
- mq nad mqtt [examples](https://github.com/iqrfsdk/iqrf-daemon-examples) in bash and python

Fixes:
- issues related to dpa timing
- issues related to iqrfapp
- switching from service mode deadlock
- ctype config mode return value formating

**0.7.0-1**

News:
- us resolution in the log
- flush the log straight away
- selection of the running mode added into config.json

Fixes:
- pending transaction in DpaHandler
- mutex deadlock

**0.6.0-1**

News:
- json dpa structure according to the description in [wiki](https://github.com/iqrfsdk/iqrf-daemon/wiki/JsonStructureDpa-v1)
(Only type raw and raw-hdp are fully implemented!)

**0.5.1-1**

Fixes:
- iqrfapp cmdline parser
- json dpa response timestamps 
- json dpa response fields order
