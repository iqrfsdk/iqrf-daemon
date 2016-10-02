#include "IqrfCdcChannel.h"
#include "IqrfLogging.h"
#include <thread>
#include <chrono>

IqrfCdcChannel::IqrfCdcChannel(const std::string& portIqrf)
  : m_cdc(portIqrf.c_str())
{
  if (!m_cdc.test()) {
    THROW_EX(CDCImplException, "CDC Test failed");
  }
}

IqrfCdcChannel::~IqrfCdcChannel()
{
}

void IqrfCdcChannel::sendTo(const std::basic_string<unsigned char>& message)
{
  static int counter = 0;
  DSResponse dsResponse = DSResponse::BUSY;
  int attempt = 0;
  counter++;

  TRC_DBG("Sending to IQRF CDC: " << std::endl << FORM_HEX(message.data(), message.size()));

  while (attempt++ < 4) {
    TRC_DBG("Trying to sent: " << counter << "." << attempt);
    dsResponse = m_cdc.sendData(message);
    if (dsResponse != DSResponse::BUSY)
      break;
    //wait for next attempt
    TRC_DBG("Sleep for a while ... ");
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }
  if (dsResponse != DSResponse::OK) {
    THROW_EX(CDCImplException, "CDC send failed" << PAR(dsResponse));
  }
}

void IqrfCdcChannel::registerReceiveFromHandler(ReceiveFromFunc receiveFromFunc)
{
  m_receiveFromFunc = receiveFromFunc;
  m_cdc.registerAsyncMsgListener([&](unsigned char* data, unsigned int length) {
    m_receiveFromFunc(std::basic_string<unsigned char>(data, length)); });
}
