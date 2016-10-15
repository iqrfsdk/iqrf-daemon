#pragma once

#include <string>
#include <atomic>

typedef std::basic_string<unsigned char> ustring;

class Channel;
class MessageQueue;

class MessageHandler
{
public:
  MessageHandler(const std::string& iqrf_port_name, const std::string& remote_ip_port, const std::string& local_ip_port);
  virtual ~MessageHandler();

  void getGwIdent(ustring& message);
  void getGwStatus(ustring& message);

  int handleMessageFromUdp(const ustring& udpMessage);
  int handleMessageFromIqrf(const ustring& iqrfMessage);
  void encodeMessageUdp(ustring& udpMessage, const ustring& message = ustring());
  void decodeMessageUdp(const ustring& udpMessage, ustring& message);

  void watchDog();
  void exit();

private:
  void start();
  void stop();

  Channel *m_iqrfChannel;
  Channel *m_udpChannel;

  MessageQueue *m_toUdpMessageQueue;
  MessageQueue *m_toIqrfMessageQueue;

  std::string m_iqrfPortName;
  unsigned long int m_remotePort;
  unsigned long int m_localPort;

  std::atomic_bool m_running;

};
