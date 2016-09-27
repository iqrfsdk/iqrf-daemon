#include "UdpChannel.h"
#include <iostream>

UdpChannel::UdpChannel(const std::string& portIqrf)
{
}

UdpChannel::~UdpChannel()
{
}

void UdpChannel::sendTo(const std::basic_string<unsigned char>& message)
{
  //TODO
  for (int i = 0; i < message.size(); i++) {
    std::cout << "0x" << std::hex << (int)message[i] << " ";
  }
  std::cout << std::dec << std::endl;
}

void UdpChannel::registerReceiveFromHandler(ReceiveFromFunc receiveFromFunc)
{
  //m_receiveFromFunc = receiveFromFunc;
  //m_cdc.registerAsyncMsgListener([&](unsigned char* data, unsigned int length) {
  //  receiveFromFunc(std::basic_string<unsigned char>(data, length)); });
}
