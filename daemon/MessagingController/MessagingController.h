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
  bool m_enabled = false;
  rapidjson::Document m_doc;
};

class MessagingController : public IDaemon
{
public:
  MessagingController(const std::string& cfgFileName);
  virtual ~MessagingController();
  virtual void executeDpaTransaction(DpaTransaction& dpaTransaction);
  void abortAllDpaTransactions();
  //TODO unregister
  virtual void registerAsyncDpaMessageHandler(std::function<void(const DpaMessage&)> message_handler);
  virtual void unregisterAsyncDpaMessageHandler();
  virtual std::set<IMessaging*>& getSetOfMessaging();
  virtual void registerMessaging(IMessaging& messaging);
  virtual void unregisterMessaging(IMessaging& messaging);
  virtual IScheduler* getScheduler() { return m_scheduler; }

  void exit();
  void watchDog();

  void exclusiveAccess(bool mode); 
  IChannel* getIqrfInterface();

private:
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

  IChannel* m_iqrfInterface;
  DpaHandler* m_dpaHandler;
  std::atomic_bool m_exclusiveMode;

  void executeDpaTransactionFunc(DpaTransaction* dpaTransaction);

  void insertMessaging(std::unique_ptr<IMessaging> messaging);

  TaskQueue<DpaTransaction*> *m_dpaTransactionQueue;
  std::atomic_bool m_running;
  std::set<IMessaging*> m_protocols;
  
  std::map<std::string, IClient*> m_clients;
  std::vector<ISerializer*> m_serializers;
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

  //
  std::string m_configurationDir;
  std::map<std::string, ComponentDescriptor> m_componentMap;

};
