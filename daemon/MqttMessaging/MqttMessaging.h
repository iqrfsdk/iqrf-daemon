#pragma once

#include "TaskQueue.h"
#include "IMessaging.h"
#include <string>

class IDaemon;
class MqChannel;

typedef std::basic_string<unsigned char> ustring;

class MqttMessaging : public IMessaging
{
public:
  MqttMessaging();
  virtual ~MqttMessaging();

  virtual void setDaemon(IDaemon* daemon);
  virtual void start();
  virtual void stop();

private:
  void sendMessageToMqtt(const std::string& message);
  int handleMessageFromMqtt(const ustring& mqMessage);

  IDaemon* m_daemon;
  //MqChannel* m_mqChannel;
  TaskQueue<ustring>* m_toMqttMessageQueue;

  //std::string m_localMqName;
  //std::string m_remoteMqName;

};
