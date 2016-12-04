#pragma once

#include "IClient.h"
#include "MqttMessaging.h"
#include "SimpleSerializer.h"
#include "TaskQueue.h"
#include "IMessaging.h"
#include "IScheduler.h"
#include <string>
#include <chrono>
#include <map>
#include <memory>

class IDaemon;

typedef std::basic_string<unsigned char> ustring;

class TestClient : public IClient
{
public:
  TestClient();
  virtual ~TestClient();

  virtual void setDaemon(IDaemon* daemon);
  virtual void start();
  virtual void stop();

  //virtual void makeCommand(const std::string& clientId, const std::string& command);
  //virtual void registerResponseHandler(const std::string& clientId, ResponseHandlerFunc fun);
  //virtual void unregisterResponseHandler(const std::string& clientId);

private:
  void handleMsgFromMessaging(const ustring& msg);

  IMessaging* m_messaging;
  IDaemon* m_daemon;
  ISerializer* m_serializer;
};
