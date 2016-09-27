#pragma once

#include "Channel.h"
#include "CdcInterface.h"
#include "CDCImpl.h"

class IqrfCdcChannel : public Channel
{
public:
  IqrfCdcChannel(const std::string& portIqrf);
  virtual ~IqrfCdcChannel();
  virtual void sendTo(const std::basic_string<unsigned char>& message);
  virtual void registerReceiveFromHandler(ReceiveFromFunc receiveFromFunc);

private:
  IqrfCdcChannel();
  CDCImpl m_cdc;
  ReceiveFromFunc m_receiveFromFunc;

};
