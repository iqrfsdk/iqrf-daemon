#pragma once

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
class UdpMessaging;

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
  MessagingController(const MessagingController &) = delete;

  static MessagingController& getController();
  void run(const std::string& cfgFileName);

  //from IDaemon
  virtual void executeDpaTransaction(DpaTransaction& dpaTransaction) override;
  virtual void registerAsyncDpaMessageHandler(std::function<void(const DpaMessage&)> message_handler) override;
  virtual IScheduler* getScheduler() override { return m_scheduler; }

  void exit();

  void exclusiveAccess(bool mode); 
  IChannel* getIqrfInterface();

private:
  MessagingController();
  virtual ~MessagingController();

  void startTrace();
  void startIqrfIf();
  void startUdp();
  void startDpa();
  void startClients();
  void startScheduler();

  void stopIqrfIf();
  void stopUdp();
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
  std::atomic_bool m_exclusiveMode;

  void executeDpaTransactionFunc(DpaTransaction* dpaTransaction);

  TaskQueue<DpaTransaction*> *m_dpaTransactionQueue;

  std::map<std::string, std::unique_ptr<ISerializer>> m_serializers;
  std::map<std::string, std::unique_ptr<IClient>> m_clients;
  std::map<std::string, std::unique_ptr<IMessaging>> m_messagings;

  UdpMessaging* m_udpMessaging = nullptr;
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
