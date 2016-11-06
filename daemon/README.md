#udp-daemon
It is a prototype of executable binary working as a User Gateway between IQRF IDE4 and IQRF network. The traffic is passed via UDP socket.

#//TODO
It is under development now and thus not everything is working as expected.

#Build
**udp-daemon** is built by **build64.bat** on Windows via `Visual Studio 12 2013 Win64 x64. Other platforms will be supported soon.

**udp-daemon** project depends on a library [clibcdc](https://github.com/iqrfsdk/clibcdc). It has to be built in advance.

Referenced library path is added to CMake command line. See inside the batch file.