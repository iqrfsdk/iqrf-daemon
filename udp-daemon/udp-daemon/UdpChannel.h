#pragma once

#include "Channel.h"

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

};
