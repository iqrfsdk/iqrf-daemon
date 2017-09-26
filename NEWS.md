# IQRF GW daemon

## Changes for iqrf-daemon

0.8.0-2 -> 0.8.0-3

Fixes:
- dependance on libssl1.0.2 for debian stretch packages

0.8.0-1 -> 0.8.0-2

Fixes:
- config version set back to v0.0 

0.7.0-1 -> 0.8.0-1

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

0.6.0-1 -> 0.7.0-1

News:
- us resolution in the log
- flush the log straight away
- selection of the running mode added into config.json

Fixes:
- pending transaction in DpaHandler
- mutex deadlock

0.5.1-1 -> 0.6.0-1

News:
- json dpa structure according to the description in [wiki](https://github.com/iqrfsdk/iqrf-daemon/wiki/JsonStructureDpa-v1)
(Only type raw and raw-hdp are fully implemented!)

0.5.0-1 -> 0.5.1-1

Fixes:
- iqrfapp cmdline parser
- json dpa response timestamps 
- json dpa response fields order
