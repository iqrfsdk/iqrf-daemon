#include "IqrfCdcChannel.h"
#include <iostream>
#include <thread>
#include <chrono>

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
  static int counter = 0;
  DSResponse dsResponse = DSResponse::BUSY;
  int attempt = 0;
  counter++;
  
  std::cout << "Sending to CDC: " << std::endl;
  for (int i = 0; i < message.size(); i++) {
    std::cout << "0x" << std::hex << (int)message[i] << " ";
  }
  std::cout << std::endl;

  while (attempt++ < 4) {
    std::cout << "Try to sent: " << counter << "." << attempt << std::endl;
    dsResponse = m_cdc.sendData(message);
    if (dsResponse != DSResponse::BUSY)
      break;
    //wait for next attempt
    std::cout << "Sleep for a while ... " << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }
  if (dsResponse != DSResponse::OK) {
    // TODO bad response processing...
    std::cout << "Response not OK: " << dsResponse << std::endl;
  }
}

void IqrfCdcChannel::registerReceiveFromHandler(ReceiveFromFunc receiveFromFunc)
{
  m_receiveFromFunc = receiveFromFunc;
  m_cdc.registerAsyncMsgListener([&](unsigned char* data, unsigned int length) {
    std::cout << "Received from to CDC: " << std::endl;
    for (int i = 0; i < length; i++) {
      std::cout << "0x" << std::hex << (int)data[i] << " ";
    }
    std::cout << std::endl;

    m_receiveFromFunc(std::basic_string<unsigned char>(data, length)); });
}
