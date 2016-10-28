#pragma once

#include "DpaTransaction.h"
#include "UdpChannel.h"
#include "IDaemon.h"
#include "IMessaging.h"
#include <string>
#include <atomic>

typedef std::basic_string<unsigned char> ustring;

class MessageQueue;

class UdpMessagingTransaction : public DpaTransaction
{
public:
  UdpMessagingTransaction(MessageQueue* messageQueue);
  virtual ~UdpMessagingTransaction();
  virtual const DpaMessage& getMessage() const;
  virtual void processConfirmationMessage(const DpaMessage& confirmation);
  virtual void processResponseMessage(const DpaMessage& response);
  virtual void processFinished(bool success);
  void setMessage(ustring message);
private:
  DpaMessage m_message;
  MessageQueue* m_messageQueue;
};

class UdpMessaging: public IMessaging
{
public:
  UdpMessaging();
  virtual ~UdpMessaging();

  virtual void sendMessage(const std::basic_string<unsigned char>& message);
  virtual void setDaemon(IDaemon* daemon);

  void getGwIdent(ustring& message);
  void getGwStatus(ustring& message);

  int handleMessageFromUdp(const ustring& udpMessage);
  int handleMessageFromIqrf(const ustring& iqrfMessage);
  void encodeMessageUdp(ustring& udpMessage, const ustring& message = ustring());
  void decodeMessageUdp(const ustring& udpMessage, ustring& message);

  virtual void start();
  virtual void stop();

private:
  IDaemon *m_daemon;
  UdpChannel *m_udpChannel;
  MessageQueue *m_toUdpMessageQueue;

  unsigned long int m_remotePort;
  unsigned long int m_localPort;

  std::atomic_bool m_running;

};
