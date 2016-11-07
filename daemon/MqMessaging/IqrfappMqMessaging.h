#pragma once

#include "TaskQueue.h"
#include "DpaTransaction.h"
#include "MqChannel.h"
#include "IDaemon.h"
#include "IMessaging.h"
#include <string>
#include <atomic>
#include <future>

typedef std::basic_string<unsigned char> ustring;

class DpaTask;

class IqrfappMqMessaging : public IMessaging, public DpaTransaction
{
public:
  IqrfappMqMessaging();
  virtual ~IqrfappMqMessaging();

  virtual void setDaemon(IDaemon* daemon);
  virtual void start();
  virtual void stop();

  virtual const DpaMessage& getMessage() const;
  virtual void processConfirmationMessage(const DpaMessage& confirmation);
  virtual void processResponseMessage(const DpaMessage& response);
  virtual void processFinish(DpaRequest::DpaRequestStatus status);
  virtual bool isReady();
private:
  void sendMessageToMq(const std::string& message);
  int handleMessageFromMq(const ustring& mqMessage);

  IDaemon* m_daemon;
  DpaTask* m_task;
  bool m_success;
  std::promise<bool> m_promise;

  MqChannel* m_mqChannel;
  TaskQueue<ustring> *m_toMqMessageQueue;

  std::string m_localMqName;
  std::string m_remoteMqName;

  std::atomic_bool m_running;

};
