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
  MqMessaging() = delete;
  MqMessaging(const std::string& name);

  virtual ~MqMessaging();

  //component
  void start() override;
  void stop() override;
  void update(const rapidjson::Value& cfg) override;
  const std::string& getName() const override { return m_name; }

  //interface
  void registerMessageHandler(MessageHandlerFunc hndl) override;
  void unregisterMessageHandler() override;
  void sendMessage(const ustring& msg) override;

private:
  int handleMessageFromMq(const ustring& mqMessage);

  MqChannel* m_mqChannel;
  TaskQueue<ustring>* m_toMqMessageQueue;

  //configuration
  std::string m_name;
  std::string m_localMqName;
  std::string m_remoteMqName;

  IMessaging::MessageHandlerFunc m_messageHandlerFunc;
};
