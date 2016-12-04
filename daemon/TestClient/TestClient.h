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

  void setDaemon(IDaemon* daemon) override;
  void start() override;
  void stop() override;
  const std::string& getClientName() override {
    return "TestClient";
  }

private:
  void handleMsgFromMessaging(const ustring& msg);

  IMessaging* m_messaging;
  IDaemon* m_daemon;
  ISerializer* m_serializer;
};
