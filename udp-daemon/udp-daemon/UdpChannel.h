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

//******************************************************************************
//          Public Constants
//******************************************************************************
#define IQRF_UDP_BUFFER_SIZE        512

//--- IQRF UDP commands (CMD) ---
#define IQRF_UDP_GET_GW_INFO		0x01	// Returns GW identification
#define IQRF_UDP_GET_GW_STATUS      0x02	// Returns GW status
#define IQRF_UDP_WRITE_IQRF_SPI		0x03	// Writes data to the TR module's SPI
#define IQRF_UDP_IQRF_SPI_DATA		0x04	// Data from TR module's SPI (async)

//--- IQRF UDP subcommands (SUBCMD) ---
#define IQRF_UDP_ACK			0x50	// Positive answer
#define IQRF_UDP_NAK			0x60	// Negative answer
#define IQRF_UDP_BUS_BUSY		0x61	// Communication channel (IQRF SPI or RS485) is busy

//--- IQRF UDP header ---
struct T_IQRF_UDP_HEADER
{
  uint8_t gwAddr;
  uint8_t cmd;
  uint8_t subcmd;
  uint8_t res0;
  uint8_t res1;
  uint8_t pacid_H;
  uint8_t pacid_L;
  uint8_t dlen_H;
  uint8_t dlen_L;
};

/*
//--- IQRF UDP data part ---
struct T_IQRF_UDP_PDATA
{
  uint8_t buffer[(IQRF_UDP_BUFFER_SIZE - sizeof(T_IQRF_UDP_HEADER))];
};

//--- IQRF UDP Rx/Tx packet ---
union T_IQRF_UDP_RXTXDATA
{
  uint8_t buffer[IQRF_UDP_BUFFER_SIZE];

  struct
  {
    T_IQRF_UDP_HEADER header;
    T_IQRF_UDP_PDATA pdata;
  } data;

};
*/

//--- IQRF UDP data part ---
struct T_IQRF_UDP_PDATA
{
  T_IQRF_UDP_HEADER header;
  uint8_t buffer[(IQRF_UDP_BUFFER_SIZE - sizeof(T_IQRF_UDP_HEADER))];
};

//--- IQRF UDP Rx/Tx packet ---
union T_IQRF_UDP_RXTXDATA
{
  uint8_t allbuffer[IQRF_UDP_BUFFER_SIZE];
  T_IQRF_UDP_PDATA data;
};

#include "Channel.h"
#include <thread>

class UdpChannel : public Channel
{
public:
  UdpChannel(const std::string& portIqrf);
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
  socklen_t iqrfUdpServerLength;

  unsigned short remotePort;
  unsigned short localPort;

  T_IQRF_UDP_RXTXDATA m_tx;
  T_IQRF_UDP_RXTXDATA m_rx;
};
