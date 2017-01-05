#pragma once

#include "TaskQueue.h"
#include "DpaTransaction.h"
#include "UdpChannel.h"
#include "IDaemon.h"
#include "IMessaging.h"
#include <string>
#include <atomic>

typedef std::basic_string<unsigned char> ustring;

class UdpMessaging;

class UdpMessagingTransaction : public DpaTransaction
{
public:
  UdpMessagingTransaction(UdpMessaging* udpMessaging);
  virtual ~UdpMessagingTransaction();
  virtual const DpaMessage& getMessage() const;
  virtual int getTimeout() const { return -1; }
  virtual void processConfirmationMessage(const DpaMessage& confirmation);
  virtual void processResponseMessage(const DpaMessage& response);
  virtual void processFinish(DpaRequest::DpaRequestStatus status);
  void setMessage(ustring message);
private:
  DpaMessage m_message;
  UdpMessaging* m_udpMessaging;
  bool m_success;
};

class UdpMessaging: public IMessaging
{
public:
  UdpMessaging();
  virtual ~UdpMessaging();

  virtual void setDaemon(IDaemon* daemon);
  virtual void start();
  virtual void stop();

  void registerMessageHandler(MessageHandlerFunc hndl) override {}
  void unregisterMessageHandler() override {}
  void sendMessage(const ustring& msg) override {}

  void sendDpaMessageToUdp(const DpaMessage&  dpaMessage);

  void getGwIdent(ustring& message);
  void getGwStatus(ustring& message);

  int handleMessageFromUdp(const ustring& udpMessage);
  void encodeMessageUdp(ustring& udpMessage, const ustring& message = ustring());
  void decodeMessageUdp(const ustring& udpMessage, ustring& message);

private:
  IDaemon *m_daemon;
  UdpMessagingTransaction* m_transaction;

  UdpChannel *m_udpChannel;
  TaskQueue<ustring> *m_toUdpMessageQueue;

  unsigned long int m_remotePort;
  unsigned long int m_localPort;

  std::atomic_bool m_running;

};
