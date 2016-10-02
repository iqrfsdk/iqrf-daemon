#pragma once

//#include <arpa/inet.h>
#ifdef WIN32

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <stdint.h>
#include <winsock2.h>
#include <ws2tcpip.h>
typedef int clientlen_t;
#else

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
typedef int SOCKET;
typedef void * SOCKADDR_STORAGE;
typedef size_t clientlen_t;

#endif

#include "Channel.h"
#include <exception>
#include <thread>

class UdpChannel : public Channel
{
public:
  UdpChannel(unsigned short remotePort, unsigned short localPort);
  virtual ~UdpChannel();
  virtual void sendTo(const std::basic_string<unsigned char>& message);
  virtual void registerReceiveFromHandler(ReceiveFromFunc receiveFromFunc);

private:
  UdpChannel();
  ReceiveFromFunc m_receiveFromFunc;
  
  bool m_runListenThread;
  std::thread m_listenThread;
  void listen();

  SOCKET iqrfUdpSocket;
  sockaddr_in iqrfUdpListener;
  sockaddr_in iqrfUdpTalker;

  unsigned short m_remotePort;
  unsigned short m_localPort;

  unsigned char* m_rx;
};

class UdpChannelException : public std::exception {
public:
  UdpChannelException(const std::string& cause)
    :m_cause(cause)
  {}

  virtual const char* what() const
  {
    return m_cause.c_str();
  }
  
  virtual ~UdpChannelException()
  {}

protected:
  std::string m_cause;
};

