/**
 * Copyright 2016-2017 MICRORISC s.r.o.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

//#include "UdpMessaging.h"
#include "ISerializer.h"
#include "IMessaging.h"
#include "IClient.h"
#include "TaskQueue.h"
#include "IDaemon.h"
#include "JsonUtils.h"
#include <map>
#include <string>
#include <atomic>
#include <vector>

typedef std::basic_string<unsigned char> ustring;

class IChannel;
class DpaHandler;
class IDpaMessageForwarding;
class IDpaExclusiveAccess;
//class UdpMessaging;

class ComponentDescriptor {
public:
  ComponentDescriptor(const std::string& componentName, bool enabled)
    : m_componentName(componentName)
    , m_enabled(enabled)
  {}
  void loadConfiguration(const std::string configurationDir);

  rapidjson::Value& getConfiguration() {
    return m_doc;
  }

  std::string m_componentName;
  std::string m_interfaceName;
  bool m_enabled = false;
  rapidjson::Document m_doc;
};

class MessagingController : public IDaemon
{
public:
  enum class Mode {
    Operational,
    Service,
    Forwarding
  };

  MessagingController(const MessagingController &) = delete;

  static MessagingController& getController();
  void run(const std::string& cfgFileName);

  //from IDaemon
  void executeDpaTransaction(DpaTransaction& dpaTransaction) override;
  void registerAsyncDpaMessageHandler(std::function<void(const DpaMessage&)> message_handler) override;
  IScheduler* getScheduler() override { return m_scheduler; }
  std::string doCommand(const std::string& cmd) override;

  void exit();

  void setMode(Mode mode);

  //void exclusiveAccess(bool mode);
  //IChannel* getIqrfInterface();

private:
  Mode m_mode;

  MessagingController();
  virtual ~MessagingController();

  void startTrace();
  void startIqrfIf();
  //void startUdp();
  void startDpa();
  void startClients();
  void startScheduler();

  void stopIqrfIf();
  //void stopUdp();
  void stopDpa();
  void stopClients();
  void stopScheduler();
  void stopTrace();

  void start();
  void stop();

  void * getFunction(const std::string& methodName, bool mandatory) const;
  void * getCreateFunction(const std::string& componentName, bool mandatory) const;

  IChannel* m_iqrfInterface;
  DpaHandler* m_dpaHandler;
  
  //std::atomic_bool m_exclusiveMode;
  void executeDpaTransactionFunc(DpaTransaction* dpaTransaction);

  TaskQueue<DpaTransaction*> *m_dpaTransactionQueue;

  std::map<std::string, std::unique_ptr<ISerializer>> m_serializers;
  std::map<std::string, std::unique_ptr<IClient>> m_clients;
  std::map<std::string, std::unique_ptr<IMessaging>> m_messagings;

  //UdpMessaging* m_udpMessaging = nullptr;
  IDpaMessageForwarding* m_dpaMessageForwarding = nullptr;
  IDpaExclusiveAccess* m_dpaExclusiveAccess = nullptr;

  IScheduler* m_scheduler = nullptr;

  std::function<void(const DpaMessage&)> m_asyncHandler;

  //watchDog
  void watchDogPet();
  bool m_running = false;
  std::mutex m_stopConditionMutex;
  std::condition_variable m_stopConditionVariable;
  std::chrono::system_clock::time_point m_lastRefreshTime;
  std::chrono::milliseconds m_timeout;

  //configuration
  void loadConfiguration(const std::string& cfgFileName);
  rapidjson::Document m_configuration;
  std::string m_cfgFileName;
  const std::string m_cfgVersion = "v0.0";
  std::string m_traceFileName;
  int m_traceFileSize = 0;
  std::string m_iqrfInterfaceName;

  std::string m_configurationDir;
  int m_watchDogTimeoutMilis = 10000;
  std::map<std::string, ComponentDescriptor> m_componentMap;

  void loadSerializerComponent(const ComponentDescriptor& componentDescriptor);
  void loadMessagingComponent(const ComponentDescriptor& componentDescriptor);
  void loadClientComponent(const ComponentDescriptor& componentDescriptor);

};
