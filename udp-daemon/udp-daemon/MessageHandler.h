#pragma once

#include <string>

typedef std::basic_string<unsigned char> ustring;

class Channel;
class MessageQueue;

class MessageHandler
{
public:
  MessageHandler(const std::string& iqrf_port_name, const std::string& remote_ip_port, const std::string& local_ip_port);
  virtual ~MessageHandler();

  ustring getGwIdent();

  int handleMessageFromUdp(const ustring& udpMessage);
  int handleMessageFromIqrf(const ustring& iqrfMessage);
  void encodeMessageUdp(const ustring& message, ustring& udpMessage);
  void decodeMessageUdp(const ustring& udpMessage, ustring& message);

  void watchDog();

private:
  Channel *m_iqrfChannel;
  Channel *m_udpChannel;

  MessageQueue *m_toUdpMessageQueue;
  MessageQueue *m_toIqrfMessageQueue;
};
