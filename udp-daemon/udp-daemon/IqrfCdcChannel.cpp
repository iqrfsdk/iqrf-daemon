#include "IqrfCdcChannel.h"
#include <iostream>

IqrfCdcChannel::IqrfCdcChannel(const std::string& portIqrf)
  : m_cdc(portIqrf.c_str())
{
  if (!m_cdc.test()) {
    throw CDCImplException("CDC Test Failed");
  }
}

IqrfCdcChannel::~IqrfCdcChannel()
{
}

void IqrfCdcChannel::sendTo(const std::basic_string<unsigned char>& message)
{
  DSResponse dsResponse = m_cdc.sendData(message);
  if (dsResponse != OK) {
    // bad response processing...
    std::cout << "Response not OK: " << dsResponse << "\n";
  }
}

void IqrfCdcChannel::registerReceiveFromHandler(ReceiveFromFunc receiveFromFunc)
{
  m_receiveFromFunc = receiveFromFunc;
  m_cdc.registerAsyncMsgListener([&](unsigned char* data, unsigned int length) {
    m_receiveFromFunc(std::basic_string<unsigned char>(data, length)); });
}
