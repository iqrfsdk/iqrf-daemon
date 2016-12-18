#pragma once

#include "TaskQueue.h"
#include "IMessaging.h"
#include <string>

class IDaemon;
class MqChannel;

typedef std::basic_string<unsigned char> ustring;

class MqMessaging : public IMessaging
{
public:
  MqMessaging();
  virtual ~MqMessaging();

  virtual void setDaemon(IDaemon* daemon);
  virtual void start();
  virtual void stop();

  void registerMessageHandler(MessageHandlerFunc hndl) override;
  void unregisterMessageHandler() override;
  void sendMessage(const ustring& msg) override;
  const std::string& getName() const override { return m_name; }

private:
  int handleMessageFromMq(const ustring& mqMessage);

  IDaemon* m_daemon;
  MqChannel* m_mqChannel;
  TaskQueue<ustring>* m_toMqMessageQueue;

  //configuration
  std::string m_name;
  std::string m_localMqName;
  std::string m_remoteMqName;

  IMessaging::MessageHandlerFunc m_messageHandlerFunc;
};
