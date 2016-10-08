#pragma once

#include "Channel.h"
#include "spi_iqrf.h"
#include "sysfs_gpio.h"
#include <mutex>
#include <thread>
#include <atomic>

class IqrfSpiChannel : public Channel
{
public:
  IqrfSpiChannel(const std::string& portIqrf);
  virtual ~IqrfSpiChannel();
  virtual void sendTo(const std::basic_string<unsigned char>& message);
  virtual void registerReceiveFromHandler(ReceiveFromFunc receiveFromFunc);

  void setCommunicationMode(_spi_iqrf_CommunicationMode mode) const;
  _spi_iqrf_CommunicationMode getCommunicationMode() const;

private:
  IqrfSpiChannel();
  ReceiveFromFunc m_receiveFromFunc;
  //void readFrom(void* readBuffer, unsigned dataLength);
  //void setCommunicationMode(_spi_iqrf_CommunicationMode mode) const;
  //_spi_iqrf_CommunicationMode getCommunicationMode() const;

  //spi_iqrf_SPIStatus getCommunicationStatus() const;

  bool m_runListenThread;
  std::thread m_listenThread;
  void listen();

  std::string m_port;

  unsigned char* m_rx;
  unsigned m_bufsize;

  std::mutex m_commMutex;
};

class SpiChannelException : public std::exception {
public:
  SpiChannelException(const std::string& cause)
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

  virtual ~SpiChannelException()
  {}

protected:
  std::string m_cause;
};
