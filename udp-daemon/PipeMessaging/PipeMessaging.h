#pragma once

#include "TaskQueue.h"
#include "DpaTransaction.h"
#include "MqChannel.h"
#include "IDaemon.h"
#include "IMessaging.h"
#include <string>
#include <atomic>

typedef std::basic_string<unsigned char> ustring;

class PipeMessaging;

class PipeMessagingTransaction : public DpaTransaction
{
public:
  PipeMessagingTransaction(PipeMessaging* mqMessaging);
  virtual ~PipeMessagingTransaction();
  virtual const DpaMessage& getMessage() const;
  virtual void processConfirmationMessage(const DpaMessage& confirmation);
  virtual void processResponseMessage(const DpaMessage& response);
  virtual void processFinished(bool success);
  void setMessage(ustring message);
private:
  DpaMessage m_message;
  PipeMessaging* m_mqMessaging;
  bool m_success;
};

class PipeMessaging: public IMessaging
{
public:
  PipeMessaging();
  virtual ~PipeMessaging();

  virtual void setDaemon(IDaemon* daemon);
  virtual void start();
  virtual void stop();

  void sendDpaMessageToMq(const DpaMessage&  dpaMessage);

  int handleMessageFromMq(const ustring& mqMessage);

private:
  IDaemon *m_daemon;
  PipeMessagingTransaction* m_transaction;

  MqChannel *m_mqChannel;
  TaskQueue<ustring> *m_toMqMessageQueue;

  std::string m_localMqName;
  std::string m_remoteMqName;

  std::atomic_bool m_running;

};
