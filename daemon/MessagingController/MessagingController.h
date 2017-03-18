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
  void loadConfiguration(const std::string& cfgFileName);

  //from IDaemon
  virtual void executeDpaTransaction(DpaTransaction& dpaTransaction) override;
  virtual void registerAsyncDpaMessageHandler(std::function<void(const DpaMessage&)> message_handler) override;

  //virtual std::set<IMessaging*>& getSetOfMessaging() override;
  //virtual void registerMessaging(IMessaging& messaging) override;
  //virtual void unregisterMessaging(IMessaging& messaging) override;
  //virtual void registerClientService(IClient& cs) override;
  //virtual void unregisterClientService(IClient& cs) override;
  virtual IScheduler* getScheduler() override { return m_scheduler; }

  void exit();
  void watchDog();

  void exclusiveAccess(bool mode); 
  IChannel* getIqrfInterface();

  //virtual void unregisterAsyncDpaMessageHandler();
  //void abortAllDpaTransactions();
  //TODO unregister

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
  std::atomic_bool m_running;
  
  //std::vector<ISerializer*> m_serializers;
  std::map<std::string, std::unique_ptr<ISerializer>> m_serializers;
  std::map<std::string, std::unique_ptr<IClient>> m_clients;
  std::map<std::string, std::unique_ptr<IMessaging>> m_messagings;

  UdpMessaging* m_udpMessaging = nullptr;
  IScheduler* m_scheduler = nullptr;

  std::function<void(const DpaMessage&)> m_asyncHandler;

  //configuration
  rapidjson::Document m_configuration;
  std::string m_cfgFileName;
  const std::string m_cfgVersion = "v0.0";
  std::string m_traceFileName;
  int m_traceFileSize = 0;
  std::string m_iqrfInterfaceName;

  std::string m_configurationDir;
  std::map<std::string, ComponentDescriptor> m_componentMap;

  void loadSerializerComponent(const ComponentDescriptor& componentDescriptor);
  void loadMessagingComponent(const ComponentDescriptor& componentDescriptor);
  void loadClientComponent(const ComponentDescriptor& componentDescriptor);

};
