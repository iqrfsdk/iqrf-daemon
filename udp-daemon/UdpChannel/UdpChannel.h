#pragma once

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
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
typedef int SOCKET;
typedef void * SOCKADDR_STORAGE;
typedef size_t clientlen_t;

#endif

#include "IChannel.h"
#include <stdint.h>
#include <exception>
#include <thread>
#include <vector>
#include <atomic>

class UdpChannel : public IChannel
{
public:
  UdpChannel(unsigned short remotePort, unsigned short localPort, unsigned bufsize);
  virtual ~UdpChannel();
  virtual void sendTo(const std::basic_string<unsigned char>& message);
  virtual void registerReceiveFromHandler(ReceiveFromFunc receiveFromFunc);

  std::string getListeningIpAdress() { return m_myIpAdress; }
  unsigned short getListeningIpPort() { return m_localPort; }
  bool isListening() { return m_isListening; }

private:
  UdpChannel();
  ReceiveFromFunc m_receiveFromFunc;
  
  std::atomic_bool m_isListening;
  bool m_runListenThread;
  std::thread m_listenThread;
  void listen();
  void getMyIpAddress();

  SOCKET iqrfUdpSocket;
  sockaddr_in iqrfUdpListener;
  sockaddr_in iqrfUdpTalker;

  unsigned short m_remotePort;
  unsigned short m_localPort;

  unsigned char* m_rx;
  unsigned m_bufsize;

  std::string m_myIpAdress;
};

class UdpChannelException : public std::exception {
public:
  UdpChannelException(const std::string& cause)
    :m_cause(cause)
  {}

//TODO ?
#ifndef WIN32
  virtual const char* what() const noexcept(true)
#else
  virtual const char* what() const
#endif
  {
    return m_cause.c_str();
  }
  
  virtual ~UdpChannelException()
  {}

protected:
  std::string m_cause;
};

